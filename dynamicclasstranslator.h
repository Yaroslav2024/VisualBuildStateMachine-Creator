/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef DYNAMICCLASSTRANSLATOR_H
#define DYNAMICCLASSTRANSLATOR_H

#include <QObject>
#include <QGraphicsScene>
#include <QString>
#include <QMap>
#include <QGraphicsItem>

// Структура результата: возвращаем тексты для ТРЕХ файлов и карту ID
struct ClassResult {
    QString headerCode; // Код для .h файла
    QString sourceCode; // Код для .cpp файла
    QString mainCode;   // Код для .ino файла (пример)
    QString error;
    QMap<int, QGraphicsItem*> idMap; // Карта ID -> Графический элемент (для раскраски в GUI)
};

class DynamicClassTranslator : public QObject
{
    Q_OBJECT
public:
    explicit DynamicClassTranslator(QObject *parent = nullptr);

    // Главная функция генерации
    // className - имя класса (например, "TrafficLight")
    // manualHeader - пользовательские библиотеки (#include ...)
    static ClassResult generate(QGraphicsScene *scene, const QString &className, const QString &manualHeader);

private:
    struct VariableInfo {
        QString type;
        QString name;
        QString initValue;
    };

    // Вспомогательные функции (парсинг переменных и очистка кода)
    static QString extractVariables(const QString &code, QMap<QString, VariableInfo> &vars);
    static QString sanitizeForArduino(const QString &code, bool isCondition);
};

#endif // DYNAMICCLASSTRANSLATOR_H
