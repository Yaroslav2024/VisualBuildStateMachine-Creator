/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef GRID_H
#define GRID_H

#include <QGraphicsItem>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QGraphicsScene>

// Класс Grid — рисует сетку на сцене
// Наследуется от QGraphicsItem, чтобы быть частью QGraphicsScene
class Grid : public QGraphicsItem
{
public:

    // Возвращает шаг сетки
    qreal getStep() const { return m_step; }

    // Привязать точку к сетке (в логических единицах, без учёта пикселей)
    QPointF snapToGrid(const QPointF &pos) const {
        qreal x = std::round(pos.x() / m_step) * m_step;
        qreal y = std::round(pos.y() / m_step) * m_step;
        return QPointF(x, y);
    }

    // Привязать прямоугольник к сетке (округляет top-left и bottom-right)
    QRectF snapRectToGrid(const QRectF &rect) const {
        QPointF topLeft = snapToGrid(rect.topLeft());
        QPointF bottomRight = snapToGrid(rect.bottomRight());
        return QRectF(topLeft, bottomRight);
    }



    // Конструктор. step — шаг сетки в логических единицах, scale — коэффициент масштабирования
    Grid(qreal step = 16, QGraphicsItem* parent = nullptr)
        : QGraphicsItem(parent), m_step(step), m_scale(1.0)
    {
        setZValue(-100); // Рисуем сетку под всеми элементами
        updateSceneRect();
    }

    // Устанавливаем коэффициент масштаба (например, при zoom)
    void setScaleFactor(qreal scale) {
        m_scale = scale;
        update(); // Перерисовываем сетку
    }

    // Устанавливаем шаг сетки в логических единицах
    void setStep(qreal step) {
        m_step = step;
        update();
    }

    // Обновление прямоугольника сцены (например, при изменении размеров)
    void updateSceneRect() {
        if (scene()) {
            m_sceneRect = scene()->sceneRect();
        } else {
            m_sceneRect = QRectF(-10000, -10000, 20000, 20000); // Большой прямоугольник по умолчанию
        }
        update();
    }

    // Определяем область, которую занимает сетка. Для производительности возвращаем сценическую область
    QRectF boundingRect() const override {
        return m_sceneRect;
    }

protected:
    // Основной метод рисования сетки
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override
    {
        Q_UNUSED(widget);
        Q_UNUSED(option);

        painter->save();

        // Настраиваем стиль линии: тонкая серая линия
        QPen pen(Qt::lightGray);
        pen.setWidth(0); // тонкая линия 0 означает "в масштабе с экраном"
        painter->setPen(pen);

        // Шаг сетки с учетом текущего масштаба
        qreal stepPx = m_step * m_scale;

        if (stepPx < 4) {
            // Если слишком мелко, не рисуем
            painter->restore();
            return;
        }

        QRectF rect = m_sceneRect;

        // Вертикальные линии
        qreal xStart = std::floor(rect.left() / stepPx) * stepPx;
        qreal xEnd = rect.right();
        for (qreal x = xStart; x <= xEnd; x += stepPx) {
            painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
        }

        // Горизонтальные линии
        qreal yStart = std::floor(rect.top() / stepPx) * stepPx;
        qreal yEnd = rect.bottom();
        for (qreal y = yStart; y <= yEnd; y += stepPx) {
            painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
        }

        painter->restore();
    }

private:
    qreal m_step;       // Шаг сетки в логических единицах
    qreal m_scale;      // Масштаб (коэффициент zoom)
    QRectF m_sceneRect; // Прямоугольник сцены, на который растянута сетка
};

#endif // GRID_H
