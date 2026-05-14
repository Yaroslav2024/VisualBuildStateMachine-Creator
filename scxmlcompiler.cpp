/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "scxmlcompiler.h"
#include "syntaxregistry.h"
#include "transitiongroup.h"
#include <QSet>
#include "codeexecutor.h"
#include <QXmlStreamWriter>
#include <QFile>
#include <QMessageBox>
#include <QMap>
#include <QQueue>
#include "connect.h"
#include "bridgeitem.h"
#include <QRegularExpression>
#include "backitem.h"
#include "historyitem.h"
#include "groupitem.h"

// === 1. ФУНКЦИЯ ПОИСКА ПЕРЕМЕННЫХ (FIX для универсальности) ===
struct VarInfo {
    QString name;
    QString val;
    // ПРОФЕССИОНАЛЬНО: Добавляем конструктор для безопасной инициализации
    VarInfo(const QString& n, const QString& v) : name(n), val(v) {}
};

static QList<VarInfo> scanForVariables(const QList<DiagramItem*> &blocks) {
    QList<VarInfo> vars;
    QSet<QString> foundNames;

    // Ищем объявления: int name = val; или bool flag;
    // Поддерживаем типы: int, double, float, bool
    QRegularExpression re(R"(\b(int|double|float|bool)\s+(\w+)\s*(=\s*([^;]+))?;)");

    for (DiagramItem* block : blocks) {
        // Сканируем код входа и выхода
        QString code = block->getOnEnter() + "\n" + block->getOnExit();
        QRegularExpressionMatchIterator i = re.globalMatch(code);

        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString name = match.captured(2);
            QString val = match.captured(4).trimmed(); // Значение после =

            if (!foundNames.contains(name)) {
                if (val.isEmpty()) val = "0"; // Если значение не задано, ставим 0

                // Для bool заменяем true/false на 1/0 для совместимости с ECMAscript в SCXML
                if (val == "true") val = "true";
                if (val == "false") val = "false";

                // ПРОФЕССИОНАЛЬНО: Используем конструктор, а не неявный список инициализации
                vars.append(VarInfo(name, val));
                foundNames.insert(name);
            }
        }
    }
    return vars;
}

// === 2. ВАЛИДАЦИЯ (С проверкой Мостов) ===
QList<ValidationError> ScxmlCompiler::validate(QGraphicsScene *scene)
{
    QList<ValidationError> errors;
    if (!scene) return errors;

    QList<DiagramItem*> blocks;
    QList<ConnectionItem*> arrows;
    QMap<QString, DiagramItem*> nameToBlockMap;
    QSet<QString> checkedLabels;

    // Сбор элементов
    for (QGraphicsItem *item : scene->items()) {
        if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
            blocks.append(b);
            QString lbl = b->getLabel();
            if (!lbl.isEmpty()) nameToBlockMap.insert(lbl, b);

            if (checkedLabels.contains(lbl)) {
                // ПРОФЕССИОНАЛЬНО: Явное указание типа ValidationError
                errors.append(ValidationError{QObject::tr("Дубликат имени блока: ") + lbl, b, nullptr, true});
            } else {
                checkedLabels.insert(lbl);
            }
        }
        else if (item->data(ConnectionRole).toBool()) {
             if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
                 if (!conn->isBlocked()) arrows.append(conn);
             }
        }
    }

    if (blocks.isEmpty()) {
        errors.append(ValidationError{QObject::tr("Сцена пуста."), nullptr, nullptr, true});
        return errors;
    }

    // Построение графа связей (с учетом Мостов)
    QMap<DiagramItem*, QList<DiagramItem*>> adjacency;

    // Физические связи
    for (ConnectionItem* conn : arrows) {
        if (conn->startBlock() && conn->endBlock()) {
            adjacency[conn->startBlock()].append(conn->endBlock());
            adjacency[conn->endBlock()].append(conn->startBlock());
        }
    }

    // Логические связи (Мосты)
    for (DiagramItem* b : blocks) {
        if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(b)) {
            QString target = bridge->targetName();
            if (target.isEmpty()) {
                errors.append(ValidationError{QObject::tr("Мост никуда не ведет."), bridge, nullptr, true});
            }
            else if (nameToBlockMap.contains(target)) {
                DiagramItem* targetBlock = nameToBlockMap[target];
                adjacency[bridge].append(targetBlock);
                adjacency[targetBlock].append(bridge);
            } else {
                errors.append(ValidationError{QObject::tr("Мост ведет в никуда: ") + target, bridge, nullptr, true});
            }
        }
        // --- НОВАЯ ЛОГИКА ДЛЯ BACK ITEM ---
        else if (BackItem* back = dynamic_cast<BackItem*>(b)) {
            if (back->backMode() == BackItem::BackReturn) {
                QString target = back->targetName();
                if (target.isEmpty()) {
                    errors.append(ValidationError{QObject::tr("Back Return никуда не ведет."), back, nullptr, true});
                }
                else if (nameToBlockMap.contains(target)) {
                    DiagramItem* targetBlock = nameToBlockMap[target];
                    adjacency[back].append(targetBlock);
                    adjacency[targetBlock].append(back);
                } else {
                    errors.append(ValidationError{QObject::tr("Back Return ведет в никуда: ") + target, back, nullptr, true});
                }
            } else {
                // Режим Back (OLD) - Возвращает в вызывающий блок
                for (ConnectionItem* conn : arrows) {
                    if (conn->endBlock() == back && conn->startBlock()) {
                        adjacency[back].append(conn->startBlock());
                    }
                }
            }
        }
    }

    // Поиск островов (BFS)
    QSet<DiagramItem*> visited;
    for (DiagramItem* block : blocks) {
        if (visited.contains(block)) continue;

        QList<DiagramItem*> component;
        QQueue<DiagramItem*> queue;
        queue.enqueue(block);
        visited.insert(block);
        bool hasStart = false;

        while (!queue.isEmpty()) {

            DiagramItem* curr = queue.dequeue();
                        component.append(curr);

                        // Поддержка локализации для стартового блока
                        QString type = curr->getType();
                        if (type == "Стартовый блок" || type == "Start Block") {
                            hasStart = true;
                        }


            for (DiagramItem* neighbor : adjacency[curr]) {
                if (!visited.contains(neighbor)) {
                    visited.insert(neighbor);
                    queue.enqueue(neighbor);
                }
            }
        }

        if (!hasStart) {
            errors.append(ValidationError{QObject::tr("Изолированный фрагмент без Стартового блока."), component.first(), nullptr, true});
        }
    }

    // Проверка синтаксиса стрелок
    for (ConnectionItem* conn : arrows) {
        if (!conn->startBlock() || !conn->endBlock()) {
            errors.append(ValidationError{QObject::tr("Висячая стрелка."), nullptr, conn, true});
            continue;
        }
        checkArrowSyntax(conn, errors);
    }

    return errors;
}

// === 3. СБОР КОДА ===
void ScxmlCompiler::collectCode(QGraphicsScene *scene, CodeExecutor *executor)
{
    if (!scene || !executor) return;
    executor->clear();

    for (QGraphicsItem *item : scene->items()) {
        if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
            if (dynamic_cast<BridgeItem*>(b) || dynamic_cast<BackItem*>(b)) continue;

            QString id = b->getLabel();
            if (!b->getOnEnter().trimmed().isEmpty())
                executor->registerCode(id, "onEnter", b->getOnEnter(), true);
            if (!b->getOnExit().trimmed().isEmpty())
                executor->registerCode(id, "onExit", b->getOnExit(), true);
        }
    }

    for (QGraphicsItem *item : scene->items()) {
        if (item->data(ConnectionRole).toBool()) {
            if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
                if (!conn->isBlocked() && conn->transition()) {
                     QString act = conn->transition()->actionText();
                     if (!act.trimmed().isEmpty() && act != "action") {
                         executor->registerCode(conn->id(), "action", act, true);
                     }
                }
            }
        }
    }
}


// === 4. ГЕНЕРАЦИЯ SCXML (С ПОДДЕРЖКОЙ ИСТОРИИ И ГРУПП) ===
bool ScxmlCompiler::compileToScxml(QGraphicsScene *scene, const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement("scxml");
    xml.writeAttribute("xmlns", "http://www.w3.org/2005/07/scxml");
    xml.writeAttribute("version", "1.0");
    // ВАЖНО: Модель данных ECMAScript
    xml.writeAttribute("datamodel", "ecmascript");

    // --- 1. Сбор блоков и Рамок (Групп) ---
    QList<DiagramItem*> allBlocks;
    QList<GroupItem*> allGroups;
    QList<DiagramItem*> startBlocks;


    for (QGraphicsItem *item : scene->items()) {
            if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
                allBlocks.append(b);

                // Поддержка локализации
                QString type = b->getType();
                if (type == "Стартовый блок" || type == "Start Block") {
                    startBlocks.append(b);
                }
            } else if (GroupItem *g = dynamic_cast<GroupItem*>(item)) {
                allGroups.append(g);
            }
        }


    bool isMulti = (startBlocks.size() > 1);
    QMap<DiagramItem*, int> blockToThread;
    QMap<GroupItem*, int> groupToThread;

    if (isMulti) {
        xml.writeAttribute("initial", "MainParallel");

        // Строим граф связей для определения потоков (BFS)
        QMap<DiagramItem*, QList<DiagramItem*>> adj;
        for (QGraphicsItem *item : scene->items()) {
            if (item->data(ConnectionRole).toBool()) {
                if (auto conn = dynamic_cast<ConnectionItem*>(item)) {
                    if (conn->startBlock() && conn->endBlock() && !conn->isBlocked()) {
                        adj[conn->startBlock()].append(conn->endBlock());
                    }
                }
            }
        }

        // Бронебойный поиск для мостов
        for (DiagramItem* b : allBlocks) {
            if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(b)) {
                QString targetName = bridge->targetName().trimmed();
                QString searchName = targetName;
                if (searchName.startsWith("_")) searchName = searchName.mid(1);

                for (DiagramItem* tb : allBlocks) {
                    QString blockLabel = tb->getLabel().trimmed();
                    QString cleanLabel = blockLabel;
                    if (cleanLabel.startsWith("_")) cleanLabel = cleanLabel.mid(1);

                    if (cleanLabel.compare(searchName, Qt::CaseInsensitive) == 0 ||
                        blockLabel.compare(targetName, Qt::CaseInsensitive) == 0) {
                        adj[bridge].append(tb);
                        break;
                    }
                }
            }
        }

        // Запускаем BFS от каждого старта
        for (int i = 0; i < startBlocks.size(); ++i) {
            QQueue<DiagramItem*> q;
            q.enqueue(startBlocks[i]);
            while (!q.isEmpty()) {
                DiagramItem* curr = q.dequeue();
                if (!blockToThread.contains(curr)) {
                    blockToThread[curr] = i;
                    for (DiagramItem* next : adj[curr]) {
                        q.enqueue(next);
                    }
                }
            }
        }

        // Привязываем рамки (History) к потоку
        for (GroupItem* g : allGroups) {
            for (DiagramItem* b : allBlocks) {
                if (g->sceneBoundingRect().contains(b->sceneBoundingRect().center())) {
                    if (blockToThread.contains(b)) {
                        groupToThread[g] = blockToThread[b];
                        break;
                    }
                }
            }
        }
    } else if (!startBlocks.isEmpty()) {
        xml.writeAttribute("initial", startBlocks.first()->getLabel());
    }

    // --- 2. ГЕНЕРАЦИЯ DATAMODEL ---
    QList<VarInfo> vars = scanForVariables(allBlocks);
    if (!vars.isEmpty()) {
        xml.writeStartElement("datamodel");
        for (const VarInfo &v : vars) {
            xml.writeEmptyElement("data");
            xml.writeAttribute("id", v.name);
            xml.writeAttribute("expr", v.val);
        }
        xml.writeEndElement(); // datamodel
    }

    // --- 3. ЛЯМБДА: Правило генерации одного физического блока ---
    auto generateSingleState = [&](DiagramItem *b) {
        // BackItem и HistoryItem не являются физическими состояниями SCXML
        if (dynamic_cast<BackItem*>(b) || dynamic_cast<HistoryItem*>(b)) return;

        xml.writeStartElement("state");
        xml.writeAttribute("id", b->getLabel());

        // A. Если это МОСТ
        if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(b)) {
            xml.writeEmptyElement("transition");
            xml.writeAttribute("event", "tick");
            xml.writeAttribute("target", bridge->targetName());
        }
        // B. Если это ОБЫЧНЫЙ БЛОК
        else {
            if (!b->getOnEnter().isEmpty()) {
                xml.writeStartElement("onentry");
                xml.writeStartElement("script");
                xml.writeCharacters(QString("_event_dispatcher.execute('%1', 'onEnter');").arg(b->getLabel()));
                xml.writeEndElement(); // script
                xml.writeEndElement(); // onentry
            }

            QList<ConnectionItem*> outgoingArrows;
            for (QGraphicsItem *connItem : scene->items()) {
                if (connItem->data(ConnectionRole).toBool()) {
                    auto conn = dynamic_cast<ConnectionItem*>(connItem);
                    if (conn && conn->startBlock() == b && !conn->isBlocked()) {
                        outgoingArrows.append(conn);
                    }
                }
            }

            std::sort(outgoingArrows.begin(), outgoingArrows.end(), [](ConnectionItem* a, ConnectionItem* b){
                int p1 = (a->transition()) ? a->transition()->getPriority() : 0;
                int p2 = (b->transition()) ? b->transition()->getPriority() : 0;
                return p1 < p2;
            });

            for (ConnectionItem* arrow : outgoingArrows) {
                xml.writeStartElement("transition");

                if (arrow->endBlock()) {
                    DiagramItem* targetBlock = arrow->endBlock();

                    if (BackItem* back = dynamic_cast<BackItem*>(targetBlock)) {
                        if (back->backMode() == BackItem::Back) {
                            targetBlock = arrow->startBlock();
                        } else {
                            QString tName = back->targetName();
                            targetBlock = nullptr;
                            for (DiagramItem* sb : allBlocks) {
                                if (sb->getLabel() == tName || sb->getDescription() == tName) {
                                    targetBlock = sb;
                                    break;
                                }
                            }
                        }
                    }

                    if (targetBlock) {
                        xml.writeAttribute("target", targetBlock->getLabel());
                    }
                }

                xml.writeAttribute("event", "tick");

                if (arrow->transition()) {
                    QString cond = arrow->transition()->conditionText();
                    cond = cond.replace("if", "").replace("(", "").replace(")", "").trimmed();
                    if (!cond.isEmpty()) {
                        xml.writeAttribute("cond", cond);
                    }

                    QString act = arrow->transition()->actionText();
                    if (!act.trimmed().isEmpty() && act != "action") {
                        xml.writeStartElement("script");
                        xml.writeCharacters(QString("_event_dispatcher.execute('%1', 'action');").arg(arrow->id()));
                        xml.writeEndElement();
                    }
                }
                xml.writeEndElement(); // transition
            }

            if (!b->getOnExit().isEmpty()) {
                xml.writeStartElement("onexit");
                xml.writeStartElement("script");
                xml.writeCharacters(QString("_event_dispatcher.execute('%1', 'onExit');").arg(b->getLabel()));
                xml.writeEndElement(); // script
                xml.writeEndElement(); // onexit
            }
        }
        xml.writeEndElement(); // state
    };

    // --- 4. ОСНОВНОЙ ЦИКЛ: Генерируем Рамки (Составные состояния) ---
    QSet<DiagramItem*> processedBlocks;
    int numThreads = isMulti ? startBlocks.size() : 1;

    // Открываем нативную параллельность SCXML
    if (isMulti) {
        xml.writeStartElement("parallel");
        xml.writeAttribute("id", "MainParallel");
    }

    for (int i = 0; i < numThreads; ++i) {
        if (isMulti) {
            xml.writeStartElement("state");
            xml.writeAttribute("id", "Thread_" + QString::number(i + 1));
            xml.writeAttribute("initial", startBlocks[i]->getLabel());
        }

        for (GroupItem* g : allGroups) {
            // Если мультипоток, пропускаем группы чужих потоков
            if (isMulti && groupToThread.value(g, -1) != i) continue;

            xml.writeStartElement("state");
            QString groupId = "group_" + g->name().replace(" ", "_");
            xml.writeAttribute("id", groupId);

            QList<DiagramItem*> blocksInGroup;
            HistoryItem* histInGroup = nullptr;

            for (DiagramItem* b : allBlocks) {
                if (g->sceneBoundingRect().contains(b->sceneBoundingRect().center())) {
                    if (!isMulti || blockToThread.value(b, -1) == i) {
                        blocksInGroup.append(b);
                        processedBlocks.insert(b);
                        if (HistoryItem* h = dynamic_cast<HistoryItem*>(b)) {
                            histInGroup = h;
                        }
                    }
                }
            }

            // --- ГЕНЕРАЦИЯ ТЕГА <history> ---
            if (histInGroup) {
                xml.writeStartElement("history");
                xml.writeAttribute("id", histInGroup->getLabel());
                xml.writeAttribute("type", histInGroup->historyMode() == HistoryItem::Shallow ? "shallow" : "deep");

                for (QGraphicsItem *connItem : scene->items()) {
                    if (connItem->data(ConnectionRole).toBool()) {
                        if (auto conn = dynamic_cast<ConnectionItem*>(connItem)) {
                            if (conn->startBlock() == histInGroup && conn->endBlock() && !conn->isBlocked()) {
                                xml.writeEmptyElement("transition");
                                xml.writeAttribute("target", conn->endBlock()->getLabel());
                                break;
                            }
                        }
                    }
                }
                xml.writeEndElement(); // history
            }

            for (DiagramItem* b : blocksInGroup) {
                generateSingleState(b);
            }

            xml.writeEndElement(); // state group
        }

        // --- 5. Генерируем оставшиеся блоки (вне рамок) ---
        for (DiagramItem *b : allBlocks) {
            if (!processedBlocks.contains(b)) {
                if (!isMulti || blockToThread.value(b, -1) == i) {
                    generateSingleState(b);
                    processedBlocks.insert(b);
                }
            }
        }

        if (isMulti) {
            xml.writeEndElement(); // Thread_i
        }
    }

    if (isMulti) {
        xml.writeEndElement(); // parallel
    }

    xml.writeEndElement(); // scxml

    xml.writeEndDocument();
    file.close();
    return true;
}

// === ВСПОМОГАТЕЛЬНЫЕ ПРОВЕРКИ ===
void ScxmlCompiler::checkBlockSyntax(DiagramItem* block, QList<ValidationError> &errors)
{
    SyntaxRegistry &reg = SyntaxRegistry::instance();

    SyntaxError err = reg.checkAction(block->getOnEnter());
    if (err.hasError) errors.append(ValidationError{"onEnter: " + err.message, block, nullptr, false});

    err = reg.checkAction(block->getOnExit());
    if (err.hasError) errors.append(ValidationError{"onExit: " + err.message, block, nullptr, false});

    if (HistoryItem* hist = dynamic_cast<HistoryItem*>(block)) {
        int outgoingCount = 0;
        if (hist->scene()) {
            for (QGraphicsItem *item : hist->scene()->items()) {
                if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
                    if (conn->startBlock() == hist) outgoingCount++;
                }
            }
        }

        if (outgoingCount != 1) {
            errors.append(ValidationError{QObject::tr("Блок 'История' должен иметь строго ОДНУ исходящую стрелку!"), hist, nullptr, true});
        }
    }
}

void ScxmlCompiler::checkArrowSyntax(ConnectionItem* arrow, QList<ValidationError> &errors)
{
    TransitionGroup* trans = arrow->transition();
    if (!trans) return;

    QString cond = trans->conditionText();
    if (!cond.trimmed().isEmpty() && cond != "if") {

        // 1. ЗАЩИТА ОТ СЛОЖНОГО C++
        QRegularExpression cppSpecific("(\\->|::|\\b(int|double|float|char|void|auto|std|static_cast)\\b)");
        if (cppSpecific.match(cond).hasMatch()) {
            errors.append(ValidationError{QObject::tr("Недопустимый синтаксис (сложный C++) в условии (IF)."), nullptr, arrow, true});
            return;
        }

        // 2. ЗАЩИТА ОТ PYTHON
        QRegularExpression pySpecific("\\b(and|or|not|is|None|True|False|elif)\\b");
        if (pySpecific.match(cond).hasMatch()) {
            errors.append(ValidationError{QObject::tr("Используйте C-style (&&, ||, !, true) вместо Python (and, or, not, True)."), nullptr, arrow, true});
            return;
        }

        SyntaxError err = SyntaxRegistry::instance().checkCondition(cond);
        if (err.hasError) errors.append(ValidationError{QObject::tr("Ошибка синтаксиса: ") + err.message, nullptr, arrow, true});
    }

    QString act = trans->actionText();
    if (!act.trimmed().isEmpty() && act != "action") {
        SyntaxError err = SyntaxRegistry::instance().checkAction(act);
        if (err.hasError) errors.append(ValidationError{QObject::tr("Ошибка в действии: ") + err.message, nullptr, arrow, true});
    }
}
