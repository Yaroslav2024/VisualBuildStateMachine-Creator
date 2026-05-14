/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef STATETABLE_H
#define STATETABLE_H

#include <QDialog>
#include <QTableWidget>
#include <QGraphicsScene>

class StateTable : public QDialog
{
    Q_OBJECT

public:
    explicit StateTable(QWidget *parent = nullptr);

    // Этот метод мы наполним логикой в следующем шаге.
    // Он будет сканировать сцену и заполнять таблицу.
    void updateFromScene(QGraphicsScene *scene);

private:
    void setupUi(); // Настройка внешнего вида таблицы

    QTableWidget *m_table;
};

#endif // STATETABLE_H
