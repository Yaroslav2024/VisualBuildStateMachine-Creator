/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "microcontrollertranslator.h"
#include "diagramitem.h"
#include "connect.h"
#include "transitiongroup.h"
#include <QTextStream>
#include <QRegularExpression>
#include <algorithm>
#include "commentitem.h"
#include "bridgeitem.h"
#include "backitem.h"
#include "groupitem.h"
#include "historyitem.h"

MicroTranslator::MicroTranslator(QObject *parent) : QObject(parent) {}

MicroCode MicroTranslator::generate(QGraphicsScene *scene, const QString &manualHeader)
{
    MicroCode result;
    QMap<QString, VariableInfo> globalVars;

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
            } else {
                blocks.append({b, 0, "", ""});
            }
        }
    }

    if (startBlocks.isEmpty()) {
        result.error = QObject::tr("ERROR: Start block is missing!");
        return result;
    }

    for (DiagramItem* sb : startBlocks) {
        blocks.prepend({sb, 0, "", ""});
    }

    bool isMulti = (startBlocks.size() > 1);
    QString finishCmd = isMulti ? "token_finished = true;" : "isRunning = false;";

    // 2. Нумерация и чистка кода
    int idCounter = 1;
    QMap<DiagramItem*, int> blockIdMap;

    for (int i = 0; i < blocks.size(); ++i) {
        blocks[i].id = idCounter++;
        blockIdMap[blocks[i].item] = blocks[i].id;

        // Чистим код и достаем переменные (int x = 0 -> x = 0)
        QString rawEnter = sanitizeForArduino(blocks[i].item->getOnEnter(), false);
        QString rawExit = sanitizeForArduino(blocks[i].item->getOnExit(), false);

        blocks[i].cleanOnEnter = extractVariables(rawEnter, globalVars);
        blocks[i].cleanOnExit = extractVariables(rawExit, globalVars);
    }
    // --- ГЕНЕРАЦИЯ ИМЕН ДЛЯ ENUM ---
        // ==========================================
        QMap<int, QString> enumNames;
        for (const auto& block : blocks) {
            QString cleanLabel = block.item->getLabel().toUpper();
            cleanLabel.replace(QRegularExpression("[^A-Z0-9_]"), "_");
            cleanLabel.replace(QRegularExpression("_+"), "_");
            if (cleanLabel.isEmpty() || cleanLabel == "_") cleanLabel = "BLOCK";
            enumNames[block.id] = QString("STATE_%1_%2").arg(block.id).arg(cleanLabel);
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

                QString defTarget = defaultTargetId != 0 ? enumNames[defaultTargetId] : "0";
                                if (isMulti) {
                                    QStringList inits;
                                    for (int i = 0; i < startBlocks.size(); ++i) inits.append(defTarget);
                                    historyDeclarations += "int " + varName + "[" + QString::number(startBlocks.size()) + "] = {" + inits.join(", ") + "};\n";
                                } else {
                                    historyDeclarations += "int " + varName + " = " + defTarget + ";\n";
                                }
                                      // ==========================================

                // Привязываем все блоки внутри рамки к этой переменной
                for (DiagramItem* b : blockIdMap.keys()) {
                    if (g->sceneBoundingRect().contains(b->sceneBoundingRect().center()) && b != hist) {
                        blockToHistoryVar[blockIdMap[b]] = varName;
                    }
                }
            }
        }
    }
    // ==========================================

    // 3. Генерация .INO файла
    QString code;
    QTextStream out(&code);

    // --- ЗАГОЛОВОК ---
    out << "// Generated for Arduino / ESP32 / STM32\n";
    out << "// Built with VisioLite\n\n";

    if (!manualHeader.isEmpty()) {
        out << "// --- User Libraries ---\n";
        out << manualHeader << "\n\n";
    }

    // --- STATE MACHINE ENUM ---
        out << "// --- State Enum ---\n";
        out << "enum States {\n";
        QStringList enumLines;
        for (const auto& block : blocks) {
            if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;
            enumLines.append("    " + enumNames[block.id] + " = " + QString::number(block.id));
        }
        out << enumLines.join(",\n") << "\n};\n\n";
    // --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
    out << "// --- Global Variables ---\n";
    if (isMulti) {
        out << "int activeCount = " << startBlocks.size() << ";\n";

        out << "int activeStates[" << startBlocks.size() << "] = {";
        for (int i = 0; i < startBlocks.size(); ++i) out << enumNames[blockIdMap[startBlocks[i]]] << (i < startBlocks.size() - 1 ? ", " : "");
        out << "};\n";

        out << "int previousStates[" << startBlocks.size() << "] = {";
        for (int i = 0; i < startBlocks.size(); ++i) out << "-1" << (i < startBlocks.size() - 1 ? ", " : "");
        out << "};\n";
        out << "bool stateChanged[" << startBlocks.size() << "] = {";
        for (int i = 0; i < startBlocks.size(); ++i) out << "true" << (i < startBlocks.size() - 1 ? ", " : "");
        out << "};\n";
        out << "unsigned long stateStartTimes[" << startBlocks.size() << "] = {0};\n";
    } else {
        out << "int currentState = " << enumNames[blockIdMap[startBlocks.first()]] << ";\n";
        out << "int previousState = -1;\n";
        out << "bool stateChanged = true;\n";
        out << "unsigned long stateStartTime = 0;\n";
    }
    out << "bool isRunning = true;\n\n";

    for (auto it = globalVars.begin(); it != globalVars.end(); ++it) {
        QString init = it.value().initValue;
        if (init.isEmpty()) {
            if (it.value().type == "int" || it.value().type == "float" || it.value().type == "double") init = " = 0";
            else if (it.value().type == "bool") init = " = false";
            else if (it.value().type == "String") init = " = \"\"";
        } else {
            init = " " + init;
        }
        out << it.value().type << " " << it.value().name << init << ";\n";
    }
    out << "\n";
    out << historyDeclarations;

    // --- SETUP ---
    out << "void setup() {\n";
    out << "    Serial.begin(115200);\n";
    out << "    while(!Serial) { delay(10); } // Wait for USB\n";
    out << "    Serial.println(\"--- Robot Started ---\");\n";
    out << "}\n\n";

    // --- LOOP ---
    out << "void loop() {\n";
    if (isMulti) {
        out << "    if (!isRunning || activeCount == 0) return;\n\n";
        out << "    for (int i = 0; i < activeCount; ) {\n";
        out << "        int currentState = activeStates[i];\n";
        out << "        int nextState = currentState;\n";
        out << "        bool token_finished = false;\n\n";
        out << "        // --- ON ENTER PHASE ---\n";
        out << "        if (stateChanged[i]) {\n";
    } else {
        out << "    if (!isRunning) return;\n\n";
        out << "    int nextState = currentState;\n\n";
        out << "    // --- ON ENTER PHASE ---\n";
        out << "    if (stateChanged) {\n";
    }

    out << "            switch (currentState) {\n";
    for (const auto& block : blocks) {
        if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;

        bool hasEnterCode = !block.cleanOnEnter.isEmpty() || blockToHistoryVar.contains(block.id);
        QString comments = CommentItem::getAssociatedCode(block.item, "// ");
        bool hasComments = !comments.isEmpty();

        if (hasEnterCode || hasComments) {

            out << "                case " << enumNames[block.id] << ": {\n";

            if (blockToHistoryVar.contains(block.id)) {
                                        out << "                    " << blockToHistoryVar[block.id] << (isMulti ? "[i]" : "") << " = " << enumNames[block.id] << "; // Update History\n";
                        }

            if (hasComments) {
                out << "                    " << comments.replace("\n", "\n                    ");
            }
            if (!block.cleanOnEnter.isEmpty()) {
                out << "                    " << QString(block.cleanOnEnter).replace("\n", "\n                    ") << "\n";
            }
            out << "                } break;\n";
        }
    }
    out << "            }\n"; // end switch onEnter

    if (isMulti) {
        out << "            stateStartTimes[i] = millis();\n";
        out << "            stateChanged[i] = false;\n";
        out << "        }\n\n";
        out << "        // --- TRANSITION PHASE ---\n";
        out << "        switch (currentState) {\n";
    } else {
        out << "            stateStartTime = millis();\n";
        out << "            stateChanged = false;\n";
        out << "    }\n\n";
        out << "    // --- TRANSITION PHASE ---\n";
        out << "    switch (currentState) {\n";
    }

    QString indent = isMulti ? "            " : "        ";
    QString indent2 = isMulti ? "                " : "            ";

    for (const auto& block : blocks) {
            if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;
            out << indent << "case " << enumNames[block.id] << ": {\n";

        out << indent2 << "// Block: " << (block.item->getType() == "Стартовый блок" ? "START" : "STATE") << "\n";

        // ==========================================
        // БРОНЕБОЙНЫЙ ПОИСК ЦЕЛИ ДЛЯ МОСТА
        // ==========================================
        if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(block.item)) {
            QString targetName = bridge->targetName().trimmed();
            int targetId = -1;

            QString searchName = targetName;
            if (searchName.startsWith("_")) searchName = searchName.mid(1);

            for (auto it = blockIdMap.begin(); it != blockIdMap.end(); ++it) {
                QString blockLabel = it.key()->getLabel().trimmed();
                QString cleanLabel = blockLabel;
                if (cleanLabel.startsWith("_")) cleanLabel = cleanLabel.mid(1);

                if (cleanLabel.compare(searchName, Qt::CaseInsensitive) == 0 ||
                    blockLabel.compare(targetName, Qt::CaseInsensitive) == 0) {
                    targetId = it.value(); break;
                }
            }
            if (targetId == -1) {
                for (auto it = blockIdMap.begin(); it != blockIdMap.end(); ++it) {
                    QString blockDesc = it.key()->getDescription().trimmed();
                    if (blockDesc.compare(searchName, Qt::CaseInsensitive) == 0 ||
                        blockDesc.compare(targetName, Qt::CaseInsensitive) == 0) {
                        targetId = it.value(); break;
                    }
                }
            }


            if (targetId != -1) {
                            out << indent2 << "// Bridge jump to: " << targetName << "\n";
                            out << indent2 << "nextState = " << enumNames[targetId] << ";\n";

                out << indent2 << "break;\n";
            } else {
                out << indent2 << "// ERROR: Bridge target not found\n";
                out << indent2 << "Serial.print(\"Error: Bridge target not found: \");\n";
                out << indent2 << "Serial.println(\"" << targetName << "\");\n";
                out << indent2 << finishCmd << "\n";
                out << indent2 << "break;\n";
            }
            out << indent << "} break;\n\n";
            continue;
        }

        // Логика переходов
        QList<ConnectionItem*> transitions;
        for (QGraphicsItem *item : scene->items()) {
             if (item->data(ConnectionRole).toBool()) {
                 ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item);
                 if (conn && conn->startBlock() == block.item && conn->endBlock()) {
                     transitions.append(conn);
                 }
             }
        }
        std::sort(transitions.begin(), transitions.end(), [](ConnectionItem *a, ConnectionItem *b) {
            int p1 = (a->transition()) ? a->transition()->getPriority() : 999;
            int p2 = (b->transition()) ? b->transition()->getPriority() : 999;
            return p1 < p2;
        });

        bool hasUncond = false;
        for (ConnectionItem *conn : transitions) {
            QString arrowComm = CommentItem::getAssociatedCode(conn, "// ");
            if (!arrowComm.isEmpty()) {
                out << indent2 << arrowComm.replace("\n", "\n" + indent2);
            }

            QString act = extractVariables(sanitizeForArduino(conn->transition()->actionText(), false), globalVars);
            QString cond = sanitizeForArduino(conn->transition()->conditionText(), true).trimmed();
            if (cond.toLower() == "if") cond = "";
            if (cond.toLower().startsWith("if ")) cond = cond.mid(3).trimmed();

            QString targetStr = enumNames[blockIdMap[conn->endBlock()]];



            if (BackItem* back = dynamic_cast<BackItem*>(conn->endBlock())) {
                            if (back->backMode() != BackItem::Back) {
                                int targetId = block.id;
                                QString tName = back->targetName();
                                for (auto it = blockIdMap.begin(); it != blockIdMap.end(); ++it) {
                                    if (it.key()->getLabel() == tName || it.key()->getDescription() == tName) {
                                        targetId = it.value(); break;
                                    }
                                }
                                targetStr = enumNames[targetId];
                            } else {
                                targetStr = isMulti ? "(previousStates[i] != -1 ? previousStates[i] : currentState)"
                                                    : "(previousState != -1 ? previousState : currentState)";
                            }
                        } else if (HistoryItem* hist = dynamic_cast<HistoryItem*>(conn->endBlock())) {
                            if (historyTargetVar.contains(hist)) {
                                targetStr = historyTargetVar[hist] + (isMulti ? "[i]" : "");
                            }
                        }



            bool isTrue = (cond.isEmpty() || cond == "true" || cond == "1");

            if (isTrue) {
                if (!act.isEmpty()) out << indent2 << act << "\n";
                out << indent2 << "nextState = " << targetStr << ";\n";
                hasUncond = true;
                break;
            } else {
                out << indent2 << "if (" << cond << ") {\n";
                if (!act.isEmpty()) out << indent2 << "    " << act.replace("\n", "\n    " + indent2) << "\n";
                out << indent2 << "    nextState = " << targetStr << ";\n";
                out << indent2 << "    break;\n";
                out << indent2 << "}\n";
            }
        }

        if (!hasUncond && transitions.isEmpty()) {
             out << indent2 << "Serial.println(\"Finished.\");\n";
             out << indent2 << finishCmd << "\n";
        }
        out << indent << "} break;\n\n";
    }

    out << indent << "default: " << finishCmd << " break;\n";

    if (isMulti) {
        out << "        }\n\n"; // end switch
        out << "        // --- ON EXIT & STATE UPDATE PHASE ---\n";
        out << "        if (token_finished) {\n";
        out << "            for (int j = i; j < activeCount - 1; j++) {\n";
        out << "                activeStates[j] = activeStates[j + 1];\n";
        out << "                previousStates[j] = previousStates[j + 1];\n";
        out << "                stateChanged[j] = stateChanged[j + 1];\n";
        out << "                stateStartTimes[j] = stateStartTimes[j + 1];\n";
        out << "            }\n";
        out << "            activeCount--;\n";
        out << "        } else {\n";
        out << "            if (nextState != currentState) {\n";
        out << "                switch (currentState) {\n";
    } else {
        out << "    }\n\n"; // end switch
        out << "    // --- ON EXIT & STATE UPDATE PHASE ---\n";
        out << "    if (nextState != currentState) {\n";
        out << "        switch (currentState) {\n";
    }

    // --- ОБРАБОТКА ON EXIT ---
        for (const auto& block : blocks) {
            if (!block.cleanOnExit.isEmpty()) {
            out << indent2 << "case " << enumNames[block.id] << ": {\n";

            out << indent2 << "    " << QString(block.cleanOnExit).replace("\n", "\n    " + indent2) << "\n";
            out << indent2 << "} break;\n";
        }
    }

    if (isMulti) {
        out << "                }\n";
        out << "                previousStates[i] = currentState;\n";
        out << "                activeStates[i] = nextState;\n";
        out << "                stateChanged[i] = true;\n";
        out << "            }\n";
        out << "            i++;\n";
        out << "        }\n";
        out << "    }\n";
    } else {
        out << "        }\n";
        out << "        previousState = currentState;\n";
        out << "        currentState = nextState;\n";
        out << "        stateChanged = true;\n";
        out << "    }\n";
    }

    out << "    \n";

    out << "    yield(); // Yield to RTOS\n";
        out << "}\n";

        // ====================================================================
        // ====================================================================
        // --- ГЕНЕРАЦИЯ .H ФАЙЛА (Профессиональный стиль на указателях) ---
        // ==// ====================================================================
        // --- ГЕНЕРАЦИЯ .H ФАЙЛА (ООП и Умный парсинг Header) ---
        // ====================================================================
        QString hCode;
        QTextStream hOut(&hCode);

        hOut << "#ifndef FSM_CORE_H\n#define FSM_CORE_H\n\n";
        hOut << "#include <stdint.h>\n\n";

        // Внешняя функция времени (пользователь реализует её сам для своей платформы)
        hOut << "extern unsigned long millis();\n\n";

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

        // Добавляем библиотеки пользователя во взрослый проект
        if (!includesCode.isEmpty()) {
            hOut << "// --- User Libraries ---\n";
            hOut << includesCode << "\n";
        }

        // Переиспользуем сгенерированный ранее enum
        hOut << "// --- State Enum ---\n";
        hOut << "enum States {\n";
        hOut << enumLines.join(",\n") << "\n};\n\n";

        hOut << "class StateMachine;\n";
        hOut << "typedef void (StateMachine::*StateFunc)(int tokenIdx);\n\n";

        hOut << "// --- Flags ---\n";
        hOut << "#define FLAG_ENTER (1u << 0)\n";
        hOut << "#define FLAG_EXIT  (1u << 1)\n\n";



        hOut << "// --- FSM structure (with parallelism & history support) ---\n";
                hOut << "typedef struct {\n";
                hOut << "    StateFunc     current;        // Current state function\n";
                hOut << "    StateFunc     nextState;      // Target function during transition\n";
                hOut << "    StateFunc     previousState;  // Function for the return block\n";
                // Динамические ячейки памяти для каждой группы Истории
                for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                    hOut << "    StateFunc     " << it.value() << ";\n";
                }
                hOut << "    uint8_t       flags;          // Enter/Exit flags\n";
                hOut << "    unsigned long stateStartTime; // Non-blocking timer\n";
                hOut << "} FSM;\n\n";



        int maxTokens = isMulti ? startBlocks.size() : 1;

        hOut << "class StateMachine {\n";
        if (!membersCode.isEmpty()) {
            hOut << "// --- User Variables & Methods ---\n";
            hOut << membersCode << "\n";
        }
        hOut << "public:\n";
        hOut << "    StateMachine();\n";
        hOut << "    void FSM_Init();\n";
        hOut << "    void FSM_Tick();\n";
        hOut << "    void requestTransition(int i, StateFunc target);\n\n";
        hOut << "private:\n";
        hOut << "    FSM activeFSMs[" << maxTokens << "];\n";
        hOut << "    int activeCount;\n\n";

        for (auto it = globalVars.begin(); it != globalVars.end(); ++it) {
            hOut << "    " << it.value().type << " " << it.value().name << ";\n";
        }

        hOut << "\n    // --- Prototypes of states ---\n";
        for (const auto& block : blocks) {
            if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;
            hOut << "    void state_" << block.id << "_func(int i);\n";
        }
        hOut << "};\n\n#endif // FSM_CORE_H\n";

        // Сохраняем в объект (Предполагается, что в MicroCode есть поля header и source)
        result.header = hCode;


        // ====================================================================
        // --- ГЕНЕРАЦИЯ .CPP ФАЙЛА (ООП и Умный парсинг Header) ---
        // ====================================================================
        QString cppCode;
        QTextStream cppOut(&cppCode);

        cppOut << "#include \"FsmCore.h\"\n\n";

        cppOut << "StateMachine::StateMachine() {\n";
        cppOut << "    activeCount = " << maxTokens << ";\n";
        for (auto it = globalVars.begin(); it != globalVars.end(); ++it) {
            QString init = it.value().initValue;
            if (init.isEmpty()) {
                if (it.value().type == "int" || it.value().type == "float" || it.value().type == "double") init = " = 0";
                else if (it.value().type == "bool") init = " = false";
                else if (it.value().type == "String") init = " = \"\"";
            } else init = " " + init;
            cppOut << "    " << it.value().name << init << ";\n";
        }
        cppOut << "}\n\n";

        cppOut << "void StateMachine::requestTransition(int i, StateFunc target) {\n";
        cppOut << "    activeFSMs[i].nextState = target;\n";
        cppOut << "    activeFSMs[i].flags |= FLAG_EXIT;\n";
        cppOut << "}\n\n";

        cppOut << "void StateMachine::FSM_Init() {\n";
                for (int i = 0; i < maxTokens; ++i) {
                    cppOut << "    activeFSMs[" << i << "].current = &StateMachine::state_" << (isMulti ? blockIdMap[startBlocks[i]] : blockIdMap[startBlocks.first()]) << "_func;\n";
                    cppOut << "    activeFSMs[" << i << "].flags = FLAG_ENTER;\n";
                    for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                        cppOut << "    activeFSMs[" << i << "]." << it.value() << " = nullptr;\n";
                    }
                }
                cppOut << "}\n\n";

        cppOut << "void StateMachine::FSM_Tick() {\n";
        cppOut << "    for(int i = 0; i < activeCount; i++) {\n";
        cppOut << "        if (activeFSMs[i].current == nullptr) continue; // Token completed\n\n";
        cppOut << "        if (activeFSMs[i].flags & FLAG_EXIT) {\n";
        cppOut << "            (this->*(activeFSMs[i].current))(i); // Call OnExit\n";
        cppOut << "            activeFSMs[i].previousState = activeFSMs[i].current;\n";
        cppOut << "            activeFSMs[i].current = activeFSMs[i].nextState;\n";
        cppOut << "            if (activeFSMs[i].current != nullptr) activeFSMs[i].flags = FLAG_ENTER;\n";
        cppOut << "            else activeFSMs[i].flags = 0;\n";
        cppOut << "        } else {\n";
        cppOut << "            (this->*(activeFSMs[i].current))(i); // Call OnEnter и OnLoop\n";
        cppOut << "        }\n";
        cppOut << "    }\n";
        cppOut << "}\n\n";

        // Генерация функций состояний
        for (const auto& block : blocks) {
            if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;

            cppOut << "// ==========================================\n";
            cppOut << "// State: " << block.item->getLabel() << "\n";
            cppOut << "void StateMachine::state_" << block.id << "_func(int i) {\n";

            // --- ОБНОВЛЕНИЕ ИСТОРИИ (УМНЫЙ ЗАХВАТ РАМКИ) ---
                        if (blockToHistoryVar.contains(block.id)) {
                            cppOut << "    // Update History for Group\n";
                            cppOut << "    activeFSMs[i]." << blockToHistoryVar[block.id] << " = &StateMachine::state_" << block.id << "_func;\n\n";
                        }

            // --- ON ENTER ---
            cppOut << "    if (activeFSMs[i].flags & FLAG_ENTER) {\n";
            if (!block.cleanOnEnter.isEmpty()) cppOut << "        " << QString(block.cleanOnEnter).replace("\n", "\n        ") << "\n";
            cppOut << "        activeFSMs[i].stateStartTime = millis();\n";
            cppOut << "        activeFSMs[i].flags &= ~FLAG_ENTER;\n";
            cppOut << "        return;\n";
            cppOut << "    }\n\n";

            // --- ON EXIT ---
            cppOut << "    if (activeFSMs[i].flags & FLAG_EXIT) {\n";
            if (!block.cleanOnExit.isEmpty()) cppOut << "        " << QString(block.cleanOnExit).replace("\n", "\n        ") << "\n";
            cppOut << "        return;\n";
            cppOut << "    }\n\n";

            // --- TRANSITIONS ---
            if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(block.item)) {
                QString targetName = bridge->targetName().trimmed();
                int targetId = -1;
                // Ищем цель моста (сокращенно, используем логику из .ino)
                QString searchName = targetName.startsWith("_") ? targetName.mid(1) : targetName;
                for (auto it = blockIdMap.begin(); it != blockIdMap.end(); ++it) {
                    QString blockLabel = it.key()->getLabel().trimmed();
                    QString cleanLabel = blockLabel.startsWith("_") ? blockLabel.mid(1) : blockLabel;
                    if (cleanLabel.compare(searchName, Qt::CaseInsensitive) == 0) { targetId = it.value(); break; }
                }
                if (targetId != -1) cppOut << "    requestTransition(i, &StateMachine::state_" << targetId << "_func);\n";
                else cppOut << "    requestTransition(i, nullptr); // Error: Bridge target not found\n";
                cppOut << "}\n\n";
                continue;
            }

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
                QString cond = sanitizeForArduino(conn->transition()->conditionText(), true).trimmed();
                if (cond.toLower() == "if") cond = "";
                if (cond.toLower().startsWith("if ")) cond = cond.mid(3).trimmed();
                QString act = extractVariables(sanitizeForArduino(conn->transition()->actionText(), false), globalVars);

                QString targetFunc = "&StateMachine::state_" + QString::number(blockIdMap[conn->endBlock()]) + "_func";



                // Обработка блока BackItem или HistoryItem для функций
                                if (BackItem* back = dynamic_cast<BackItem*>(conn->endBlock())) {
                                    if (back->backMode() != BackItem::Back) {
                                        int targetId = block.id; // Fallback на себя
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



                bool isTrue = (cond.isEmpty() || cond == "true" || cond == "1");

                if (isTrue) {
                    if (!act.isEmpty()) cppOut << "    " << act << "\n";
                    cppOut << "    requestTransition(i, " << targetFunc << ");\n";
                    hasUncond = true;
                    break;
                } else {
                    cppOut << "    if (" << cond << ") {\n";
                    if (!act.isEmpty()) cppOut << "        " << act.replace("\n", "\n        ") << "\n";
                    cppOut << "        requestTransition(i, " << targetFunc << ");\n";
                    cppOut << "        return;\n";
                    cppOut << "    }\n";
                }
            }

            if (!hasUncond && transitions.isEmpty()) {
                 cppOut << "    requestTransition(i, nullptr); // Finish token\n";
            }
            cppOut << "}\n\n";
        }

        result.source = cppCode;

        // =========================================================
        // ФИНАЛ: Отдаем классический .ino код (switch-case) без смешивания!
        // =========================================================
        result.code = code;

        return result;
    }


// === Утилиты ===
QString MicroTranslator::sanitizeForArduino(const QString &rawCode, bool isCondition) {
    QString code = rawCode.trimmed();

    // Защита от синтаксических ошибок в метках/экшенах
    if (code.endsWith(":")) code.chop(1);
    if (code.endsWith(":;")) code.chop(2);

    // Заменяем log на Serial.println
    code.replace(QRegularExpression("log\\s*\\((.*)\\)"), "Serial.println(\\1)");
    // string -> String (для удобства Arduino)
    code.replace("std::to_string", "String");
    code.replace("std::string", "String");
    code.replace("string", "String");

    if (!isCondition && !code.isEmpty() && !code.endsWith(';') && !code.endsWith('}')) {
        code += ";";
    }
    return code;
}

QString MicroTranslator::extractVariables(const QString &rawCode, QMap<QString, VariableInfo> &vars) {
    QString code = rawCode;
    // Ищем: int x = 10; float y; String s;
    QRegularExpression varRegex(R"(\b(int|float|double|bool|String|char|long|unsigned)\s+(\w+)(\s*=[^;]+)?\s*;)");

    QRegularExpressionMatchIterator i = varRegex.globalMatch(code);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString full = match.captured(0);
        QString type = match.captured(1);
        QString name = match.captured(2);
        QString val = match.captured(3); // "= 10"

        if (!vars.contains(name)) {
            vars.insert(name, {type, name, val});
        }

        // В коде setup/loop оставляем только присваивание
        if (!val.isEmpty()) {
            code.replace(full, name + " " + val + ";");
        } else {
            code.replace(full, ""); // Просто объявление удаляем, оно ушло в глобал
        }
    }
    return code;
}
