/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "historyitem.h"
#include <QPainter>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <cmath>
#include "groupitem.h"
#include <QMessageBox>
HistoryItem::HistoryItem(QGraphicsItem *parent)
    : DiagramItem(parent), m_mode(Deep) // По умолчанию Deep (H*)
{
    // Начальный размер
    setRect(0, 0, 60, 60);
    setType(QObject::tr("История"));
    setLabel("History_" + QString::number(id()));
    m_anchorPoints.clear();
    m_lastValidPos = pos(); // Инициализируем начальной позицией
    // Флаги взаимодействия
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
}

void HistoryItem::setHistoryMode(HistoryMode mode)
{
    m_mode = mode;
    update(); // Перерисовываем блок, чтобы обновить букву (H или H*)
}

HistoryItem::HistoryMode HistoryItem::historyMode() const
{
    return m_mode;
}

QRectF HistoryItem::boundingRect() const
{
    return rect().adjusted(-2, -2, 2, 2);
}

QPainterPath HistoryItem::shape() const
{
    QPainterPath path;
    path.addEllipse(rect()); // Форма коллизии - круг
    return path;
}

// --- ПРИВЯЗКА К СЕТКЕ ---
QVariant HistoryItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene()) {
        qreal gridSize = 16.0;
        QPointF newPos = value.toPointF();
        qreal x = std::round(newPos.x() / gridSize) * gridSize;
        qreal y = std::round(newPos.y() / gridSize) * gridSize;
        return DiagramItem::itemChange(change, QPointF(x, y));
    }
    return DiagramItem::itemChange(change, value);
}

void HistoryItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    DiagramItem::mouseReleaseEvent(event);

    qreal gridSize = 16.0;
    QRectF r = rect();

    qreal newW = std::round(r.width() / gridSize) * gridSize;
    qreal newH = std::round(r.height() / gridSize) * gridSize;

    if (newW < gridSize) newW = gridSize;
    if (newH < gridSize) newH = gridSize;

    if (!qFuzzyCompare(newW, r.width()) || !qFuzzyCompare(newH, r.height())) {
        prepareGeometryChange();
        QPointF currentPos = pos();
        QPointF rectOffset = r.topLeft();
        setRect(0, 0, newW, newH);
        setPos(currentPos + rectOffset);
    }

    QPointF finalPos = pos();
    qreal x = std::round(finalPos.x() / gridSize) * gridSize;
    qreal y = std::round(finalPos.y() / gridSize) * gridSize;
    if (finalPos != QPointF(x, y)) {
        setPos(x, y);
    }

    updateHandlesPosition();
    m_anchorPoints.clear();
    generateAnchorPoints();
    update();
    // === НОВАЯ ЗАЩИТА ПРИ ПЕРЕТАСКИВАНИИ ===
        bool inGroup = false;
        if (scene()) {
                    for (QGraphicsItem *item : scene()->items()) {
                        if (GroupItem *g = dynamic_cast<GroupItem*>(item)) {
                            // Проверяем, находится ли центр нашего блока внутри какой-либо рамки
                            if (g->sceneBoundingRect().contains(sceneBoundingRect().center())) {
                                inGroup = true;
                                break;
                            }
                        }
                    }
                }

                // Если отпустили мышь, а рамки под блоком нет — предупреждаем
                if (!inGroup) {
                    QMessageBox::warning(nullptr, QObject::tr("Внимание!"), QObject::tr("Вы вытащили блок 'История' за пределы рамки!\n\nИсторическое состояние не имеет смысла вне группы. Пожалуйста, верните блок обратно в рамку."));
                }
        // =======================================

}

// --- ГЕНЕРАЦИЯ ТОЧЕК ---
void HistoryItem::generateAnchorPoints()
{
    m_anchorPoints.clear();
    QRectF r = rect();

    // Вверх, Вниз, Влево, Вправо
    m_anchorPoints.append(QPointF(r.center().x(), r.top()));
    m_anchorPoints.append(QPointF(r.center().x(), r.bottom()));
    m_anchorPoints.append(QPointF(r.left(), r.center().y()));
    m_anchorPoints.append(QPointF(r.right(), r.center().y()));
}

// --- ОТРИСОВКА ---
void HistoryItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QPen pen(Qt::black, 2, Qt::SolidLine);
    if (isSelected()) pen.setColor(Qt::blue);

    painter->setPen(pen);

    // Светло-зеленый оттенок для отличия от других круглых блоков
    painter->setBrush(QColor(235, 255, 235));

    // Рисуем КРУГ
    painter->drawEllipse(rect());

    // Текст по центру
    painter->setPen(Qt::black);
    QFont font = painter->font();
    QRectF textRect = rect();

    font.setBold(true);
    font.setPointSize(16);
    painter->setFont(font);

    // Рисуем H или H* в зависимости от режима
    if (m_mode == Shallow) {
        painter->drawText(textRect, Qt::AlignCenter, "H");
    } else {
        painter->drawText(textRect, Qt::AlignCenter, "H*");
    }

    // Точки подключения
    if (m_showAnchorPoints) {
        painter->setBrush(Qt::red);
        painter->setPen(Qt::NoPen);

        if (m_anchorPoints.isEmpty()) {
            generateAnchorPoints();
        }

        qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
        if (lod < 0.1) lod = 1.0;
        qreal visualRadius = 3.0 / lod;

        foreach (const QPointF &pt, m_anchorPoints) {
            painter->drawEllipse(pt, visualRadius, visualRadius);
        }
    }
}

// --- ДВОЙНОЙ КЛИК: НАСТРОЙКА ---
void HistoryItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QDialog dialog;
    dialog.setWindowTitle(QObject::tr("Настройка истории"));
    dialog.setMinimumWidth(380); // Немного увеличили ширину для удобства

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    layout->addWidget(new QLabel(QObject::tr("Выберите тип исторического состояния:")));
    QComboBox *modeCombo = new QComboBox(&dialog);

    // Поменяли порядок: Deep (H*) теперь первый
    modeCombo->addItem(QObject::tr("Deep History (H*) - Вся вложенность"), Deep);
    modeCombo->addItem(QObject::tr("Shallow History (H) - Только текущий уровень"), Shallow);

    // Устанавливаем текущий выбор (индекс 0 - Deep, индекс 1 - Shallow)
    modeCombo->setCurrentIndex(m_mode == Deep ? 0 : 1);
    layout->addWidget(modeCombo);

    // Горизонтальный контейнер для подсказки и кнопки "i"
    QHBoxLayout *infoLayout = new QHBoxLayout();

    QLabel *infoLabel = new QLabel(
        QObject::tr("Подсказка: исходящая стрелка от этого блока\n"
                    "укажет состояние по умолчанию (если автомат\n"
                    "входит в группу впервые).")
    );
    infoLabel->setStyleSheet("color: gray; font-style: italic;");

    // Создаем круглую синюю кнопку информации
    QPushButton *infoBtn = new QPushButton("i", &dialog);
    infoBtn->setFixedSize(22, 22);
    infoBtn->setStyleSheet(
        "QPushButton { "
        "   border-radius: 11px; "
        "   background-color: #0078D7; "
        "   color: white; "
        "   font-weight: bold; "
        "   font-family: Arial; "
        "   border: none; "
        "} "
        "QPushButton:hover { background-color: #005A9E; }"
    );
    infoBtn->setCursor(Qt::PointingHandCursor);

    // Логика окна справки
    QObject::connect(infoBtn, &QPushButton::clicked, [&dialog]() {
        QMessageBox::information(&dialog,
            QObject::tr("Справка: Типы истории"),
            QObject::tr("Глубокая история (Deep History, H*) используется по умолчанию для всех кодогенераторов "
                        "(C++, Java, Python, Arduino, Встраиваемый C++), так как трансляторы всегда запоминают "
                        "самое точное состояние внутри рамки.\n\n"
                        "Поверхностная история (Shallow History, H) имеет смысл только при экспорте схемы в формат SCXML, "
                        "где поддерживается запоминание только верхнего уровня.\n\n"
                        "Для генерации программного кода всегда используйте Глубокую историю (H*)."));
    });

    infoLayout->addWidget(infoLabel);
    infoLayout->addStretch(); // Сдвигаем кнопку вправо
    infoLayout->addWidget(infoBtn);

    layout->addLayout(infoLayout);

    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *btnOk = new QPushButton(QObject::tr("OK"));
    QPushButton *btnCancel = new QPushButton(QObject::tr("Отмена"));
    buttons->addWidget(btnOk);
    buttons->addWidget(btnCancel);
    layout->addLayout(buttons);

    QObject::connect(btnOk, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        setHistoryMode(static_cast<HistoryMode>(modeCombo->currentData().toInt()));
    }
}
