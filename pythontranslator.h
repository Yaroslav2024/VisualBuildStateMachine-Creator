/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef PYTHONTRANSLATOR_H
#define PYTHONTRANSLATOR_H

#include <QString>
#include <QGraphicsScene>
#include <QList>
#include <QMap>
#include "diagramitem.h"
class ConnectionItem;
class PythonTranslator
{
public:
    struct GeneratedPyCode {
        QString code;      // Полный код скрипта
        QString error;     // Сообщение об ошибке (если есть)
        QMap<int, QGraphicsItem*> idMap;
    };

    // Главная функция генерации
    // Добавили debugMode = false (по умолчанию выключен)
    static GeneratedPyCode generate(QGraphicsScene* scene, const QString &manualHeader, bool debugMode = false);

private:
    // Помощник для добавления отступов (Python это любит)
    static QString addIndent(const QString &code, int level);

    // Очистка кода от типов C++ (int x -> x)
    static QString sanitizeForPython(const QString &rawCode);
};

#endif // PYTHONTRANSLATOR_H
