/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "commentitem.h"
#include <QPainter>
#include <QInputDialog>
#include <QCursor>
#include "diagramitem.h"
#include "connect.h"
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include "groupitem.h" // Если нужно явно проверять группы (опционально)
#include "bridgeitem.h"
#include "historyitem.h"
#include "backitem.h"
CommentItem::CommentItem(const QString &text, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), m_linkedItem(nullptr), m_generateCode(false)
{
    // 1. Настройка фона (Светло-желтый, как стикер)
    setBrush(QColor(255, 255, 200));
    setPen(QPen(Qt::gray, 1));

    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
    setZValue(10); // Поверх блоков

    // 2. Создаем текстовый объект внутри
    m_textItem = new QGraphicsTextItem(text, this);
    m_textItem->setFont(QFont("Comic Sans MS", 9)); // Или Arial
    m_textItem->setTextInteractionFlags(Qt::NoTextInteraction);

    // 3. Обновляем размер при старте
    updateGeometry();
}

void CommentItem::setText(const QString &text)
{
    m_textItem->setPlainText(text);
    updateGeometry();
}

QString CommentItem::text() const
{
    return m_textItem->toPlainText();
}

void CommentItem::setLinkedItem(QGraphicsItem *item)
{
    m_linkedItem = item;
    update(); // Перерисовать, чтобы появилась линия связи
}

QGraphicsItem* CommentItem::linkedItem() const
{
    return m_linkedItem;
}

void CommentItem::setGenerateCode(bool enable)
{
    m_generateCode = enable;
    update();
}

bool CommentItem::isGenerateCode() const
{
    return m_generateCode;
}
// --- НОВОЕ: УНИВЕРСАЛЬНЫЙ ГЕНЕРАТОР ---
QString CommentItem::getAssociatedCode(QGraphicsItem* target, const QString& prefix)
{
    if (!target || !target->scene()) return "";

    QString result;
    // Проходим по всем элементам сцены
    foreach (QGraphicsItem* item, target->scene()->items()) {
        // Ищем стикеры
        if (CommentItem* comm = dynamic_cast<CommentItem*>(item)) {
            // 1. Стикер должен быть привязан к нашему объекту (target)
            // 2. У стикера должна стоять галочка "Генерировать код"
            if (comm->linkedItem() == target && comm->isGenerateCode()) {

                // Добавляем префикс к каждой строке комментария
                QStringList lines = comm->text().split('\n');
                for (const QString& line : lines) {
                    result += prefix + line + "\n";
                }
            }
        }
    }
    return result;
}
// --------------------------------------
// АВТО-РАЗМЕР
void CommentItem::updateGeometry()
{
    // Берем размер текста
    QRectF textRect = m_textItem->boundingRect();
    // Делаем рамку чуть больше текста (padding)
    setRect(0, 0, textRect.width() + 10, textRect.height() + 10);

    // Центрируем текст внутри рамки
    m_textItem->setPos(5, 5);
}

void CommentItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 1. Если есть привязка, рисуем линию-выноску к цели
    if (m_linkedItem) {
        // --- ЗАЩИТА ---
        // Если объект удалили (в том числе Рамку Группы), его сцена изменится или станет null.
        if (m_linkedItem->scene() != this->scene()) {
            m_linkedItem = nullptr; // Разрываем связь автоматически
        }
        else {
            // Рисуем линию
            QPointF targetPos = m_linkedItem->sceneBoundingRect().center();
            QPointF localTarget = mapFromScene(targetPos);

            painter->save();
            QPen linePen(Qt::gray, 1, Qt::DotLine);
            painter->setPen(linePen);
            painter->drawLine(rect().center(), localTarget);
            painter->restore();
        }
    }

    // 2. Рисуем сам стикер (фон)
    painter->setBrush(brush());

    // --- ИЗМЕНЕНИЕ: Убрали зеленую рамку генерации кода (временно) ---
    painter->setPen(pen());
    // ----------------------------------------------------------------

    // Рисуем прямоугольник с загнутым уголком
    QRectF r = rect();
    QPolygonF poly;
    poly << r.topLeft() << r.topRight()
         << QPointF(r.right(), r.bottom() - 10) // Начало загиба
         << QPointF(r.right() - 10, r.bottom()) // Конец загиба
         << r.bottomLeft();

    painter->drawPolygon(poly);

    // Рисуем сам уголок
    QPolygonF corner;
    corner << QPointF(r.right(), r.bottom() - 10)
           << QPointF(r.right() - 10, r.bottom() - 10)
           << QPointF(r.right() - 10, r.bottom());
    painter->setBrush(QColor(240, 240, 180)); // Чуть темнее
    painter->drawPolygon(corner);

    if (isSelected()) {
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(Qt::blue, 1, Qt::DashLine));
        painter->drawRect(r);
    }
}

void CommentItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    bool ok;
    QString newText = QInputDialog::getMultiLineText(nullptr,
                                                         QObject::tr("Редактировать комментарий"),
                                                         QObject::tr("Текст:"),
                                                         m_textItem->toPlainText(),
                                                         &ok);
    if (ok) {
        setText(newText);
    }

    // --- ИЗМЕНЕНИЕ: Убрали вопрос про генерацию кода ---
    // QMessageBox::StandardButton reply = ...
    // setGenerateCode(reply == QMessageBox::Yes);
    // ---------------------------------------------------

    QGraphicsRectItem::mouseDoubleClickEvent(event);
}

QVariant CommentItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    // Если стикер двигают, нужно перерисовывать линию связи
    if (change == ItemPositionChange && scene()) {
        update();
    }
    return QGraphicsRectItem::itemChange(change, value);
}

void CommentItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QMenu menu;

    QAction *editAction = menu.addAction(QObject::tr("✏ Редактировать текст"));
    menu.addSeparator();

    // 1. Действие "Генерировать вместе с кодом"
    QAction *genAction = menu.addAction(QObject::tr("📝 Генерировать вместе с кодом"));
    genAction->setCheckable(true); // Делаем галочку
    genAction->setChecked(m_generateCode); // Ставим текущее состояние

    // --- ЛОГИКА БЛОКИРОВКИ ---
    if (!m_linkedItem) {
        genAction->setEnabled(false);
        genAction->setText(QObject::tr("📝 Генерировать (нужна привязка)"));
    }
    else if (dynamic_cast<BridgeItem*>(m_linkedItem) ||
             dynamic_cast<HistoryItem*>(m_linkedItem) ||
             dynamic_cast<BackItem*>(m_linkedItem) ||
             dynamic_cast<CommentItem*>(m_linkedItem)) {
        // Если привязан к вспомогательным блокам (Мост, История, Возврат)
        genAction->setEnabled(false);
        genAction->setChecked(false);
        genAction->setText(QObject::tr("📝 Генерировать (недоступно для спец. блоков)"));
    }
    else if (dynamic_cast<GroupItem*>(m_linkedItem)) {
        genAction->setEnabled(false);
        genAction->setChecked(false);
        genAction->setText(QObject::tr("📝 Генерировать (недоступно для групп)"));
    }
    // -------------------------

    menu.addSeparator();
    QAction *bindAction = menu.addAction(QObject::tr("🔗 Привязать к выделенному"));

    QAction *unlinkAction = nullptr;
    if (m_linkedItem) {
        unlinkAction = menu.addAction(QObject::tr("✂ Отвязать"));
    }

    // ==========================================================
    // --- ДОБАВЛЯЕМ КНОПКУ УДАЛЕНИЯ В КОНЕЦ МЕНЮ ---
    // ==========================================================
    menu.addSeparator();
    QAction* deleteAction = menu.addAction(QObject::tr("❌ Удалить стикер"));
    // ==========================================================

    QAction *selectedAction = menu.exec(event->screenPos());

    if (selectedAction == editAction) {
        bool ok;
        QString currentText = (m_textItem) ? m_textItem->toPlainText() : "";
        QString newText = QInputDialog::getMultiLineText(nullptr, QObject::tr("Редактировать"), QObject::tr("Текст:"), currentText, &ok);
        if (ok) setText(newText);
    }
    else if (selectedAction == genAction) {
        m_generateCode = genAction->isChecked();
        update();
    }
    else if (selectedAction == unlinkAction) {
        setLinkedItem(nullptr);
        m_generateCode = false;
        update();
    }
    else if (selectedAction == bindAction) {
        if (!scene()) return;
        QList<QGraphicsItem*> selected = scene()->selectedItems();
        selected.removeAll(this);

        if (selected.isEmpty()) {
            QMessageBox::information(nullptr, QObject::tr("Привязка"), QObject::tr("Сначала выделите объект (Блок, Мост или Группу), потом стикер."));
        } else {
            QGraphicsItem* target = selected.first();

            // --- ЗАПРЕТ ПРИВЯЗКИ К СТРЕЛКАМ И СТИКЕРАМ ---
                        if (dynamic_cast<ConnectionItem*>(target) || dynamic_cast<CommentItem*>(target)) {
                            QMessageBox::warning(nullptr, QObject::tr("Запрет"), QObject::tr("Стикер нельзя привязывать к стрелкам или другим стикерам!"));
                            return;
                        }

            setLinkedItem(target);

            // Сразу выключаем генерацию кода для не-кодовых блоков
            if (dynamic_cast<BridgeItem*>(target) || dynamic_cast<GroupItem*>(target) ||
                dynamic_cast<HistoryItem*>(target) || dynamic_cast<BackItem*>(target)) {
                m_generateCode = false;
                update();
            }
        }
    }
    // ==========================================================
    // *** НОВОЕ: ЛОГИКА УДАЛЕНИЯ СТИКЕРА ***
    // ==========================================================
    // ==========================================================
        // *** НОВОЕ: ЛОГИКА УДАЛЕНИЯ СТИКЕРА (СТРОГИЙ ЗАПРЕТ) ***
        // ==========================================================
        else if (selectedAction == deleteAction) {
            if (!scene()) return;

            // СТРОГИЙ ЗАПРЕТ: Если стикер привязан — вообще не даём удалить
            if (m_linkedItem) {
                QMessageBox::warning(nullptr, QObject::tr("Удаление заблокировано"),
                                     QObject::tr("Этот стикер привязан к объекту!\n\nСначала отвяжите его через меню (✂ Отвязать), чтобы безопасно удалить."));
                return; // Мгновенно прерываем попытку удаления
            }

            // Если мы дошли сюда, значит стикер висит в воздухе (не привязан)
            scene()->removeItem(this); // Убираем с экрана

            // ВАЖНО: Вместо 'delete this;' используем 'deleteLater();'
            // Это скажет Qt безопасно удалить объект из памяти сразу после завершения текущего клика.
            // Это на 100% избавит программу от вылетов при удалении!
            delete this;
        }
        // ==========================================================
    // ==========================================================
}
