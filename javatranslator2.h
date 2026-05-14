/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef JAVATRANSLATOR2_H
#define JAVATRANSLATOR2_H

#include <QObject>
#include <QGraphicsScene>
#include <QString>
#include <QMap>
#include "diagramitem.h"

// Структура результата специально для Дебаггера
struct JavaDebugResult {
    QString javaCode;                // Код программы
    QString error;                   // Ошибка (если есть)
    QMap<int, QGraphicsItem*> idMap;   // Карта: ID блока -> Графический элемент (для подсветки)
};

class JavaTranslator2 : public QObject
{
    Q_OBJECT
public:
    explicit JavaTranslator2(QObject *parent = nullptr);

    // Метод генерации кода для симуляции
    static JavaDebugResult generateDebug(QGraphicsScene *scene, const QString &className, const QString &manualHeader);

private:
    struct VariableInfo {
        QString type;
        QString name;
    };

    // Те же вспомогательные функции, что и в основном трансляторе
    static QString extractAndRemoveTypes(const QString &code, QMap<QString, VariableInfo> &variables);
    static QString sanitizeJavaCode(const QString &code, bool isCondition);
};

#endif // JAVATRANSLATOR2_H
