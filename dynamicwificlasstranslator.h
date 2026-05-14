/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef DYNAMICWIFICLASSTRANSLATOR_H
#define DYNAMICWIFICLASSTRANSLATOR_H

#include "dynamicclasstranslator.h" // Используем ClassResult

class DynamicWifiClassTranslator : public QObject
{
    Q_OBJECT
public:
    explicit DynamicWifiClassTranslator(QObject *parent = nullptr);
    static ClassResult generate(QGraphicsScene *scene, const QString &className, const QString &manualHeader, const QString &ssid, const QString &pass);

private:
    struct VariableInfo { QString type; QString name; QString initValue; };
    static QString extractVariables(const QString &code, QMap<QString, VariableInfo> &vars);
    static QString sanitizeForArduino(const QString &code, bool isCondition);
};

#endif // DYNAMICWIFICLASSTRANSLATOR_H
