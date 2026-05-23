/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "pythontranslator.h"
#include <QTextStream>
#include <QRegularExpression>
#include "diagramitem.h"    // Чтобы видеть блоки
#include "connect.h" // Впишите сюда ТОЧНОЕ имя файла, которое нашли в cpptranslator.cpp
#include <algorithm>
#include "bridgeitem.h"
#include "backitem.h"
#include "groupitem.h"
#include "historyitem.h"
#include "commentitem.h"

PythonTranslator::GeneratedPyCode PythonTranslator::generate(QGraphicsScene *scene, const QString &manualHeader, bool debugMode)
{
    GeneratedPyCode result;

    // --- 1. Сбор блоков ---
    struct BlockData {
        DiagramItem* item;
        int id;
        QString codeEnter;
        QString codeExit;
    };
    QList<BlockData> blocks;
    QList<DiagramItem*> startBlocks;

    for (QGraphicsItem *item : scene->items()) {
        if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
            if (b->getType() == "Стартовый блок" || b->getType() == "Start Block") startBlocks.append(b);
            else blocks.append({b, 0, "", ""});
        }
    }

    if (startBlocks.isEmpty()) {
        result.error = QObject::tr("ERROR: Start block not found!");
        return result;
    }

    for (DiagramItem* sb : startBlocks) {
        blocks.prepend({sb, 0, "", ""});
    }

    bool isMulti = (startBlocks.size() > 1);

    // Присваиваем ID
    int idCounter = 1;
    QMap<DiagramItem*, int> blockIds;

    for (int i = 0; i < blocks.size(); ++i) {
        blocks[i].id = idCounter++;
        blockIds[blocks[i].item] = blocks[i].id;
        result.idMap.insert(blocks[i].id, blocks[i].item); // Для подсветки интерфейса
        blocks[i].codeEnter = sanitizeForPython(blocks[i].item->getOnEnter());
        blocks[i].codeExit = sanitizeForPython(blocks[i].item->getOnExit());
    }

    // --- АНАЛИЗ ИСТОРИИ (HISTORY) ---
    QMap<int, QString> blockToHistoryVar;
    QMap<DiagramItem*, QString> historyTargetVar;
    QString historyDeclarations;

    for (QGraphicsItem *item : scene->items()) {
        if (GroupItem *g = dynamic_cast<GroupItem*>(item)) {
            HistoryItem* hist = nullptr;
            for (QGraphicsItem* child : scene->items()) {
                if (HistoryItem* h = dynamic_cast<HistoryItem*>(child)) {
                    if (g->sceneBoundingRect().contains(h->sceneBoundingRect().center())) {
                        hist = h; break;
                    }
                }
            }
            if (hist) {
                QString varName = "self.hist_group_" + QString::number(hist->id());
                historyTargetVar[hist] = varName;

                int defaultTargetId = 0;
                for (QGraphicsItem *connItem : scene->items()) {
                    if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(connItem)) {
                        if (conn->startBlock() == hist && conn->endBlock() && !conn->isBlocked()) {
                            defaultTargetId = blockIds[conn->endBlock()];
                            break;
                        }
                    }
                }

                if (isMulti) {
                                    historyDeclarations += "        " + varName + " = [" + QString::number(defaultTargetId) + "] * " + QString::number(startBlocks.size()) + "\n";
                                } else {
                                    historyDeclarations += "        " + varName + " = " + QString::number(defaultTargetId) + "\n";
                                }

                for (DiagramItem* b : blockIds.keys()) {
                    if (g->sceneBoundingRect().contains(b->sceneBoundingRect().center()) && b != hist) {
                        blockToHistoryVar[blockIds[b]] = varName;
                    }
                }
            }
        }
    }




    // --- УМНЫЙ ПАРСИНГ USER HEADER (Отделяем импорты от методов класса) ---
        QStringList headerLines = manualHeader.split('\n');
        QString importsCode, membersCode;
        for (const QString& line : headerLines) {
            if (line.trimmed().startsWith("import ") || line.trimmed().startsWith("from ")) {
                importsCode += line + "\n";
            } else {
                membersCode += line + "\n";
            }
        }

        // ==========================================
        // --- 2. Генерация КОДА (Python) ---
        QString pyCode;
        QTextStream out(&pyCode);

        out << "#!/usr/bin/env python3\n";
        out << "import time\nimport math\nimport random\n\n";

        if (!importsCode.trimmed().isEmpty()) {
            out << "# --- User Imports ---\n";
            out << importsCode << "\n";
        }

        out << "def log(msg):\n    print(f\"[LOG] {msg}\", flush=True)\n\n";
        out << "def send(msg):\n    print(f\"[SEND] {msg}\", flush=True)\n\n";

        // --- ПРОФЕССИОНАЛЬНАЯ ООП АРХИТЕКТУРА ---
        out << "class RobotStateMachine:\n";
        if (!membersCode.trimmed().isEmpty()) {
            out << "    # --- User Defined Members ---\n";
            out << addIndent(membersCode, 1);
        }
        out << "    def __init__(self):\n";
        out << "        self.running = True\n";

    if (isMulti) {
        out << "        self.active_states = [";
        for (int i = 0; i < startBlocks.size(); ++i) out << blockIds[startBlocks[i]] << (i < startBlocks.size() - 1 ? ", " : "");
        out << "]\n";
        out << "        self.previous_states = [-1] * " << startBlocks.size() << "\n";
        out << "        self.state_changed = [True] * " << startBlocks.size() << "\n\n";
    } else {
        out << "        self.current_state = " << blockIds[startBlocks.first()] << "\n";
        out << "        self.previous_state = -1\n";
        out << "        self.state_changed = True\n\n";
    }
    out << historyDeclarations << "\n";

    out << "    def run(self):\n";
    out << "        log(\"System Started\")\n";
    out << "        time.sleep(0.5)\n"; // Для подключения дебаггера

    if (isMulti) {
        out << "        while self.running and len(self.active_states) > 0:\n";
        out << "            i = 0\n";
        out << "            while i < len(self.active_states):\n";
        out << "                current_state = self.active_states[i]\n";
        out << "                next_state = current_state\n";
        out << "                token_finished = False\n\n";
        out << "                transition_taken = False\n\n";
        out << "                # --- ON ENTER PHASE ---\n";
        out << "                if self.state_changed[i]:\n";
    } else {
        out << "        while self.running:\n";
        out << "            current_state = self.current_state\n";
        out << "            next_state = current_state\n";
        out << "            token_finished = False\n\n";
        out << "            transition_taken = False\n\n";
        out << "            # --- ON ENTER PHASE ---\n";
        out << "            if self.state_changed:\n";
    }

    // --- РАСЧЕТ ОТСТУПОВ ---
    int enterLevel = isMulti ? 6 : 5;
    int transLevel = isMulti ? 5 : 4;
    int exitLevel  = isMulti ? 8 : 6;

    QString enterIndBase = QString((enterLevel - 1) * 4, ' ');
    QString enterIndBody = QString(enterLevel * 4, ' ');
    QString transIndBase = QString((transLevel - 1) * 4, ' ');
    QString transIndBody = QString(transLevel * 4, ' ');
    QString exitIndBase  = QString((exitLevel - 1) * 4, ' ');
    QString exitIndBody  = QString(exitLevel * 4, ' ');
    QString finishCmd    = isMulti ? "token_finished = True" : "self.running = False";

    // --- ФАЗА 1: ON ENTER ---
    bool firstEnter = true;
    for (const auto& block : blocks) {
        if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;

        out << enterIndBase << (firstEnter ? "if" : "elif") << " current_state == " << block.id << ":\n";
        out << enterIndBody << "try:\n";

        if (blockToHistoryVar.contains(block.id)) {
                    out << addIndent(blockToHistoryVar[block.id] + (isMulti ? "[i]" : "") + " = " + QString::number(block.id) + " # Update History", enterLevel + 1);
                }

        if (debugMode && block.item->data(ROLE_DEBUG_ENABLED).toBool()) {
            out << addIndent(QString("print(f\"[DBG_BLOCK] %1\", flush=True)").arg(block.id), enterLevel + 1);
            out << addIndent("time.sleep(0.5)", enterLevel + 1);
        }

        QString comments = CommentItem::getAssociatedCode(block.item, "# ");
        if (!comments.isEmpty()) out << addIndent(comments, enterLevel + 1);
        if (!block.codeEnter.isEmpty()) out << addIndent(block.codeEnter, enterLevel + 1) << "\n";
        if (block.codeEnter.isEmpty() && comments.isEmpty() && !debugMode) out << addIndent("pass", enterLevel + 1);

        out << enterIndBody << "except Exception as e:\n";
        out << addIndent(QString("print(f\"[DBG_ERROR] %1 {e}\", flush=True)").arg(block.id), enterLevel + 1);
        out << addIndent(finishCmd, enterLevel + 1);

        firstEnter = false;
    }
    if (firstEnter) out << enterIndBase << "pass\n";

    if (isMulti) out << "\n                self.state_changed[i] = False\n\n";
    else out << "\n            self.state_changed = False\n\n";

    // --- ФАЗА 2: TRANSITIONS ---
    out << transIndBase << "# --- TRANSITIONS PHASE ---\n";
    bool firstTrans = true;

    for (const auto& block : blocks) {
        if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;

        out << transIndBase << (firstTrans ? "if" : "elif") << " current_state == " << block.id << ":\n";
        firstTrans = false;

        // 1. Мосты
        if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(block.item)) {
            QString targetName = bridge->targetName().trimmed();
            int targetId = -1;
            QString searchName = targetName.startsWith("_") ? targetName.mid(1) : targetName;
            for (auto it = blockIds.begin(); it != blockIds.end(); ++it) {
                QString blockLabel = it.key()->getLabel().trimmed();
                QString cleanLabel = blockLabel.startsWith("_") ? blockLabel.mid(1) : blockLabel;
                if (cleanLabel.compare(searchName, Qt::CaseInsensitive) == 0) { targetId = it.value(); break; }
            }
            if (targetId != -1) {
                            out << transIndBody << "# Bridge jump to: " << targetName << "\n";
                            if (debugMode && block.item->data(ROLE_DEBUG_ENABLED).toBool()) out << transIndBody << "print(f\"[DBG_CLEAR] " << block.id << "\", flush=True)\n";
                            out << transIndBody << "next_state = " << targetId << "\n";
                            out << transIndBody << "transition_taken = True\n";
                        } else {
                out << transIndBody << "print(f\"[DBG_ERROR] " << block.id << " Target not found: " << targetName << "\", flush=True)\n";
                out << transIndBody << finishCmd << "\n";
            }
            continue;
        }

        // 2. Стрелки
        QList<ConnectionItem*> transitions;
        for (QGraphicsItem *item : scene->items()) {
             if (item->data(ConnectionRole).toBool()) {
                 ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item);
                 if (conn && !conn->isBlocked() && conn->startBlock() == block.item && conn->endBlock()) transitions.append(conn);
             }
        }
        std::sort(transitions.begin(), transitions.end(), [](ConnectionItem *a, ConnectionItem *b) {
            int p1 = (a->transition()) ? a->transition()->getPriority() : 999;
            int p2 = (b->transition()) ? b->transition()->getPriority() : 999;
            return p1 < p2;
        });

        // Запускаем безопасную проверку стрелок
        out << transIndBody << "try:\n";
        out << addIndent("_arrow_id = -1", transLevel + 1);

        int nestLevel = transLevel + 1;
        bool hasUnconditional = false;
        bool firstArrow = true;

        for (ConnectionItem *conn : transitions) {
            int arrowId = idCounter++;
            result.idMap.insert(arrowId, conn);

            QString rawCond = conn->transition()->conditionText();
            QString cleanCond = sanitizeForPython(rawCond).trimmed();
            QString cleanAct = sanitizeForPython(conn->transition()->actionText()).trimmed();

            if (cleanCond.toLower() == "if") cleanCond = "";
            if (cleanCond.toLower().startsWith("if ")) cleanCond = cleanCond.mid(3).trimmed();
            if (cleanAct.toLower() == "action") cleanAct = "";

            QString arrowComm = CommentItem::getAssociatedCode(conn, "# ");
            if (!arrowComm.isEmpty()) out << addIndent(arrowComm, nestLevel);


            QString targetStr = QString::number(blockIds[conn->endBlock()]);
                        if (BackItem* back = dynamic_cast<BackItem*>(conn->endBlock())) {
                            if (back->backMode() != BackItem::Back) {
                                int targetId = block.id; // Fallback
                                QString tName = back->targetName();
                                for (auto it = blockIds.begin(); it != blockIds.end(); ++it) {
                                    if (it.key()->getLabel() == tName || it.key()->getDescription() == tName) {
                                        targetId = it.value(); break;
                                    }
                                }
                                targetStr = QString::number(targetId);
                            } else {
                                targetStr = isMulti ? "(self.previous_states[i] if self.previous_states[i] != -1 else current_state)"
                                                    : "(self.previous_state if self.previous_state != -1 else current_state)";
                            }
                        } else if (HistoryItem* hist = dynamic_cast<HistoryItem*>(conn->endBlock())) {
                            if (historyTargetVar.contains(hist)) {
                                targetStr = historyTargetVar[hist] + (isMulti ? "[i]" : "");
                            } else {
                                int defaultTargetId = 0;
                                for (QGraphicsItem *ci : scene->items()) {
                                    if (ConnectionItem *c = dynamic_cast<ConnectionItem*>(ci)) {
                                        if (c->startBlock() == hist && c->endBlock() && !c->isBlocked()) {
                                            defaultTargetId = blockIds[c->endBlock()]; break;
                                        }
                                    }
                                }
                                targetStr = QString::number(defaultTargetId);
                            }
                        }

            bool isAlwaysTrue = (cleanCond.isEmpty() || cleanCond.toLower() == "true" || cleanCond == "1");

            out << addIndent(QString("_arrow_id = %1").arg(arrowId), nestLevel);

            if (isAlwaysTrue) {
                if (debugMode && conn->data(ROLE_DEBUG_ENABLED).toBool()) {
                    out << addIndent(QString("print(f\"[DBG_TRANSITION] %1\", flush=True)").arg(arrowId), nestLevel);
                    out << addIndent("time.sleep(0.3)", nestLevel);
                }
                if (!cleanAct.isEmpty()) out << addIndent(cleanAct, nestLevel);
                out << addIndent(QString("next_state = %1").arg(targetStr), nestLevel);
                out << addIndent("transition_taken = True", nestLevel);
                hasUnconditional = true;
                break;
            } else {
                out << addIndent(QString("if %1:").arg(cleanCond), nestLevel);
                if (debugMode && conn->data(ROLE_DEBUG_ENABLED).toBool()) {
                    out << addIndent(QString("print(f\"[DBG_TRANSITION] %1\", flush=True)").arg(arrowId), nestLevel + 1);
                    out << addIndent("time.sleep(0.3)", nestLevel + 1);
                }
                if (!cleanAct.isEmpty()) out << addIndent(cleanAct, nestLevel + 1);
                out << addIndent(QString("next_state = %1").arg(targetStr), nestLevel + 1);
                out << addIndent("transition_taken = True", nestLevel + 1);
                out << addIndent("else:", nestLevel);
                if (debugMode && conn->data(ROLE_DEBUG_ENABLED).toBool()) {
                    out << addIndent(QString("print(f\"[DBG_FALSE] %1\", flush=True)").arg(arrowId), nestLevel + 1);
                }
                nestLevel++; // Увеличиваем вложенность для следующей стрелки
            }
            firstArrow = false;
        }

        if (!hasUnconditional) {
            if (transitions.isEmpty()) out << addIndent(finishCmd, nestLevel);
            else out << addIndent("pass # Wait for valid condition", nestLevel);
        }

        // Завершаем try-catch для стрелок этого блока
        out << transIndBody << "except Exception as e:\n";
        out << addIndent("print(f\"[DBG_ARROW_ERROR] {_arrow_id} {e}\", flush=True)", transLevel + 1);
        out << addIndent(finishCmd, transLevel + 1);
    }
    if (firstTrans) out << transIndBase << "pass\n";

    // --- ФАЗА 3: ON EXIT & UPDATE ---
        if (isMulti) {
                out << "\n                # --- ON EXIT & UPDATE PHASE ---\n";
                out << "                if token_finished:\n";
                out << "                    self.active_states.pop(i)\n";
                out << "                    self.previous_states.pop(i)\n";
                out << "                    self.state_changed.pop(i)\n";
                for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                    out << "                    " << it.value() << ".pop(i)\n";
                }
                out << "                else:\n";
                out << "                    if transition_taken:\n";
                out << "                        if next_state != current_state:\n";
        } else {
            out << "\n            # --- ON EXIT & UPDATE PHASE ---\n";
            out << "            if token_finished:\n";
            out << "                self.running = False\n";
            out << "            elif transition_taken:\n";
            out << "                if next_state != current_state:\n";
        }

    bool firstExit = true;
    for (const auto& block : blocks) {
        if (!block.codeExit.isEmpty()) {
            out << exitIndBase << (firstExit ? "if" : "elif") << " current_state == " << block.id << ":\n";
            out << exitIndBody << "try:\n";
            out << addIndent(block.codeExit, exitLevel + 1) << "\n";
            out << exitIndBody << "except Exception as e:\n";
            out << addIndent(QString("print(f\"[DBG_ERROR] %1 {e}\", flush=True)").arg(block.id), exitLevel + 1);
            firstExit = false;
        }
    }
    if (firstExit) out << exitIndBase << "pass\n";

    if (isMulti) {
        out << "                            self.previous_states[i] = current_state\n";
        out << "                            self.active_states[i] = next_state\n";
        out << "                            self.state_changed[i] = True\n";
        out << "                    i += 1\n";
        out << "            time.sleep(0.01)\n";
    } else {
        out << "                    self.previous_state = current_state\n";
        out << "                    self.current_state = next_state\n";
        out << "                    self.state_changed = True\n";
        out << "            time.sleep(0.01)\n";
    }

    out << "\nif __name__ == \"__main__\":\n";
    out << "    app = RobotStateMachine()\n";
    out << "    app.run()\n";

    result.code = pyCode;
    return result;
}

// Помощник: добавляет отступы к каждой строке кода
QString PythonTranslator::addIndent(const QString &code, int level) {
    QStringList lines = code.split("\n");
    QString indentString;
    // 1 уровень = 4 пробела (стандарт Python PEP8)
    for (int i = 0; i < level * 4; ++i) indentString += " ";

    QString result;
    for (const QString &line : lines) {
        if (!line.trimmed().isEmpty()) {
            result += indentString + line + "\n";
        }
    }
    return result;
}

// Помощник: Пытается превратить C++ код в Python
QString PythonTranslator::sanitizeForPython(const QString &rawCode) {
    QString clean = rawCode.trimmed();
    if (clean.endsWith(":")) clean.chop(1); // Защита от случайных двоеточий

    // 1. Умная замена точек с запятой на перенос строки
    clean.replace(QRegularExpression(";\\s*"), "\n");
    clean.replace(";", "");

    // 2. Убираем типы
    clean.replace(QRegularExpression("\\b(int|double|float|bool|string|void|std::string)\\s+"), "");

    // 3. Магия C++ -> Python
    clean.replace(QRegularExpression("(\\w+)\\+\\+"), "\\1 += 1");
    clean.replace(QRegularExpression("(\\w+)--"), "\\1 -= 1");
    clean.replace("std::to_string", "str");

    // 4. Стандартные замены
    clean.replace("true", "True", Qt::CaseInsensitive);
    clean.replace("false", "False", Qt::CaseInsensitive);
    clean.replace("&&", " and ");
    clean.replace("||", " or ");
    clean.replace("//", "#");

    return clean.trimmed();
}
