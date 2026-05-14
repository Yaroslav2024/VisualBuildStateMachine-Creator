/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef EDITABLETEXTITEM_H
#define EDITABLETEXTITEM_H

#include <QGraphicsTextItem> // Базовый класс для редактируемого текста на графической сцене

class DiagramItem; // Предварительное объявление класса DiagramItem для указателя владельца текста

// Класс EditableTextItem — текстовый элемент, который можно редактировать прямо на сцене
// Наследуется от QGraphicsTextItem, что позволяет отображать и редактировать текст
class EditableTextItem : public QGraphicsTextItem
{
public:
    // Конструктор класса
    // parent — родительский графический объект (на сцене)
    // owner — указатель на DiagramItem, которому принадлежит этот текст
    EditableTextItem(QGraphicsItem *parent, DiagramItem *owner);

protected:
    // Обработчик потери фокуса (например, когда пользователь закончил редактирование текста)
    // Переопределяет метод QGraphicsTextItem
    void focusOutEvent(QFocusEvent *event) override;

    // Обработчик нажатий клавиш, например Enter или Escape
    // Позволяет контролировать поведение при редактировании текста
    void keyPressEvent(QKeyEvent *event) override;

private:
    DiagramItem *m_owner; // Указатель на элемент диаграммы, которому принадлежит этот текст
};

#endif // EDITABLETEXTITEM_H
