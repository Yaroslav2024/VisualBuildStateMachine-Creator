/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef PYTHONTRANSLATOR2_H
#define PYTHONTRANSLATOR2_H

#include <QString>
#include <QGraphicsScene>
#include <QList>
#include <QMap>
#include "diagramitem.h"

class ConnectionItem;

class PythonTranslator2
{
public:
    struct GeneratedPyCode {
        QString code;      // Полный код скрипта
        QString error;     // Сообщение об ошибке (если есть)
    };

    // Главная функция генерации
    static GeneratedPyCode generate(QGraphicsScene* scene, const QString &manualHeader);

private:
    // Помощник для добавления отступов
    static QString addIndent(const QString &code, int level);

    // Очистка кода от типов C++
    static QString sanitizeForPython(const QString &rawCode);
};

#endif // PYTHONTRANSLATOR2_H
