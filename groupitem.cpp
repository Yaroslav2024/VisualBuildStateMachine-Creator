/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "groupitem.h"
#include <QPainter>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneHoverEvent>
#include <QCursor>
#include <cmath>
GroupItem::GroupItem(const QRectF &rect, const QString &name, QGraphicsItem *parent)
    : QGraphicsRectItem(rect, parent), m_name(name)
{
    // 1. ВНЕШНИЙ ВИД
        // Пунктирная линия (DashLine) черного цвета
        QPen pen(Qt::black);
        pen.setStyle(Qt::DashLine);
        pen.setWidth(2);
        setPen(pen);

        // Полупрозрачный фон (чтобы видеть сетку под ним, но понимать границы)
        // Alpha = 50 (из 255) — очень легкий фон
        setBrush(QColor(200, 200, 255, 50));

        // 2. СЛОИ (Z-Value)
        // Ставим -100. Это гарантирует, что рамка будет ПОД блоками (у которых Z обычно 0 или выше)
        // и ПОД стрелками.
        setZValue(-100);

        // 3. ФЛАГИ
        // ItemIsSelectable -> Позволяет выделить рамку кликом (нужно для удаления!)
        // ItemIsMovable -> Позволяет таскать группу мышкой (вместе с блоками или отдельно)
        // Добавляем ItemSendsGeometryChanges и включаем отслеживание наведения мыши
            setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges);
            setAcceptHoverEvents(true);
}

void GroupItem::setName(const QString &name)
{
    m_name = name;
    update();
}

QString GroupItem::name() const
{
    return m_name;
}

void GroupItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Рисуем сам прямоугольник (рамку и фон)
    QGraphicsRectItem::paint(painter, option, widget);

    // Рисуем название группы в левом верхнем углу
    painter->setPen(Qt::black);
    painter->setFont(QFont("Arial", 10, QFont::Bold));

    // Рисуем текст с небольшим отступом (5px), чтобы не прилипал к линии
    painter->drawText(rect().adjusted(5, 5, -5, -5), Qt::AlignLeft | Qt::AlignTop, m_name);

    // (Опционально) Если рамка выделена, можно подсветить её дополнительно
    if (isSelected()) {
        painter->setPen(QPen(Qt::blue, 1, Qt::DotLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect());
    }
    // Рисуем скобки/квадратики масштабирования, если режим включен
        if (m_isScaleMode) {
            painter->setPen(QPen(Qt::black, 1, Qt::SolidLine));
            painter->setBrush(Qt::white);
            qreal s = 6.0; // Размер квадратика
            QRectF r = rect();

            // 8 ручек-квадратиков по углам и центрам
            painter->drawRect(QRectF(r.left() - s/2, r.top() - s/2, s, s));
            painter->drawRect(QRectF(r.right() - s/2, r.top() - s/2, s, s));
            painter->drawRect(QRectF(r.left() - s/2, r.bottom() - s/2, s, s));
            painter->drawRect(QRectF(r.right() - s/2, r.bottom() - s/2, s, s));

            painter->drawRect(QRectF(r.center().x() - s/2, r.top() - s/2, s, s));
            painter->drawRect(QRectF(r.center().x() - s/2, r.bottom() - s/2, s, s));
            painter->drawRect(QRectF(r.left() - s/2, r.center().y() - s/2, s, s));
            painter->drawRect(QRectF(r.right() - s/2, r.center().y() - s/2, s, s));
        }
}

// --- НОВОЕ: ЛОГИКА ПЕРЕИМЕНОВАНИЯ ---
void GroupItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    // Открываем стандартное диалоговое окно
    bool ok;
    QString text = QInputDialog::getText(nullptr, QObject::tr("Переименовать группу"),
                                             QObject::tr("Новое имя группы:"), QLineEdit::Normal,
                                             m_name, &ok);

    // Если пользователь нажал OK и текст не пустой -> меняем имя
    if (ok && !text.isEmpty()) {
        setName(text);
    }

    // Важно вызвать базовый метод, чтобы событие не "застряло"
    QGraphicsRectItem::mouseDoubleClickEvent(event);
}
// --- ДОБАВЛЕНИЕ БЛОКА ---
void GroupItem::addBlock(DiagramItem* block) {
    if (!m_blocks.contains(block)) {
        m_blocks.append(block);
        updateGeometry(); // Сразу расширяемся!
    }
}

// --- ПРОВЕРКА ---
bool GroupItem::hasBlock(DiagramItem* block) {
    return m_blocks.contains(block);
}

// --- УМНОЕ РАСШИРЕНИЕ ---
void GroupItem::updateGeometry() {
    // ====================================================================

    if (m_isSizeLocked) return; // 1. Локальная заморозка (рамка игнорирует блоки)

    bool isAutoExpandOff = false; // <---  чтение вашей настройки (Off)!
    if (isAutoExpandOff) return;  // 2. Глобально выключено -> не расширяется автоматически
    // ====================================================================

    if (m_blocks.isEmpty()) return;

    QRectF newRect;
    bool first = true;

    // 1. Считаем общую площадь всех своих блоков
    foreach (DiagramItem* b, m_blocks) {
        // Проверяем, жив ли блок (на всякий случай)
        if (b->scene()) {
            if (first) {
                newRect = b->sceneBoundingRect();
                first = false;
            } else {
                newRect = newRect.united(b->sceneBoundingRect());
            }
        }
    }

    // 2. Добавляем отступы (Padding), чтобы было красиво
    if (!newRect.isEmpty()) {
        newRect.adjust(-20, -30, 20, 20);
        setRect(newRect); // Меняем размер рамки
    }
}
// --- УДАЛЕНИЕ БЛОКА ИЗ ГРУППЫ ---
void GroupItem::removeBlock(DiagramItem* block) {
    // Если блок был в списке, удаляем его
    if (m_blocks.removeAll(block) > 0) {
        // Обязательно пересчитываем размер рамки без этого блока!
        updateGeometry();
    }
}


GroupItem::ResizeState GroupItem::getResizeStateAt(const QPointF &pos)
{
    if (!m_isScaleMode) return None;
    // ====================================================================
    // ЛОГИКА РУЧНОГО ИЗМЕНЕНИЯ РАЗМЕРА:
    bool isAutoExpandOff = false; // <--- ЗАМЕНИТЕ на чтение вашей настройки (Off)!

    // Если Авторасширение ВКЛЮЧЕНО (!isAutoExpandOff) И рамка НЕ заморожена -> отключаем ручной ресайз мышкой
    if (!isAutoExpandOff && !m_isSizeLocked) {
        return None;
    }
    // ====================================================================

    QRectF r = rect();
    bool left = (pos.x() >= r.left() && pos.x() <= r.left() + m_margin);
    bool right = (pos.x() >= r.right() - m_margin && pos.x() <= r.right());
    bool top = (pos.y() >= r.top() && pos.y() <= r.top() + m_margin);
    bool bottom = (pos.y() >= r.bottom() - m_margin && pos.y() <= r.bottom());

    if (top && left) return TopLeft;
    if (top && right) return TopRight;
    if (bottom && left) return BottomLeft;
    if (bottom && right) return BottomRight;
    if (top) return Top;
    if (bottom) return Bottom;
    if (left) return Left;
    if (right) return Right;

    return None;
}

void GroupItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    ResizeState state = getResizeStateAt(event->pos());
    switch (state) {
        case TopLeft:
        case BottomRight: setCursor(Qt::SizeFDiagCursor); break;
        case TopRight:
        case BottomLeft: setCursor(Qt::SizeBDiagCursor); break;
        case Left:
        case Right: setCursor(Qt::SizeHorCursor); break;
        case Top:
        case Bottom: setCursor(Qt::SizeVerCursor); break;
        default: setCursor(Qt::ArrowCursor); break;
    }
    QGraphicsRectItem::hoverMoveEvent(event);
}

void GroupItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_resizeState = getResizeStateAt(event->pos());
        if (m_resizeState != None) {
            m_isResizing = true;
            event->accept();
            return;
        }
    }
    QGraphicsRectItem::mousePressEvent(event);
}

void GroupItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_isResizing) {
        prepareGeometryChange();
        QRectF r = rect();
        QPointF pos = event->pos();
        const qreal minSize = 40.0; // Минимальный размер рамки

        switch (m_resizeState) {
            case TopLeft: r.setTopLeft(pos); break;
            case TopRight: r.setTopRight(pos); break;
            case BottomLeft: r.setBottomLeft(pos); break;
            case BottomRight: r.setBottomRight(pos); break;
            case Top: r.setTop(pos.y()); break;
            case Bottom: r.setBottom(pos.y()); break;
            case Left: r.setLeft(pos.x()); break;
            case Right: r.setRight(pos.x()); break;
            default: break;
        }

        if (r.width() > minSize && r.height() > minSize) {
            setRect(r);
        }
        event->accept();
    } else {
        QGraphicsRectItem::mouseMoveEvent(event);
    }
}

void GroupItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_isResizing) {
        m_isResizing = false;
        m_resizeState = None;

        // Привязка к сетке (16x16) после отпускания мыши
        qreal step = 16.0;
        QRectF r = rect();
        qreal newX = std::round(r.x() / step) * step;
        qreal newY = std::round(r.y() / step) * step;
        qreal newW = std::round(r.width() / step) * step;
        qreal newH = std::round(r.height() / step) * step;

        prepareGeometryChange();
        setRect(newX, newY, newW, newH);
        event->accept();
    } else {
        QGraphicsRectItem::mouseReleaseEvent(event);
    }
}
void GroupItem::setScaleMode(bool enabled) {
    if (m_isScaleMode != enabled) {
        m_isScaleMode = enabled;
        update(); // Перерисовываем, чтобы показать/скрыть квадратики
    }
}

bool GroupItem::isScaleMode() const {
    return m_isScaleMode;
}
