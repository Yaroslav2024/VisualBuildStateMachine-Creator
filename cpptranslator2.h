/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef CPPTRANSLATOR2_H
#define CPPTRANSLATOR2_H

#include <QObject>
#include <QGraphicsScene>
#include <QString>
#include <QMap>
#include <QGraphicsItem>

// Структура результата: код для компилятора + карта ID для дебаггера
struct DebugBuildResult {
    QString sourceCode;                 // Готовый текст C++ программы
    QMap<int, QGraphicsItem*> idMap;    // Карта: ID из кода -> Объект на сцене
    QString error;                      // Текст ошибки, если есть
};

class CppTranslator2 : public QObject
{
    Q_OBJECT
public:
    // Главный метод генерации
    static DebugBuildResult generateDebugSource(QGraphicsScene *scene, const QString &manualHeader);

private:
    // Структура для хранения инфо о переменной
    struct VariableInfo {
        QString type;
        QString name;
    };

    // Вспомогательные методы (копии вашей логики с адаптацией)
    static QString extractAndRemoveTypes(const QString &code, QMap<QString, VariableInfo> &variables);
    static QString sanitizeCppCode(const QString &code, bool isCondition);
    static QString addIndent(const QString &code, int level);
};

#endif // CPPTRANSLATOR2_H
