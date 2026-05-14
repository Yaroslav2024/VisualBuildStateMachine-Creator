/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "codeexecutor.h"

#include <QRegularExpression>

CodeExecutor::CodeExecutor(QObject *parent) : QObject(parent) {}

void CodeExecutor::registerCode(const QString &id, const QString &type, const QString &code, bool isCpp)
{
    QString key = id + "_" + type;
    m_codeMap[key] = {code, isCpp};
}

void CodeExecutor::clear()
{
    m_codeMap.clear();
}

void CodeExecutor::execute(const QString &id, const QString &type)
{
    QString key = id + "_" + type;
    if (!m_codeMap.contains(key)) return;

    CodeSnippet snippet = m_codeMap[key];


    // Эмуляция выполнения (лог в консоль)
    if (snippet.isCpp) runCpp(snippet.code);
    else runPython(snippet.code);
}

void CodeExecutor::runCpp(const QString &code)
{

    // Тут можно добавить парсер log("...")
}

void CodeExecutor::runPython(const QString &code)
{

}
