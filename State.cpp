/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "state.h"

State::State(QObject *parent)
    : QObject(parent)
{
}

// Добавляем элемент
void State::addItem(DiagramItem* item)
{
    if (!item) return;
    if (!m_items.contains(item))
        m_items.append(item);
}

// Удаляем элемент
void State::removeItem(DiagramItem* item)
{
    if (!item) return;
    m_items.removeAll(item);
}

// Получаем список элементов
QList<DiagramItem*> State::getItems() const
{
    return m_items;
}

// Полностью очищаем состояние
void State::clear()
{
    m_items.clear();
}
