#include "testeditableitem.h"
#include <QGraphicsSceneMouseEvent>
#include <QFocusEvent>
#include <QKeyEvent>

TestEditableItem::TestEditableItem(QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    // простой прямоугольник
    setRect(0, 0, 200, 80);
    setFlag(ItemIsSelectable);
    setFlag(ItemIsMovable);

    // создаём заголовок
    m_text = new EditableText(this, this);
    m_text->setPlainText("Двойной клик");
    m_text->setTextInteractionFlags(Qt::NoTextInteraction);
    m_text->setFlag(ItemIsFocusable);

    // центрируем над блоком
    QRectF tr = m_text->boundingRect();
    m_text->setPos((rect().width() - tr.width())/2, -tr.height() - 5);
}

void TestEditableItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF scenePt = event->scenePos();

    // mapFromScene и contains
    QPointF pt = m_text->mapFromScene(scenePt);
    if (m_text->boundingRect().contains(pt)) {
        m_text->setTextInteractionFlags(Qt::TextEditorInteraction);
        m_text->setFocus(Qt::MouseFocusReason);
        return;
    }
    QGraphicsRectItem::mouseDoubleClickEvent(event);
}

void TestEditableItem::finishEditing()
{
    m_text->setTextInteractionFlags(Qt::NoTextInteraction);
    // центрируем заново
    QRectF tr = m_text->boundingRect();
    m_text->setPos((rect().width() - tr.width())/2, -tr.height() - 5);
}

// реализация вложенного класса
TestEditableItem::EditableText::EditableText(QGraphicsItem *parentItem, TestEditableItem *owner)
    : QGraphicsTextItem(parentItem), m_owner(owner)
{}

void TestEditableItem::EditableText::focusOutEvent(QFocusEvent *event)
{
    QGraphicsTextItem::focusOutEvent(event);
    m_owner->finishEditing();
}

void TestEditableItem::EditableText::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        clearFocus();  // вызовет focusOutEvent
    } else {
        QGraphicsTextItem::keyPressEvent(event);
    }
}
