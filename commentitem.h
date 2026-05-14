/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef COMMENTITEM_H
#define COMMENTITEM_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsScene> // <--- ВАЖНО: Добавлено для доступа к scene()->selectedItems()

class CommentItem : public QGraphicsRectItem
{
public:
    CommentItem(const QString &text = "Комментарий", QGraphicsItem *parent = nullptr);

    // Текст
    void setText(const QString &text);
    QString text() const;

    // Привязка (к блоку или стрелке)
    void setLinkedItem(QGraphicsItem *item);
    QGraphicsItem* linkedItem() const;

    // Настройка генерации кода
    void setGenerateCode(bool enable);
    bool isGenerateCode() const;

    // Тип для сцены
    enum { Type = UserType + 10 };
    int type() const override { return Type; }
    // --- НОВОЕ: УНИВЕРСАЛЬНЫЙ ПОИСК КОММЕНТАРИЕВ ---
        // prefix - это символы комментария (например "// " для C++, "# " для Python)
        static QString getAssociatedCode(QGraphicsItem* target, const QString& prefix);
        // -----------------------------------------------
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    void updateGeometry(); // Пересчет размера фона под текст

    QGraphicsTextItem *m_textItem; // Сам текст
    QGraphicsItem *m_linkedItem;   // К чему привязан (может быть nullptr)
    bool m_generateCode;           // Генерировать ли в код?
};

#endif // COMMENTITEM_H
