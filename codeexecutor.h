/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef CODEEXECUTOR_H
#define CODEEXECUTOR_H

#include <QObject>
#include <QMap>
#include <QString>

struct CodeSnippet {
    QString code;
    bool isCpp;
};

class CodeExecutor : public QObject
{
    Q_OBJECT
public:
    explicit CodeExecutor(QObject *parent = nullptr);

    void registerCode(const QString &id, const QString &type, const QString &code, bool isCpp);
    void clear();

public slots:
    // Этот метод будет вызываться из SCXML
    void execute(const QString &id, const QString &type);

private:
    QMap<QString, CodeSnippet> m_codeMap;
    void runCpp(const QString &code);
    void runPython(const QString &code);
};

#endif // CODEEXECUTOR_H
