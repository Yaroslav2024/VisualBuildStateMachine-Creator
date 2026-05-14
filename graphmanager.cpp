/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "graphmanager.h"
#include <QDebug>
#include <limits>
#include "groupitem.h"
#include "commentitem.h"
#include "bridgeitem.h"
#include "backitem.h"
#include "historyitem.h"

// --- ЗАЩИТА ОТ ОШИБКИ ConnectionRole ---

GraphManager::GraphManager(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), m_scene(scene)
{
    // --- ВАЖНО: Инициализируем "мозг" истории перед тем, как делать снимок! ---
        m_historyManager = new HistoryManager(this);
    // Создаем пустой начальный снимок
    createSnapshot();
}

// ==========================================
// ЛОГИКА СОХРАНЕНИЯ (Сцена -> JSON)
// ==========================================
QJsonObject GraphManager::sceneToJson()
{
    QJsonObject rootObj;
    QJsonArray blocksArray;
    QJsonArray connectionsArray;

    // 1. Сначала даем временные ID всем блокам
    QMap<QGraphicsItem*, int> itemToIdMap;
    int idCounter = 0;


    // Собираем БЛОКИ
        foreach (QGraphicsItem *item, m_scene->items()) {
            // Проверка: это Блок или Мост? (BridgeItem наследуется от DiagramItem)
            if (DiagramItem *block = dynamic_cast<DiagramItem*>(item)) {

                QJsonObject blockObj;
                blockObj["id"] = idCounter;
                blockObj["x"] = block->x();
                blockObj["y"] = block->y();

                blockObj["width"] = block->rect().width();
                blockObj["height"] = block->rect().height();
                blockObj["rect_x"] = block->rect().x();
                blockObj["rect_y"] = block->rect().y();

                // --- ПРОВЕРКА НА МОСТ ---
                if (BridgeItem *bridge = dynamic_cast<BridgeItem*>(item)) {
                    blockObj["type"] = "bridge"; // Специальная метка типа
                    blockObj["targetName"] = bridge->targetName();
                }
                else if (BackItem *backItem = dynamic_cast<BackItem*>(item)) {
                                    blockObj["type"] = "back"; // Специальная метка типа
                                    blockObj["targetName"] = backItem->targetName();
                                    blockObj["backMode"] = static_cast<int>(backItem->backMode()); // Сохраняем режим (0 или 1)
                                }
                else if (HistoryItem *histItem = dynamic_cast<HistoryItem*>(item)) {
                                    blockObj["type"] = "history"; // Метка для истории
                                    // --- Сохраняем режим истории (0 - Shallow, 1 - Deep) ---
                                    blockObj["historyMode"] = static_cast<int>(histItem->historyMode());
                                }
                else {
                    // Это ОБЫЧНЫЙ блок
                    blockObj["type"] = block->getType();
                    blockObj["description"] = block->getDescription();
                    blockObj["label"] = block->getLabel();
                    blockObj["codeEnter"] = block->getOnEnter();
                    blockObj["codeExit"] = block->getOnExit();
                }
                // ------------------------

                itemToIdMap[block] = idCounter; // Регистрируем ID (важно для стрелок)
                blocksArray.append(blockObj);
                idCounter++;
            }
        }
    // Собираем СТРЕЛКИ
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
            if (conn->startBlock() && conn->endBlock()) {
                QJsonObject connObj;
                connObj["startId"] = itemToIdMap[conn->startBlock()];
                connObj["endId"] = itemToIdMap[conn->endBlock()];

                // --- НОВОЕ: Сохраняем ТОЧНЫЕ ЛОКАЛЬНЫЕ КООРДИНАТЫ ---
                // Это позволяет восстановить стрелку точь-в-точь как она была,
                // без угадывания "ближайшей" точки.
                QPointF sLoc = conn->startAnchorLocal();
                QPointF eLoc = conn->endAnchorLocal();

                connObj["startAnchorX"] = sLoc.x();
                connObj["startAnchorY"] = sLoc.y();
                connObj["endAnchorX"] = eLoc.x();
                connObj["endAnchorY"] = eLoc.y();
                // ----------------------------------------------------

                connObj["condition"] = conn->transition()->conditionText();
                connObj["action"] = conn->transition()->actionText();
                connObj["priority"] = conn->transition()->getPriority();

                connectionsArray.append(connObj);
            }
        }
    }

    // --- СОХРАНЕНИЕ ГРУПП (Вставляем этот блок перед return rootObj) ---
        QJsonArray groupsArray;
        foreach (QGraphicsItem *item, m_scene->items()) {
            if (GroupItem *group = dynamic_cast<GroupItem*>(item)) {
                QJsonObject groupObj;
                // Сохраняем абсолютные координаты рамки на сцене
                QRectF rect = group->sceneBoundingRect();
                groupObj["x"] = rect.x();
                groupObj["y"] = rect.y();
                groupObj["w"] = rect.width();
                groupObj["h"] = rect.height();
                groupObj["name"] = group->name();
                groupObj["isSizeLocked"] = group->isSizeLocked();
                groupObj["isScaleMode"] = group->isScaleMode();
                groupsArray.append(groupObj);
            }
        }
        rootObj["groups"] = groupsArray;
        // ------------------------------------------------------------------

    rootObj["blocks"] = blocksArray;
    rootObj["connections"] = connectionsArray;
    // --- СОХРАНЕНИЕ КОММЕНТАРИЕВ ---
        QJsonArray commentsArray;
        foreach (QGraphicsItem *item, m_scene->items()) {
            if (CommentItem *comm = dynamic_cast<CommentItem*>(item)) {
                QJsonObject commObj;
                commObj["x"] = comm->x();
                commObj["y"] = comm->y();
                commObj["text"] = comm->text();
                commObj["gen"] = comm->isGenerateCode();

                // ПРОВЕРКА ПРИВЯЗКИ
                if (comm->linkedItem()) {
                    QGraphicsItem* target = comm->linkedItem();

                    // ВАРИАНТ 1: Привязан к БЛОКУ
                    if (DiagramItem* block = dynamic_cast<DiagramItem*>(target)) {
                        if (itemToIdMap.contains(block)) {
                            commObj["targetType"] = "block";
                            commObj["targetId"] = itemToIdMap[block];
                        }
                    }
                    // ВАРИАНТ 2: Привязан к СТРЕЛКЕ (НОВОЕ)
                    else if (ConnectionItem* arrow = dynamic_cast<ConnectionItem*>(target)) {
                        // Чтобы найти стрелку при загрузке, сохраняем ID её начала и конца
                        if (arrow->startBlock() && arrow->endBlock()) {
                            if (itemToIdMap.contains(arrow->startBlock()) &&
                                itemToIdMap.contains(arrow->endBlock())) {

                                commObj["targetType"] = "arrow";
                                commObj["arrowStartId"] = itemToIdMap[arrow->startBlock()];
                                commObj["arrowEndId"] = itemToIdMap[arrow->endBlock()];
                            }
                        }
                    }
                }
                commentsArray.append(commObj);
            }
        }
        rootObj["comments"] = commentsArray;
        // -------------------------------
    return rootObj;
}

// ==========================================
// ЛОГИКА ЗАГРУЗКИ (JSON -> Сцена)
// ==========================================
// ==========================================
// ЛОГИКА ЗАГРУЗКИ (JSON -> Сцена)
// ==========================================
// Добавили аргумент clearScene
void GraphManager::jsonToScene(const QJsonObject &json, bool clearScene, QPointF dropPos, bool useDropPos)
{
    // --- 1. ОЧИСТКА (Только если просили очистить) ---
    if (clearScene) {
        QList<QGraphicsItem*> allItems = m_scene->items();
        // Сначала удаляем стрелки
        for (QGraphicsItem *item : allItems) {
            if (dynamic_cast<ConnectionItem*>(item)) {
                m_scene->removeItem(item);
                delete item;
            }
        }
        // --- УДАЛЯЕМ КОММЕНТАРИИ (ЗАЩИТА ОТ ВЫЛЕТОВ!) ---
                allItems = m_scene->items();
                for (QGraphicsItem *item : allItems) {
                    if (dynamic_cast<CommentItem*>(item)) {
                        m_scene->removeItem(item);
                        delete item;
                    }
                }
                allItems = m_scene->items();
                        for (QGraphicsItem *item : allItems) {
                            if (dynamic_cast<GroupItem*>(item)) {
                                m_scene->removeItem(item);
                                delete item;
                            }
                        }
        // Потом блоки
        allItems = m_scene->items();
        for (QGraphicsItem *item : allItems) {
            if (dynamic_cast<DiagramItem*>(item)) {
                m_scene->removeItem(item);
                delete item;
            }
        }
    }

    // --- 2. РАСЧЕТ СДВИГА (ИМПОРТ ПО ЦЕНТРУ ЭКРАНА) ---
        qreal offsetX = 0.0;
        qreal offsetY = 0.0;

        if (useDropPos) {
            QJsonArray blocksArray = json["blocks"].toArray();
            qreal minX = std::numeric_limits<qreal>::max();
            qreal minY = std::numeric_limits<qreal>::max();
            qreal maxX = std::numeric_limits<qreal>::lowest();
            qreal maxY = std::numeric_limits<qreal>::lowest();

            // Находим реальные границы импортируемого графа (из файла)
            for (const QJsonValue &val : blocksArray) {
                QJsonObject obj = val.toObject();
                if (obj["x"].toDouble() < minX) minX = obj["x"].toDouble();
                if (obj["x"].toDouble() > maxX) maxX = obj["x"].toDouble();
                if (obj["y"].toDouble() < minY) minY = obj["y"].toDouble();
                if (obj["y"].toDouble() > maxY) maxY = obj["y"].toDouble();
            }

            // Если в файле есть блоки — вычисляем сдвиг к центру экрана
            if (minX != std::numeric_limits<qreal>::max()) {
                qreal importWidth = maxX - minX;
                qreal importHeight = maxY - minY;

                // Смещаем весь граф так, чтобы его математический центр
                // идеально совпал с точкой взгляда камеры (dropPos)
                offsetX = dropPos.x() - (minX + importWidth / 2.0);
                offsetY = dropPos.y() - (minY + importHeight / 2.0);
            }
        }

    // --- 3. ЗАГРУЗКА БЛОКОВ ---
    QJsonArray blocksArray = json["blocks"].toArray();
    QJsonArray connArray = json["connections"].toArray();

    QMap<int, DiagramItem*> idToBlockMap;


    for (const QJsonValue &val : blocksArray) {
            QJsonObject obj = val.toObject();
            int id = obj["id"].toInt();

            // --- НОВОЕ: Читаем размеры и смещение для восстановления масштаба ---
                        double w = obj["width"].toDouble();
                        double h = obj["height"].toDouble();
                        double rx = obj.contains("rect_x") ? obj["rect_x"].toDouble() : 0.0;
                        double ry = obj.contains("rect_y") ? obj["rect_y"].toDouble() : 0.0;
                        // -------------------------------------------------------------------

            DiagramItem *block = nullptr;

            // --- ПРОВЕРКА НА МОСТ ---
            if (obj["type"].toString() == "bridge") {
                // Создаем МОСТ
                BridgeItem* bridge = new BridgeItem();
                bridge->setTargetName(obj["targetName"].toString());
                block = bridge;
            }
            else if (obj["type"].toString() == "back") {
                            // Создаем ВОЗВРАТ
                            BackItem* backItem = new BackItem();
                            backItem->setTargetName(obj["targetName"].toString());
                            if (obj.contains("backMode")) {
                                backItem->setBackMode(static_cast<BackItem::BackMode>(obj["backMode"].toInt()));
                            }
                            block = backItem;
                        }
            else if (obj["type"].toString() == "history") {
                            // Создаем ИСТОРИЮ
                            HistoryItem* historyItem = new HistoryItem();
                            // --- ДОБАВЛЕНО: Восстанавливаем состояние H или H* ---
                            // --- Восстанавливаем режим истории ---
                                            if (obj.contains("historyMode")) {
                                                historyItem->setHistoryMode(static_cast<HistoryItem::HistoryMode>(obj["historyMode"].toInt()));
                                            }
                            block = historyItem;
                        }
            else {
                // Создаем ОБЫЧНЫЙ БЛОК
                block = new DiagramItem();
                block->setType(obj["type"].toString()); // "Стартовый" и т.д.
                block->setDescription(obj["description"].toString());
                block->setOnEnter(obj["codeEnter"].toString());
                block->setOnExit(obj["codeExit"].toString());
                block->setLabel(obj["label"].toString());
            }
            // ------------------------

            // Общая настройка координат (с учетом сдвига при импорте)
            block->setPos(obj["x"].toDouble() + offsetX, obj["y"].toDouble() + offsetY);

            // --- НОВОЕ: Устанавливаем точный размер и смещение ---
                        if (w > 0 && h > 0) {
                            block->setRect(rx, ry, w, h);
                        }
            m_scene->addItem(block);
            idToBlockMap[id] = block;
        }
    // --- 4. ЗАГРУЗКА СТРЕЛОК ---
    for (const QJsonValue &val : connArray) {
        QJsonObject obj = val.toObject();
        int startId = obj["startId"].toInt();
        int endId = obj["endId"].toInt();

        DiagramItem* startBlock = idToBlockMap.value(startId, nullptr);
        DiagramItem* endBlock = idToBlockMap.value(endId, nullptr);

        if (startBlock && endBlock) {
            ConnectionItem *conn = new ConnectionItem();
            m_scene->addItem(conn);
            conn->setData(ConnectionRole, true);

            // Визуал
            QPolygonF tri;
            tri << QPointF(0, 0) << QPointF(-10, -3) << QPointF(-10, 3);
            QGraphicsPolygonItem *arrow = new QGraphicsPolygonItem(tri);
            arrow->setBrush(Qt::black);
            arrow->setPen(Qt::NoPen);
            conn->addToGroup(arrow);
            conn->registerItem(arrow);

            TransitionGroup *transGroup = new TransitionGroup();
            conn->addToGroup(transGroup);
            conn->registerItem(transGroup);

            // КООРДИНАТЫ (Ваш корректный код для точного восстановления)
            QPointF startAnchor;
            QPointF endAnchor;

            if (obj.contains("startAnchorX")) {
                startAnchor = QPointF(obj["startAnchorX"].toDouble(), obj["startAnchorY"].toDouble());
                endAnchor = QPointF(obj["endAnchorX"].toDouble(), obj["endAnchorY"].toDouble());
            } else {
                QPointF startCenterScene = startBlock->scenePos() + startBlock->rect().center();
                QPointF endCenterScene = endBlock->scenePos() + endBlock->rect().center();
                startAnchor = startBlock->nearestAnchorToScenePoint(endCenterScene);
                endAnchor = endBlock->nearestAnchorToScenePoint(startCenterScene);
            }

            conn->setStart(startBlock, startAnchor);
            conn->setEnd(endBlock, endAnchor);

            startBlock->addConnection(conn);
            endBlock->addConnection(conn);

            if (conn->transition()) {
                conn->transition()->setConditionText(obj["condition"].toString());
                conn->transition()->setActionText(obj["action"].toString());
                conn->transition()->setPriority(obj["priority"].toInt());
            }

            conn->updatePosition();
        }
    }
    // --- 5. ЗАГРУЗКА ГРУПП (Вставляем в конец функции jsonToScene) ---
        if (json.contains("groups") && json["groups"].isArray()) {
            QJsonArray groupsArray = json["groups"].toArray();

            for (const QJsonValue &val : groupsArray) {
                QJsonObject groupObj = val.toObject();

                // 1. Восстанавливаем координаты с учетом сдвига (offsetX/offsetY для Импорта)
                qreal x = groupObj["x"].toDouble() + offsetX;
                qreal y = groupObj["y"].toDouble() + offsetY;
                qreal w = groupObj["w"].toDouble();
                qreal h = groupObj["h"].toDouble();
                QString name = groupObj["name"].toString();

                QRectF rect(x, y, w, h);

                // 2. Создаем группу
                GroupItem *group = new GroupItem(rect, name);
                m_scene->addItem(group);
                if (groupObj.contains("isSizeLocked")) {
                                    group->setSizeLocked(groupObj["isSizeLocked"].toBool());
                                }
                                if (groupObj.contains("isScaleMode")) {
                                    group->setScaleMode(groupObj["isScaleMode"].toBool());
                                }
                // 3. ВАЖНО: "Ловим" блоки, которые оказались внутри загруженной рамки
                // Это восстанавливает логику "умного расширения" и привязки
                foreach (QGraphicsItem *item, m_scene->items()) {
                    if (DiagramItem *block = dynamic_cast<DiagramItem*>(item)) {
                        // Если центр блока находится внутри прямоугольника группы
                        if (rect.contains(block->sceneBoundingRect().center())) {
                            group->addBlock(block);
                        }
                    }
                }
            }
        }
        // ------------------------------------------------------------------
        // --- ЗАГРУЗКА КОММЕНТАРИЕВ ---
            if (json.contains("comments") && json["comments"].isArray()) {
                QJsonArray commArray = json["comments"].toArray();

                for (const QJsonValue &val : commArray) {
                    QJsonObject obj = val.toObject();

                    CommentItem *comm = new CommentItem(obj["text"].toString());
                    comm->setPos(obj["x"].toDouble() + offsetX, obj["y"].toDouble() + offsetY);
                    comm->setGenerateCode(obj["gen"].toBool());

                    // ВОССТАНОВЛЕНИЕ СВЯЗИ
                    QString type = obj["targetType"].toString();

                    // СЛУЧАЙ 1: Привязка к БЛОКУ (или старый формат без type)
                    if (type == "block" || (!obj.contains("targetType") && obj.contains("targetId"))) {
                        int targetId = obj["targetId"].toInt();
                        if (idToBlockMap.contains(targetId)) {
                            comm->setLinkedItem(idToBlockMap[targetId]);
                        }
                    }
                    // СЛУЧАЙ 2: Привязка к СТРЕЛКЕ (НОВОЕ)
                    else if (type == "arrow") {
                        int startId = obj["arrowStartId"].toInt();
                        int endId = obj["arrowEndId"].toInt();

                        // Проверяем, существуют ли такие блоки
                        if (idToBlockMap.contains(startId) && idToBlockMap.contains(endId)) {
                            DiagramItem* startBlock = idToBlockMap[startId];
                            DiagramItem* endBlock = idToBlockMap[endId];

                            // Теперь ищем стрелку на сцене, которая соединяет эти два блока
                            foreach (QGraphicsItem *itm, m_scene->items()) {
                                if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(itm)) {
                                    // Проверяем направление стрелки
                                    if (conn->startBlock() == startBlock && conn->endBlock() == endBlock) {
                                        comm->setLinkedItem(conn);
                                        break; // Нашли нужную стрелку!
                                    }
                                }
                            }
                        }
                    }

                    m_scene->addItem(comm);
                }
            }
            // -----------------------------
            // -----------------------------
}
// ==========================================
// ФАЙЛОВЫЕ ОПЕРАЦИИ
// ==========================================
bool GraphManager::saveToFile(const QString &fileName, const QString &headerCode, int langId, const QJsonArray &sectors)
{
    QJsonObject json = sceneToJson();

    // ---  Сохраняем  ---
    json["headerCode"] = headerCode;
    json["projectLanguage"] = langId;
    json["sectors"] = sectors;
    // -----------------------------------

    QJsonDocument doc(json);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Could not open file for writing";
        return false;
    }

    file.write(doc.toJson());
    file.close();
    return true;
}

bool GraphManager::loadFromFile(const QString &fileName, QString &headerCode, int &langId, QJsonArray &sectors, QPointF dropPos, bool useDropPos)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open file for reading";
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(data));

    if (doc.isNull()) return false;

    // true = Очистить сцену перед загрузкой
    jsonToScene(doc.object(), true, dropPos, useDropPos);

    // Загружаем Header ---
    if (doc.object().contains("headerCode")) {
        headerCode = doc.object()["headerCode"].toString();
    } else {
        headerCode = ""; // Очищаем, если в старом файле не было хедера
    }
    langId = doc.object()["projectLanguage"].toInt(0);
    // -----------------------------------
    sectors = doc.object()["sectors"].toArray();
    clearHistory(); // Железобетонно очищаем историю при загрузке файла

    return true;
}

// НОВОЕ: Импорт (Добавляет рядом)
bool GraphManager::importFromFile(const QString &fileName, QString &headerCode, QPointF dropPos)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Could not open file for reading";
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(data));
    if (doc.isNull()) return false;



        // Передаем dropPos, чтобы блоки появились там, куда смотрит пользователь!
        jsonToScene(doc.object(), false, dropPos, true);

    // --- ДОБАВЛЕНО: Импортируем Header, только если текущий пуст ---
    if (headerCode.trimmed().isEmpty() && doc.object().contains("headerCode")) {
        headerCode = doc.object()["headerCode"].toString();
    }
    // -----------------------------------

    // Добавляем это действие в историю Undo
    createSnapshot();
    return true;
}
// ==========================================
// ==========================================
// ==========================================
// UNDO / REDO (Через HistoryManager)
// ==========================================
void GraphManager::createSnapshot()
{
    // Упаковываем сцену в JSON (как и раньше)
    QJsonObject json = sceneToJson();
    QJsonDocument doc(json);
    QByteArray snapshot = doc.toJson(QJsonDocument::Compact);

    // Передаем сохранение кадра новому классу!
    if (m_historyManager) {
        m_historyManager->saveState(snapshot);
    }
}

void GraphManager::undo()
{
    if (!m_historyManager) return;

    QByteArray snapshot = m_historyManager->undo();
    if (!snapshot.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(snapshot);
        // true = Очистить сцену перед восстановлением снимка
        jsonToScene(doc.object(), true);
    }
}

void GraphManager::redo()
{
    if (!m_historyManager) return;

    QByteArray snapshot = m_historyManager->redo();
    if (!snapshot.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(snapshot);
        // true = Очистить сцену перед восстановлением снимка
        jsonToScene(doc.object(), true);
    }
}

// --- НОВАЯ ФУНКЦИЯ ДЛЯ ЗАЩИТЫ ОТ ВЫЛЕТОВ ---
void GraphManager::clearHistory()
{
    if (m_historyManager) {
        m_historyManager->clear(); // Полностью чистим память истории
    }
    createSnapshot(); // Делаем первый "чистый" кадр для нового проекта
}
// ==========================================
// ЛОГИКА КОПИРОВАНИЯ ВЫДЕЛЕНИЯ (Selection -> JSON)
// ==========================================
QJsonObject GraphManager::selectionToJson()
{
    QJsonObject rootObj;
    QJsonArray blocksArray;
    QJsonArray connectionsArray;
    QJsonArray groupsArray;
    QJsonArray commentsArray;

    QList<QGraphicsItem*> selectedItems = m_scene->selectedItems();
    if (selectedItems.isEmpty()) return rootObj; // Нечего копировать

    QMap<QGraphicsItem*, int> itemToIdMap;
    int idCounter = 0;

    // 1. СОБИРАЕМ ВЫДЕЛЕННЫЕ БЛОКИ
    foreach (QGraphicsItem *item, selectedItems) {
        if (DiagramItem *block = dynamic_cast<DiagramItem*>(item)) {
            QJsonObject blockObj;
            blockObj["id"] = idCounter;
            blockObj["x"] = block->x();
            blockObj["y"] = block->y();
            blockObj["width"] = block->rect().width();
            blockObj["height"] = block->rect().height();
            blockObj["rect_x"] = block->rect().x();
            blockObj["rect_y"] = block->rect().y();

            if (BridgeItem *bridge = dynamic_cast<BridgeItem*>(item)) {
                blockObj["type"] = "bridge";
                blockObj["targetName"] = bridge->targetName();
            } else if (BackItem *backItem = dynamic_cast<BackItem*>(item)) {
                blockObj["type"] = "back";
                blockObj["targetName"] = backItem->targetName();
                blockObj["backMode"] = static_cast<int>(backItem->backMode());
            } else if (HistoryItem *histItem = dynamic_cast<HistoryItem*>(item)) {
                blockObj["type"] = "history";
                blockObj["historyMode"] = static_cast<int>(histItem->historyMode());
            } else {
                blockObj["type"] = block->getType();
                blockObj["description"] = block->getDescription();

                // Добавляем маркер "_copy" к имени переменной, чтобы код C++ потом не сломался от дубликатов
                QString lbl = block->getLabel();
                if (!lbl.isEmpty() && !lbl.endsWith("_copy")) lbl += "_copy";
                blockObj["label"] = lbl;

                blockObj["codeEnter"] = block->getOnEnter();
                blockObj["codeExit"] = block->getOnExit();
            }

            itemToIdMap[block] = idCounter;
            blocksArray.append(blockObj);
            idCounter++;
        }
        // 2. СОБИРАЕМ ВЫДЕЛЕННЫЕ РАМКИ
        else if (GroupItem *group = dynamic_cast<GroupItem*>(item)) {
            QJsonObject groupObj;
            QRectF rect = group->sceneBoundingRect();
            groupObj["x"] = rect.x();
            groupObj["y"] = rect.y();
            groupObj["w"] = rect.width();
            groupObj["h"] = rect.height();
            groupObj["name"] = group->name();
            groupObj["isSizeLocked"] = group->isSizeLocked();
            groupObj["isScaleMode"] = group->isScaleMode();
            groupsArray.append(groupObj);
        }
    }

    // 3. СОБИРАЕМ СТРЕЛКИ (Умное копирование)
    // Сканируем всю сцену, а не только выделение, потому что стрелка может быть не выделена,
    // но мы ДОЛЖНЫ её скопировать, если ОБА её блока скопированы.
    foreach (QGraphicsItem *item, m_scene->items()) {
        if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
            if (conn->startBlock() && conn->endBlock()) {
                // Строгое правило: копируем стрелку ТОЛЬКО если оба конца в списке скопированных блоков
                if (itemToIdMap.contains(conn->startBlock()) && itemToIdMap.contains(conn->endBlock())) {
                    QJsonObject connObj;
                    connObj["startId"] = itemToIdMap[conn->startBlock()];
                    connObj["endId"] = itemToIdMap[conn->endBlock()];

                    QPointF sLoc = conn->startAnchorLocal();
                    QPointF eLoc = conn->endAnchorLocal();
                    connObj["startAnchorX"] = sLoc.x();
                    connObj["startAnchorY"] = sLoc.y();
                    connObj["endAnchorX"] = eLoc.x();
                    connObj["endAnchorY"] = eLoc.y();

                    connObj["condition"] = conn->transition()->conditionText();
                    connObj["action"] = conn->transition()->actionText();
                    connObj["priority"] = conn->transition()->getPriority();

                    connectionsArray.append(connObj);
                }
            }
        }
    }

    // 4. СОБИРАЕМ ВЫДЕЛЕННЫЕ КОММЕНТАРИИ
    foreach (QGraphicsItem *item, selectedItems) {
        if (CommentItem *comm = dynamic_cast<CommentItem*>(item)) {
            QJsonObject commObj;
            commObj["x"] = comm->x();
            commObj["y"] = comm->y();
            commObj["text"] = comm->text();
            commObj["gen"] = comm->isGenerateCode();

            if (comm->linkedItem()) {
                QGraphicsItem* target = comm->linkedItem();
                if (DiagramItem* block = dynamic_cast<DiagramItem*>(target)) {
                    if (itemToIdMap.contains(block)) {
                        commObj["targetType"] = "block";
                        commObj["targetId"] = itemToIdMap[block];
                    }
                } else if (ConnectionItem* arrow = dynamic_cast<ConnectionItem*>(target)) {
                    if (arrow->startBlock() && arrow->endBlock()) {
                        if (itemToIdMap.contains(arrow->startBlock()) && itemToIdMap.contains(arrow->endBlock())) {
                            commObj["targetType"] = "arrow";
                            commObj["arrowStartId"] = itemToIdMap[arrow->startBlock()];
                            commObj["arrowEndId"] = itemToIdMap[arrow->endBlock()];
                        }
                    }
                }
            }
            commentsArray.append(commObj);
        }
    }

    rootObj["blocks"] = blocksArray;
    rootObj["connections"] = connectionsArray;
    rootObj["groups"] = groupsArray;
    rootObj["comments"] = commentsArray;

    return rootObj;
}
