/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef BACKITEM_H
#define BACKITEM_H

#include "diagramitem.h"

class BackItem : public DiagramItem
{
public:
    // Перечисление режимов возврата
    enum BackMode {
        Back,       // Простой возврат назад (OLD)
        BackReturn  // Возврат в указанный блок
    };

    BackItem(QGraphicsItem *parent = nullptr);

    // Уникальный тип для нового блока (Мост был 20, этот будет 21)
    int type() const override { return UserType + 21; }

    void setTargetName(const QString &name);
    QString targetName() const;

    void setBackMode(BackMode mode);
    BackMode backMode() const;

    // Переопределяем геометрию под круг
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    // Своя генерация точек подключения (строго 4 штуки)
    void generateAnchorPoints();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
private:
    BackMode m_mode;
    QString m_targetName;
    bool validateTarget(const QString &name);
};

#endif // BACKITEM_H
