/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef DYNAMICWIFITRANSLATOR_H
#define DYNAMICWIFITRANSLATOR_H

#include "dynamicmicrotranslator.h" // Используем ту же структуру результата

class DynamicWifiTranslator : public QObject
{
    Q_OBJECT
public:
    explicit DynamicWifiTranslator(QObject *parent = nullptr);
    static DynamicResult generate(QGraphicsScene *scene, const QString &manualHeader, const QString &ssid, const QString &pass);

private:
    // Копируем утилиты, так как они приватные в оригинале
    struct VariableInfo { QString type; QString name; QString initValue; };
    static QString extractVariables(const QString &code, QMap<QString, VariableInfo> &vars);
    static QString sanitizeForArduino(const QString &code, bool isCondition);
};

#endif // DYNAMICWIFITRANSLATOR_H
