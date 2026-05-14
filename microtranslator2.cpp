/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "microtranslator2.h"
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

// ИЗМЕНЕНИЕ: Возвращаемый тип MicroDebugResult
MicroDebugResult MicroTranslator2::generateDebugSource(QGraphicsScene *scene, const QString &manualHeader)
{
    MicroDebugResult result;
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
                QObject::tr("Внимание: Симулятор Arduino (C++)"),
                QObject::tr("Умный сканер обнаружил потенциально опасный код\n"
                "(деление, массивы, указатели или выделение памяти).\n\n"
                "Так как симуляция выполняется на ПК, грубые ошибки (например, деление на ноль) "
                "могут привести к вылету симулятора.\n\n"
                "Рекомендуется сохранить проект. Хотите продолжить?"),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::No) {
                result.error = QObject::tr("Симуляция отменена пользователем.");
                return result;
            }
        }
    }
    // ==========================================

    // 1. Сбор блоков
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
            }
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

    // 2. ID и предварительная очистка
    int blockIdCounter = 1;
    QMap<DiagramItem*, int> blockIdMap;

    for (int i = 0; i < blocks.size(); ++i) {
        blocks[i].id = blockIdCounter++;
        blockIdMap[blocks[i].item] = blocks[i].id;
        result.idMap.insert(blocks[i].id, blocks[i].item);

        QString rawEnter = sanitizeForArduinoDebug(blocks[i].item->getOnEnter(), false);
        QString rawExit = sanitizeForArduinoDebug(blocks[i].item->getOnExit(), false);

        QString pyErr;
        if (hasPythonSyntax(rawEnter, pyErr)) { result.error = QObject::tr("Block ") + QString::number(blocks[i].id) + QObject::tr(" contains Python code: ") + pyErr; return result; }
        if (hasPythonSyntax(rawExit, pyErr)) { result.error = QObject::tr("Block ") + QString::number(blocks[i].id) + QObject::tr(" contains Python code: ") + pyErr; return result; }

        blocks[i].cleanOnEnter = extractAndRemoveTypes(rawEnter, autoVariables);
        blocks[i].cleanOnExit = extractAndRemoveTypes(rawExit, autoVariables);
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
                historyDeclarations += "int " + varName + " = " + QString::number(defaultTargetId) + ";\n";

                for (DiagramItem* b : blockIdMap.keys()) {
                    if (g->sceneBoundingRect().contains(b->sceneBoundingRect().center()) && b != hist) {
                        blockToHistoryVar[blockIdMap[b]] = varName;
                    }
                }
            }
        }
    }

    // 3. ГЕНЕРАЦИЯ КОДА (Монолитный файл для ПК)
    QString code;
    QTextStream out(&code);

    out << "#include <iostream>\n";
    out << "#include <string>\n";
    out << "#include <thread>\n";
    out << "#include <chrono>\n";
    out << "#include <cmath>\n";
    out << "#include <vector>\n";
    out << "#include <stdexcept>\n\n";

    // === ARDUINO COMPATIBILITY LAYER FOR PC ===
    out << "// --- ARDUINO TYPES MAPPING ---\n";
    out << "using String = std::string;\n";
    out << "using byte = unsigned char;\n";
    out << "using boolean = bool;\n";
    out << "using uint8_t = unsigned char;\n";
    out << "using uint32_t = unsigned int;\n\n";

    out << "// --- PC MOCKS FOR TIME & DELAY ---\n";
    out << "void delay(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }\n";
    out << "unsigned long millis() {\n";
    out << "    static auto start = std::chrono::steady_clock::now();\n";
    out << "    auto now = std::chrono::steady_clock::now();\n";
    out << "    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();\n";
    out << "}\n\n";

    // Mock Serial
    out << "// --- PC MOCK FOR SERIAL ---\n";
    out << "class SerialMock {\n";
    out << "public:\n";
    out << "    void begin(int baud) { std::cout << \"[SERIAL START] Baud: \" << baud << std::endl; }\n";
    out << "    void println(String s) { std::cout << \"[SERIAL] \" << s << std::endl; }\n";
    out << "    void println(const char* s) { std::cout << \"[SERIAL] \" << s << std::endl; }\n";
    out << "    void println(int i) { std::cout << \"[SERIAL] \" << i << std::endl; }\n";
    out << "    void println(double d) { std::cout << \"[SERIAL] \" << d << std::endl; }\n";
    out << "    void print(String s) { std::cout << \"[SERIAL_P] \" << s; }\n";
    out << "    void print(const char* s) { std::cout << \"[SERIAL_P] \" << s; }\n";
    out << "    operator bool() { return true; }\n";
    out << "};\n";
    out << "SerialMock Serial;\n\n";

    // Debug Tools (TELEMETRY)
    out << "// --- TELEMETRY TOOLS ---\n";
    out << "void dbg_block(int id) { std::cout << \"[DBG_BLOCK] \" << id << std::endl; }\n";
    out << "void dbg_arrow(int id) { std::cout << \"[DBG_TRANSITION] \" << id << std::endl; }\n";
    out << "void dbg_false(int id) { std::cout << \"[DBG_FALSE] \" << id << std::endl; }\n";
    out << "void dbg_clear(int id) { std::cout << \"[DBG_CLEAR] \" << id << std::endl; }\n";
    out << "void dbg_arrow_error(int id, std::string msg) { std::cout << \"[DBG_ARROW_ERROR] \" << id << \" \" << msg << std::endl; }\n";
    out << "void dbg_error(int id, std::string msg) { std::cout << \"[DBG_ERROR] \" << id << \" \" << msg << std::endl; }\n\n";


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

        if (!includesCode.isEmpty()) {
            out << "// --- User Headers ---\n" << includesCode << "\n\n";
        }

        // --- FSM ARCHITECTURE (OOP PRO) ---
        out << "class StateMachine;\n";
        out << "typedef void (StateMachine::*StateFunc)(int tokenIdx);\n\n";

        out << "#define FLAG_ENTER (1u << 0)\n";
        out << "#define FLAG_EXIT  (1u << 1)\n\n";

        out << "typedef struct {\n";
                out << "    StateFunc     current;\n";
                out << "    StateFunc     nextState;\n";
                out << "    StateFunc     previousState;\n";
                for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                    out << "    StateFunc     " << it.value() << ";\n";
                }
                out << "    uint8_t       flags;\n";
                out << "    unsigned long stateStartTime;\n";
                out << "} FSM;\n\n";

        int maxTokens = isMulti ? startBlocks.size() : 1;

        out << "class StateMachine {\n";
        out << "public:\n";
        out << "    bool isRunning = true;\n\n";

        if (!membersCode.isEmpty()) {
            out << "// --- User Variables & Methods ---\n";
            out << membersCode << "\n";
        }

        out << "// --- GLOBAL VARIABLES ---\n";
        if (!autoVariables.isEmpty()) {
            for (auto it = autoVariables.begin(); it != autoVariables.end(); ++it) {
                QString type = it.value().type;
                if (type == "String") type = "std::string";
                QString initVal;
                if (type == "int" || type == "long" || type == "byte") initVal = " = 0";
                else if (type == "float" || type == "double") initVal = " = 0.0";
                else if (type == "bool" || type == "boolean") initVal = " = false";
                out << "    " << type << " " << it.value().name << initVal << ";\n";
            }
        }

        out << "\npublic:\n";
        out << "    StateMachine() {\n";
        out << "        activeCount = " << maxTokens << ";\n";
        out << "    }\n";
        out << "    void FSM_Init();\n";
        out << "    void FSM_Tick();\n";
        out << "    void requestTransition(int i, StateFunc target);\n";
        out << "    bool anyAlive();\n\n";

        out << "private:\n";
        out << "    FSM activeFSMs[" << maxTokens << "];\n";
        out << "    int activeCount;\n\n";

        // State Prototypes
        for (const auto& block : blocks) {
            if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;
            out << "    void state_" << block.id << "_func(int i);\n";
        }
        out << "};\n\n";

        // --- METHODS IMPLEMENTATION ---
        out << "void StateMachine::requestTransition(int i, StateFunc target) {\n";
        out << "    activeFSMs[i].nextState = target;\n";
        out << "    activeFSMs[i].flags |= FLAG_EXIT;\n";
        out << "}\n\n";


        out << "void StateMachine::FSM_Init() {\n";
                for (int i = 0; i < maxTokens; ++i) {
                    out << "    activeFSMs[" << i << "].current = &StateMachine::state_" << blockIdMap[isMulti ? startBlocks[i] : startBlocks.first()] << "_func;\n";
                    out << "    activeFSMs[" << i << "].flags = FLAG_ENTER;\n";
                    for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                        out << "    activeFSMs[" << i << "]." << it.value() << " = nullptr;\n";
                    }
                }
                out << "}\n\n";


        out << "void StateMachine::FSM_Tick() {\n";
        out << "    for(int i = 0; i < activeCount; i++) {\n";
        out << "        if (activeFSMs[i].current == nullptr) continue;\n\n";
        out << "        if (activeFSMs[i].flags & FLAG_EXIT) {\n";
        out << "            (this->*(activeFSMs[i].current))(i);\n";
        out << "            activeFSMs[i].previousState = activeFSMs[i].current;\n";
        out << "            activeFSMs[i].current = activeFSMs[i].nextState;\n";
        out << "            if (activeFSMs[i].current != nullptr) activeFSMs[i].flags = FLAG_ENTER;\n";
        out << "            else activeFSMs[i].flags = 0;\n";
        out << "        } else {\n";
        out << "            (this->*(activeFSMs[i].current))(i);\n";
        out << "        }\n";
        out << "    }\n";
        out << "}\n\n";

        out << "bool StateMachine::anyAlive() {\n";
        out << "    for (int i = 0; i < activeCount; i++) {\n";
        out << "        if (activeFSMs[i].current != nullptr) return true;\n";
        out << "    }\n";
        out << "    return false;\n";
        out << "}\n\n";

        // Генерация состояний
        int arrowIdCounter = 1000;
        for (const auto& block : blocks) {
            if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;

            out << "// ==========================================\n";
            out << "// State ID: " << block.id << " (" << block.item->getLabel() << ")\n";
            out << "void StateMachine::state_" << block.id << "_func(int i) {\n";

            // --- ON ENTER ---
            out << "    if (activeFSMs[i].flags & FLAG_ENTER) {\n";
            out << "        try {\n";


            if (blockToHistoryVar.contains(block.id)) {
                            out << "            activeFSMs[i]." << blockToHistoryVar[block.id] << " = &StateMachine::state_" << block.id << "_func;\n";
                        }


            if (block.item->data(ROLE_DEBUG_ENABLED).toBool()) {
                out << "            dbg_block(" << block.id << ");\n";
                out << "            delay(500);\n";
            }
            if (!block.cleanOnEnter.isEmpty()) out << addIndent(block.cleanOnEnter, 3) << "\n";
            out << "            activeFSMs[i].stateStartTime = millis();\n";
            out << "            activeFSMs[i].flags &= ~FLAG_ENTER;\n";
            out << "            return;\n";
            out << "        } catch (const std::exception& e) {\n";
            out << "            dbg_error(" << block.id << ", e.what()); isRunning = false; return;\n";
            out << "        } catch (...) {\n";
            out << "            dbg_error(" << block.id << ", \"Unknown Enter Error\"); isRunning = false; return;\n";
            out << "        }\n";
            out << "    }\n\n";

            // --- ON EXIT ---
            out << "    if (activeFSMs[i].flags & FLAG_EXIT) {\n";
            out << "        try {\n";
            if (!block.cleanOnExit.isEmpty()) out << addIndent(block.cleanOnExit, 3) << "\n";
            out << "            return;\n";
            out << "        } catch (const std::exception& e) {\n";
            out << "            dbg_error(" << block.id << ", e.what()); isRunning = false; return;\n";
            out << "        } catch (...) {\n";
            out << "            dbg_error(" << block.id << ", \"Unknown Exit Error\"); isRunning = false; return;\n";
            out << "        }\n";
            out << "    }\n\n";

            // --- TRANSITIONS ---
            // 1. Мосты
            if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(block.item)) {
                QString targetName = bridge->targetName().trimmed();
                int targetId = -1;
                QString searchName = targetName.startsWith("_") ? targetName.mid(1) : targetName;
                for (auto it = blockIdMap.begin(); it != blockIdMap.end(); ++it) {
                    QString blockLabel = it.key()->getLabel().trimmed();
                    QString cleanLabel = blockLabel.startsWith("_") ? blockLabel.mid(1) : blockLabel;
                    if (cleanLabel.compare(searchName, Qt::CaseInsensitive) == 0) { targetId = it.value(); break; }
                }
                if (targetId != -1) {
                    if (block.item->data(ROLE_DEBUG_ENABLED).toBool()) out << "    dbg_clear(" << block.id << ");\n";
                    out << "    requestTransition(i, &StateMachine::state_" << targetId << "_func);\n";
                } else {
                    out << "    dbg_error(" << block.id << ", \"Bridge target not found: " << targetName << "\");\n";
                    out << "    requestTransition(i, nullptr);\n";
                }
                out << "    return;\n";
                out << "}\n\n";
                continue;
            }

            // 2. Обычные стрелки
            QList<ConnectionItem*> transitions;
            for (QGraphicsItem *item : scene->items()) {
                 if (item->data(ConnectionRole).toBool()) {
                     ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item);
                     if (conn && conn->startBlock() == block.item && conn->endBlock()) transitions.append(conn);
                 }
            }
            std::sort(transitions.begin(), transitions.end(), [](ConnectionItem *a, ConnectionItem *b) {
                int p1 = (a->transition()) ? a->transition()->getPriority() : 999;
                int p2 = (b->transition()) ? b->transition()->getPriority() : 999;
                return p1 < p2;
            });

            bool hasUncond = false;
            for (ConnectionItem *conn : transitions) {
                int arrowId = arrowIdCounter++;
                result.idMap.insert(arrowId, conn);

                QString cond = sanitizeForArduinoDebug(conn->transition()->conditionText(), true).trimmed();
                if (cond.toLower() == "if") cond = "";
                if (cond.toLower().startsWith("if ")) cond = cond.mid(3).trimmed();

                QString act = extractAndRemoveTypes(sanitizeForArduinoDebug(conn->transition()->actionText(), false), autoVariables);
                QString cleanActTest = act.trimmed().toLower();
                if (cleanActTest == "action" || cleanActTest == "action;") act = "";

                QString targetFunc = "&StateMachine::state_" + QString::number(blockIdMap[conn->endBlock()]) + "_func";


                if (BackItem* back = dynamic_cast<BackItem*>(conn->endBlock())) {
                                    if (back->backMode() != BackItem::Back) {
                                        int targetId = block.id; // Fallback
                                        QString tName = back->targetName();
                                        for (auto it = blockIdMap.begin(); it != blockIdMap.end(); ++it) {
                                            if (it.key()->getLabel() == tName || it.key()->getDescription() == tName) {
                                                targetId = it.value(); break;
                                            }
                                        }
                                        targetFunc = "&StateMachine::state_" + QString::number(targetId) + "_func";
                                    } else {
                                        targetFunc = "activeFSMs[i].previousState";
                                    }
                                } else if (HistoryItem* hist = dynamic_cast<HistoryItem*>(conn->endBlock())) {
                                    if (historyTargetVar.contains(hist)) {
                                        QString varName = historyTargetVar[hist];
                                        int defaultTargetId = 0;
                                        for (QGraphicsItem *ci : scene->items()) {
                                            if (ConnectionItem *c = dynamic_cast<ConnectionItem*>(ci)) {
                                                if (c->startBlock() == hist && c->endBlock() && !c->isBlocked()) {
                                                    defaultTargetId = blockIdMap[c->endBlock()]; break;
                                                }
                                            }
                                        }
                                        if (defaultTargetId != 0) {
                                            targetFunc = "(activeFSMs[i]." + varName + " != nullptr) ? activeFSMs[i]." + varName + " : &StateMachine::state_" + QString::number(defaultTargetId) + "_func";
                                        } else {
                                            targetFunc = "activeFSMs[i]." + varName;
                                        }
                                    }
                                }



                bool isAlwaysTrue = (cond.trimmed().isEmpty() || cond == "true" || cond == "1");

                if (isAlwaysTrue) {
                    out << "    try {\n";
                    if (conn->data(ROLE_DEBUG_ENABLED).toBool()) {
                        out << "        dbg_arrow(" << arrowId << "); delay(300);\n";
                    }
                    if (!act.isEmpty()) out << "        " << act << "\n";
                    out << "        requestTransition(i, " << targetFunc << ");\n";
                    out << "        return;\n";
                    out << "    } catch (const std::exception& e) {\n";
                    out << "        dbg_arrow_error(" << arrowId << ", e.what()); isRunning = false; return;\n";
                    out << "    }\n";
                    hasUncond = true;
                    break;
                } else {
                    out << "    try {\n";
                    out << "        if (" << cond << ") {\n";
                    if (conn->data(ROLE_DEBUG_ENABLED).toBool()) {
                        out << "            dbg_arrow(" << arrowId << "); delay(300);\n";
                    }
                    if (!act.isEmpty()) out << "            " << act << "\n";
                    out << "            requestTransition(i, " << targetFunc << ");\n";
                    out << "            return;\n";
                    out << "        } else {\n";
                    if (conn->data(ROLE_DEBUG_ENABLED).toBool()) {
                        out << "            dbg_false(" << arrowId << ");\n";
                    }
                    out << "        }\n";
                    out << "    } catch (const std::exception& e) {\n";
                    out << "        dbg_arrow_error(" << arrowId << ", e.what()); isRunning = false; return;\n";
                    out << "    }\n";
                }
            }

            if (!hasUncond && transitions.isEmpty()) {
                 out << "    std::cout << \"[LOG] Token finished.\" << std::endl;\n";
                 out << "    requestTransition(i, nullptr);\n";
            }
            out << "}\n\n";
        }

        // --- MAIN FUNCTION ---
        out << "int main() {\n";
        out << "    Serial.begin(9600);\n";
        out << "    StateMachine sm;\n";
        out << "    sm.FSM_Init();\n";
        out << "    delay(500);\n\n";
        out << "    while (sm.isRunning) {\n";
        out << "        sm.FSM_Tick();\n";
        out << "        if (!sm.anyAlive()) break;\n";
        out << "    }\n";
        out << "    return 0;\n";
        out << "}\n";

        result.sourceCode = code;
        return result;
    }



// === УТИЛИТЫ (PRO VERSION) ===

QString MicroTranslator2::extractAndRemoveTypes(const QString &rawCode, QMap<QString, VariableInfo> &variables) {
    QString code = rawCode;
    QRegularExpression varRegex(R"(\b(int|float|double|bool|boolean|String|char|long|unsigned|byte|uint8_t|uint16_t|uint32_t|word|short)\s+(\w+)(\s*=[^;]+)?\s*;)");

    QRegularExpressionMatchIterator i = varRegex.globalMatch(code);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString type = match.captured(1);
        QString name = match.captured(2);
        QString val = match.captured(3);
        if (!variables.contains(name)) variables.insert(name, {type, name});

        if (!val.isEmpty()) code.replace(match.captured(0), name + " " + val + ";");
        else code.replace(match.captured(0), "");
    }
    return code;
}

QString MicroTranslator2::sanitizeForArduinoDebug(const QString &rawCode, bool isCondition) {
    QString code = rawCode;

    code.replace(QRegularExpression("\\bboolean\\b"), "bool");
    code.replace(QRegularExpression("\\band\\b"), "&&");
    code.replace(QRegularExpression("\\bor\\b"), "||");

    if (!isCondition && !code.trimmed().isEmpty() && !code.trimmed().endsWith(';') && !code.trimmed().endsWith('}')) {
        code += ";";
    }
    return code;
}

QString MicroTranslator2::addIndent(const QString &code, int level) {
    QStringList lines = code.split("\n");
    QString indent; for(int i=0; i<level*4; ++i) indent+=" ";
    QString res; for(const QString &l : lines) if(!l.trimmed().isEmpty()) res += indent + l + "\n";
    return res;
}

bool MicroTranslator2::hasPythonSyntax(const QString &code, QString &foundFeature) {
    if (code.contains("def ")) { foundFeature = "def"; return true; }
    if (code.contains("import ")) { foundFeature = "import"; return true; }
    if (code.contains("print(")) { foundFeature = "print() (use Serial.print)"; return true; }
    return false;
}
