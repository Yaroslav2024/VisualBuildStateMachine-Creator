/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef CODEPREVIEWDIALOG_H
#define CODEPREVIEWDIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>
#include <QPair>
#include <QList> // <-- Добавили библиотеку для списков

// Предварительное объявление классов (Forward Declaration)
// Это ускоряет компиляцию и делает хедер "легким"
class QTabWidget;
class QLabel;

class CodePreviewDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Конструктор для отображения кода в виде вкладок
     * @param files Список пар: {Имя файла/вкладки, Текст кода}
     * @param translatorName Имя транслятора (C++, Python и т.д.)
     * @param status Статус компиляции (true - успех, false - ошибка)
     */
    explicit CodePreviewDialog(const QVector<QPair<QString, QString>> &files,
                               const QString &translatorName,
                               bool status,
                               QList<int> errorLines = QList<int>(),
                               QWidget *parent = nullptr);

private slots:
    /**
     * @brief Копирует содержимое текущей активной вкладки в буфер обмена
     */
    void copyToClipboard();
    void matchBrackets();
private:
    QTabWidget *tabWidget; // Панель вкладок для разделения файлов (например, .h и .cpp)
    QLabel *statusLabel;   // Верхняя надпись со статусом и иконкой
};

#endif // CODEPREVIEWDIALOG_H
