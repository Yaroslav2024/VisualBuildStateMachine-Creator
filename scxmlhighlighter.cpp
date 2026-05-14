/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "scxmlhighlighter.h"
#include "syntaxregistry.h"

ScxmlHighlighter::ScxmlHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Паттерн ловит любые слова
    m_wordPattern.setPattern("([^\\s\\(\\)\\[\\]\\{\\}\\;\\,]+)");

    // Настройка ошибки (красный + волна)
    m_errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    m_errorFormat.setUnderlineColor(Qt::red);
    m_errorFormat.setForeground(Qt::red);

    // Настройка правильных слов (синий жирный)
    m_keywordFormat.setForeground(Qt::darkBlue);
    m_keywordFormat.setFontWeight(QFont::Bold);
}

void ScxmlHighlighter::highlightBlock(const QString &text)
{
    QStringList knownWords = SyntaxRegistry::instance().allKeywords();

    QRegularExpressionMatchIterator i = m_wordPattern.globalMatch(text);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString word = match.captured(1);

        int start = match.capturedStart();
        int length = match.capturedLength();

        // *** ИЗМЕНЕНИЕ: Сравниваем с нижним регистром ***
        if (knownWords.contains(word.toLower())) {
            // Слово есть в словаре -> СИНИЙ
            setFormat(start, length, m_keywordFormat);
        } else {
            // Слова нет в словаре -> КРАСНЫЙ
            setFormat(start, length, m_errorFormat);
        }
    }
}
