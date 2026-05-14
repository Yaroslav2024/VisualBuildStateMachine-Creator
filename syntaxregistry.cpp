/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "syntaxregistry.h"
#include <QRegularExpression>
#include <QFile>
#include <QTextStream>

#include <QObject>
// Реализация Singleton
SyntaxRegistry& SyntaxRegistry::instance()
{
    static SyntaxRegistry obj;
    return obj;
}

// Конструктор
SyntaxRegistry::SyntaxRegistry()
{
    updateKeywords();
}
// === НОВАЯ ФУНКЦИЯ ПЕРЕКЛЮЧЕНИЯ ЯЗЫКА ===
void SyntaxRegistry::setLanguage(Language lang)
{
    m_currentLanguage = lang;
    updateKeywords(); // Перезагружаем слова под новый язык
}

// === ГЛАВНАЯ БАЗА СЛОВ ===
void SyntaxRegistry::updateKeywords()
{
    m_keywords.clear();
    m_functions.clear();

    // 1. ОБЩИЕ СЛОВА (Есть почти везде, кроме Python, где они специфичны)
    if (m_currentLanguage != Lang_Python) {
        m_keywords.insert("if"); m_keywords.insert("else");
        m_keywords.insert("return"); m_keywords.insert("while");
        m_keywords.insert("for"); m_keywords.insert("break");
        m_keywords.insert("switch"); m_keywords.insert("case");
        m_keywords.insert("default"); m_keywords.insert("true");
        m_keywords.insert("false");
    }

    // 2. НАСТРОЙКА ДЛЯ C++ (Как было раньше)
    if (m_currentLanguage == Lang_Cpp) {
        m_keywords.insert("int"); m_keywords.insert("float");
        m_keywords.insert("double"); m_keywords.insert("bool");
        m_keywords.insert("void"); m_keywords.insert("char");
        m_keywords.insert("string"); m_keywords.insert("class");
        m_keywords.insert("struct"); m_keywords.insert("public");
        m_keywords.insert("private"); m_keywords.insert("std");
        m_keywords.insert("const"); m_keywords.insert("new");
        m_keywords.insert("delete"); m_keywords.insert("nullptr");

        m_functions.insert("log");
        m_functions.insert("send");
    }
    // 3. НАСТРОЙКА ДЛЯ PYTHON
    else if (m_currentLanguage == Lang_Python) {
        m_keywords.insert("def"); m_keywords.insert("class");
        m_keywords.insert("if"); m_keywords.insert("elif");
        m_keywords.insert("else"); m_keywords.insert("return");
        m_keywords.insert("print"); m_keywords.insert("import");
        m_keywords.insert("from"); m_keywords.insert("True");
        m_keywords.insert("False"); m_keywords.insert("None");
        m_keywords.insert("and"); m_keywords.insert("or");
        m_keywords.insert("not"); m_keywords.insert("in");
        m_keywords.insert("for"); m_keywords.insert("while");
        m_keywords.insert("pass"); m_keywords.insert("try");
        m_keywords.insert("except");
    }
    // 4. НАСТРОЙКА ДЛЯ JAVA (НОВОЕ!)
    else if (m_currentLanguage == Lang_Java) {
        // Типы
        m_keywords.insert("boolean"); // Java использует boolean, а не bool
        m_keywords.insert("int"); m_keywords.insert("double");
        m_keywords.insert("float"); m_keywords.insert("char");
        m_keywords.insert("void"); m_keywords.insert("String"); // String с большой буквы

        // Ключевые слова
        m_keywords.insert("class"); m_keywords.insert("public");
        m_keywords.insert("private"); m_keywords.insert("static");
        m_keywords.insert("final"); m_keywords.insert("import");
        m_keywords.insert("package"); m_keywords.insert("new");
        m_keywords.insert("try"); m_keywords.insert("catch");
        m_keywords.insert("this"); m_keywords.insert("super");
        m_keywords.insert("null"); m_keywords.insert("extends");
        m_keywords.insert("implements");

        // Функции Java
        m_functions.insert("System");
        m_functions.insert("out");
        m_functions.insert("println");
        m_functions.insert("Math");
    }
}

// Загрузка дополнительных функций из файла
void SyntaxRegistry::loadKeywordsFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Если файл не найден — ничего страшного, просто тихо выходим.
        // Никаких логов в консоль для релизной версии.
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        // Игнорируем пустые строки и комментарии
        if (!line.isEmpty() && !line.startsWith("//")) {
            // Если в файле написано "timer.start()", убираем скобки
            int parenIndex = line.indexOf('(');
            if (parenIndex != -1) {
                line = line.left(parenIndex).trimmed();
            }
            m_functions.insert(line.toLower());
        }
    }
    file.close();
}

QStringList SyntaxRegistry::allKeywords() const
{
    QStringList list;
    list.append(m_keywords.values());
    list.append(m_functions.values());
    list.sort();
    return list;
}

// Проверка скобок
bool SyntaxRegistry::checkBrackets(const QString &code)
{
    int round = 0; // ()
    int square = 0; // []
    int curly = 0;  // {}

    for (QChar c : code) {
        if (c == '(') round++;
        else if (c == ')') round--;
        else if (c == '[') square++;
        else if (c == ']') square--;
        else if (c == '{') curly++;
        else if (c == '}') curly--;

        if (round < 0 || square < 0 || curly < 0) return false;
    }
    return (round == 0 && square == 0 && curly == 0);
}

// === ПРОВЕРКА УСЛОВИЯ (МЯГКАЯ) ===
SyntaxError SyntaxRegistry::checkCondition(const QString &code)
{
    QString clean = code.trimmed();
    if (clean.isEmpty()) return {false, "", -1};

    // 1. Проверка скобок (ОСТАВЛЯЕМ — ЭТО ВАЖНО)


    if (!checkBrackets(clean)) {
            return {true, QObject::tr("Ошибка: Непарные скобки"), 0};
        }
    // 2. Проверка недопустимых символов
    // (Разрешаем буквы, цифры, пробелы и основные знаки пунктуации)
    QRegularExpression validChars("^[\\w\\s\\.\\(\\)\\[\\]\\'\\\"\\>\\<\\=\\!\\&\\|\\+\\-\\*\\/\\%\\,]+$");






    return {false, "", -1};
}

// === ПРОВЕРКА ДЕЙСТВИЯ (МЯГКАЯ) ===
SyntaxError SyntaxRegistry::checkAction(const QString &code)
{
    QString clean = code.trimmed();
    if (clean.isEmpty()) return {false, "", -1};

    // 1. Проверка скобок


    if (!checkBrackets(clean)) {
            return {true, QObject::tr("Ошибка: Непарные скобки"), 0};
        }

    // 2. Проверка символов (добавлена ; и =)
    QRegularExpression validChars("^[\\w\\s\\.\\(\\)\\[\\]\\'\\\"\\>\\<\\=\\!\\&\\|\\+\\-\\*\\/\\%\\,\\;]+$");




    return {false, "", -1};
}
