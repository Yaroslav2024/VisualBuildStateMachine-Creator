/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef MICROCONTROLLERTRANSLATOR_H
#define MICROCONTROLLERTRANSLATOR_H

#include <QObject>
#include <QGraphicsScene>
#include <QString>
#include <QMap>

// Структура для результата
struct MicroCode {
    QString header; // Для .h
    QString source; // Для .cpp
    QString code;       // Полный код .ino
    QString error;      // Ошибка, если есть
};

class MicroTranslator : public QObject
{
    Q_OBJECT
public:
    explicit MicroTranslator(QObject *parent = nullptr);

    // Главная функция генерации
    static MicroCode generate(QGraphicsScene *scene, const QString &manualHeader);

private:
    struct VariableInfo {
        QString type;
        QString name;
        QString initValue;
    };

    // Вспомогательные функции (похожи на C++, но адаптированы под Arduino)
    static QString extractVariables(const QString &code, QMap<QString, VariableInfo> &vars);
    static QString sanitizeForArduino(const QString &code, bool isCondition);
};

#endif // MICROCONTROLLERTRANSLATOR_H
