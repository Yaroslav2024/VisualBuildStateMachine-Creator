/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef ZOOMCONTROLLER_H
#define ZOOMCONTROLLER_H

#include <QObject>
#include <QGraphicsView>

class ZoomController : public QObject
{
    Q_OBJECT
public:
    explicit ZoomController(QGraphicsView *view, QObject *parent = nullptr);

    // Увеличить масштаб
    void zoomIn();

    // Уменьшить масштаб
    void zoomOut();

    // Установить конкретный коэффициент масштаба
    void setZoom(qreal factor);

    // Получить текущий коэффициент масштаба
    qreal currentZoom() const;
 void applyZoom();
private:
    QGraphicsView *m_view;   // Виджет, на котором масштабируем сцену
    qreal m_zoomFactor;      // Текущий коэффициент масштаба
    const qreal m_step = 1.1; // Шаг масштабирования (например 10%)
};

#endif // ZOOMCONTROLLER_H
