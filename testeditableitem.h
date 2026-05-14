#ifndef TESTEDITABLEITEM_H
#define TESTEDITABLEITEM_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

/**
 *  Простой класс: прямоугольник с одним EditableTextItem-заголовком,
 *  который можно править по двойному клику.
 */
class TestEditableItem : public QGraphicsRectItem
{
public:
    explicit TestEditableItem(QGraphicsItem *parent = nullptr);

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

    // Вызывается из фокуса текстового элемента
    void finishEditing();

private:
    class EditableText : public QGraphicsTextItem {
    public:
        EditableText(QGraphicsItem *parentItem, TestEditableItem *owner);

    protected:
        void focusOutEvent(QFocusEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;

    private:
        TestEditableItem *m_owner;
    };

    EditableText *m_text;
};

#endif // TESTEDITABLEITEM_H
