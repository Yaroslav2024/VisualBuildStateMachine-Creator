/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "diagramview.h"       // Заголовок нашего класса
#include <QKeyEvent>           // Обработка клавиш
#include <QGraphicsScene>      // Работа с графической сценой
#include <QMenu>               // Для контекстного меню
#include <QContextMenuEvent>   // Для события правого клика
#include "diagramitem.h"
#include "State.h"
#include <QScrollBar>
#include "connect.h"
#include <QMouseEvent>
#include <QGraphicsItemGroup>

#include "transitiongroup.h"
#include <QGraphicsSceneMouseEvent>
#include "commentitem.h"
#include "backitem.h"
#include <QMessageBox>
#include "groupitem.h"
#include "bridgeitem.h"
// Конструктор класса DiagramView
DiagramView::DiagramView(QWidget *parent)
    : QGraphicsView(parent) // вызываем конструктор базового класса QGraphicsView
{
    m_zoomController = new ZoomController(this, this); // передаем текущий view в контроллер
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        // По умолчанию разрешим выбирать рамкой левым (если нужно), но для нашей задачи мы управляем DragMode динамически.
        setDragMode(QGraphicsView::NoDrag);

}

// Обработка нажатий клавиш на виджете
void DiagramView::keyPressEvent(QKeyEvent *event)
{
    // Обрабатываем только Delete (и только если сцена существует)
    if (event->key() == Qt::Key_Delete && scene()) {
            // ====================================================================
            // --- ЗАЩИТА СТИКЕРОВ И СПЕЦБЛОКОВ ---
            for (QGraphicsItem *item : scene()->selectedItems()) {
                if (CommentItem *comment = dynamic_cast<CommentItem*>(item)) {
                    if (comment->linkedItem()) {
                        QMessageBox::warning(this, "Удаление заблокировано", "Этот стикер привязан к объекту!\n\nСначала отвяжите его (✂ Отвязать).");
                        return;
                    }
                } else {
                    // --- НОВОЕ: ЗАЩИТА СПЕЦБЛОКОВ ОТ УДАЛЕНИЯ СО СТРЕЛКАМИ ---
                    if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(item)) {
                        QString type = dItem->getType();
                        if ((type == "История" || type == "Возврат" || type == "Мост") && dItem->hasConnections()) {
                            QMessageBox::warning(this, "Удаление заблокировано",
                                "К спецблоку («" + type + "») подключены стрелки!\n\nСначала удалите все привязанные стрелки.");
                            return;
                        }
                    }
                    // ---------------------------------------------------------

                    for (QGraphicsItem *sceneItem : scene()->items()) {
                        if (CommentItem *comment = dynamic_cast<CommentItem*>(sceneItem)) {
                            QGraphicsItem* linked = comment->linkedItem();
                            if (linked && (linked == item || linked == item->topLevelItem() || linked->topLevelItem() == item->topLevelItem())) {
                                QMessageBox::warning(this, "Удаление заблокировано", "К этому объекту привязан стикер!\n\nСначала отвяжите или удалите сам стикер.");
                                return;
                            }
                        }
                    }
                }
            }
            // ====================================================================

            // 1. Разделяем выбранные объекты на списки, чтобы избежать двойного удаления
            QList<ConnectionItem*> arrowsToDelete;
            QList<DiagramItem*> blocksToDelete;
            QList<QGraphicsItem*> othersToDelete;

            QList<QGraphicsItem*> selected = scene()->selectedItems();

            for (QGraphicsItem *item : selected) {
                // Это Блок?
                if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(item)) {
                    blocksToDelete.append(dItem);
                }
                // Это Стрелка (группа) или её часть?
                else {
                    ConnectionItem* conn = nullptr;

                    // Проверяем сам элемент
                    if (item->data(ConnectionRole).toBool()) {
                        conn = dynamic_cast<ConnectionItem*>(item);
                    }
                    // Проверяем родителя (для линий и текста внутри группы)
                    else if (item->parentItem() && item->parentItem()->data(ConnectionRole).toBool()) {
                         conn = dynamic_cast<ConnectionItem*>(item->parentItem());
                    }
                    // Проверяем группу (для старых реализаций)
                    else if (item->group() && item->group()->data(ConnectionRole).toBool()) {
                         conn = dynamic_cast<ConnectionItem*>(item->group());
                    }

                    if (conn) {
                        // Добавляем в список стрелок (без дубликатов)
                        if (!arrowsToDelete.contains(conn)) arrowsToDelete.append(conn);
                    } else {
                        // Что-то другое (артефакты, сетка и т.д.)
                        othersToDelete.append(item);
                    }
                }
            }

            // 2. Сначала удаляем СТРЕЛКИ (БЕЗОПАСНО)
            for (ConnectionItem *conn : arrowsToDelete) {
                if (conn && conn->scene()) {
                    // --- ОТВЯЗКА ДЛЯ ПРЕДОТВРАЩЕНИЯ ВЫЛЕТОВ ---
                    if (conn->startBlock()) conn->startBlock()->removeConnection(conn);
                    if (conn->endBlock()) conn->endBlock()->removeConnection(conn);
                    // ------------------------------------------
                    scene()->removeItem(conn);
                    delete conn;
                }
            }

            // 3. Теперь удаляем БЛОКИ
            // Выделенные стрелки уже удалены. removeAllConnections почистит только
            // оставшиеся (невыделенные) хвосты.
            for (DiagramItem *dItem : blocksToDelete) {
                if (dItem && dItem->scene()) {
                    if (m_state) {
                        m_state->removeItem(dItem);
                    }
                    dItem->removeAllConnections();
                    // =========================================================
                                    // --- НОВОЕ: ОТВЯЗЫВАЕМ БЛОК ОТ РАМОК ПЕРЕД УДАЛЕНИЕМ ---
                                    for (QGraphicsItem *sceneItem : scene()->items()) {
                                        if (GroupItem *group = dynamic_cast<GroupItem*>(sceneItem)) {
                                            group->removeBlock(dItem); // Безопасно выписываем блок
                                        }
                                    }
                                    // =========================================================
                    scene()->removeItem(dItem);
                    delete dItem;
                }
            }

            // 4. Удаляем остальной мусор
            for (QGraphicsItem *item : othersToDelete) {
                if (item && item->scene()) {
                    scene()->removeItem(item);
                    delete item;
                }
            }

            return;
        }

    // Все остальные клавиши обрабатываем стандартно
    QGraphicsView::keyPressEvent(event);
}


// Обработка контекстного меню (обычно вызывается правой кнопкой мыши)
// В файле diagramview.cpp
// Обработка контекстного меню (обычно вызывается правой кнопкой мыши)
// В файле diagramview.cpp
void DiagramView::contextMenuEvent(QContextMenuEvent *event)
{
    // 1. Проверяем, активен ли Connector
    if (m_connector) {
        if (m_connector->isActive()) {
            event->accept();
            return;
        }
        if (m_connector->wasJustStopped()) {
            m_connector->consumeJustStopped();
            event->accept();
            return;
        }
    }
    // Если под курсором стикер, отдаем управление ему (он сам покажет свое меню)
        QGraphicsItem* checkItem = itemAt(event->pos());
        if (checkItem) {
            // Проверяем: это сам стикер ИЛИ текст внутри стикера (parentItem)
            if (dynamic_cast<CommentItem*>(checkItem) ||
                (checkItem->parentItem() && dynamic_cast<CommentItem*>(checkItem->parentItem()))) {

                // Вызываем стандартный обработчик Qt.
                // Он сам найдет item под мышкой и вызовет у него contextMenuEvent.
                QGraphicsView::contextMenuEvent(event);
                return; // Выходим, чтобы не рисовать меню "Удалить" поверх
            }
        }
        // ------------------------------------
    QMenu menu(this);
    QAction *deleteAction = nullptr;
    QAction *scaleAction = nullptr;
    QAction *showAnchorsAction = nullptr;
    QAction *detachEndAction = nullptr;
    //  Переменные для кнопок рамки ---
    QAction *freezeGroupAction = nullptr;
    QAction *scaleGroupAction = nullptr;
    DiagramItem* rightClickedItem = nullptr;
    GroupItem* rightClickedGroup = nullptr;
    QGraphicsItem* itemUnderMouse = itemAt(event->pos());
    ConnectionItem* rightClickedConn = nullptr;

    // Определяем элемент под курсором (Блок или Стрелка)
    if (itemUnderMouse) {
        if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(itemUnderMouse)) {
            rightClickedItem = dItem;
        }
        // --- Проверяем, кликнули ли по рамке ---
                else if (GroupItem *gItem = dynamic_cast<GroupItem*>(itemUnderMouse)) {
                    rightClickedGroup = gItem;
                }

        // Проверка на стрелку (или её часть: линию, текст, фон)
        if (itemUnderMouse->data(ConnectionRole).toBool()) {
            rightClickedConn = static_cast<ConnectionItem*>(itemUnderMouse);
        }
        else if (itemUnderMouse->parentItem() &&
                 itemUnderMouse->parentItem()->data(ConnectionRole).toBool()) {
            rightClickedConn = static_cast<ConnectionItem*>(itemUnderMouse->parentItem());
        }
        else if (itemUnderMouse->parentItem() &&
                 itemUnderMouse->parentItem()->parentItem() &&
                 itemUnderMouse->parentItem()->parentItem()->data(ConnectionRole).toBool()) {
            rightClickedConn = static_cast<ConnectionItem*>(itemUnderMouse->parentItem()->parentItem());
        }
        else if (itemUnderMouse->data(ConnectionRole).toBool()) {
            rightClickedConn = dynamic_cast<ConnectionItem*>(itemUnderMouse);
        } else if (itemUnderMouse->group() && itemUnderMouse->group()->data(ConnectionRole).toBool()) {
            rightClickedConn = dynamic_cast<ConnectionItem*>(itemUnderMouse->group());
        }
    }
    // =========================================================


    // =========================================================
            // --- ПРОПУСКАЕМ КЛИК ТОЛЬКО ДЛЯ НАСТОЯЩИХ МОСТОВ И ВОЗВРАТОВ ---
            // =========================================================
            if (rightClickedItem) {
                // Проверяем принадлежность к реальному классу, а не просто текст!
                if (dynamic_cast<BridgeItem*>(rightClickedItem) || dynamic_cast<BackItem*>(rightClickedItem)) {
                    // Передаем клик самому элементу, чтобы он открыл СВОЕ меню, и выходим!
                    QGraphicsView::contextMenuEvent(event);
                    return;
                }
            }
            // =========================================================


        // =========================================================
    bool hasSelectedItems = (scene() && !scene()->selectedItems().isEmpty());
    bool hasDiagramItem = false;

    if (rightClickedItem) {
        hasDiagramItem = true;
    } else if (hasSelectedItems) {
        for (QGraphicsItem *item : scene()->selectedItems()) {
            if (dynamic_cast<DiagramItem*>(item)) {
                hasDiagramItem = true;
                break;
            }
        }
    }

    // Меню для блоков
    if (hasDiagramItem) {
        // Проверяем, не кликнул ли пользователь по блоку Back (возврат)
                bool isBackItem = (rightClickedItem && dynamic_cast<BackItem*>(rightClickedItem));

                // Показываем "Добавить стрелку" только если это НЕ блок возврата
                        if (!isBackItem) {
                            showAnchorsAction = menu.addAction(tr("Добавить стрелку"));
                        }
        scaleAction = menu.addAction(tr("Масштабировать"));
    }

    if (rightClickedGroup) {
            freezeGroupAction = menu.addAction(rightClickedGroup->isSizeLocked() ? tr("Разморозить авто-размер") : tr("Заморозить авто-размер"));
            scaleGroupAction = menu.addAction(tr("Масштабировать"));
        }
    // Меню для стрелок
    if (rightClickedConn && rightClickedConn->isSelected()) {
        detachEndAction = menu.addAction(tr("Отсоединить конечную точку"));
    }

    // Общее меню удаления
    if (hasSelectedItems) {
        deleteAction = menu.addAction(tr("Удалить"));
    }

    if (menu.isEmpty()) {
        event->accept();
        return;
    }

    QAction *selectedAction = menu.exec(event->globalPos());


    // --- ОБРАБОТКА ДЕЙСТВИЙ ---

        // Обработка кнопок рамки
        if (selectedAction == freezeGroupAction && rightClickedGroup) {
            rightClickedGroup->setSizeLocked(!rightClickedGroup->isSizeLocked());
            rightClickedGroup->update();
            event->accept();
            return;
        }

        // ДОБАВЛЕННЫЙ БЛОК:
        if (selectedAction == scaleGroupAction && rightClickedGroup) {
            rightClickedGroup->setSizeLocked(true); // Автоматически замораживаем авторасширение
            rightClickedGroup->setScaleMode(true);  // Показываем квадратики для ресайза
            event->accept();
            return;
        }

        if (selectedAction == detachEndAction) {
        if (m_arrowEditor && rightClickedConn) {
            m_arrowEditor->startRepin(rightClickedConn);
        }
        event->accept();
        return;
    }

    if (selectedAction == showAnchorsAction) {
        if (m_connector) {
            if (rightClickedItem) {
                m_connector->startFrom(rightClickedItem);
            } else {
                for (QGraphicsItem *item : scene()->selectedItems()) {
                    if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(item)) {
                        if (dynamic_cast<BackItem*>(dItem)) continue;
                        dItem->generateAnchorPoints();
                        dItem->setShowAnchorPoints(true);
                    }
                }
                m_connector->start();
            }
        }
        event->accept();
        return;
    }

    if (selectedAction == scaleAction) {
        QList<QGraphicsItem *> selectedItems = scene()->selectedItems();
        for (QGraphicsItem *item : selectedItems) {
            if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(item)) {
                dItem->showResizeHandles();
            }
        }
        event->accept();
        return;
    }

    // === БЕЗОПАСНОЕ УДАЛЕНИЕ (ВОТ ЗДЕСЬ БЫЛА ОШИБКА) ===
    if (selectedAction == deleteAction) {
            // ====================================================================
            // --- ЗАЩИТА СТИКЕРОВ И СПЕЦБЛОКОВ ---
            for (QGraphicsItem *item : scene()->selectedItems()) {
                if (CommentItem *comment = dynamic_cast<CommentItem*>(item)) {
                    if (comment->linkedItem()) {
                        QMessageBox::warning(this, "Удаление заблокировано", "Этот стикер привязан к объекту!\n\nСначала отвяжите его (✂ Отвязать).");
                        return;
                    }
                } else {
                    // --- НОВОЕ: ЗАЩИТА СПЕЦБЛОКОВ ОТ УДАЛЕНИЯ СО СТРЕЛКАМИ ---
                    if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(item)) {
                        QString type = dItem->getType();
                        if ((type == "История" || type == "Возврат" || type == "Мост") && dItem->hasConnections()) {
                            QMessageBox::warning(this, "Удаление заблокировано",
                                "К спецблоку («" + type + "») подключены стрелки!\n\nСначала удалите все привязанные стрелки.");
                            return;
                        }
                    }
                    // ---------------------------------------------------------

                    for (QGraphicsItem *sceneItem : scene()->items()) {
                        if (CommentItem *comment = dynamic_cast<CommentItem*>(sceneItem)) {
                            QGraphicsItem* linked = comment->linkedItem();
                            if (linked && (linked == item || linked == item->topLevelItem() || linked->topLevelItem() == item->topLevelItem())) {
                                QMessageBox::warning(this, "Удаление заблокировано", "К этому объекту привязан стикер!\n\nСначала отвяжите или удалите сам стикер.");
                                return;
                            }
                        }
                    }
                }
            }
            // ====================================================================

            // 1. Сортируем объекты по спискам
            QList<ConnectionItem*> arrowsToDelete;
            QList<DiagramItem*> blocksToDelete;
            QList<QGraphicsItem*> othersToDelete;

            QList<QGraphicsItem*> selected = scene()->selectedItems();

            for (QGraphicsItem *item : selected) {
                if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(item)) {
                    blocksToDelete.append(dItem);
                } else {
                    ConnectionItem* conn = nullptr;
                    // Проверяем, является ли элемент стрелкой или её частью
                    if (item->data(ConnectionRole).toBool()) {
                        conn = dynamic_cast<ConnectionItem*>(item);
                    } else if (item->parentItem() && item->parentItem()->data(ConnectionRole).toBool()) {
                         conn = dynamic_cast<ConnectionItem*>(item->parentItem());
                    } else if (item->group() && item->group()->data(ConnectionRole).toBool()) {
                         conn = dynamic_cast<ConnectionItem*>(item->group());
                    }

                    if (conn) {
                        if (!arrowsToDelete.contains(conn)) arrowsToDelete.append(conn);
                    } else {
                        othersToDelete.append(item);
                    }
                }
            }

            // 2. Сначала удаляем СТРЕЛКИ (безопасно, т.к. блоки еще живы)
            for (ConnectionItem *conn : arrowsToDelete) {
                if (conn && conn->scene()) {
                    if (conn->startBlock()) conn->startBlock()->removeConnection(conn);
                    if (conn->endBlock()) conn->endBlock()->removeConnection(conn);
                    scene()->removeItem(conn);
                    delete conn;
                }
            }

            // 3. Затем удаляем БЛОКИ
            // (стрелки уже удалены, поэтому removeAllConnections удалит только невыделенные хвосты)
            for (DiagramItem *dItem : blocksToDelete) {
                if (dItem && dItem->scene()) {
                    if (m_state) {
                        m_state->removeItem(dItem);
                    }
                    dItem->removeAllConnections();
                    // =========================================================
                                    // --- НОВОЕ: ОТВЯЗЫВАЕМ БЛОК ОТ РАМОК ПЕРЕД УДАЛЕНИЕМ ---
                                    for (QGraphicsItem *sceneItem : scene()->items()) {
                                        if (GroupItem *group = dynamic_cast<GroupItem*>(sceneItem)) {
                                            group->removeBlock(dItem); // Безопасно выписываем блок
                                        }
                                    }
                                    // =========================================================
                    scene()->removeItem(dItem);
                    delete dItem;
                }
            }

            // 4. Удаляем остальное
            for (QGraphicsItem *item : othersToDelete) {
                if (item && item->scene()) {
                    scene()->removeItem(item);
                    delete item;
                }
            }
        }
    else {
        QGraphicsView::contextMenuEvent(event);
    }
}


// Обработка колесика мыши
void DiagramView::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {   // если колесико вверх
        m_zoomController->zoomIn();
    } else {                             // если колесико вниз
        m_zoomController->zoomOut();
    }
}
// Когда зажата кнопка мыши

void DiagramView::mousePressEvent(QMouseEvent *event)
{
    // Если редактор стрелок активен, передаем управление ему
        if (m_arrowEditor && m_arrowEditor->isActive()) {
            QPointF scenePos = mapToScene(event->pos());

            // *** 1. ИЗМЕНЕНИЕ: Передаем ЛКМ и ПКМ (Шаг 5.1) ***
            if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
                m_arrowEditor->onMousePress(scenePos, event->button()); // <--- Передаем кнопку
            }
            // *** КОНЕЦ ИЗМЕНЕНИЯ (1) ***

            // Если клик был мимо ручки, редактор выключился (stop())
            if (!m_arrowEditor->isActive()) {
                event->accept();
                // Мы не вызываем QGraphicsView::mousePressEvent,
                // т.к. клик по фону используется для выключения.
            } else {
                 event->accept(); // Ручка захвачена или ПКМ обработан
            }
            return;
        }
    if (!scene()) {
        QGraphicsView::mousePressEvent(event);
        return;
    }
    if (m_connector && m_connector->wasJustStopped()) {
        m_connector->consumeJustStopped();
        event->accept();
        return;
    }

    if (m_connector && m_connector->isActive()) {
        QPointF scenePos = mapToScene(event->pos());
        m_connector->onMousePress(event->button(), scenePos);
        // Если коннектор обработал LMB/RMB и прекратил режим, можно вернуть/продолжить обычный flow.
        if (!m_connector->isActive()) {
            event->accept();
            return;
        }
        // Если остался активен — блокируем дальнейшую обработку (на выбор)
        event->accept();
        return;
    }

    // Определяем элемент прямо в координатах view (единообразно)
    QGraphicsItem *clickedItem = itemAt(event->pos());

    // Если под курсором фон (например grid помечен data(0)=1) — считаем, что элемента нет
    if (clickedItem && clickedItem->data(0).toInt() == 1)
        clickedItem = nullptr;


    // Перехватываем ПКМ на *уже выбранной* стрелке,
    // чтобы предотвратить сброс выделения базовым классом.
    if (event->button() == Qt::RightButton && clickedItem) {



        ConnectionItem* conn = nullptr;



        // 1. Клик по самой группе? (Это сработает, если кликнуть НЕ на дочерний элемент, а на 'shape' группы)
        if (clickedItem->data(ConnectionRole).toBool()) {

            conn = static_cast<ConnectionItem*>(clickedItem);
        }
        // 2. Клик по прямому потомку (линия, фон, узел)?
        else if (clickedItem->parentItem() &&
                 clickedItem->parentItem()->data(ConnectionRole).toBool()) {

            conn = static_cast<ConnectionItem*>(clickedItem->parentItem());
        }
        // 3. Клик по "внуку" (текст "if", чей родитель - фон, а "дедушка" - группа)?
        else if (clickedItem->parentItem() &&
                 clickedItem->parentItem()->parentItem() &&
                 clickedItem->parentItem()->parentItem()->data(ConnectionRole).toBool()) {

            conn = static_cast<ConnectionItem*>(clickedItem->parentItem()->parentItem());
        }




        // Если мы нашли стрелку, И она УЖЕ ВЫБРАНА:
        if (conn && conn->isSelected()) {

            // Принимаем событие, чтобы оно дошло до mouseRelease/contextMenu,
            // НО НЕ вызываем QGraphicsView::mousePressEvent ниже.
            // Это сохранит выделение.
            event->accept();
            return; // <-- Вот этот return; всё исправляет
        }

        // --- ОТЛАДКА ОШИБКИ ---
        if (conn && !conn->isSelected()) {

        } else if (!conn) {

        }
        // --- КОНЕЦ ОТЛАДКИ ---
    }



    // Если клик по пустому месту — скрываем ручки у всех DiagramItem и GroupItem
        if (!clickedItem) {
            if (event->button() == Qt::RightButton) {

            }
            for (QGraphicsItem *it : scene()->items()) {
                if (DiagramItem *d = dynamic_cast<DiagramItem*>(it)) {
                    d->hideResizeHandles();
                }
                // --- НОВОЕ: Скрываем ручки у рамки ---
                if (GroupItem *g = dynamic_cast<GroupItem*>(it)) {
                    g->setScaleMode(false);
                }
            }
        }


    // Если ПКМ по пустому месту — дополнительно скрываем точки-маячки у всех DiagramItem
    if (event->button() == Qt::RightButton && !clickedItem) {
        for (QGraphicsItem *it : scene()->items()) {
            if (DiagramItem *d = dynamic_cast<DiagramItem*>(it)) {
                d->setShowAnchorPoints(false);
            }
        }
    }

    // ЛКМ по пустому месту — начинаем панорамирование
    if (event->button() == Qt::LeftButton && !clickedItem) {
        m_panning = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    // ПКМ: если кликнули по пустому месту — начинаем rubber-band выделение правой кнопкой
    if (event->button() == Qt::RightButton && !clickedItem) {
        if (!scene()->items().isEmpty()) { // <-- предотвращаем зависание на пустой сцене
            m_prevDragMode = dragMode();
            setDragMode(QGraphicsView::RubberBandDrag);
            m_rightButtonRubberBand = true;
            QGraphicsView::mousePressEvent(event); // запускаем механизм рамки
        }
        return;
    }

    // Иначе — стандартная обработка (клик по элементу)
    if (clickedItem) {

    }
    QGraphicsView::mousePressEvent(event);
}

// --- КОНЕЦ ФРАГМЕНТА ---




// При движении мыши
void DiagramView::mouseMoveEvent(QMouseEvent *event)
{

    // Если активен редактор И мы в процессе перетаскивания ручки — перехватываем событие
    if (m_arrowEditor && m_arrowEditor->isActive() && m_arrowEditor->isDragging()) {
        QPointF scenePos = mapToScene(event->pos());
        m_arrowEditor->onMouseMove(scenePos);
        event->accept();
        return;
    }


    if (m_panning) {
        // Вычисляем смещение
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        // Сдвигаем вид по горизонтали и вертикали
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());

        event->accept();
        return;
    }
    if (m_connector && m_connector->isActive()) {
        QPointF scenePos = mapToScene(event->pos());
        m_connector->onMouseMove(scenePos);
        // поглотим событие, чтобы не конфликтовать с панорамированием
        event->accept();
        return;
    }

    // Если мы выделяем рамкой ПКМ, прокидываем событие базовому классу,
    // чтобы QGraphicsView обрабатывал и обновлял рамку выделения
    if (m_rightButtonRubberBand) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    // Стандартная обработка (включая перемещение блоков)
    QGraphicsView::mouseMoveEvent(event);

    // *** ГЛАВНОЕ ИСПРАВЛЕНИЕ: Блокируем refreshHandles в режиме Repin ***
    if (m_arrowEditor && m_arrowEditor->isActive() && !m_arrowEditor->isRepinning()) {
        m_arrowEditor->refreshHandles();
    }
}

// При отпускании кнопки мыши
void DiagramView::mouseReleaseEvent(QMouseEvent *event)
{
    // *** НАЧАЛО ИЗМЕНЕНИЯ ***
    if (m_arrowEditor && m_arrowEditor->isActive()) {
        QPointF scenePos = mapToScene(event->pos());
        m_arrowEditor->onMouseRelease(scenePos);
        event->accept();

        // --- ДАТЧИК 1: Закончили редактировать стрелку ---
        if (event->button() == Qt::LeftButton) {
            emit sceneChanged();
        }
        return;
    }


    if (event->button() == Qt::LeftButton && m_panning) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);  // возвращаем стандартный курсор
        event->accept();
        return;
    }

    // Завершаем rubber-band выделение, если оно шло с ПКМ
    if (event->button() == Qt::RightButton && m_rightButtonRubberBand) {
        // Завершаем режим рамочного выделения — сначала передадим событие,
        // чтобы QGraphicsView завершил выделение
        QGraphicsView::mouseReleaseEvent(event);

        // После того как рамка выбрала элементы, можно сразу показать контекстное меню
        // или оставить выделение для дальнейших действий. Здесь покажем контекстное меню.
        if (scene() && !scene()->selectedItems().isEmpty()) {
            QMenu menu(this);
            QAction *deleteAction = menu.addAction(tr("Удалить"));

            QAction *selectedAction = menu.exec(event->globalPos());

            if (selectedAction == deleteAction) {
                            // === БЕЗОПАСНОЕ УДАЛЕНИЕ (как в contextMenuEvent) ===
                            // ====================================================================
                            // --- ЗАЩИТА СТИКЕРОВ И СПЕЦБЛОКОВ ---
                            for (QGraphicsItem *item : scene()->selectedItems()) {
                                if (CommentItem *comment = dynamic_cast<CommentItem*>(item)) {
                                    if (comment->linkedItem()) {
                                        QMessageBox::warning(this, tr("Удаление заблокировано"), tr("Этот стикер привязан к объекту!\n\nСначала отвяжите его (✂ Отвязать)."));
                                        setDragMode(m_prevDragMode); // Сбрасываем рамку!
                                        m_rightButtonRubberBand = false;
                                        return;
                                    }
                                } else {
                                    // --- НОВОЕ: ЗАЩИТА СПЕЦБЛОКОВ ОТ УДАЛЕНИЯ СО СТРЕЛКАМИ ---
                                    if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(item)) {
                                        QString type = dItem->getType();
                                        if ((type == "История" || type == "Возврат" || type == "Мост") && dItem->hasConnections()) {
                                            QMessageBox::warning(this, tr("Удаление заблокировано"),
                                                tr("К спецблоку («%1») подключены стрелки!\n\nСначала удалите все привязанные стрелки.").arg(type));
                                            setDragMode(m_prevDragMode); // Сбрасываем рамку!
                                            m_rightButtonRubberBand = false;
                                            return;
                                        }
                                    }
                                    // ---------------------------------------------------------

                                    for (QGraphicsItem *sceneItem : scene()->items()) {
                                        if (CommentItem *comment = dynamic_cast<CommentItem*>(sceneItem)) {
                                            QGraphicsItem* linked = comment->linkedItem();
                                            if (linked && (linked == item || linked == item->topLevelItem() || linked->topLevelItem() == item->topLevelItem())) {
                                                QMessageBox::warning(this, tr("Удаление заблокировано"), tr("К этому объекту привязан стикер!\n\nСначала отвяжите или удалите сам стикер."));
                                                setDragMode(m_prevDragMode); // Сбрасываем рамку!
                                                m_rightButtonRubberBand = false;
                                                return;
                                            }
                                        }
                                    }
                                }
                            }
                            // ====================================================================

                            // 1. Разделяем объекты на списки
                            QList<ConnectionItem*> arrowsToDelete;
                            QList<DiagramItem*> blocksToDelete;
                            QList<QGraphicsItem*> othersToDelete;

                            QList<QGraphicsItem*> selected = scene()->selectedItems();

                            for (QGraphicsItem *item : selected) {
                                if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(item)) {
                                    blocksToDelete.append(dItem);
                                } else {
                                    ConnectionItem* conn = nullptr;
                                    // Проверка на стрелку или её часть
                                    if (item->data(ConnectionRole).toBool()) {
                                        conn = dynamic_cast<ConnectionItem*>(item);
                                    } else if (item->parentItem() && item->parentItem()->data(ConnectionRole).toBool()) {
                                         conn = dynamic_cast<ConnectionItem*>(item->parentItem());
                                    } else if (item->group() && item->group()->data(ConnectionRole).toBool()) {
                                         conn = dynamic_cast<ConnectionItem*>(item->group());
                                    }

                                    if (conn) {
                                        if (!arrowsToDelete.contains(conn)) arrowsToDelete.append(conn);
                                    } else {
                                        othersToDelete.append(item);
                                    }
                                }
                            }

                            // 2. Сначала удаляем СТРЕЛКИ (БЕЗОПАСНО)
                            for (ConnectionItem *conn : arrowsToDelete) {
                                if (conn && conn->scene()) {
                                    // --- ОТВЯЗКА ДЛЯ ПРЕДОТВРАЩЕНИЯ ВЫЛЕТОВ ---
                                    if (conn->startBlock()) conn->startBlock()->removeConnection(conn);
                                    if (conn->endBlock()) conn->endBlock()->removeConnection(conn);
                                    // ------------------------------------------
                                    scene()->removeItem(conn);
                                    delete conn;
                                }
                            }

                            // 3. Затем удаляем БЛОКИ
                            for (DiagramItem *dItem : blocksToDelete) {
                                if (dItem && dItem->scene()) {
                                    if (m_state) {
                                        m_state->removeItem(dItem);
                                    }
                                    dItem->removeAllConnections(); // Чистим хвосты
                                    // =========================================================
                                                                        // --- НОВОЕ: ОТВЯЗЫВАЕМ БЛОК ОТ РАМОК ПЕРЕД УДАЛЕНИЕМ ---
                                                                        for (QGraphicsItem *sceneItem : scene()->items()) {
                                                                            if (GroupItem *group = dynamic_cast<GroupItem*>(sceneItem)) {
                                                                                group->removeBlock(dItem); // Безопасно выписываем блок
                                                                            }
                                                                        }
                                                                        // =========================================================
                                    scene()->removeItem(dItem);
                                    delete dItem;
                                }
                            }

                            // 4. Удаляем остальное
                            for (QGraphicsItem *item : othersToDelete) {
                                if (item && item->scene()) {
                                    scene()->removeItem(item);
                                    delete item;
                                }
                            }
                            // === КОНЕЦ БЕЗОПАСНОГО УДАЛЕНИЯ ===

                            // --- ДАТЧИК 2: Сделали удаление через рамочное выделение ---
                            emit sceneChanged();
                        }
        }

        // Восстанавливаем прежний dragMode (что был до начала PCМ-rubber)
        setDragMode(m_prevDragMode);
        m_rightButtonRubberBand = false;
        return;
    }

    // Стандартная обработка (перемещение блоков, клики)
    QGraphicsView::mouseReleaseEvent(event);

    // --- ДАТЧИК 3: Закончили перетаскивать блок или завершили рисование новой стрелки ---
    if (event->button() == Qt::LeftButton) {
        emit sceneChanged();
    }
}
// diagramview.cpp

void DiagramView::mouseDoubleClickEvent(QMouseEvent *event)
{
    // Если редактор уже активен, двойной клик его выключает
    if (m_arrowEditor && m_arrowEditor->isActive()) {
        m_arrowEditor->stop();
        event->accept();
        return;
    }

    // Если активен Connector (рисуем новую стрелку), не реагируем
    if (m_connector && m_connector->isActive()) {
        event->accept();
        return;
    }

    QGraphicsItem *item = itemAt(event->pos());
    ConnectionItem* group = nullptr;

    if (item) {
        // --- ПОИСК ГРУППЫ СТРЕЛКИ ---
        if (item->data(ConnectionRole).toBool()) {
            group = dynamic_cast<ConnectionItem*>(item);
        }
        else if (item->parentItem() && item->parentItem()->data(ConnectionRole).toBool()) {
            group = dynamic_cast<ConnectionItem*>(item->parentItem());
        }
        else if (item->parentItem() && item->parentItem()->parentItem() &&
                 item->parentItem()->parentItem()->data(ConnectionRole).toBool()) {
             group = dynamic_cast<ConnectionItem*>(item->parentItem()->parentItem());
        }
        else if (item->group() && item->group()->data(ConnectionRole).toBool()) {
            group = dynamic_cast<ConnectionItem*>(item->group());
        }
    }

    // Если мы кликнули по стрелке (группе)
    if (group) {
        // *** ИСПРАВЛЕНИЕ: ПРОВЕРКА ПОПАДАНИЯ В ТЕКСТ ***
        // Даже если itemAt вернул группу, мы проверяем, попадает ли курсор
        // в область текстовых полей (TransitionGroup).

        if (group->transition()) {
             QPointF scenePos = mapToScene(event->pos());
             // Переводим координаты сцены в локальные координаты TransitionGroup
             QPointF transLocalPos = group->transition()->mapFromScene(scenePos);

             // Проверяем, попали ли мы в границы текстового блока
             if (group->transition()->contains(transLocalPos)) {

                 // Создаем искусственное событие для TransitionGroup
                 QGraphicsSceneMouseEvent fakeEvent(QEvent::GraphicsSceneMouseDoubleClick);
                 fakeEvent.setPos(transLocalPos);
                 fakeEvent.setScenePos(scenePos);
                 fakeEvent.setButton(event->button());
                 fakeEvent.setButtons(event->buttons());
                 fakeEvent.setModifiers(event->modifiers());

                 // Вызываем обработчик напрямую
                 group->transition()->mouseDoubleClickEvent(&fakeEvent);

                 // Если TransitionGroup принял событие (значит кликнули по if/action)
                 if (fakeEvent.isAccepted()) {
                     event->accept();
                     return; // ВЫХОДИМ, чтобы не открывать редактор стрелки
                 }
             }
        }


        // Если кликнули не по тексту, а по линии — открываем редактор стрелки
        if (m_arrowEditor) {
            m_arrowEditor->start(group);
            event->accept();
            return;
        }
    }

    // Стандартная обработка (для блоков и прочего)
    QGraphicsView::mouseDoubleClickEvent(event);
}
void DiagramView::initConnector(QGraphicsScene *scene) {
    if (!m_connector && scene) m_connector = new Connector(scene, this);
}


void DiagramView::initArrowEditor(QGraphicsScene *scene) {
    if (!m_arrowEditor && scene) m_arrowEditor = new ArrowEditor(scene, this);
}
