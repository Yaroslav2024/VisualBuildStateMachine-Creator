/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef SYNTAXREGISTRY_H
#define SYNTAXREGISTRY_H

#include <QString>
#include <QStringList>
#include <QSet> // Используем QSet для быстрого поиска среди тысяч функций

// Структура для описания найденной ошибки
struct SyntaxError {
    bool hasError;      // Есть ли ошибка?
    QString message;    // Текст ошибки
    int position;       // Позиция курсора (если -1, то общая ошибка)
};

class SyntaxRegistry
{
public:

    // *** ВОТ ЭТОГО НЕ ХВАТАЛО ***
        enum Language {
            Lang_Java,
            Lang_Cpp,
            Lang_Python
        };
        // ****************************


        // *** И ЭТИХ МЕТОДОВ ***
        void setLanguage(Language lang);
        Language language() const { return m_currentLanguage; }
        // **********************




    // Singleton: доступ к единому экземпляру класса
    static SyntaxRegistry& instance();

    // Загрузить список ключевых слов из текстового файла
    // (Каждая функция с новой строки)
    void loadKeywordsFromFile(const QString &filePath);

    // Проверка синтаксиса для поля "Условие" (IF)
    SyntaxError checkCondition(const QString &code);

    // Проверка синтаксиса для поля "Действие" (Action)
    SyntaxError checkAction(const QString &code);

    // Получить полный список всех известных слов (для автодополнения в будущем)
    QStringList allKeywords() const;

private:



        // Текущий язык
        Language m_currentLanguage = Lang_Cpp;



    // Приватный конструктор (для Singleton)
    SyntaxRegistry();
    // Запрещаем копирование
    SyntaxRegistry(const SyntaxRegistry&) = delete;
    SyntaxRegistry& operator=(const SyntaxRegistry&) = delete;

    // Функция обновления списков слов
        void updateKeywords();

    // Вспомогательный метод проверки парности скобок () [] {}
    bool checkBrackets(const QString &code);

    // Списки данных
    QSet<QString> m_keywords;   // if, else, true...
    QSet<QString> m_functions;  // In(), log(), и загруженные из файла
};

#endif // SYNTAXREGISTRY_H
