/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef DIAGRAMVIEW_H
#define DIAGRAMVIEW_H

#include <QGraphicsView>   // Базовый класс для виджета отображения сцены QGraphicsScene
#include <QKeyEvent>       // Для обработки событий клавиатуры
#include <QGraphicsScene>  // Для работы с графической сценой
#include <QGraphicsView>
#include "zoomcontroller.h"
#include "state.h"
#include <QScrollBar>
#include "connect.h" // добавить include

// Класс DiagramView — виджет для отображения и взаимодействия с диаграммой
// Наследуется от QGraphicsView, что позволяет просматривать и управлять QGraphicsScene
class Connector; // forward declaration для приватного поля
class ArrowEditor;
class DiagramView: public QGraphicsView
{
    Q_OBJECT
signals:
    void sceneChanged(); // <--- ДАТЧИК ИСТОРИИ
public:
    // Инициализировать connector после того, как scene установлена
    void initConnector(QGraphicsScene *scene);
void initArrowEditor(QGraphicsScene *scene);
    // Конструктор класса. parent — родительский виджет (по умолчанию nullptr)
    explicit DiagramView(QWidget *parent = nullptr);
    // Метод для установки объекта состояния
        void setState(State *s) { m_state = s; }
protected:
    // Обработчик нажатий клавиш
    // Переопределяется метод из QGraphicsView для обработки действий клавиатурой (например, удаление элементов)
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override; // для масштабирования колесом

    // Обработчик контекстного меню (обычно вызывается правой кнопкой мыши)
    // Позволяет показывать меню с действиями для элементов сцены
    void contextMenuEvent(QContextMenuEvent *event) override;
    // Для реализации панорамирования и выделения мышью:
    void mousePressEvent(QMouseEvent *event) override;   // объявление метода
        void mouseMoveEvent(QMouseEvent *event) override;    // объявление метода
        void mouseReleaseEvent(QMouseEvent *event) override; // объявление метода
        void mouseDoubleClickEvent(QMouseEvent *event) override;
private:
            ArrowEditor *m_arrowEditor = nullptr;
        Connector *m_connector = nullptr;
    ZoomController *m_zoomController; // контроллер масштаба
    State *m_state = nullptr;         // указатель на объект состояния
    bool m_panning = false;          // флаг, что мы сейчас перемещаем сцену
        QPoint m_lastMousePos;           // последняя позиция мыши для вычисления смещения
        // Флаг: мы начали "rubber band" выделение правой кнопкой
           bool m_rightButtonRubberBand = false;

           // Запомним предыдущий dragMode, чтобы восстановить
           QGraphicsView::DragMode m_prevDragMode = QGraphicsView::NoDrag;
};

#endif // DIAGRAMVIEW_H
