/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QList>
#include <QByteArray>

class HistoryManager : public QObject
{
    Q_OBJECT
public:
    explicit HistoryManager(QObject *parent = nullptr);

    void saveState(const QByteArray &state); // Сохранить новый кадр
    QByteArray undo(); // Получить кадр для шага НАЗАД
    QByteArray redo(); // Получить кадр для шага ВПЕРЕД
    void clear();      // Очистить историю (при создании нового проекта)

    bool canUndo() const;
    bool canRedo() const;

private:
    QList<QByteArray> m_history;
    int m_currentIndex;
    const int MAX_STEPS = 50; // Максимальная глубина истории
};

#endif // HISTORYMANAGER_H
