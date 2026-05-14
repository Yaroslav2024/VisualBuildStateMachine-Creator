/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "bridgeitem.h"
#include <QPainter>
#include <QInputDialog>
#include <QGraphicsScene>
#include <cmath> // Для std::round
#include <QStyleOptionGraphicsItem>
#include "connect.h" // Нужно для управления стрелками (если используется список родителя)
#include <QMessageBox> // Нужен для вывода ошибок
#include <QMenu>
#include <QAction>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsView>
#include "commentitem.h"
#include "groupitem.h"

BridgeItem::BridgeItem(QGraphicsItem *parent)
    : DiagramItem(parent)
{
    // 1. Устанавливаем начальный размер 60x60 (от 0,0)
    // ВАЖНО: Мы больше не используем m_polygon.
    // Теперь размером управляет rect(), который меняется при масштабировании.
    setRect(0, 0, 60, 60);
    setType(QObject::tr("Мост")); // Устанавливаем тип сразу!
    setLabel("Bridge_" + QString::number(id())); // Даем уникальное имя по умолчанию
    // Очищаем старые точки родителя
    m_anchorPoints.clear();

    m_targetName = "";

    // Флаги: можно двигать, выделять, сообщает об изменении геометрии
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
}

void BridgeItem::setTargetName(const QString &name)
{
    m_targetName = name;
    update();
}

QString BridgeItem::targetName() const
{
    return m_targetName;
}

QRectF BridgeItem::boundingRect() const
{
    // ВАЖНО: Используем rect(), который обновляется при изменении размера ручками
    return rect().adjusted(-2, -2, 2, 2);
}

QPainterPath BridgeItem::shape() const
{
    QPainterPath path;
    // Форма для кликов тоже должна соответствовать текущему размеру
    path.addRect(rect());
    return path;
}

// --- ПРИВЯЗКА К СЕТКЕ ---
QVariant BridgeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();

        // Шаг сетки
        qreal gridSize = 16.0;

        // Так как координаты теперь от (0,0), просто округляем до ближайшего узла
        qreal x = std::round(newPos.x() / gridSize) * gridSize;
        qreal y = std::round(newPos.y() / gridSize) * gridSize;

        // Передаем в DiagramItem уже выровненные координаты
        return DiagramItem::itemChange(change, QPointF(x, y));
    }
    return DiagramItem::itemChange(change, value);
}
// ------------------------
// --- ПРИВЯЗКА РАЗМЕРА К СЕТКЕ (ПРИ ОТПУСКАНИИ РУЧКИ) ---
void BridgeItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // 1. Вызываем базовый метод (важно для снятия выделения и логики DiagramItem)
    DiagramItem::mouseReleaseEvent(event);

    // 2. Логика "Прилипания" размера
    qreal gridSize = 16.0; // Тот же шаг, что и выше

    QRectF r = rect();

    // Округляем ширину и высоту до ближайшего шага
    qreal newW = std::round(r.width() / gridSize) * gridSize;
    qreal newH = std::round(r.height() / gridSize) * gridSize;

    // Минимальный размер (1 клетка), чтобы блок не исчез
    if (newW < gridSize) newW = gridSize;
    if (newH < gridSize) newH = gridSize;

    // Если размер изменился (стал некратным), применяем корректировку
    if (!qFuzzyCompare(newW, r.width()) || !qFuzzyCompare(newH, r.height())) {

        prepareGeometryChange();

        // Сбрасываем rect в (0,0) с новой шириной/высотой.
        // Это важно, чтобы координаты внутри блока всегда были положительными.
        // (DiagramItem::resizeFromHandle мог сместить topLeft в минус).

        // Если resizeFromHandle сместил позицию (например, тянули за левый край),
        // нам нужно компенсировать это, сдвинув сам item.
        QPointF currentPos = pos();
        QPointF rectOffset = r.topLeft(); // Смещение внутри rect

        setRect(0, 0, newW, newH);

        // Сдвигаем сам объект на то место, где оказался левый верхний угол
        setPos(currentPos + rectOffset);
    }

    // В конце еще раз привязываем позицию к сетке (на случай дробных координат)
    QPointF finalPos = pos();
    qreal x = std::round(finalPos.x() / gridSize) * gridSize;
    qreal y = std::round(finalPos.y() / gridSize) * gridSize;
    if (finalPos != QPointF(x, y)) {
        setPos(x, y);
    }

    // 3. Обновляем ручки и точки
    updateHandlesPosition();

    // Очищаем и перегенерируем точки привязки под новый размер
    m_anchorPoints.clear();
    const_cast<BridgeItem*>(this)->generateAnchorPoints(); // Метод родителя пересчитает по новому rect()

    update();
}
// -----------------------------------------------------
void BridgeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Настраиваем стиль линий
    QPen pen(Qt::black, 2, Qt::DashLine);
    if (isSelected()) pen.setColor(Qt::blue);

    painter->setPen(pen);
    painter->setBrush(QColor(240, 240, 255)); // Легкий голубой оттенок

    // ВАЖНО: Рисуем прямоугольник по текущим размерам (rect), а не фиксированный полигон
    painter->drawRect(rect());

    // Рисуем текст по центру
    painter->setPen(Qt::black);
    QFont font = painter->font();

    // Центрируем текст внутри текущего прямоугольника
    QRectF textRect = rect();

    if (m_targetName.isEmpty()) {
        font.setBold(true);
        font.setPointSize(14);
        painter->setFont(font);
        painter->drawText(textRect, Qt::AlignCenter, "?");
    } else {
        font.setPointSize(8);
        painter->setFont(font);
        painter->drawText(textRect, Qt::AlignCenter, "To:\n" + m_targetName);
    }

    // Рисуем точки привязки
    if (m_showAnchorPoints) {
        painter->setBrush(Qt::red);
        painter->setPen(Qt::NoPen);

        // Если точки пустые (или размер изменился), просим родителя пересчитать их по rect()
        if (m_anchorPoints.isEmpty()) {
            const_cast<BridgeItem*>(this)->generateAnchorPoints();
        }

        // Масштабирование размера точек при зуме
        qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
        if (lod < 0.1) lod = 1.0;
        qreal visualRadius = 3.0 / lod;

        foreach (const QPointF &pt, m_anchorPoints) {
            painter->drawEllipse(pt, visualRadius, visualRadius);
        }
    }
}
// --- НОВАЯ ЛОГИКА ВАЛИДАЦИИ ---
bool BridgeItem::validateTarget(const QString &name)
{
    if (!scene()) return false;
    if (name.trimmed().isEmpty()) return true; // Пустое имя разрешаем (сброс)

    DiagramItem* foundBlock = nullptr;

    // 1. Проверяем, существует ли такой блок
    foreach (QGraphicsItem *item, scene()->items()) {
        // Ищем среди обычных блоков (не мостов)
        if (DiagramItem *block = dynamic_cast<DiagramItem*>(item)) {
            if (dynamic_cast<BridgeItem*>(block)) continue; // Игнорируем другие мосты

            // Сравниваем описание или метку (что вы используете как имя)
            // Лучше использовать точное совпадение
            if (block->getDescription() == name || block->getLabel() == name) {
                foundBlock = block;
                break;
            }
        }
    }

    if (!foundBlock) {
        QMessageBox::warning(nullptr, "Ошибка адресации",
                             "Блок с именем \"" + name + "\" не найден!\n"
                             "Проверьте правильность написания (Description/Label).");
        return false;
    }

    // 2. Проверка на РЕКУРСИЮ (Петля)
    // Ищем стрелку, которая идет ОТ найденного блока К ЭТОМУ мосту
    foreach (QGraphicsItem *item, scene()->items()) {
        if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
            // Если стрелка выходит из найденного блока И входит в этот мост
            if (conn->startBlock() == foundBlock && conn->endBlock() == this) {
                QMessageBox::critical(nullptr, "Обнаружена рекурсия",
                                      "Нельзя ссылаться на блок \"" + name + "\",\n"
                                      "так как от него уже идет стрелка к этому мосту!\n"
                                      "Это создаст бесконечный цикл.");
                return false;
            }
        }
    }

    return true;
}
// ------------------------------
void BridgeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    bool ok;
    QString text = QInputDialog::getText(nullptr, QObject::tr("Настройка Моста"),
                                         QObject::tr("Введите точное Имя целевого блока:"), QLineEdit::Normal, m_targetName, &ok);
    if (ok) {
        if (validateTarget(text)) {
            setTargetName(text);
        }
        // Если validateTarget вернет false (ошибка), имя не изменится
        // и QMessageBox покажется внутри функции validateTarget.
    }
}
// ==========================================================
// --- КОНТЕКСТНОЕ МЕНЮ ДЛЯ МОСТА (ПКМ) ---
// ==========================================================
void BridgeItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    // --- 1. ОПРЕДЕЛЯЕМ, В КАКОМ ОКНЕ МЫ КЛИКНУЛИ ---
    bool inDebugger = false;
    QGraphicsView *currentView = nullptr;

    if (event->widget()) {
        currentView = qobject_cast<QGraphicsView*>(event->widget()->parentWidget());
        if (event->widget()->window()->objectName() == "DebuggerWindow" ||
            event->widget()->window()->windowTitle().contains("Дебагер")) {
            inDebugger = true;
        }
    }

    QMenu menu;
    QAction *goToAct = menu.addAction(QObject::tr("Перейти по адресу блока"));
    QAction *scaleAct = menu.addAction(QObject::tr("Масштабировать"));
    QAction *deleteAct = menu.addAction(QObject::tr("Удалить"));

    // --- 2. ЗАЩИТА В ДЕБАГГЕРЕ ---
    if (inDebugger) {
        scaleAct->setEnabled(false);  // Запрещаем менять размер
        deleteAct->setEnabled(false); // Запрещаем удалять мост
    }

    // Показываем меню и ждем выбора пользователя
    QAction *selectedAction = menu.exec(event->screenPos());

    if (selectedAction == goToAct) {
        if (m_targetName.isEmpty()) {
            QMessageBox::warning(nullptr, "Ошибка", "Адрес блока не указан!\nДважды кликните по мосту, чтобы задать адрес.");
            return;
        }

        DiagramItem *targetBlock = nullptr;
        // Ищем блок на сцене
        for (QGraphicsItem *item : scene()->items()) {
            if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
                if (b->getLabel() == m_targetName || b->getDescription() == m_targetName) {
                    targetBlock = b;
                    break;
                }
            }
        }

        // Если нашли — прыгаем к нему
        if (targetBlock) {
            // --- ИСПРАВЛЕНИЕ: ДВИГАЕМ ПРАВИЛЬНУЮ КАМЕРУ ---
            if (currentView) {
                currentView->centerOn(targetBlock); // Двигаем ту камеру, где кликнули
            } else if (!scene()->views().isEmpty()) {
                scene()->views().first()->centerOn(targetBlock); // Резервный вариант
            }
            scene()->clearSelection();          // Снимаем старое выделение
            targetBlock->setSelected(true);     // Выделяем найденный блок, чтобы он подсветился
        } else {
            // Если не нашли
            QMessageBox::warning(nullptr, "Ошибка", "Блок с адресом \"" + m_targetName + "\" не найден!\nПроверьте правильность написания.");
        }
    }
    // --- ОБРАБОТКА МАСШТАБИРОВАНИЯ ---
    else if (selectedAction == scaleAct) {
        showResizeHandles(); // Вызываем точки для изменения размера
    }
    // ---------------------------------
    else if (selectedAction == deleteAct) {
        // --- ЗАЩИТА: ПРОВЕРКА НАЛИЧИЯ СТРЕЛОК ---
        if (!m_connections.isEmpty()) {
            QMessageBox::warning(nullptr, QObject::tr("Удаление заблокировано"),
                QObject::tr("К этому мосту подключены стрелки!\n\nСначала удалите все привязанные стрелки."));
            return; // Отменяем удаление
        }
        // ==========================================================

        // --- ЗАЩИТА: ПРОВЕРКА ПРИВЯЗАННОГО СТИКЕРА ---
        if (scene()) {
            for (QGraphicsItem *sceneItem : scene()->items()) {
                if (CommentItem *comment = dynamic_cast<CommentItem*>(sceneItem)) {
                    QGraphicsItem* linked = comment->linkedItem();
                    // Сравниваем привязку с текущим объектом (this)
                    if (linked && (linked == this || linked == this->topLevelItem() || linked->topLevelItem() == this->topLevelItem())) {
                        QMessageBox::warning(nullptr, QObject::tr("Удаление заблокировано"),
                            QObject::tr("К этому мосту привязан стикер-комментарий!\n\nСначала отвяжите или удалите сам стикер."));
                        return; // Мгновенно отменяем удаление!
                    }
                }
            }
        }
        // ==========================================================

        // --- 1. АВТООЧИСТКА: Безопасно удаляем и отвязываем все стрелки ---
        removeAllConnections();

        // --- НОВОЕ: ОТВЯЗЫВАЕМ БЛОК ОТ РАМОК ПЕРЕД УДАЛЕНИЕМ ---
        if (scene()) {
            for (QGraphicsItem *sceneItem : scene()->items()) {
                if (GroupItem *group = dynamic_cast<GroupItem*>(sceneItem)) {
                    group->removeBlock(this); // Безопасно выписываем Мост
                }
            }
        }
        // =========================================================

        // --- 2. Убираем сам Мост со сцены ---
        scene()->removeItem(this);
        delete this;
    }
}
