/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "statetable.h"
#include <QVBoxLayout>
#include <QHeaderView>
// *** ПОДКЛЮЧАЕМ КЛАССЫ ЭЛЕМЕНТОВ ***
#include "connect.h"         // Чтобы видеть ConnectionItem
#include "diagramitem.h"     // Чтобы видеть DiagramItem
#include "transitiongroup.h" // Чтобы брать текст условий
#include <QGraphicsItem>
#include <QPushButton>      // <-- Для кнопки ID
#include <QClipboard>       // <-- Для копирования
#include <QApplication>     // Доступ к буферу обмена
#include "bridgeitem.h"
#include "historyitem.h"
#include "backitem.h"

StateTable::StateTable(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Transition Table (SCXML View)"));
    resize(1100, 500); // Размер окна
    setupUi();
}

void StateTable::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_table = new QTableWidget(this);

    // Настраиваем колонки
    QStringList headers;
    headers << tr("Source")             // 0
            << tr("Target")             // 1
            << tr("Condition (if)")     // 2
            << tr("ID")                 // 3
            << tr("Action (onTransit)") // 4
            << tr("Priority");          // 5

    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);

    QHeaderView *header = m_table->horizontalHeader();

    // 1. Сначала делаем ВСЕ колонки резиновыми (Stretch)
    header->setSectionResizeMode(QHeaderView::Stretch);

    // 2. Настраиваем исключения:

    // Колонка 3 (ID): Фиксированная ширина (узкая)
    header->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->setColumnWidth(3, 40);

    // Колонка 5 (Priority): Ширина по содержимому (цифре)
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);

    layout->addWidget(m_table);
}

void StateTable::updateFromScene(QGraphicsScene *scene)
{
    if (!scene) return;

    // 1. Очищаем таблицу
    m_table->setRowCount(0);
    m_table->setSortingEnabled(false); // Отключаем сортировку на время вставки (для скорости)

    // 2. Ищем все стрелки на сцене
    QList<ConnectionItem*> connections;
    for (QGraphicsItem *item : scene->items()) {
        // Проверяем, является ли элемент стрелкой (по нашему флагу ConnectionRole)
        if (item->data(ConnectionRole).toBool()) {
            ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item);
            if (conn) connections.append(conn);
        }
    }

    // Умное определение имени блока
    auto getDisplayName = [](DiagramItem *block) -> QString {
        if (!block) return "NULL";
        if (BridgeItem *bridge = dynamic_cast<BridgeItem*>(block)) {
            return tr("[Мост] -> ") + bridge->targetName();
        } else if (dynamic_cast<HistoryItem*>(block)) {
            return tr("[H] История");
        } else if (dynamic_cast<BackItem*>(block)) {
            return tr("[<-] Возврат");
        }
        // Если это обычный блок
        QString label = block->getLabel();
        return label.isEmpty() ? tr("Без имени") : label;
    };

    // Умное определение подсказки (Tooltip)
    auto getDisplayTooltip = [](DiagramItem *block) -> QString {
        if (!block) return "";
        if (dynamic_cast<BridgeItem*>(block)) return tr("Беспроводной переход (Мост)");
        if (dynamic_cast<HistoryItem*>(block)) return tr("Память состояний (History)");
        if (dynamic_cast<BackItem*>(block)) return tr("Возврат из под-автомата (Back)");
        return block->getDescription();
    };

    // 3. Пробегаем по стрелкам и заполняем таблицу
    for (ConnectionItem *conn : connections) {
        DiagramItem *start = conn->startBlock();
        DiagramItem *end = conn->endBlock();
        TransitionGroup *trans = conn->transition();

        // Если стрелка не до конца подключена — пропускаем её
        if (!start || !end || !trans) continue;
        if (conn->isBlocked()) continue; // Если заблокирована — не показываем в таблице

        int row = m_table->rowCount();
        m_table->insertRow(row);

        // --- ИЗМЕНИЛИ КОЛОНКУ 0 ---
        // Используем умную функцию getDisplayName вместо getLabel
        QTableWidgetItem *srcItem = new QTableWidgetItem(getDisplayName(start));
        // Используем умную функцию getDisplayTooltip вместо getDescription
        srcItem->setToolTip(getDisplayTooltip(start));
        m_table->setItem(row, 0, srcItem);

        // --- ИЗМЕНИЛИ КОЛОНКУ 1 ---
        QTableWidgetItem *dstItem = new QTableWidgetItem(getDisplayName(end));
        dstItem->setToolTip(getDisplayTooltip(end));
        m_table->setItem(row, 1, dstItem);

        // Колонка 2: Condition (Условие)
        QTableWidgetItem *condItem = new QTableWidgetItem(trans->conditionText());
        m_table->setItem(row, 2, condItem);

        // --- КОЛОНКА 3: ID (Кнопка) ---
        QPushButton *idBtn = new QPushButton("#");
        idBtn->setFlat(true);
        idBtn->setCursor(Qt::PointingHandCursor);

        QString fullId = conn->id();
        // Подсказка при наведении (Используем .arg() для правильного перевода)
        idBtn->setToolTip(tr("ID: %1\n(Нажмите, чтобы скопировать)").arg(fullId));

        // Копирование при клике
        connect(idBtn, &QPushButton::clicked, [fullId]() {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(fullId);
        });

        m_table->setCellWidget(row, 3, idBtn);
        // ------------------------------

        // Колонка 4: Action (Действие)
        QTableWidgetItem *actItem = new QTableWidgetItem(trans->actionText());
        m_table->setItem(row, 4, actItem);

        // Колонка 5: Priority (Приоритет)
        // Используем setData(DisplayRole), чтобы сортировка работала как числа (1, 2, 10), а не текст (1, 10, 2)
        QTableWidgetItem *prioItem = new QTableWidgetItem();
        prioItem->setData(Qt::DisplayRole, trans->getPriority());
        prioItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(row, 5, prioItem);
    }

    m_table->setSortingEnabled(true); // Включаем сортировку обратно
}
