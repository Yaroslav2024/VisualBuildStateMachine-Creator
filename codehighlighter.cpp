/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "codehighlighter.h"
#include "syntaxregistry.h" // Чтобы узнать текущий язык

CodeHighlighter::CodeHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Настраиваем цвета (общие для всех языков)
    keywordFormat.setForeground(Qt::darkBlue);
    keywordFormat.setFontWeight(QFont::Bold);

    classFormat.setFontWeight(QFont::Bold);
    classFormat.setForeground(Qt::darkMagenta);

    quotationFormat.setForeground(Qt::darkRed);

    functionFormat.setFontItalic(true);
    functionFormat.setForeground(Qt::blue);

    commentFormat.setForeground(Qt::darkGreen);

    numberFormat.setForeground(Qt::magenta);

    // Загружаем правила в зависимости от текущего выбора в меню
    rehighlightRules();
}

void CodeHighlighter::rehighlightRules()
{
    highlightingRules.clear();

    SyntaxRegistry::Language lang = SyntaxRegistry::instance().language();

    // --- ИЗМЕНЕНО: Для Java используем те же правила подсветки, что и для C++ ---
    if (lang == SyntaxRegistry::Lang_Cpp || lang == SyntaxRegistry::Lang_Java) {
        loadCppRules();
    } else {
        loadPythonRules();
    }
}

void CodeHighlighter::loadCppRules()
{
    HighlightingRule rule;

    // 1. Ключевые слова C++
    QStringList keywordPatterns;
    keywordPatterns << "\\bchar\\b" << "\\bclass\\b" << "\\bconst\\b" << "\\bdouble\\b"
                    << "\\benum\\b" << "\\bexplicit\\b" << "\\bfriend\\b" << "\\binline\\b"
                    << "\\bint\\b" << "\\blong\\b" << "\\bnamespace\\b" << "\\boperator\\b"
                    << "\\bprivate\\b" << "\\bprotected\\b" << "\\bpublic\\b" << "\\bshort\\b"
                    << "\\bsignals\\b" << "\\bsigned\\b" << "\\bslots\\b" << "\\bstatic\\b"
                    << "\\bstruct\\b" << "\\btemplate\\b" << "\\btypedef\\b" << "\\btypename\\b"
                    << "\\bunion\\b" << "\\bunsigned\\b" << "\\bvirtual\\b" << "\\bvoid\\b"
                    << "\\bvolatile\\b" << "\\bbool\\b" << "\\bif\\b" << "\\belse\\b"
                    << "\\bwhile\\b" << "\\bfor\\b" << "\\breturn\\b" << "\\btrue\\b" << "\\bfalse\\b";

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // C++ классы
    rule.pattern = QRegularExpression("\\bQ[A-Za-z]+\\b");
    rule.format = classFormat;
    highlightingRules.append(rule);

    // C++ комментарии //
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    // C++ строки "..."
    rule.pattern = QRegularExpression("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Функции
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // Числа
    rule.pattern = QRegularExpression("\\b\\d+\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);
}

void CodeHighlighter::loadPythonRules()
{
    HighlightingRule rule;

    // 1. Ключевые слова Python
    QStringList keywordPatterns;
    keywordPatterns << "\\band\\b" << "\\bas\\b" << "\\bassert\\b" << "\\bbreak\\b"
                    << "\\bclass\\b" << "\\bcontinue\\b" << "\\bdef\\b" << "\\bdel\\b"
                    << "\\belif\\b" << "\\belse\\b" << "\\bexcept\\b" << "\\bfinally\\b"
                    << "\\bfor\\b" << "\\bfrom\\b" << "\\bglobal\\b" << "\\bif\\b"
                    << "\\bimport\\b" << "\\bin\\b" << "\\bis\\b" << "\\blambda\\b"
                    << "\\bnot\\b" << "\\bor\\b" << "\\bpass\\b" << "\\braise\\b"
                    << "\\breturn\\b" << "\\btry\\b" << "\\bwhile\\b" << "\\bwith\\b"
                    << "\\byield\\b" << "\\bNone\\b" << "\\bTrue\\b" << "\\bFalse\\b";

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Python декораторы @...
    rule.pattern = QRegularExpression("@[A-Za-z_]+");
    rule.format = classFormat;
    highlightingRules.append(rule);

    // Python комментарии #
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);

    // Python строки '...' и "..."
    rule.pattern = QRegularExpression("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("\'.*\'");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // Функции def myFunc():
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // Числа
    rule.pattern = QRegularExpression("\\b\\d+\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);
}

void CodeHighlighter::highlightBlock(const QString &text)
{
    // --- ЭТАП 1: Подсветка ключевых слов (Мягкий режим) ---
    // Ищем слова. Если слово есть в словаре -> красим.
    // Если нет -> оставляем черным (не ошибка).

    QRegularExpression wordPattern("([a-zA-Z_][\\w\\.]*)");
    QRegularExpressionMatchIterator i = wordPattern.globalMatch(text);

    QStringList externalFuncs = SyntaxRegistry::instance().allKeywords();

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString word = match.captured(1);

        // Проверяем наличие слова в словарях (без учета регистра)
        bool isExternal = externalFuncs.contains(word.toLower());

        if (isExternal) {
            setFormat(match.capturedStart(), match.capturedLength(), keywordFormat);
        }
    }

    // --- ЭТАП 2: Основные правила (Числа, Строки, Комментарии, Ключевые слова) ---
    for (const HighlightingRule &rule : qAsConst(highlightingRules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // --- ЭТАП 3: УМНАЯ ПРОВЕРКА СКОБОК (МНОГОСТРОЧНАЯ) ---
    // Настраиваем формат ошибки (Красный)
    QTextCharFormat errorFormat;
    errorFormat.setForeground(Qt::red);
    errorFormat.setFontWeight(QFont::Bold);
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline); // Можно раскомментировать

    // Читаем "память" с предыдущей строки (сколько скобок осталось открытыми)
    int state = previousBlockState();
    if (state == -1) state = 0;

    // Распаковываем счетчики (по 8 бит на каждый тип скобок)
    int round  = state & 0xFF;         // Круглые ()
    int square = (state >> 8) & 0xFF;  // Квадратные []
    int curly  = (state >> 16) & 0xFF; // Фигурные {}

    for (int j = 0; j < text.length(); ++j) {
        QChar c = text[j];
        // --- НОВАЯ ЗАЩИТА: Игнорируем скобки внутри комментариев и строк ---
                QTextCharFormat currentFmt = format(j);
                if (currentFmt == commentFormat || currentFmt == quotationFormat) {
                    continue;
                }
        // Если открывается — увеличиваем счетчик
        if (c == '(') round++;
        else if (c == '[') square++;
        else if (c == '{') curly++;

        // Если закрывается — уменьшаем счетчик и проверяем на ошибку
        else if (c == ')') {
            round--;
            if (round < 0) {
                setFormat(j, 1, errorFormat); // Ошибка: закрывающая скобка без открывающей
                round = 0; // Сбрасываем, чтобы не красить всё дальше
            }
        }
        else if (c == ']') {
            square--;
            if (square < 0) {
                setFormat(j, 1, errorFormat);
                square = 0;
            }
        }
        else if (c == '}') {
            curly--;
            if (curly < 0) {
                setFormat(j, 1, errorFormat);
                curly = 0;
            }
        }
    }

    // Запаковываем актуальное состояние и передаем его на следующую строку
    state = (round & 0xFF) | ((square & 0xFF) << 8) | ((curly & 0xFF) << 16);
    setCurrentBlockState(state);
}
