/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef RESIZEHANDLE_H
#define RESIZEHANDLE_H


#include <QGraphicsRectItem>
#include <QCursor>

class DiagramItem;

class ResizeHandle : public QGraphicsRectItem
{
public:
    enum Position {
        Left,
        Right,
        Top,
        Bottom,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };
    void updatePosition(); // пересчет позиции ручки

    ResizeHandle(Position pos, DiagramItem *owner, QGraphicsItem *parent = nullptr);

protected:
    // Перехватываем события мыши на ручке
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    // НОВАЯ ФУНКЦИЯ: Ограничивает dx и dy, если они упираются в подключенные точки
        void restrictToAnchors(qreal &dx, qreal &dy, const QRectF &r);
    QRectF m_startOccupied;
    QPointF m_ownerStartPos;
    Position m_pos;
    DiagramItem *m_owner;
    bool m_dragging = false;
    QPointF m_pressScenePos;
    QRectF m_ownerStartRect;
};
#endif // RESIZEHANDLE_H

