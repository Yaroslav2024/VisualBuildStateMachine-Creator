/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef HISTORYITEM_H
#define HISTORYITEM_H

#include "diagramitem.h"

class HistoryItem : public DiagramItem
{
public:
    // Перечисление режимов истории
    enum HistoryMode {
        Shallow, // H  - Запоминает только текущий уровень
        Deep     // H* - Запоминает всю вложенность
    };

    HistoryItem(QGraphicsItem *parent = nullptr);

    // Уникальный тип для блока истории (BackItem был 21, сделаем 22)
    int type() const override { return UserType + 22; }

    void setHistoryMode(HistoryMode mode);
    HistoryMode historyMode() const;

    // Переопределяем геометрию под круг
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    // Генерация точек подключения (строго 4 штуки, как у круга)
    void generateAnchorPoints();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QPointF m_lastValidPos; // Хранит последнюю позицию внутри рамки
    HistoryMode m_mode;
};

#endif // HISTORYITEM_H
