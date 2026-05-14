/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef PROPERTIESDIALOG_H
#define PROPERTIESDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>

class PropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    // Структура для передачи данных туда и обратно
    struct Data {
        QString type;       // "Вершина" или "Стартовый блок"
        QString description;// Описание (Кириллица)
        QString label;      // Label (Латиница, автогенерация)
        QString onEnter;    // Код входа
        QString onExit;     // Код выхода
        int clickedSection = 0;
    };

    explicit PropertiesDialog(const Data &initialData, QWidget *parent = nullptr);

    // Получить обновленные данные
    Data getData() const;

private slots:
    // Слот для авто-транслитерации при вводе описания
    void onDescriptionChanged(const QString &text);
    // *** НОВОЕ: Слот для проверки перед закрытием ***
        void onOkClicked();
private:
    // Функция транслитерации
    QString transliterate(const QString &text);

    QComboBox *m_typeCombo;
    QLineEdit *m_descriptionEdit;
    QLineEdit *m_labelEdit;
    QTextEdit *m_onEnterEdit;
    QTextEdit *m_onExitEdit;
};

#endif // PROPERTIESDIALOG_H
