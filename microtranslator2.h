/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef MICROTRANSLATOR2_H
#define MICROTRANSLATOR2_H

#include <QObject>
#include <QGraphicsScene>
#include <QString>
#include <QMap>
#include <QGraphicsItem>

// --- ИЗМЕНЕНИЕ: Переименовали структуру, чтобы не конфликтовала с CppTranslator2 ---
struct MicroDebugResult {
    QString sourceCode;
    QMap<int, QGraphicsItem*> idMap;
    QString error;
};

class MicroTranslator2 : public QObject
{
    Q_OBJECT
public:
    // Возвращаем новый тип
    static MicroDebugResult generateDebugSource(QGraphicsScene *scene, const QString &manualHeader);

private:
    struct VariableInfo {
        QString type;
        QString name;
    };

    static QString extractAndRemoveTypes(const QString &code, QMap<QString, VariableInfo> &variables);
    static QString sanitizeForArduinoDebug(const QString &code, bool isCondition);
    static QString addIndent(const QString &code, int level);
    static bool hasPythonSyntax(const QString &code, QString &foundFeature);
};

#endif // MICROTRANSLATOR2_H
