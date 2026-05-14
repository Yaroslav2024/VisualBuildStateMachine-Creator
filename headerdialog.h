/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef HEADERDIALOG_H
#define HEADERDIALOG_H

#include <QDialog>
#include <QTextEdit>

class HeaderDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HeaderDialog(const QString &currentText, QWidget *parent = nullptr);
    QString getCode() const;
void setPlaceholderText(const QString &text);
private slots:
    void matchBrackets();
private:
    QTextEdit *m_editor;
};

#endif // HEADERDIALOG_H
