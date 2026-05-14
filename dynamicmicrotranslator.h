/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef DYNAMICMICROTRANSLATOR_H
#define DYNAMICMICROTRANSLATOR_H

#include <QObject>
#include <QGraphicsScene>
#include <QString>
#include <QMap>
#include <QGraphicsItem>

// Расширенная структура результата: Код + Карта ID для дебаггера
struct DynamicResult {
    QString code;
    QString error;
    QMap<int, QGraphicsItem*> idMap; // Связь: Числовой ID <-> Элемент сцены
};

class DynamicMicroTranslator : public QObject
{
    Q_OBJECT
public:
    explicit DynamicMicroTranslator(QObject *parent = nullptr);

    // Главная функция генерации
    static DynamicResult generate(QGraphicsScene *scene, const QString &manualHeader);

private:
    struct VariableInfo {
        QString type;
        QString name;
        QString initValue;
    };

    // Вспомогательные функции
    static QString extractVariables(const QString &code, QMap<QString, VariableInfo> &vars);
    static QString sanitizeForArduino(const QString &code, bool isCondition);
};

#endif // DYNAMICMICROTRANSLATOR_H
