/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef BRIDGEITEM_H
#define BRIDGEITEM_H

#include "diagramitem.h"
#include <QPolygonF>

class BridgeItem : public DiagramItem
{
public:
    BridgeItem(QGraphicsItem *parent = nullptr);

    int type() const override { return UserType + 20; }

    void setTargetName(const QString &name);
    QString targetName() const;

    // Геометрия
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    // Привязка к сетке
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
private:
    QString m_targetName;
    bool validateTarget(const QString &name);
};

#endif // BRIDGEITEM_H
