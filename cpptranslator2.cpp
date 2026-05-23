/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "cpptranslator2.h"
#include "diagramitem.h"
#include "connect.h"
#include "transitiongroup.h"
#include <QTextStream>
#include <QRegularExpression>
#include <algorithm>
#include "bridgeitem.h"
#include "backitem.h"
#include "groupitem.h"
#include "historyitem.h"
#include <QMessageBox>
#include <QSettings>

DebugBuildResult CppTranslator2::generateDebugSource(QGraphicsScene *scene, const QString &manualHeader)
{
    DebugBuildResult result;
    QMap<QString, VariableInfo> autoVariables;

    // ==========================================
    // --- УМНЫЙ СКАНЕР ОПАСНОГО КОДА ---
    // ==========================================
    bool hasDangerousCode = false;
    QRegularExpression dangerRegex("(\\[|\\]|\\->|\\bnew\\b|\\bdelete\\b|\\bmalloc\\b|\\bfree\\b|\\/)");

    for (QGraphicsItem *item : scene->items()) {
        if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
            if (dangerRegex.match(b->getOnEnter()).hasMatch() || dangerRegex.match(b->getOnExit()).hasMatch()) {
                hasDangerousCode = true; break;
            }
        } else if (item->data(ConnectionRole).toBool()) {
            if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
                if (conn->transition()) {
                    if (dangerRegex.match(conn->transition()->conditionText()).hasMatch() ||
                        dangerRegex.match(conn->transition()->actionText()).hasMatch()) {
                        hasDangerousCode = true; break;
                    }
                }
            }
        }
    }

    if (hasDangerousCode) {
        QSettings settings("VisualBuild", "IDE");
        if (!settings.value("DisableCrashWarnings", false).toBool()) {
            QMessageBox::StandardButton reply = QMessageBox::warning(nullptr,
                QObject::tr("Внимание: C++ Симулятор"),
                QObject::tr("Умный сканер обнаружил в вашей схеме потенциально опасный код\n"
                "(деление, массивы, указатели или ручное выделение памяти).\n\n"
                "Грубые ошибки в таких операциях (например, деление на ноль) "
                "могут привести к вылету процесса симуляции.\n\n"
                "Рекомендуется сохранить проект. Хотите продолжить?"),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::No) {
                result.error = QObject::tr("Симуляция отменена пользователем.");
                return result;
            }
        }
    }
    // ==========================================

    // --- 1. Сбор блоков ---
    struct BlockData {
        DiagramItem* item;
        int id;
        QString cleanOnEnter;
        QString cleanOnExit;
    };

    QList<BlockData> blocks;
    QList<DiagramItem*> startBlocks;

    for (QGraphicsItem *item : scene->items()) {
        if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
            if (b->getType() == "Стартовый блок" || b->getType() == "Start Block") {
                startBlocks.append(b);
            } else {
                blocks.append({b, 0, "", ""});
            }
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
    QString finishCmd = isMulti ? "token_finished = true;" : "running = false;";

    // --- 2. Присваиваем ID и чистим код ---
    int blockIdCounter = 1;
    QMap<DiagramItem*, int> blockIdMap;

    for (int i = 0; i < blocks.size(); ++i) {
        blocks[i].id = blockIdCounter++;
        blockIdMap[blocks[i].item] = blocks[i].id;
        result.idMap.insert(blocks[i].id, blocks[i].item);

        QString rawEnter = sanitizeCppCode(blocks[i].item->getOnEnter(), false);
        QString rawExit = sanitizeCppCode(blocks[i].item->getOnExit(), false);

        blocks[i].cleanOnEnter = extractAndRemoveTypes(rawEnter, autoVariables);
        blocks[i].cleanOnExit = extractAndRemoveTypes(rawExit, autoVariables);
    }

    // ==========================================
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
                QString varName = "hist_group_" + QString::number(hist->id());
                historyTargetVar[hist] = varName;

                int defaultTargetId = 0;
                for (QGraphicsItem *connItem : scene->items()) {
                    if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(connItem)) {
                        if (conn->startBlock() == hist && conn->endBlock() && !conn->isBlocked()) {
                            defaultTargetId = blockIdMap[conn->endBlock()];
                            break;
                        }
                    }
                }

                if (isMulti) {
                                    historyDeclarations += "    std::vector<int> " + varName + " = std::vector<int>(" + QString::number(startBlocks.size()) + ", " + QString::number(defaultTargetId) + ");\n";
                                } else {
                                    historyDeclarations += "    int " + varName + " = " + QString::number(defaultTargetId) + ";\n";
                                }

                for (DiagramItem* b : blockIdMap.keys()) {
                    if (g->sceneBoundingRect().contains(b->sceneBoundingRect().center()) && b != hist) {
                        blockToHistoryVar[blockIdMap[b]] = varName;
                    }
                }
            }
        }
    }
    // ==========================================



    // --- УМНЫЙ ПАРСИНГ USER HEADER (Отделяем библиотеки от переменных класса) ---
        QStringList headerLines = manualHeader.split('\n');
        QString includesCode, membersCode;
        for (const QString& line : headerLines) {
            if (line.trimmed().startsWith("#include") || line.trimmed().startsWith("#define")) {
                includesCode += line + "\n";
            } else {
                membersCode += line + "\n";
            }
        }

        // --- 3. Генерация C++ Кода ---
        QString code;
        QTextStream out(&code);

        // Хедеры
        out << "#include <iostream>\n";
        out << "#include <string>\n";
        out << "#include <thread>\n";
        out << "#include <chrono>\n";
        out << "#include <cmath>\n";
        out << "#include <vector>\n";
        out << "#include <stdexcept>\n\n"; // Для std::exception

        if (!includesCode.trimmed().isEmpty()) {
            out << "// --- User Headers ---\n";
            out << includesCode << "\n\n";
        }

        // Макросы для отладки
        out << "// Debug Helpers\n";
        out << "void delay(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }\n";
        out << "void log(std::string msg) { std::cout << \"[LOG] \" << msg << std::endl; }\n";
        out << "void send(std::string msg) { std::cout << \"[SEND] \" << msg << std::endl; }\n";
        out << "void dbg_block(int id) { std::cout << \"[DBG_BLOCK] \" << id << std::endl; }\n";
        out << "void dbg_arrow(int id) { std::cout << \"[DBG_TRANSITION] \" << id << std::endl; }\n";
        out << "void dbg_false(int id) { std::cout << \"[DBG_FALSE] \" << id << std::endl; }\n";
        out << "void dbg_clear(int id) { std::cout << \"[DBG_CLEAR] \" << id << std::endl; }\n";
        out << "void dbg_error(int id, std::string msg) { std::cout << \"[DBG_ERROR] \" << id << std::endl; std::cout << \"[LOG] ERROR: \" << msg << std::endl; }\n";
        out << "void dbg_arrow_error(int id, std::string msg) { std::cout << \"[DBG_ARROW_ERROR] \" << id << std::endl; std::cout << \"[LOG] ARROW ERROR: \" << msg << std::endl; }\n\n";

        // Класс StateMachine
        out << "class StateMachine {\n";
        out << "public:\n";
        if (!membersCode.isEmpty()) {
            out << "    // --- User Variables & Methods ---\n";
            out << membersCode << "\n";
        }
        out << "private:\n";

        if (!autoVariables.isEmpty()) {
            out << "    // Variables\n";
            for (auto it = autoVariables.begin(); it != autoVariables.end(); ++it) {
                QString initVal;
                if (it.value().type == "int" || it.value().type == "double") initVal = " = 0";
                else if (it.value().type == "bool") initVal = " = false";
                out << "    " << it.value().type << " " << it.value().name << initVal << ";\n";
            }
        }
        out << historyDeclarations;



    // --- ПРОФЕССИОНАЛЬНАЯ АРХИТЕКТУРА ---
    if (isMulti) {
        out << "    std::vector<int> active_states;\n";
        out << "    std::vector<int> previous_states;\n";
        out << "    std::vector<bool> state_changed;\n";
    } else {
        out << "    int currentState;\n";
        out << "    int previousState;\n";
        out << "    bool stateChanged;\n";
    }
    out << "    bool running;\n\n";

    out << "public:\n";
    if (isMulti) {
        out << "    StateMachine() : running(true) {\n";
        for (DiagramItem* sb : startBlocks) {
            out << "        active_states.push_back(" << blockIdMap[sb] << ");\n";
            out << "        previous_states.push_back(-1);\n";
            out << "        state_changed.push_back(true);\n";
        }
        out << "    }\n\n";
    } else {
        out << "    StateMachine() : currentState(" << blockIdMap[startBlocks.first()] << "), previousState(-1), stateChanged(true), running(true) {}\n\n";
    }

    out << "    void run() {\n";
    out << "        log(\"Debugger Started\");\n";
    out << "        delay(500);\n\n";

    if (isMulti) {
        out << "        while (running && !active_states.empty()) {\n";
        out << "            for (size_t i = 0; i < active_states.size(); ) {\n";
        out << "                int currentState = active_states[i];\n";
        out << "                int nextState = currentState;\n";
        out << "                bool token_finished = false;\n\n";
        out << "                bool transition_taken = false;\n\n";
        out << "                // --- ON ENTER PHASE ---\n";
        out << "                if (state_changed[i]) {\n";
    } else {
        out << "        while (running) {\n";
        out << "            int nextState = currentState;\n\n";
        out << "            // --- ON ENTER PHASE ---\n";
        out << "            if (stateChanged) {\n";
    }

    QString ind1 = isMulti ? "                " : "            ";
    QString ind2 = isMulti ? "                    " : "                ";

    out << ind1 << "switch (currentState) {\n";
    for (const auto& pb : blocks) {
        if (dynamic_cast<BackItem*>(pb.item) || dynamic_cast<HistoryItem*>(pb.item)) continue;

        out << ind1 << "case " << pb.id << ": {\n";
        out << ind2 << "try {\n";

        if (blockToHistoryVar.contains(pb.id)) {
                    out << ind2 << "    " << blockToHistoryVar[pb.id] << (isMulti ? "[i]" : "") << " = " << pb.id << "; // Update History\n";
                }

        if (pb.item->data(ROLE_DEBUG_ENABLED).toBool()) {
            out << ind2 << "    dbg_block(" << pb.id << "); delay(500);\n";
        }
        if (!pb.cleanOnEnter.isEmpty()) {
            out << ind2 << "    " << QString(pb.cleanOnEnter).replace("\n", "\n" + ind2 + "    ") << "\n";
        }
        out << ind2 << "} catch(const std::exception& e) { dbg_error(" << pb.id << ", std::string(\"Runtime error: \") + e.what()); " << finishCmd << " }\n";
        out << ind2 << "catch(...) { dbg_error(" << pb.id << ", \"Unknown Error\"); " << finishCmd << " }\n";
        out << ind1 << "} break;\n";
    }
    out << ind1 << "}\n";

    if (isMulti) out << ind1 << "state_changed[i] = false;\n            }\n\n";
    else out << ind1 << "stateChanged = false;\n        }\n\n";

    // --- TRANSITIONS PHASE ---
    out << ind1 << "// --- TRANSITIONS PHASE ---\n";
    out << ind1 << "switch (currentState) {\n";

    int arrowIdCounter = 1000;
    for (const auto& pb : blocks) {
        if (dynamic_cast<BackItem*>(pb.item) || dynamic_cast<HistoryItem*>(pb.item)) continue;
        out << ind1 << "case " << pb.id << ": {\n";

        if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(pb.item)) {
            QString targetName = bridge->targetName().trimmed();
            int targetId = -1;
            QString searchName = targetName.startsWith("_") ? targetName.mid(1) : targetName;
            for (auto it = blockIdMap.begin(); it != blockIdMap.end(); ++it) {
                QString blockLabel = it.key()->getLabel().trimmed();
                QString cleanLabel = blockLabel.startsWith("_") ? blockLabel.mid(1) : blockLabel;
                if (cleanLabel.compare(searchName, Qt::CaseInsensitive) == 0) { targetId = it.value(); break; }
            }
            if (targetId != -1) {
                out << ind2 << "// Bridge jump to: " << targetName << "\n";
                if (pb.item->data(ROLE_DEBUG_ENABLED).toBool()) out << ind2 << "dbg_clear(" << pb.id << ");\n";
                out << ind2 << "nextState = " << targetId << ";\n";
                out << ind2 << "transition_taken = true;\n";
            } else {
                out << ind2 << "dbg_error(" << pb.id << ", \"Bridge target not found: " << targetName << "\");\n";
                out << ind2 << finishCmd << "\n";
            }
            out << ind1 << "} break;\n";
            continue;
        }

        QList<ConnectionItem*> transitions;
        for (QGraphicsItem *item : scene->items()) {
             if (item->data(ConnectionRole).toBool()) {
                 ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item);
                 if (conn && conn->startBlock() == pb.item && conn->endBlock() && !conn->isBlocked()) transitions.append(conn);
             }
        }
        std::sort(transitions.begin(), transitions.end(), [](ConnectionItem *a, ConnectionItem *b) {
            int p1 = (a->transition()) ? a->transition()->getPriority() : 999;
            int p2 = (b->transition()) ? b->transition()->getPriority() : 999;
            return p1 < p2;
        });

        bool hasUnconditional = false;
        for (ConnectionItem *conn : transitions) {
            int arrowId = arrowIdCounter++;
            result.idMap.insert(arrowId, conn);

            QString cleanCond = sanitizeCppCode(conn->transition()->conditionText(), true).trimmed();
            if (cleanCond.toLower() == "if") cleanCond = "";
            if (cleanCond.toLower().startsWith("if ")) cleanCond = cleanCond.mid(3).trimmed();

            QString cleanAct = extractAndRemoveTypes(sanitizeCppCode(conn->transition()->actionText(), false), autoVariables);
            QString cleanActTest = cleanAct.trimmed().toLower();
            if (cleanActTest == "action" || cleanActTest == "action;") cleanAct = "";



            QString targetStr = QString::number(blockIdMap[conn->endBlock()]);
                        if (BackItem* back = dynamic_cast<BackItem*>(conn->endBlock())) {
                            if (back->backMode() != BackItem::Back) {
                                int targetId = pb.id; // Fallback
                                QString tName = back->targetName();
                                for (auto it = blockIdMap.begin(); it != blockIdMap.end(); ++it) {
                                    if (it.key()->getLabel() == tName || it.key()->getDescription() == tName) {
                                        targetId = it.value(); break;
                                    }
                                }
                                targetStr = QString::number(targetId);
                            } else {
                                targetStr = isMulti ? "(previous_states[i] != -1 ? previous_states[i] : currentState)" : "(previousState != -1 ? previousState : currentState)";
                            }
                        } else if (HistoryItem* hist = dynamic_cast<HistoryItem*>(conn->endBlock())) {
                            if (historyTargetVar.contains(hist)) {
                                targetStr = historyTargetVar[hist] + (isMulti ? "[i]" : "");
                            } else {
                                int defaultTargetId = 0;
                                for (QGraphicsItem *ci : scene->items()) {
                                    if (ConnectionItem *c = dynamic_cast<ConnectionItem*>(ci)) {
                                        if (c->startBlock() == hist && c->endBlock() && !c->isBlocked()) {
                                            defaultTargetId = blockIdMap[c->endBlock()]; break;
                                        }
                                    }
                                }
                                targetStr = QString::number(defaultTargetId);
                            }
                        }




            out << ind2 << "try {\n";
            if (cleanCond.isEmpty() || cleanCond == "true" || cleanCond == "1") {
                if (conn->data(ROLE_DEBUG_ENABLED).toBool()) out << ind2 << "    dbg_arrow(" << arrowId << "); delay(300);\n";
                if (!cleanAct.isEmpty()) out << ind2 << "    " << cleanAct.replace("\n", "\n" + ind2 + "    ") << "\n";
                out << ind2 << "    nextState = " << targetStr << ";\n";
                out << ind2 << "    transition_taken = true;\n";
                out << ind2 << "} catch(const std::exception& e) { dbg_arrow_error(" << arrowId << ", e.what()); " << finishCmd << " } catch(...) { dbg_arrow_error(" << arrowId << ", \"Unknown Error\"); " << finishCmd << " }\n";
                hasUnconditional = true;
                break;
            } else {
                out << ind2 << "    if (" << cleanCond << ") {\n";
                if (conn->data(ROLE_DEBUG_ENABLED).toBool()) out << ind2 << "        dbg_arrow(" << arrowId << "); delay(300);\n";
                if (!cleanAct.isEmpty()) out << ind2 << "        " << cleanAct.replace("\n", "\n" + ind2 + "        ") << "\n";
                out << ind2 << "        nextState = " << targetStr << ";\n";
                out << ind2 << "        transition_taken = true;\n";
                out << ind2 << "        break;\n";
                out << ind2 << "    } else {\n";
                if (conn->data(ROLE_DEBUG_ENABLED).toBool()) out << ind2 << "        dbg_false(" << arrowId << ");\n";
                out << ind2 << "    }\n";
                out << ind2 << "} catch(const std::exception& e) { dbg_arrow_error(" << arrowId << ", e.what()); " << finishCmd << " break; } catch(...) { dbg_arrow_error(" << arrowId << ", \"Unknown Error\"); " << finishCmd << " break; }\n";
            }
        }

        if (!hasUnconditional && transitions.isEmpty()) {
             out << ind2 << "delay(100);\n";
             out << ind2 << finishCmd << "\n";
        }
        out << ind1 << "} break;\n";
    }

    out << ind1 << "default: " << finishCmd << " break;\n";
    out << ind1 << "}\n\n";

    // --- ON EXIT & UPDATE PHASE ---
    if (isMulti) {
        out << "            // --- ON EXIT & UPDATE PHASE ---\n";
                out << "            if (token_finished) {\n";
                out << "                active_states.erase(active_states.begin() + i);\n";
                out << "                previous_states.erase(previous_states.begin() + i);\n";
                out << "                state_changed.erase(state_changed.begin() + i);\n";
                for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                    out << "                " << it.value() << ".erase(" << it.value() << ".begin() + i);\n";
                }
                out << "            } else {\n";
                out << "                if (transition_taken) {\n";
        out << "                if (nextState != currentState) {\n";
        out << "                    switch (currentState) {\n";
    } else {
        out << "        // --- ON EXIT & UPDATE PHASE ---\n";
        out << "        if (transition_taken) {\n";
        out << "        if (nextState != currentState) {\n";
        out << "            switch (currentState) {\n";
    }

    for (const auto& pb : blocks) {
        if (!pb.cleanOnExit.isEmpty()) {
            out << ind2 << "case " << pb.id << ": {\n";
            out << ind2 << "    try {\n";
            out << ind2 << "        " << QString(pb.cleanOnExit).replace("\n", "\n" + ind2 + "        ") << "\n";
            out << ind2 << "    } catch(const std::exception& e) { dbg_error(" << pb.id << ", e.what()); " << finishCmd << " } catch(...) { dbg_error(" << pb.id << ", \"Unknown Exit Error\"); " << finishCmd << " }\n";
            out << ind2 << "} break;\n";
        }
    }

    if (isMulti) {
        out << "                        }\n";
        out << "                    }\n";
        out << "                    previous_states[i] = currentState;\n";
        out << "                    active_states[i] = nextState;\n";
        out << "                    state_changed[i] = true;\n";
        out << "                }\n";
        out << "                i++;\n";
        out << "            }\n";
        out << "        }\n";
        out << "        delay(10);\n";
    } else {
        out << "                }\n";
        out << "            }\n";
        out << "            previousState = currentState;\n";
        out << "            currentState = nextState;\n";
        out << "            stateChanged = true;\n";
        out << "        }\n";
        out << "        delay(10);\n";
    }

    out << "    }\n";    // Закрываем цикл while
    out << "}\n";      // Закрываем функцию run()
    out << "};\n\n";

    // Main для запуска
    out << "int main() {\n";
    out << "    StateMachine app;\n";
    out << "    app.run();\n";
    out << "    return 0;\n";
    out << "}\n";

    result.sourceCode = code;
    return result;
}

// === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===
QString CppTranslator2::extractAndRemoveTypes(const QString &rawCode, QMap<QString, VariableInfo> &variables)
{
    QString code = rawCode;
    QRegularExpression varRegex(R"(\b(int|double|float|bool|std::string|String|char|long|short|unsigned|auto)\s+(\w+)(\s*=[^;]+)?\s*;)");
    QRegularExpressionMatchIterator i = varRegex.globalMatch(code);

    struct MatchInfo { QString fullMatch, type, name, initPart; };
    QList<MatchInfo> matches;

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        matches.append({ match.captured(0), match.captured(1), match.captured(2), match.captured(3) });
    }


    for (const auto &m : matches) {
            QString type = m.type;
            if (type == "String") type = "std::string";

            // Явное создание структуры для QMap (защита от ошибок компилятора)
            if (!variables.contains(m.name)) {
                VariableInfo vInfo;
                vInfo.type = type;
                vInfo.name = m.name;
                variables.insert(m.name, vInfo);
            }

            QString replacement;
            if (!m.initPart.isEmpty()) replacement = m.name + " " + m.initPart + ";";
            else replacement = "";
            code.replace(m.fullMatch, replacement);
        }
    return code;
}

QString CppTranslator2::sanitizeCppCode(const QString &rawCode, bool isCondition) {
    QString code = rawCode.trimmed();
    if (code.endsWith(":")) code.chop(1);
    if (code.endsWith(":;")) code.chop(2);

    code.replace(QRegularExpression("\\bboolean\\b"), "bool");
    code.replace("System.out.println", "log");
    code.replace(QRegularExpression("\\band\\b"), "&&");
    code.replace(QRegularExpression("\\bor\\b"), "||");
    if (!isCondition && !code.isEmpty() && !code.endsWith(';') && !code.endsWith('}')) code += ";";
    return code;
}

QString CppTranslator2::addIndent(const QString &code, int level) {
    QStringList lines = code.split("\n");
    QString indentString;
    for (int i = 0; i < level * 4; ++i) indentString += " ";
    QString result;
    for (const QString &line : lines) {
        if (!line.trimmed().isEmpty()) result += indentString + line + "\n";
    }
    return result;
}
