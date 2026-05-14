/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef GRAPHMANAGER_H
#define GRAPHMANAGER_H

#include <QObject>
#include <QGraphicsScene>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QList>
#include "diagramitem.h"
#include "connect.h"
#include "historymanager.h"

class GraphManager : public QObject
{
    Q_OBJECT
public:
    explicit GraphManager(QGraphicsScene* scene, QObject *parent = nullptr);
    // НОВОЕ: Импорт (добавляет к текущему, сдвигая вправо)
        bool importFromFile(const QString &fileName, QString &headerCode, QPointF dropPos = QPointF(0,0));
    // Основные функции сохранения/загрузки в файл     
            bool saveToFile(const QString &fileName, const QString &headerCode, int langId = 0, const QJsonArray &sectors = QJsonArray());
            bool loadFromFile(const QString &fileName, QString &headerCode, int &langId, QJsonArray &sectors, QPointF dropPos = QPointF(), bool useDropPos = false);
    // Функции Undo / Redo
    void createSnapshot(); // Вызывать после каждого изменения (отпускание мыши, редактирование)
    void undo();
    void redo();
    void clearHistory();
    // Функция для копирования выделенных элементов
        QJsonObject selectionToJson();
    void jsonToScene(const QJsonObject &json, bool clearScene, QPointF dropPos = QPointF(), bool useDropPos = false); // Восстановить сцену из JSON
private:
    QGraphicsScene* m_scene;

    // Для Undo/Redo мы будем хранить "снимки" сцены в формате JSON строки
    HistoryManager* m_historyManager;
    // Внутренние методы
    QJsonObject sceneToJson();     // Упаковать сцену в JSON объект

};

#endif // GRAPHMANAGER_H
