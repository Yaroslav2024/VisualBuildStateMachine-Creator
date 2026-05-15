/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "pythontranslator2.h"
#include <QTextStream>
#include <QRegularExpression>
#include "diagramitem.h"
#include "connect.h"
#include <algorithm>
#include "commentitem.h"
#include "bridgeitem.h"
#include "backitem.h"
#include "groupitem.h"
#include "historyitem.h"

PythonTranslator2::GeneratedPyCode PythonTranslator2::generate(QGraphicsScene *scene, const QString &manualHeader)
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
                // В Python свойства класса живут в self
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
    // ==========================================




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

        // --- 2. Генерация КОДА ---
        QString pyCode;
        QTextStream out(&pyCode);

        out << "#!/usr/bin/env python3\n";
        out << "import time\nimport math\nimport random\n\n";

        if (!importsCode.trimmed().isEmpty()) {
            out << "# --- User Imports ---\n";
            out << importsCode << "\n";
        }

        out << "def log(msg):\n    print(f\"[LOG] {msg}\")\n\n";
        out << "def send(msg):\n    print(f\"[SEND] {msg}\")\n\n";

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
        out << "            next_state = current_state\n\n";
        out << "            transition_taken = False\n\n";
        out << "            # --- ON ENTER PHASE ---\n";
        out << "            if self.state_changed:\n";
    }

    // --- РАСЧЕТ ОТСТУПОВ ---
    int enterLevel = isMulti ? 5 : 4;
    int transLevel = isMulti ? 4 : 3;
    int exitLevel  = isMulti ? 7 : 5;

    QString enterIndBase = QString((enterLevel - 1) * 4, ' ');
    QString enterIndBody = QString(enterLevel * 4, ' ');
    QString transIndBase = QString((transLevel - 1) * 4, ' ');
    QString transIndBody = QString(transLevel * 4, ' ');
    QString exitIndBase  = QString((exitLevel - 1) * 4, ' ');
    QString exitIndBody  = QString(exitLevel * 4, ' ');

    // --- ФАЗА 1: ON ENTER ---
    bool firstEnter = true;
    for (const auto& block : blocks) {
        if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;

        bool hasCode = !block.codeEnter.isEmpty() || blockToHistoryVar.contains(block.id);
        QString comments = CommentItem::getAssociatedCode(block.item, "# ");

        if (hasCode || !comments.isEmpty()) {
            out << enterIndBase << (firstEnter ? "if" : "elif") << " current_state == " << block.id << ":\n";

            if (blockToHistoryVar.contains(block.id)) {
                            out << enterIndBody << blockToHistoryVar[block.id] << (isMulti ? "[i]" : "") << " = " << block.id << " # Update History\n";
                        }

            if (!comments.isEmpty()) out << addIndent(comments, enterLevel);
            if (!block.codeEnter.isEmpty()) out << addIndent(block.codeEnter, enterLevel) << "\n";
            firstEnter = false;
        }
    }
    if (firstEnter) out << enterIndBase << "pass\n";

    if (isMulti) out << "                self.state_changed[i] = False\n\n";
    else out << "            self.state_changed = False\n\n";

    // --- ФАЗА 2: TRANSITIONS ---
    out << transIndBase << "# --- TRANSITIONS PHASE ---\n";
    bool firstTrans = true;

    for (const auto& block : blocks) {
        if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;

        out << transIndBase << (firstTrans ? "if" : "elif") << " current_state == " << block.id << ":\n";
        firstTrans = false;

        // 1. Мосты
        if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(block.item)) {
            QString targetName = bridge->targetName();
            int targetId = -1;
            for (auto it = blockIds.begin(); it != blockIds.end(); ++it) {
                if (it.key()->getLabel() == targetName) { targetId = it.value(); break; }
            }
            if (targetId != -1) {
                out << transIndBody << "# Bridge jump to: " << targetName << "\n";               
                out << transIndBody << "next_state = " << targetId << "\n";
                out << transIndBody << "transition_taken = True\n";
            } else {
                out << transIndBody << "# ERROR: Bridge target not found\n";
                if (isMulti) out << transIndBody << "token_finished = True\n";
                else out << transIndBody << "self.running = False\n";
            }
            continue;
        }

        // 2. Стрелки
        QList<ConnectionItem*> transitions;
        for (QGraphicsItem *item : scene->items()) {
             if (item->data(ConnectionRole).toBool()) {
                 ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item);
                 if (conn && !conn->isBlocked() && conn->startBlock() == block.item && conn->endBlock()) {
                     transitions.append(conn);
                 }
             }
        }
        std::sort(transitions.begin(), transitions.end(), [](ConnectionItem *a, ConnectionItem *b) {
            int p1 = (a->transition()) ? a->transition()->getPriority() : 999;
            int p2 = (b->transition()) ? b->transition()->getPriority() : 999;
            return p1 < p2;
        });

        bool hasUnconditional = false;
        bool firstArrow = true;

        for (ConnectionItem *conn : transitions) {
            QString rawCond = conn->transition()->conditionText();
            QString cleanCond = sanitizeForPython(rawCond).trimmed();
            QString cleanAct = sanitizeForPython(conn->transition()->actionText()).trimmed();

            if (cleanCond.toLower() == "if") cleanCond = "";
            if (cleanCond.toLower().startsWith("if ")) cleanCond = cleanCond.mid(3).trimmed();
            if (cleanAct.toLower() == "action") cleanAct = "";

            QString arrowComm = CommentItem::getAssociatedCode(conn, "# ");
            if (!arrowComm.isEmpty()) out << addIndent(arrowComm, transLevel);


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

            if (isAlwaysTrue) {
                if (firstArrow) {
                    if (!cleanAct.isEmpty()) out << addIndent(cleanAct, transLevel);
                    out << transIndBody << "next_state = " << targetStr << "\n";
                    out << transIndBody << "transition_taken = True\n";
                } else {
                    out << transIndBody << "else:\n";
                    if (!cleanAct.isEmpty()) out << addIndent(cleanAct, transLevel + 1);
                    out << transIndBody << "    next_state = " << targetStr << "\n";
                    out << transIndBody << "    transition_taken = True\n";
                }
                hasUnconditional = true;
                break;
            } else {
                out << transIndBody << (firstArrow ? "if " : "elif ") << cleanCond << ":\n";
                if (!cleanAct.isEmpty()) out << addIndent(cleanAct, transLevel + 1);
                out << transIndBody << "    next_state = " << targetStr << "\n";
                out << transIndBody << "    transition_taken = True\n";
            }
            firstArrow = false;
        }

        if (!hasUnconditional) {
            if (!firstArrow) out << transIndBody << "else:\n";
            int idleLevel = firstArrow ? transLevel : transLevel + 1;
            QString idleInd = QString(idleLevel * 4, ' ');

            if (transitions.isEmpty()) {
                out << idleInd << (isMulti ? "token_finished = True\n" : "self.running = False\n");
            } else {
                out << idleInd << "pass # Wait for transition\n";
            }
        }
    }
    if (firstTrans) out << transIndBase << "pass\n";

    // --- ФАЗА 3: ON EXIT & UPDATE ---
    if (isMulti) {
            out << "                # --- ON EXIT & UPDATE PHASE ---\n";
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
        out << "            if transition_taken:\n";
        out << "                if next_state != current_state:\n";
    }

    bool firstExit = true;
    for (const auto& block : blocks) {
        if (!block.codeExit.isEmpty()) {
            out << exitIndBase << (firstExit ? "if" : "elif") << " current_state == " << block.id << ":\n";
            out << addIndent(block.codeExit, exitLevel) << "\n";
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

QString PythonTranslator2::addIndent(const QString &code, int level) {
    QStringList lines = code.split("\n");
    QString indentString;
    for (int i = 0; i < level * 4; ++i) indentString += " ";
    QString result;
    for (const QString &line : lines) {
        if (!line.trimmed().isEmpty()) result += indentString + line + "\n";
    }
    return result;
}

QString PythonTranslator2::sanitizeForPython(const QString &rawCode) {
    QString clean = rawCode;

    // Защита: отсекаем двоеточия в конце, чтобы не сломать "if cond::"
    clean = clean.trimmed();
    if (clean.endsWith(":")) clean.chop(1);

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
