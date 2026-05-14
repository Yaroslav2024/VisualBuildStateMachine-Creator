/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "zoomcontroller.h"
#include "diagramitem.h"
#include <QGraphicsItem>
#include <QGraphicsScene>
ZoomController::ZoomController(QGraphicsView *view, QObject *parent)
    : QObject(parent), m_view(view), m_zoomFactor(1.0)
{
    // Конструктор сохраняет указатель на QGraphicsView
    // Инициализируем масштаб 1.0 (100%)
}

// Увеличить масштаб
void ZoomController::zoomIn()
{
    if (!m_view) return;

    m_zoomFactor *= m_step;
    if (m_zoomFactor > 5.0) m_zoomFactor = 5.0; // ограничиваем максимум

    applyZoom();

}

// Уменьшить масштаб
void ZoomController::zoomOut()
{
    if (!m_view) return;

    m_zoomFactor /= m_step;
    if (m_zoomFactor < 0.2) m_zoomFactor = 0.2; // ограничиваем минимум

    applyZoom();

}


// Установить конкретный коэффициент масштаба
void ZoomController::setZoom(qreal factor)
{
    if (!m_view) return;
    if (factor <= 0) return;              // запрещаем отрицательные или нулевые масштабы

    m_zoomFactor = factor;
    applyZoom();

}

// Получить текущий коэффициент масштаба
qreal ZoomController::currentZoom() const
{
    return m_zoomFactor;
}
void ZoomController::applyZoom()
{
    if (!m_view) return;

    m_view->resetTransform();
    m_view->scale(m_zoomFactor, m_zoomFactor);

    // ----------------- новый безопасный вариант -----------------
    // обновляем anchor points только для выбранных элементов
    for (QGraphicsItem* item : m_view->scene()->selectedItems()) {
        if (DiagramItem* dItem = dynamic_cast<DiagramItem*>(item)) {
            dItem->generateAnchorPoints();
            dItem->update();
        }
    }
    // ------------------------------------------------------------
}
