/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef STATE_H
#define STATE_H

#include <QObject>
#include <QList>
#include "diagramitem.h"
#include "editabletextitem.h"

// Класс State — хранит все состояния элементов диаграммы
class State : public QObject
{
    Q_OBJECT
public:
    explicit State(QObject *parent = nullptr);

    // Добавить новый элемент на сцену в состояние
    void addItem(DiagramItem* item);

    // Удалить элемент из состояния
    void removeItem(DiagramItem* item);

    // Получить список всех элементов
    QList<DiagramItem*> getItems() const;

    // Очистить все элементы
    void clear();

private:
    QList<DiagramItem*> m_items; // список всех элементов диаграммы обычными указателями
};

#endif // STATE_H
