/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "resizehandle.h"
#include "diagramitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QBrush>
#include <QPen>
#include <QDebug>
#include <QDebug>
ResizeHandle::ResizeHandle(Position pos, DiagramItem *owner, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), m_pos(pos), m_owner(owner)
{
    // Небольшой квадрат 8x8, центрируем относительно (0,0) ручки
    setRect(-4, -4, 8, 8);
    setBrush(QBrush(Qt::white));
    setPen(QPen(Qt::black));
    setZValue(1000); // поверх всего
    setAcceptHoverEvents(true);
    // Ручка сама не двигается, она двигает владельца
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);

    // Устанавливаем курсор в зависимости от положения ручки
    switch (m_pos) {
    case Left: case Right:
        setCursor(Qt::SizeHorCursor); break;
    case Top: case Bottom:
        setCursor(Qt::SizeVerCursor); break;
    case TopLeft: case BottomRight:
        setCursor(Qt::SizeFDiagCursor); break;
    case TopRight: case BottomLeft:
        setCursor(Qt::SizeBDiagCursor); break;
    }
}

void ResizeHandle::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    // можно подсветить ручку — не обязательно
    setBrush(QBrush(Qt::lightGray));
}

void ResizeHandle::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event);
    setBrush(QBrush(Qt::white));
}

void ResizeHandle::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_owner) return;
    m_dragging = true;
    m_pressScenePos = event->scenePos();

    event->accept();
}

// В файле resizehandle.cpp

// В файле resizehandle.cpp

// В файле resizehandle.cpp

void ResizeHandle::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!m_dragging || !m_owner) return;

    QPointF currPos = event->scenePos();
    QPointF delta = currPos - m_pressScenePos;

    QRectF r = m_owner->rect();
    qreal dx = delta.x();
    qreal dy = delta.y();

    // ВАЖНО: restrictToAnchors УДАЛЕНА! Больше никаких взрывов размера.

    // 1. Применяем ограничения по минимальному размеру блока
    const qreal minW = 40.0;
    const qreal minH = 24.0;

    if (m_pos == Left || m_pos == TopLeft || m_pos == BottomLeft) {
        dx = qMin(dx, r.width() - minW);
    } else if (m_pos == Right || m_pos == TopRight || m_pos == BottomRight) {
        dx = qMax(dx, -(r.width() - minW));
    }

    if (m_pos == Top || m_pos == TopLeft || m_pos == TopRight) {
        dy = qMin(dy, r.height() - minH);
    } else if (m_pos == Bottom || m_pos == BottomLeft || m_pos == BottomRight) {
        dy = qMax(dy, -(r.height() - minH));
    }

    // 2. Вычисляем смещение и новый размер
    QPointF posShift(0,0);

    switch (m_pos) {
    case Left:   r.setWidth(r.width() - dx); posShift.setX(dx); break;
    case Right:  r.setWidth(r.width() + dx); break;
    case Top:    r.setHeight(r.height() - dy); posShift.setY(dy); break;
    case Bottom: r.setHeight(r.height() + dy); break;
    case TopLeft:
        r.setWidth(r.width() - dx); r.setHeight(r.height() - dy);
        posShift.setX(dx); posShift.setY(dy); break;
    case TopRight:
        r.setWidth(r.width() + dx); r.setHeight(r.height() - dy);
        posShift.setY(dy); break;
    case BottomLeft:
        r.setWidth(r.width() - dx); r.setHeight(r.height() + dy);
        posShift.setX(dx); break;
    case BottomRight:
        r.setWidth(r.width() + dx); r.setHeight(r.height() + dy); break;
    }

    // 3. Финализируем изменения
    QRectF oldRect = m_owner->rect(); // Запоминаем старый размер

    if (!posShift.isNull()) {
        m_owner->setPos(m_owner->pos() + posShift);
        // ВАЖНО: shiftAnchors УДАЛЕНА!
    }

    m_owner->setRect(0, 0, r.width(), r.height());
    m_owner->updateHandlesPosition();

    // 4. Запускаем нашу умную математику, которая сдвинет якоря куда надо!
    m_owner->updateConnectionsAfterResize(oldRect, m_owner->rect(), posShift);

    m_pressScenePos = currPos;
    event->accept();
}



void ResizeHandle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragging = false;
    // После того как пользователь отпустил ручку — вызываем финализацию resize у владельца,
        // чтобы выполнить snap к сетке один раз.
    if (m_owner) {
            m_owner->finalizeResize(); // snap к сетке, пересчет rect

        }
}
void ResizeHandle::updatePosition()
{
    if (!m_owner) return;

    QRectF r = m_owner->rect();
    QRectF hRect = rect(); // размеры ручки (обычно -4,-4, 8,8)

    // Центр ручки должен быть в нужной точке границы владельца.
    // Так как ручка имеет rect(-4,-4, 8,8), её "центр" — это (0,0) в локальных координатах ручки.
    // Поэтому setPos(x,y) поместит центр ручки в точку (x,y) родителя.

    QPointF newPos;
    switch (m_pos) {
    case Left:       newPos = QPointF(r.left(), r.center().y()); break;
    case Right:      newPos = QPointF(r.right(), r.center().y()); break;
    case Top:        newPos = QPointF(r.center().x(), r.top()); break;
    case Bottom:     newPos = QPointF(r.center().x(), r.bottom()); break;
    case TopLeft:    newPos = r.topLeft(); break;
    case TopRight:   newPos = r.topRight(); break;
    case BottomLeft: newPos = r.bottomLeft(); break;
    case BottomRight:newPos = r.bottomRight(); break;
    }

    setPos(newPos);
}
// --- НОВАЯ ФУНКЦИЯ ---
void ResizeHandle::restrictToAnchors(qreal &dx, qreal &dy, const QRectF &r)
{
    QRectF occ = m_owner->getOccupiedAnchorRect();

    // ОТЛАДКА: смотрим, что вернул getOccupiedAnchorRect
    if (!occ.isNull()) {
         // Раскомментируйте следующую строку, если нужно видеть каждое движение (может быть много спама)

    } else {
         // Если rect null, значит блок думает, что связей нет

    }

    if (occ.isNull()) return;

    switch (m_pos) {
    case Left:
        // Было: dx = 5, occ.left() = 20. Стало: dx = min(5, 20) = 5. OK.
        // Было: dx = 25, occ.left() = 20. Стало: dx = min(25, 20) = 20. LIMIT!
        if (dx > occ.left()) {          
            dx = occ.left();
        }
        break;
    case Right:
        // dx отрицательный при уменьшении. occ.right() < r.width().
        // Лимит: -(r.width() - occ.right())
        if (dx < -(r.width() - occ.right())) {
             qDebug() << " -> [LIMIT RIGHT] dx wanted:" << dx << " limit:" << -(r.width() - occ.right());
             dx = -(r.width() - occ.right());
        }
        break;
    case Top:
        if (dy > occ.top()) {
            qDebug() << " -> [LIMIT TOP] dy wanted:" << dy << " limit:" << occ.top();
            dy = occ.top();
        }
        break;
    case Bottom:
        if (dy < -(r.height() - occ.bottom())) {
            qDebug() << " -> [LIMIT BOTTOM] dy wanted:" << dy << " limit:" << -(r.height() - occ.bottom());
            dy = -(r.height() - occ.bottom());
        }
        break;
    // Для угловых ручек логика аналогична, можно добавить логи и туда, если нужно
    case TopLeft:
        dx = qMin(dx, occ.left());
        dy = qMin(dy, occ.top());
        break;
    case TopRight:
        dx = qMax(dx, -(r.width() - occ.right()));
        dy = qMin(dy, occ.top());
        break;
    case BottomLeft:
        dx = qMin(dx, occ.left());
        dy = qMax(dy, -(r.height() - occ.bottom()));
        break;
    case BottomRight:
        dx = qMax(dx, -(r.width() - occ.right()));
        dy = qMax(dy, -(r.height() - occ.bottom()));
        break;
    }
}
