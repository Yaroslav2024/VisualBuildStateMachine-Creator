/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef CPPTRANSLATOR_H
#define CPPTRANSLATOR_H

#include <QObject>
#include <QGraphicsScene>
#include <QString>
#include <QMap>
// Структура для возврата двух файлов
struct GeneratedCode {
    QString headerContent; // Содержимое .h
    QString sourceContent; // Содержимое .cpp
    QString error;         // Если есть ошибка
};
class CppTranslator : public QObject
{
    Q_OBJECT
public:
    explicit CppTranslator(QObject *parent = nullptr);

    // Теперь принимает manualHeader (код из таблицы Хедера)
    QString translateAndCompile(QGraphicsScene *scene, const QString &manualHeader);

    // Генерирует код, объединяя Manual Header и Auto-Extracted переменные
    QString generateSourceCode(QGraphicsScene *scene, const QString &manualHeader);
    // Главный метод теперь возвращает структуру
        GeneratedCode generateCodePair(QGraphicsScene *scene, const QString &manualHeader, const QString &headerFileName);
private:
    // Структура для автоматических переменных
    struct VariableInfo {
        QString type;      // int, double...
        QString name;      // count...
    };

    // Хранилище переменных, найденных в блоках
    QMap<QString, VariableInfo> m_autoVariables;

    // Метод-"Пылесос": находит "int x = 0;" -> превращает в "x = 0;" -> сохраняет "int x"
    QString extractAndRemoveTypes(const QString &code);

    // Ваши старые проверки (сохранены)
    QString sanitizeCppCode(const QString &code, bool isCondition);
    bool hasPythonSyntax(const QString &code, QString &foundFeature);
    QString invokeCompiler(const QString &sourceCode);
};

#endif // CPPTRANSLATOR_H
