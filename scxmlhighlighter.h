/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef SCXMLHIGHLIGHTER_H
#define SCXMLHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class ScxmlHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit ScxmlHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    QRegularExpression m_wordPattern;
    QTextCharFormat m_errorFormat;
    QTextCharFormat m_keywordFormat; // Опционально: можно подсвечивать правильные слова синим
};

#endif // SCXMLHIGHLIGHTER_H
