/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef JAVATRANSLATOR_H
#define JAVATRANSLATOR_H

#include <QObject>
#include <QGraphicsScene>
#include <QString>
#include <QMap>
#include <QRegularExpression>

// Структура ответа (Java генерирует всего один файл, хедеров нет)
struct GeneratedJavaCode {
    QString javaCode; // Полный код класса
    QString error;    // Ошибка, если есть
};

class JavaTranslator : public QObject
{
    Q_OBJECT
public:
    explicit JavaTranslator(QObject *parent = nullptr);

    // Главный метод генерации "Чистого кода"
    // className - имя класса (совпадает с именем файла без .java)
    static GeneratedJavaCode generateClean(QGraphicsScene *scene, const QString &className, const QString &manualHeader);

private:
    struct VariableInfo {
        QString type;
        QString name;
    };

    // "Пылесос" для переменных (Java версия)
    static QString extractAndRemoveTypes(const QString &code, QMap<QString, VariableInfo> &variables);

    // Очистка синтаксиса (замена bool -> boolean и т.д.)
    static QString sanitizeJavaCode(const QString &code, bool isCondition);
};

#endif // JAVATRANSLATOR_H
