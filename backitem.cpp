/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "backitem.h"
#include <QPainter>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <cmath>
#include "connect.h"
#include <QMenu>
#include <QAction>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsView>
#include "commentitem.h"
#include "groupitem.h"

BackItem::BackItem(QGraphicsItem *parent)
    : DiagramItem(parent), m_mode(Back), m_targetName("")
{
    // Начальный размер
    setRect(0, 0, 60, 60);
    setType(QObject::tr("Возврат"));
    setLabel("Back_" + QString::number(id()));
    m_anchorPoints.clear();

    // Флаги взаимодействия
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
}

void BackItem::setTargetName(const QString &name)
{
    m_targetName = name;
    update();
}

QString BackItem::targetName() const
{
    return m_targetName;
}

void BackItem::setBackMode(BackMode mode)
{
    m_mode = mode;
    update();
}

BackItem::BackMode BackItem::backMode() const
{
    return m_mode;
}

QRectF BackItem::boundingRect() const
{
    return rect().adjusted(-2, -2, 2, 2);
}

QPainterPath BackItem::shape() const
{
    QPainterPath path;
    path.addEllipse(rect()); // Форма коллизии - круг
    return path;
}

// --- ПРИВЯЗКА К СЕТКЕ (как в мосте) ---
QVariant BackItem::itemChange(GraphicsItemChange change, const QVariant &value)
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

void BackItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
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
}

// --- НОВАЯ ГЕНЕРАЦИЯ ТОЧЕК (Только 4 штуки по центрам) ---
void BackItem::generateAnchorPoints()
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
void BackItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QPen pen(Qt::black, 2, Qt::SolidLine);
    if (isSelected()) pen.setColor(Qt::blue);

    painter->setPen(pen);
    painter->setBrush(QColor(255, 250, 240)); // Легкий желтоватый оттенок

    // Рисуем КРУГ
    painter->drawEllipse(rect());

    // Текст по центру
    painter->setPen(Qt::black);
    QFont font = painter->font();
    QRectF textRect = rect();

    if (m_mode == Back) {
        font.setBold(true);
        font.setPointSize(12);
        painter->setFont(font);
        painter->drawText(textRect, Qt::AlignCenter, "Back");
    } else {
        font.setPointSize(8);
        painter->setFont(font);
        QString target = m_targetName.isEmpty() ? "?" : m_targetName;
        painter->drawText(textRect, Qt::AlignCenter, "Ret:\n" + target);
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

// --- ВАЛИДАЦИЯ ---
bool BackItem::validateTarget(const QString &name)
{
    if (!scene()) return false;
    // --- ЗАЩИТА ОТ ПУСТОГО ИМЕНИ ---
        if (name.trimmed().isEmpty()) {
            QMessageBox::warning(nullptr, QObject::tr("Ошибка ввода"),
                                 QObject::tr("Для режима 'Back Return' необходимо указать точное имя целевого блока!"));
            return false;
        }
        // -------------------------------------

    DiagramItem* foundBlock = nullptr;

    foreach (QGraphicsItem *item, scene()->items()) {
        if (DiagramItem *block = dynamic_cast<DiagramItem*>(item)) {
            if (dynamic_cast<BackItem*>(block)) continue; // Игнорируем другие BackItem

            if (block->getDescription() == name || block->getLabel() == name) {
                foundBlock = block;
                break;
            }
        }
    }

    if (!foundBlock) {
        QMessageBox::warning(nullptr, QObject::tr("Ошибка адресации"), QObject::tr("Блок с именем \"%1\" не найден!\n"
                                                                                   "Проверьте правильность написания (Description/Label).").arg(name));
        return false;
    }

    // Защита от рекурсии
    foreach (QGraphicsItem *item, scene()->items()) {
        if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
            if (conn->startBlock() == foundBlock && conn->endBlock() == this) {
                QMessageBox::critical(nullptr, QObject::tr("Обнаружена рекурсия"), QObject::tr("Нельзя ссылаться на блок \"%1\",\n"
                                                                                               "так как от него уже идет стрелка к этому блоку!\n"
                                                                                               "Это создаст бесконечный цикл.").arg(name));
                return false;
            }
        }
    }

    return true;
}

// --- ДВОЙНОЙ КЛИК: УМНОЕ ОКНО ---
void BackItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QDialog dialog;
    dialog.setWindowTitle(QObject::tr("Настройка возврата"));
    dialog.setMinimumWidth(300);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // Выпадающий список
    layout->addWidget(new QLabel(QObject::tr("Тип возврата:")));
    QComboBox *modeCombo = new QComboBox(&dialog);
    modeCombo->addItem(QObject::tr("Back (В текущий блок)"), Back);
    modeCombo->addItem(QObject::tr("Back Return (В указанный блок)"), BackReturn);
    modeCombo->setCurrentIndex(m_mode == Back ? 0 : 1);
    layout->addWidget(modeCombo);

    // Текстовое поле
    layout->addWidget(new QLabel(QObject::tr("Адрес блока (только для Back Return):")));
    QLineEdit *targetEdit = new QLineEdit(m_targetName, &dialog);
    targetEdit->setEnabled(m_mode == BackReturn); // Включаем только если выбран Back Return
    layout->addWidget(targetEdit);

    // Связываем изменение списка с активностью поля
    QObject::connect(modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index) {
        targetEdit->setEnabled(index == 1);
    });

    // Кнопки
    QHBoxLayout *buttons = new QHBoxLayout();
    QPushButton *btnOk = new QPushButton(QObject::tr("OK"));
    QPushButton *btnCancel = new QPushButton(QObject::tr("Отмена"));
    buttons->addWidget(btnOk);
    buttons->addWidget(btnCancel);
    layout->addLayout(buttons);

    QObject::connect(btnOk, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(btnCancel, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Если нажали ОК
    if (dialog.exec() == QDialog::Accepted) {
        BackMode selectedMode = static_cast<BackMode>(modeCombo->currentData().toInt());
        QString text = targetEdit->text();

        if (selectedMode == BackReturn) {
            if (validateTarget(text)) {
                setBackMode(selectedMode);
                setTargetName(text);
            }
        } else {
            // Если выбран просто Back, сбрасываем имя цели
            setBackMode(Back);
            setTargetName("");
        }
    }
}
// ==========================================================
// --- КОНТЕКСТНОЕ МЕНЮ ДЛЯ ВОЗВРАТА (ПКМ) ---
// ==========================================================
void BackItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    // --- 1. ОПРЕДЕЛЯЕМ, В КАКОМ ОКНЕ МЫ КЛИКНУЛИ ---
    bool inDebugger = false;
    QGraphicsView *currentView = nullptr;

    if (event->widget()) {
        currentView = qobject_cast<QGraphicsView*>(event->widget()->parentWidget());
        if (event->widget()->window()->objectName() == "DebuggerWindow" ||
            event->widget()->window()->windowTitle().contains("Debugger")) {
            inDebugger = true;
        }
    }

    QMenu menu;
    QAction *goToAct = menu.addAction(QObject::tr("Перейти по адресу блока"));
    QAction *scaleAct = menu.addAction(QObject::tr("Масштабировать"));
    QAction *deleteAct = menu.addAction(QObject::tr("Удалить"));

    // Если это обычный возврат (не по имени), отключаем кнопку перехода
    if (m_mode == Back) {
        goToAct->setEnabled(false);
    }

    // --- 2. ЗАЩИТА В ДЕБАГГЕРЕ ---
    if (inDebugger) {
        scaleAct->setEnabled(false);  // Запрещаем менять размер
        deleteAct->setEnabled(false); // Запрещаем удалять блок
    }

    QAction *selectedAction = menu.exec(event->screenPos());

    if (selectedAction == goToAct) {
        if (m_targetName.isEmpty()) {
            QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Адрес блока не указан!"));
            return;
        }

        DiagramItem *targetBlock = nullptr;
        // Ищем блок на сцене
        for (QGraphicsItem *item : scene()->items()) {
            if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
                if (b->getLabel() == m_targetName || b->getDescription() == m_targetName) {
                    targetBlock = b;
                    break;
                }
            }
        }

        // Если нашли — прыгаем к нему
        if (targetBlock) {
            // --- ИСПРАВЛЕНИЕ: ДВИГАЕМ ПРАВИЛЬНУЮ КАМЕРУ ---
            if (currentView) {
                currentView->centerOn(targetBlock);
            } else if (!scene()->views().isEmpty()) {
                scene()->views().first()->centerOn(targetBlock); // Резервный вариант
            }
            scene()->clearSelection();
            targetBlock->setSelected(true);
        } else {
            QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Блок с адресом \"%1\" не найден!\nПроверьте правильность написания.").arg(m_targetName));
        }
    }
    // --- ОБРАБОТКА МАСШТАБИРОВАНИЯ ---
    else if (selectedAction == scaleAct) {
        showResizeHandles(); // Вызываем точки для изменения размера
    }
    // --- ОБРАБОТКА УДАЛЕНИЯ ---
    else if (selectedAction == deleteAct) {
        // --- ЗАЩИТА: ПРОВЕРКА НАЛИЧИЯ СТРЕЛОК ---
        if (!m_connections.isEmpty()) {
            QMessageBox::warning(nullptr, QObject::tr("Удаление заблокировано"),
                QObject::tr("К этому спецблоку подключены стрелки!\n\nСначала удалите все привязанные стрелки."));
            return; // Отменяем удаление
        }
        // ==========================================================

        // --- ЗАЩИТА: ПРОВЕРКА ПРИВЯЗАННОГО СТИКЕРА ---
        if (scene()) {
            for (QGraphicsItem *sceneItem : scene()->items()) {
                if (CommentItem *comment = dynamic_cast<CommentItem*>(sceneItem)) {
                    QGraphicsItem* linked = comment->linkedItem();
                    // Сравниваем привязку с текущим объектом (this)
                    if (linked && (linked == this || linked == this->topLevelItem() || linked->topLevelItem() == this->topLevelItem())) {
                        QMessageBox::warning(nullptr, QObject::tr("Удаление заблокировано"),
                            QObject::tr("К этому спецблоку привязан стикер-комментарий!\n\nСначала отвяжите или удалите сам стикер."));
                        return; // Мгновенно отменяем удаление!
                    }
                }
            }
        }
        // ==========================================================

        // --- 1. АВТООЧИСТКА: Безопасно удаляем и отвязываем все стрелки ---
        removeAllConnections();

        // --- НОВОЕ: ОТВЯЗЫВАЕМ БЛОК ОТ РАМОК ПЕРЕД УДАЛЕНИЕМ ---
        if (scene()) {
            for (QGraphicsItem *sceneItem : scene()->items()) {
                if (GroupItem *group = dynamic_cast<GroupItem*>(sceneItem)) {
                    group->removeBlock(this); // Безопасно выписываем блок (this)
                }
            }
        }
        // =========================================================

        scene()->removeItem(this);
        delete this;
    }
}
