/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "historymanager.h"

HistoryManager::HistoryManager(QObject *parent)
    : QObject(parent), m_currentIndex(-1)
{
}

void HistoryManager::saveState(const QByteArray &state)
{
    // Если мы откатились назад, а потом сделали новое действие —
    // "будущее" (ветка Redo) должно быть стерто.
    if (m_currentIndex < m_history.size() - 1) {
        m_history = m_history.mid(0, m_currentIndex + 1);
    }

    // Добавляем новый снимок
    m_history.append(state);

    // Ограничиваем память (удаляем самый старый снимок, если превышен лимит)
    if (m_history.size() > MAX_STEPS) {
        m_history.removeFirst();
    }

    m_currentIndex = m_history.size() - 1;
}

QByteArray HistoryManager::undo()
{
    if (canUndo()) {
        m_currentIndex--;
        return m_history.at(m_currentIndex);
    }
    return QByteArray(); // Возвращаем пустоту, если откатываться некуда
}

QByteArray HistoryManager::redo()
{
    if (canRedo()) {
        m_currentIndex++;
        return m_history.at(m_currentIndex);
    }
    return QByteArray(); // Возвращаем пустоту, если идти вперед некуда
}

void HistoryManager::clear()
{
    m_history.clear();
    m_currentIndex = -1;
}

bool HistoryManager::canUndo() const
{
    return m_currentIndex > 0; // Можно откатиться, если мы не на самом первом кадре
}

bool HistoryManager::canRedo() const
{
    return m_currentIndex < m_history.size() - 1; // Можно вернуть, если мы не на последнем кадре
}
