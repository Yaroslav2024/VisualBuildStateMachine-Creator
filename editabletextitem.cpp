/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "editabletextitem.h"
#include "diagramitem.h"
#include <QKeyEvent> // Для обработки нажатий клавиш

// Конструктор класса EditableTextItem
EditableTextItem::EditableTextItem(QGraphicsItem *parent, DiagramItem *owner)
    : QGraphicsTextItem(parent), m_owner(owner) // вызываем конструктор базового класса и сохраняем указатель на владельца
{
    // Изначально текст не редактируемый
    setTextInteractionFlags(Qt::NoTextInteraction);

    // Разрешаем элементу получать фокус, чтобы можно было редактировать текст при необходимости
    setFlag(ItemIsFocusable);
}

// Обработчик потери фокуса (например, когда пользователь закончил редактирование текста)
// Обработчик потери фокуса
void EditableTextItem::focusOutEvent(QFocusEvent *event)
{
    QGraphicsTextItem::focusOutEvent(event); // вызываем стандартный обработчик


}

// Обработчик нажатий клавиш на текстовом элементе
void EditableTextItem::keyPressEvent(QKeyEvent *event)
{
    // Если нажата клавиша Enter или Return
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        clearFocus();  // Завершаем редактирование текста (вызывает focusOutEvent)
    } else {
        // Для всех остальных клавиш вызываем стандартный обработчик базового класса
        QGraphicsTextItem::keyPressEvent(event);
    }
}
