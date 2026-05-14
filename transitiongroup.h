/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef TRANSITIONGROUP_H
#define TRANSITIONGROUP_H

#include <QGraphicsObject>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem> // <-- Добавлено для кружка приоритета
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QLineF>

class TransitionGroup : public QGraphicsObject
{
    Q_OBJECT
public:
    enum { Type = UserType + 20 };
    int type() const override { return Type; }

    explicit TransitionGroup(QGraphicsItem *parent = nullptr);

    void updateAnchor(const QList<QGraphicsLineItem*> &segments);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setConditionText(const QString &text);
    QString conditionText() const;

    void setActionText(const QString &text);
    QString actionText() const;

    // Методы приоритета (теперь полноценные)
    void setPriority(int priority);
    int getPriority() const;

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    void updateLayout();
    void updatePriorityPosition(); // <-- Новый метод для пересчета позиции приоритета

    // ДВА отдельных фона, чтобы между ними была дырка для линии
    QGraphicsRectItem *m_condBg;   // Фон для IF
    QGraphicsRectItem *m_actionBg; // Фон для Action

    QGraphicsTextItem *m_conditionText;
    QGraphicsTextItem *m_actionText;

    // --- Элементы Приоритета (Новое) ---
    QGraphicsEllipseItem *m_priorityBg; // Кружок фона
    QGraphicsTextItem *m_priorityText;  // Цифра
    int m_priorityValue = 1;

    // Логика привязки основной группы (текста)
    int m_currentSegmentIndex = -1;
    QList<QLineF> m_cachedLines;
    qreal m_sliderPos = 0.5;
    bool m_isDragging = false;
    bool m_isVertical = false; // Флаг ориентации текущего сегмента

    // --- Логика привязки Приоритета (Новое) ---
    int m_prioSegmentIndex = -1;   // На каком сегменте приоритет
    qreal m_prioSliderPos = 0.15;   // Позиция приоритета на сегменте
    bool m_isDraggingPrio = false; // Тащим ли мы сейчас приоритет?
    qreal m_prioSideOffset = 25.0; // Смещение вбок (+/-)

    // Структура для возврата при коллизии (если налезли на текст)
    struct PrioBackup {
        int segmentIndex;
        qreal sliderPos;
        qreal sideOffset;
    } m_prioBackup;
};

#endif // TRANSITIONGROUP_H
