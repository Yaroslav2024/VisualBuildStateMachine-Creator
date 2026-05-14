/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef GROUPITEM_H
#define GROUPITEM_H

#include <QGraphicsRectItem>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include "diagramitem.h"
#include <QGraphicsSceneHoverEvent>
#include <QCursor>
class GroupItem : public QGraphicsRectItem
{
public:
    GroupItem(const QRectF &rect, const QString &name, QGraphicsItem *parent = nullptr);
    //  Режим масштабирования ---
        void setScaleMode(bool enabled);
        bool isScaleMode() const;

    void setName(const QString &name);
    QString name() const;

    // Тип для идентификации на сцене (чтобы отличать от блоков)
    enum { Type = UserType + 5 };
    int type() const override { return Type; }
    void addBlock(DiagramItem* block);      // Добавить блок в семью
    void removeBlock(DiagramItem* block);   // Удалить блок из семьи
        bool hasBlock(DiagramItem* block);      // Проверка "свой/чужой"
        void updateGeometry();                  // Пересчитать рамку под блоки

        // --- НОВОЕ: Геттер и сеттер для заморозки ---
        bool isSizeLocked() const { return m_isSizeLocked; }
        void setSizeLocked(bool locked) { m_isSizeLocked = locked; }
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
        void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
private:
        // --- НОВОЕ: Переменная для заморозки ---
        bool m_isSizeLocked = false;
        bool m_isScaleMode = false;
        // --- Переменные для изменения размера ---
            enum ResizeState {
                None, Top, Bottom, Left, Right,
                TopLeft, TopRight, BottomLeft, BottomRight
            };
            ResizeState m_resizeState = None;
            bool m_isResizing = false;
            const qreal m_margin = 10.0; // Зона захвата мышью в пикселях
            ResizeState getResizeStateAt(const QPointF &pos);
    QString m_name;
    QColor m_color;
    QList<DiagramItem*> m_blocks;
};

#endif // GROUPITEM_H
