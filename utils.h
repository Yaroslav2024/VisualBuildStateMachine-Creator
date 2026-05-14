/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QPointF>
#include <QWidget>
#include <QStringList>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
class StateTable;
class QGraphicsScene;
class Grid;
// Используем namespace или статический класс.
// Статический класс удобнее, если нам вдруг понадобятся приватные константы.
class Utils
{

public:
    // --- Работа с ошибками компилятора ---
            static QString getLastCompileError();
            // --- Настройка подсветки ошибок ---
                static bool highlightAllErrors();
                static void setHighlightAllErrors(bool enable);

                // Теперь возвращаем список строк (даже если там всего одна ошибка)
                static QList<int> getLastErrorLines();
    // Режим "Авто-маршрутизация стрелок" (в 2 клика)
        static bool isAutoRoutingEnabled();
        static void setAutoRoutingEnabled(bool enable);
    // Управление желтым фоном текста на стрелках
        static bool showYellowLabels();
        static void setShowYellowLabels(bool show);
    static void applyTheme(bool isDark);
    // Добавляем метод для управления сеткой
        static void toggleGrid(Grid* grid, bool visible);
    // Работа с папками и библиотеками
        static void openFolder(const QString &folderName);
        static QStringList getLibraryFiles(const QString &folderName, const QStringList &nameFilters);

        // Работа с Java
        static QString findJavaTool(const QString &toolName, QWidget *parent = nullptr, bool silent = false);
        static bool compileJava(const QString &sourceCode, const QString &tempDir, QWidget *parent = nullptr);

        // Работа с C++
        static bool compileCpp(const QString &sourceCode, const QString &exePath, QWidget *parent = nullptr);

        // Работа с Python
        static bool checkPythonSyntax(const QString &filePath, QWidget *parent = nullptr);
        static bool checkCppSyntax(const QString &sourceCode, const QString &headerCode, const QString &headerName, QWidget *parent = nullptr);
            static bool checkJavaSyntax(const QString &sourceCode, const QString &className, QWidget *parent = nullptr);


        // Генерация иконок для тулбара
            static QIcon createBlockIcon();
            static QIcon createBridgeIcon();
            static QIcon createBackIcon();
            static QIcon createCommentIcon();
            static QIcon createGroupIcon();
            static QIcon createHistoryIcon();
            static QIcon createPreviewIcon();
            // --- (Таблица и Скорость) ---
                static int getSpeedInterval(int index);
                static void openTransitionTable(StateTable* table, QGraphicsScene* scene);
};

#endif // UTILS_H
