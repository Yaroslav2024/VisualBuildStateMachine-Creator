/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "dynamicwificlasstranslator.h"
#include "diagramitem.h"
#include "connect.h"
#include "transitiongroup.h"
#include "bridgeitem.h"
#include <QTextStream>
#include <QRegularExpression>
#include <algorithm>
#include "backitem.h"
#include "groupitem.h"
#include "commentitem.h"
#include "historyitem.h"

DynamicWifiClassTranslator::DynamicWifiClassTranslator(QObject *parent) : QObject(parent) {}

ClassResult DynamicWifiClassTranslator::generate(QGraphicsScene *scene, const QString &className, const QString &manualHeader, const QString &ssid, const QString &pass)
{
    ClassResult result;
    QMap<QString, VariableInfo> classVars; // Переменные, которые станут полями класса

    // ==========================================
    // 1. СБОР БЛОКОВ
    // ==========================================
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
    int maxTokens = isMulti ? startBlocks.size() : 1;

    // ==========================================
    // 2. НУМЕРАЦИЯ И ПОДГОТОВКА ПЕРЕМЕННЫХ
    // ==========================================
    int idCounter = 1;
    QMap<DiagramItem*, int> blockIdMap;

    for (int i = 0; i < blocks.size(); ++i) {
        blocks[i].id = idCounter++;
        blockIdMap[blocks[i].item] = blocks[i].id;

        result.idMap.insert(blocks[i].id, blocks[i].item);

        QString rawEnter = sanitizeForArduino(blocks[i].item->getOnEnter(), false);
        QString rawExit = sanitizeForArduino(blocks[i].item->getOnExit(), false);

        blocks[i].cleanOnEnter = extractVariables(rawEnter, classVars);
        blocks[i].cleanOnExit = extractVariables(rawExit, classVars);
    }




    // ==========================================
    // ==========================================
            // --- АНАЛИЗ ИСТОРИИ (HISTORY) ---
            QMap<int, QString> blockToHistoryVar;
            QMap<DiagramItem*, QString> historyTargetVar;

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

        // ==========================================
        // 3. ГЕНЕРАЦИЯ .H ФАЙЛА (Заголовок)
        // ==========================================
        QString hCode;
        QTextStream hOut(&hCode);
        QString guard = className.toUpper() + "_H";

        hOut << "#ifndef " << guard << "\n";
        hOut << "#define " << guard << "\n\n";
        hOut << "#include <stdint.h>\n";
        hOut << "#if defined(ARDUINO)\n";
        hOut << "    #include <Arduino.h>\n";
        hOut << "    #include <WiFi.h>\n"; // Важно: библиотека Wi-Fi спрятана под макросом
        hOut << "#endif\n\n";

        if (!includesCode.isEmpty()) {
            hOut << "// --- User Libraries ---\n" << includesCode << "\n";
        }

        hOut << "// --- Flags ---\n";
        hOut << "#define FLAG_ENTER (1u << 0)\n";
        hOut << "#define FLAG_EXIT  (1u << 1)\n\n";

        hOut << "class " << className << " {\n";
        hOut << "public:\n";

        if (!membersCode.isEmpty()) {
            hOut << "    // --- User Variables & Methods ---\n";
            hOut << membersCode << "\n";
        }

        hOut << "    // --- Types of functions (Pointer to Class Member) ---\n";
        hOut << "    typedef void (" << className << "::*StateFunc)(int tokenIdx);\n\n";

        hOut << "    // --- FSM structure ---\n";


        hOut << "    typedef struct {\n";
                hOut << "        StateFunc     current;\n";
                hOut << "        StateFunc     nextState;\n";
                hOut << "        StateFunc     previousState;\n";
                for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                    hOut << "        StateFunc     " << it.value() << ";\n";
                }
                hOut << "        uint8_t       flags;\n";
                hOut << "        unsigned long stateStartTime;\n";
                hOut << "    } FSM;\n\n";



        hOut << "    " << className << "();\n";   // Конструктор
        hOut << "    void setup();\n";            // Настройка
        hOut << "    void loop();\n";             // Главный цикл автомата

        hOut << "\nprivate:\n";
        hOut << "    FSM activeFSMs[" << maxTokens << "];\n";
        hOut << "    int activeCount;\n";
        hOut << "    bool isRunning;\n";
        hOut << "    WiFiServer* server;\n";
        hOut << "    WiFiClient client;\n\n";

        hOut << "    // Core FSM API\n";
        hOut << "    void FSM_Init();\n";
        hOut << "    void FSM_Tick();\n";
        hOut << "    void requestTransition(int i, StateFunc target);\n";
        hOut << "    void dbg_sig(char type, int id);\n\n";

        hOut << "    // Variables extracted from blocks\n";
        for (auto it = classVars.begin(); it != classVars.end(); ++it) {
             hOut << "    " << it.value().type << " " << it.value().name << ";\n";
        }

        hOut << "\n    // State Prototypes\n";
        for (const auto& block : blocks) {
            if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;
            hOut << "    void state_" << block.id << "_func(int i);\n";
        }

        hOut << "};\n\n";
        hOut << "#endif\n";
        result.headerCode = hCode;

        // ==========================================
        // 4. ГЕНЕРАЦИЯ .CPP ФАЙЛА (Реализация логики)
        // ==========================================
        QString cppCode;
        QTextStream cppOut(&cppCode);

        cppOut << "#include \"" << className << ".h\"\n\n";

        cppOut << "const char* WIFI_SSID = \"" << ssid << "\";\n";
        cppOut << "const char* WIFI_PASS = \"" << pass << "\";\n\n";

        // --- Конструктор ---
        cppOut << className << "::" << className << "() {\n";
        cppOut << "    server = new WiFiServer(8080);\n";
        cppOut << "    activeCount = " << maxTokens << ";\n";
        cppOut << "    isRunning = true;\n";

        // Инициализация переменных значениями по умолчанию
        for (auto it = classVars.begin(); it != classVars.end(); ++it) {
            QString init = it.value().initValue;
            if (!init.isEmpty()) {
                if(init.trimmed().startsWith("=")) init = init.trimmed().mid(1);
                cppOut << "    " << it.value().name << " = " << init << ";\n";
            }
        }
        cppOut << "}\n\n";

        // --- Setup ---
        cppOut << "void " << className << "::setup() {\n";
        cppOut << "    Serial.begin(115200);\n";
        cppOut << "    WiFi.begin(WIFI_SSID, WIFI_PASS);\n";
        cppOut << "    while (WiFi.status() != WL_CONNECTED) delay(500);\n";
        cppOut << "    server->begin();\n";
        cppOut << "    \n";
        cppOut << "    // Wait for TCP client\n";
        cppOut << "    while (!client || !client.connected()) {\n";
        cppOut << "        client = server->available();\n";
        cppOut << "        delay(10);\n";
        cppOut << "    }\n";
        cppOut << "    FSM_Init();\n";
        cppOut << "}\n\n";

        // --- Протокол Дебаггера (По воздуху) ---
        cppOut << "void " << className << "::dbg_sig(char type, int id) {\n";
        cppOut << "    if (client && client.connected()) {\n";
        cppOut << "        client.print(\"[\"); client.print(type); client.print(\":\");\n";
        cppOut << "        client.print(id); client.println(\"]\");\n";
        cppOut << "    }\n";
        cppOut << "}\n\n";

        // --- CORE FSM LOGIC ---
        cppOut << "void " << className << "::loop() {\n";
        cppOut << "    if (isRunning) FSM_Tick();\n";
        cppOut << "}\n\n";

        cppOut << "void " << className << "::requestTransition(int i, StateFunc target) {\n";
        cppOut << "    activeFSMs[i].nextState = target;\n";
        cppOut << "    activeFSMs[i].flags |= FLAG_EXIT;\n";
        cppOut << "}\n\n";

        cppOut << "void " << className << "::FSM_Init() {\n";
                for (int i = 0; i < maxTokens; ++i) {
                    cppOut << "    activeFSMs[" << i << "].current = &" << className << "::state_" << blockIdMap[isMulti ? startBlocks[i] : startBlocks.first()] << "_func;\n";
                    cppOut << "    activeFSMs[" << i << "].flags = FLAG_ENTER;\n";
                    for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                        cppOut << "    activeFSMs[" << i << "]." << it.value() << " = nullptr;\n";
                    }
                }
                cppOut << "}\n\n";

        cppOut << "void " << className << "::FSM_Tick() {\n";
        cppOut << "    for(int i = 0; i < activeCount; i++) {\n";
        cppOut << "        if (activeFSMs[i].current == nullptr) continue;\n\n";
        cppOut << "        if (activeFSMs[i].flags & FLAG_EXIT) {\n";
        cppOut << "            if (activeFSMs[i].current != activeFSMs[i].nextState) {\n";
        cppOut << "            (this->*(activeFSMs[i].current))(i);\n";
        cppOut << "            }\n";
        cppOut << "            activeFSMs[i].previousState = activeFSMs[i].current;\n";
        cppOut << "            activeFSMs[i].current = activeFSMs[i].nextState;\n";
        cppOut << "            if (activeFSMs[i].current != nullptr) activeFSMs[i].flags = FLAG_ENTER;\n";
        cppOut << "            else activeFSMs[i].flags = 0;\n";
        cppOut << "        } else {\n";
        cppOut << "            (this->*(activeFSMs[i].current))(i);\n";
        cppOut << "        }\n";
        cppOut << "    }\n";
        cppOut << "}\n\n";

        int arrowIdCounter = 1000;

        for (const auto& block : blocks) {
            if (dynamic_cast<BackItem*>(block.item) || dynamic_cast<HistoryItem*>(block.item)) continue;

            cppOut << "// ==========================================\n";
            cppOut << "// State ID: " << block.id << "\n";
            cppOut << "void " << className << "::state_" << block.id << "_func(int i) {\n";


            // --- ON ENTER ---
                        cppOut << "    if (activeFSMs[i].flags & FLAG_ENTER) {\n";
                        if (blockToHistoryVar.contains(block.id)) {
                            cppOut << "        activeFSMs[i]." << blockToHistoryVar[block.id] << " = &" << className << "::state_" << block.id << "_func; // Update History\n";
                        }


            if (block.item->data(ROLE_DEBUG_ENABLED).toBool()) {
                cppOut << "        dbg_sig('B', " << block.id << ");\n";
                cppOut << "        delay(100);\n";
            }
            QString comments = CommentItem::getAssociatedCode(block.item, "// ");
            if (!comments.isEmpty()) cppOut << "        " << comments.replace("\n", "\n        ");
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
                    cppOut << "    // Bridge jump to: " << targetName << "\n";
                    if (block.item->data(ROLE_DEBUG_ENABLED).toBool()) cppOut << "    dbg_sig('C', " << block.id << ");\n";
                    cppOut << "    requestTransition(i, &" << className << "::state_" << targetId << "_func);\n";
                } else {
                    cppOut << "    if (client && client.connected()) client.println(\"Error: Bridge target not found!\");\n";
                    cppOut << "    requestTransition(i, nullptr);\n";
                }
                cppOut << "}\n\n";
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

                QString cond = sanitizeForArduino(conn->transition()->conditionText(), true).trimmed();
                if (cond.toLower() == "if") cond = "";
                if (cond.toLower().startsWith("if ")) cond = cond.mid(3).trimmed();

                QString actRaw = conn->transition()->actionText().trimmed();
                QString act = (actRaw.toLower() == "action") ? "" : extractVariables(sanitizeForArduino(actRaw, false), classVars);

                QString targetFunc = "&" + className + "::state_" + QString::number(blockIdMap[conn->endBlock()]) + "_func";


                if (BackItem* back = dynamic_cast<BackItem*>(conn->endBlock())) {
                                    if (back->backMode() != BackItem::Back) {
                                        int targetId = block.id; // Fallback
                                        QString tName = back->targetName();
                                        for (auto it = blockIdMap.begin(); it != blockIdMap.end(); ++it) {
                                            if (it.key()->getLabel() == tName || it.key()->getDescription() == tName) {
                                                targetId = it.value(); break;
                                            }
                                        }
                                        targetFunc = "&" + className + "::state_" + QString::number(targetId) + "_func";
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
                                            targetFunc = "(activeFSMs[i]." + varName + " != nullptr) ? activeFSMs[i]." + varName + " : &" + className + "::state_" + QString::number(defaultTargetId) + "_func";
                                        } else {
                                            targetFunc = "activeFSMs[i]." + varName;
                                        }
                                    }
                                }



                bool isTrue = (cond.isEmpty() || cond == "true" || cond == "1");

                if (isTrue) {
                    if (conn->data(ROLE_DEBUG_ENABLED).toBool()) cppOut << "    dbg_sig('T', " << arrowId << ");\n";
                    if (!act.isEmpty()) cppOut << "    " << act << "\n";
                    cppOut << "    requestTransition(i, " << targetFunc << ");\n";
                    cppOut << "    return;\n";
                    hasUncond = true;
                    break;
                } else {
                    cppOut << "    if (" << cond << ") {\n";
                    if (conn->data(ROLE_DEBUG_ENABLED).toBool()) cppOut << "        dbg_sig('T', " << arrowId << ");\n";
                    if (!act.isEmpty()) cppOut << "        " << act.replace("\n", "\n        ") << "\n";
                    cppOut << "        requestTransition(i, " << targetFunc << ");\n";
                    cppOut << "        return;\n";
                    cppOut << "    } else {\n";
                    if (conn->data(ROLE_DEBUG_ENABLED).toBool()) cppOut << "        dbg_sig('F', " << arrowId << ");\n";
                    cppOut << "    }\n";
                }
            }

            if (!hasUncond && transitions.isEmpty()) {
                 cppOut << "    if (client && client.connected()) client.println(\"Finished.\");\n";
                 cppOut << "    requestTransition(i, nullptr);\n";
            }
            cppOut << "}\n\n";
        }

        result.sourceCode = cppCode;

        // ==========================================
        // 5. ГЕНЕРАЦИЯ .INO ФАЙЛА (Точка входа)
        // ==========================================
        QString mainCode;
        QTextStream mainOut(&mainCode);

        mainOut << "// --- VisioLite Dynamic Debugger (Wi-Fi Mode) ---\n";
        mainOut << "// Этот файл просто запускает логику из " << className << ".cpp\n\n";
        mainOut << "#include \"" << className << ".h\"\n\n";
        mainOut << className << " logic;\n\n";
        mainOut << "void setup() {\n";
        mainOut << "  // Вся настройка внутри класса\n";
        mainOut << "  logic.setup();\n";
        mainOut << "}\n\n";
        mainOut << "void loop() {\n";
        mainOut << "  // Запуск шага автомата\n";
        mainOut << "  logic.loop();\n";
        mainOut << "}\n";

        result.mainCode = mainCode;

        return result;



}

// === Утилиты (с обновлениями) ===
QString DynamicWifiClassTranslator::sanitizeForArduino(const QString &rawCode, bool isCondition) {
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

QString DynamicWifiClassTranslator::extractVariables(const QString &rawCode, QMap<QString, VariableInfo> &vars) {
    QString code = rawCode;
    QRegularExpression varRegex(R"(\b(int|float|double|bool|String|char|long|unsigned)\s+(\w+)(\s*=[^;]+)?\s*;)");

    QRegularExpressionMatchIterator i = varRegex.globalMatch(code);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString full = match.captured(0);
        QString type = match.captured(1);
        QString name = match.captured(2);
        QString val = match.captured(3);

        if (!vars.contains(name)) {
            vars.insert(name, {type, name, val});
        }

        if (!val.isEmpty()) {
            code.replace(full, name + " " + val + ";");
        } else {
            code.replace(full, "");
        }
    }
    return code;
}
