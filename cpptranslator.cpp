/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "cpptranslator.h"
#include "diagramitem.h"
#include "connect.h"
#include "transitiongroup.h"
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>
#include <QTemporaryFile>
#include <algorithm> // Для std::sort
#include "commentitem.h"
#include "bridgeitem.h"
#include "backitem.h"
#include "historyitem.h"
#include "groupitem.h"
#include <QMessageBox>
#include <QSettings>

CppTranslator::CppTranslator(QObject *parent) : QObject(parent) {}

GeneratedCode CppTranslator::generateCodePair(QGraphicsScene *scene, const QString &manualHeader, const QString &headerFileName)
{
    GeneratedCode result;
    m_autoVariables.clear();
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
                QObject::tr("Внимание: Опасные операции C++"),
                QObject::tr("Умный сканер обнаружил в вашей схеме потенциально опасный код\n"
                "(деление, массивы, указатели или ручное выделение памяти).\n\n"
                "Грубые ошибки в таких операциях (например, деление на ноль) "
                "могут привести к аппаратному вылету редактора.\n\n"
                "Рекомендуется сохранить проект. Хотите продолжить?"),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::No) {
                result.error = QObject::tr("Генерация отменена пользователем.");
                return result;
            }
        }
    }
    // ==========================================

    // --- ЭТАП 1: Сбор и обработка ---
    struct BlockData {
        DiagramItem* item;
        int id;
        QString cleanOnEnter;
        QString cleanOnExit;
    };
    QList<BlockData> processedBlocks;

    QList<DiagramItem*> rawBlocks;
    QList<DiagramItem*> startBlocks;

    for (QGraphicsItem *item : scene->items()) {
        if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
            if (b->getType() == "Стартовый блок" || b->getType() == "Start Block") startBlocks.append(b);
            else rawBlocks.append(b);
        }
    }

    if (startBlocks.isEmpty()) {
        result.error = QObject::tr("ERROR: Нет стартового блока!");
        return result;
    }
    for (DiagramItem* sb : startBlocks) {
        rawBlocks.prepend(sb);
    }

    bool isMulti = (startBlocks.size() > 1);

    int idCounter = 1;
    QMap<DiagramItem*, int> blockIds;

    for (DiagramItem* b : rawBlocks) {
        blockIds[b] = idCounter;

        QString rawEnter = sanitizeCppCode(b->getOnEnter(), false);
        QString rawExit = sanitizeCppCode(b->getOnExit(), false);

        QString err;
        if (hasPythonSyntax(rawEnter, err)) { result.error = QObject::tr("Python code found: ") + err; return result; }
        if (hasPythonSyntax(rawExit, err)) { result.error = QObject::tr("Python code found: ") + err; return result; }

        QString cleanEnter = extractAndRemoveTypes(rawEnter);
        QString cleanExit = extractAndRemoveTypes(rawExit);

        processedBlocks.append({b, idCounter, cleanEnter, cleanExit});
        idCounter++;
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
                            defaultTargetId = blockIds[conn->endBlock()];
                            break;
                        }
                    }
                }

                if (isMulti) {
                                    historyDeclarations += "    std::vector<int> " + varName + " = std::vector<int>(" + QString::number(startBlocks.size()) + ", " + QString::number(defaultTargetId) + ");\n";
                                } else {
                                    historyDeclarations += "    int " + varName + " = " + QString::number(defaultTargetId) + ";\n";
                                }

                for (DiagramItem* b : blockIds.keys()) {
                    if (g->sceneBoundingRect().contains(b->sceneBoundingRect().center()) && b != hist) {
                        blockToHistoryVar[blockIds[b]] = varName;
                    }
                }
            }
        }
    }

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

        // --- ЭТАП 2: Генерация HEADER (.h) ---
        QString hCode;
        QTextStream outH(&hCode);

        QString guardName = headerFileName.toUpper();
        guardName.replace(QRegularExpression("[^A-Z0-9]"), "_");
        if (!guardName.isEmpty() && guardName.at(0).isDigit()) guardName.prepend("GUARD_");

        outH << "#ifndef " << guardName << "\n";
        outH << "#define " << guardName << "\n\n";
        outH << "#include <vector>\n\n";

        if (!includesCode.trimmed().isEmpty()) {
            outH << "// --- USER HEADERS ---\n";
            outH << includesCode << "\n";
            outH << "// --------------------\n\n";
        }

        outH << "class StateMachine {\n";
        outH << "public:\n";
        if (!membersCode.isEmpty()) {
            outH << "    // --- User Variables & Methods ---\n";
            outH << membersCode << "\n";
        }

        outH << "private:\n";

        if (!m_autoVariables.isEmpty()) {
            outH << "    // Auto-Extracted Variables\n";
            for (auto it = m_autoVariables.begin(); it != m_autoVariables.end(); ++it) {
                 if (it.value().type == "int" || it.value().type == "double")
                    outH << "    " << it.value().type << " " << it.value().name << " = 0;\n";
                else if (it.value().type == "bool")
                    outH << "    " << it.value().type << " " << it.value().name << " = false;\n";
                else
                    outH << "    " << it.value().type << " " << it.value().name << ";\n";
            }
        }

        outH << historyDeclarations;

    // --- ПРОФЕССИОНАЛЬНАЯ АРХИТЕКТУРА ---
    if (isMulti) {
        outH << "    std::vector<int> active_states;\n";
        outH << "    std::vector<int> previous_states;\n";
        outH << "    std::vector<bool> state_changed;\n";
    } else {
        outH << "    int currentState;\n";
        outH << "    int previousState;\n";
        outH << "    bool stateChanged;\n";
    }
    outH << "    bool running;\n\n";

    outH << "public:\n";
    outH << "    StateMachine();\n";
    outH << "    void run();\n";
    outH << "};\n\n";
    outH << "#endif // " << guardName << "\n";
    outH.flush();
    result.headerContent = hCode;

    // --- ЭТАП 3: Генерация SOURCE (.cpp) ---
    QString cppCode;
    QTextStream outCpp(&cppCode);

    outCpp << "#include \"" << headerFileName << "\"\n";
    outCpp << "#include <iostream>\n";
    outCpp << "#include <string>\n";
    outCpp << "#include <cmath>\n\n";

    outCpp << "// Helpers\n";
    outCpp << "void log(std::string msg) { std::cout << msg << std::endl; }\n";
    outCpp << "void send(std::string msg) { std::cout << \"[SEND] \" << msg << std::endl; }\n\n";

    // --- КОНСТРУКТОР ---
    if (isMulti) {
        outCpp << "StateMachine::StateMachine() : running(true) {\n";
        for (DiagramItem* sb : startBlocks) {
            outCpp << "    active_states.push_back(" << blockIds[sb] << ");\n";
            outCpp << "    previous_states.push_back(-1);\n";
            outCpp << "    state_changed.push_back(true);\n";
        }
        outCpp << "}\n\n";
    } else {
        outCpp << "StateMachine::StateMachine() : currentState(" << blockIds[startBlocks.first()] << "), previousState(-1), stateChanged(true), running(true) {}\n\n";
    }

    // --- РЕАЛИЗАЦИЯ RUN ---
    outCpp << "void StateMachine::run() {\n";
    if (isMulti) {
        outCpp << "    while (running && !active_states.empty()) {\n";
        outCpp << "        for (size_t i = 0; i < active_states.size(); ) {\n";
        outCpp << "            int currentState = active_states[i];\n";
        outCpp << "            int nextState = currentState;\n";
        outCpp << "            bool token_finished = false;\n\n";
        outCpp << "            bool transition_taken = false;\n\n";

        outCpp << "            // --- ON ENTER PHASE ---\n";
        outCpp << "            if (state_changed[i]) {\n";
    } else {
        outCpp << "    while (running) {\n";
        outCpp << "        int nextState = currentState;\n\n";
        outCpp << "        bool transition_taken = false;\n\n";
        outCpp << "        // --- ON ENTER PHASE ---\n";
        outCpp << "        if (stateChanged) {\n";
    }

    QString ind1 = isMulti ? "            " : "        ";
    QString ind2 = isMulti ? "                " : "            ";

    outCpp << ind1 << "    switch (currentState) {\n";
    for (const auto& pb : processedBlocks) {
        if (dynamic_cast<BackItem*>(pb.item) || dynamic_cast<HistoryItem*>(pb.item)) continue;

        bool hasCode = !pb.cleanOnEnter.isEmpty() || blockToHistoryVar.contains(pb.id);
        QString comments = CommentItem::getAssociatedCode(pb.item, "// ");
        if (hasCode || !comments.isEmpty()) {
            outCpp << ind1 << "        case " << pb.id << ": {\n";

            if (blockToHistoryVar.contains(pb.id)) {
                            outCpp << ind2 << "    " << blockToHistoryVar[pb.id] << (isMulti ? "[i]" : "") << " = " << pb.id << "; // Update History\n";
                        }

            if (!comments.isEmpty()) outCpp << ind2 << "    " << comments.replace("\n", "\n" + ind2 + "    ");
            if (!pb.cleanOnEnter.isEmpty()) outCpp << ind2 << "    " << pb.cleanOnEnter << "\n";
            outCpp << ind1 << "        } break;\n";
        }
    }
    outCpp << ind1 << "    }\n";

    if (isMulti) outCpp << ind1 << "    state_changed[i] = false;\n" << ind1 << "}\n\n";
    else outCpp << ind1 << "    stateChanged = false;\n" << ind1 << "}\n\n";

    outCpp << ind1 << "// --- TRANSITIONS PHASE ---\n";
    outCpp << ind1 << "switch (currentState) {\n";

    for (const auto& pb : processedBlocks) {
        if (dynamic_cast<BackItem*>(pb.item) || dynamic_cast<HistoryItem*>(pb.item)) continue;
        outCpp << ind1 << "    case " << pb.id << ": {\n";

        if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(pb.item)) {
            QString targetName = bridge->targetName();
            int targetId = -1;
            for (auto it = blockIds.begin(); it != blockIds.end(); ++it) {
                if (it.key()->getLabel() == targetName) { targetId = it.value(); break; }
            }        
            if (targetId != -1) {
                            outCpp << ind2 << "nextState = " << targetId << ";\n";
                            outCpp << ind2 << "transition_taken = true;\n";
                        }
            else outCpp << ind2 << (isMulti ? "token_finished = true;\n" : "running = false;\n");          
            outCpp << ind1 << "    } break;\n";
            continue;
        }

        QList<ConnectionItem*> transitions;
        for (QGraphicsItem *item : scene->items()) {
             if (item->data(ConnectionRole).toBool()) {
                 ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item);
                 if (conn && conn->startBlock() == pb.item && conn->endBlock()) transitions.append(conn);
             }
        }
        std::sort(transitions.begin(), transitions.end(), [](ConnectionItem *a, ConnectionItem *b) {
            int p1 = (a->transition()) ? a->transition()->getPriority() : 999;
            int p2 = (b->transition()) ? b->transition()->getPriority() : 999;
            return p1 < p2;
        });

        bool hasUnconditional = false;
        for (ConnectionItem *conn : transitions) {
            QString arrowComm = CommentItem::getAssociatedCode(conn, "// ");
            if (!arrowComm.isEmpty()) outCpp << ind2 << arrowComm.replace("\n", "\n" + ind2);

            QString cleanAct = extractAndRemoveTypes(sanitizeCppCode(conn->transition()->actionText(), false));
            QString cleanCond = sanitizeCppCode(conn->transition()->conditionText(), true).trimmed();
            if (cleanCond.toLower() == "if") cleanCond = "";
            if (cleanCond.toLower().startsWith("if ")) cleanCond = cleanCond.mid(3).trimmed();

            QString targetStr = QString::number(blockIds[conn->endBlock()]);
                        if (BackItem* back = dynamic_cast<BackItem*>(conn->endBlock())) {
                            if (back->backMode() != BackItem::Back) {
                                int targetId = pb.id; // Fallback
                                QString tName = back->targetName();
                                for (auto it = blockIds.begin(); it != blockIds.end(); ++it) {
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
                                            defaultTargetId = blockIds[c->endBlock()]; break;
                                        }
                                    }
                                }
                                targetStr = QString::number(defaultTargetId);
                            }
                        }

            if (cleanCond.isEmpty() || cleanCond == "true" || cleanCond == "1") {
                if (!cleanAct.isEmpty()) outCpp << ind2 << cleanAct << "\n";
                outCpp << ind2 << "nextState = " << targetStr << ";\n";
                outCpp << ind2 << "transition_taken = true;\n";
                hasUnconditional = true;
                break;
            } else {
                outCpp << ind2 << "if (" << cleanCond << ") {\n";
                if (!cleanAct.isEmpty()) outCpp << ind2 << "    " << cleanAct << "\n";
                outCpp << ind2 << "    nextState = " << targetStr << ";\n";
                outCpp << ind2 << "    transition_taken = true;\n";
                outCpp << ind2 << "    break;\n";
                outCpp << ind2 << "}\n";
            }
        }

        if (!hasUnconditional && transitions.isEmpty()) {
             outCpp << ind2 << (isMulti ? "token_finished = true;\n" : "running = false;\n");
        }
        outCpp << ind1 << "    } break;\n";
    }

    outCpp << ind1 << "    default: " << (isMulti ? "token_finished = true;" : "running = false;") << " break;\n";
    outCpp << ind1 << "}\n\n";

    // --- ON EXIT & UPDATE PHASE ---
    if (isMulti) {        
        outCpp << "            // --- ON EXIT & UPDATE PHASE ---\n";
                outCpp << "            if (token_finished) {\n";
                outCpp << "                active_states.erase(active_states.begin() + i);\n";
                outCpp << "                previous_states.erase(previous_states.begin() + i);\n";
                outCpp << "                state_changed.erase(state_changed.begin() + i);\n";
                for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                    outCpp << "                " << it.value() << ".erase(" << it.value() << ".begin() + i);\n";
                }
                outCpp << "            } else {\n";
        outCpp << "                if (transition_taken) {\n";
        outCpp << "                if (nextState != currentState) {\n";
        outCpp << "                    switch (currentState) {\n";
    } else {
        outCpp << "        // --- ON EXIT & UPDATE PHASE ---\n";
        outCpp << "        if (transition_taken) {\n";
        outCpp << "        if (nextState != currentState) {\n";       
        outCpp << "            switch (currentState) {\n";
    }

    for (const auto& pb : processedBlocks) {
        if (!pb.cleanOnExit.isEmpty()) {
            outCpp << ind2 << "case " << pb.id << ": {\n";
            outCpp << ind2 << "    " << pb.cleanOnExit << "\n";
            outCpp << ind2 << "} break;\n";
        }
    }

    if (isMulti) {
        outCpp << "                        }\n";
        outCpp << "                    }\n";
        outCpp << "                    previous_states[i] = currentState;\n";
        outCpp << "                    active_states[i] = nextState;\n";
        outCpp << "                    state_changed[i] = true;\n";
        outCpp << "                }\n";
        outCpp << "                i++;\n";
        outCpp << "            }\n";
        outCpp << "        }\n";
    } else {
        outCpp << "                }\n";
        outCpp << "            }\n";
        outCpp << "            previousState = currentState;\n";
        outCpp << "            currentState = nextState;\n";
        outCpp << "            stateChanged = true;\n";
        outCpp << "        }\n";
    }

    outCpp << "    }\n";
    outCpp << "}\n\n";

    // Main для теста
    outCpp << "int main() {\n";
    outCpp << "    StateMachine sm;\n";
    outCpp << "    sm.run();\n";
    outCpp << "    return 0;\n";
    outCpp << "}\n";

    outCpp.flush();
    result.sourceContent = cppCode;

    // --- Сквозная Валидация ---
    QString validationCode = result.headerContent + "\n\n";
    QString cleanSource = result.sourceContent;
    QString includeLine = "#include \"" + headerFileName + "\"";
    cleanSource.replace(includeLine, "// [Validation] Skip include");

    validationCode += cleanSource;
    QString checkMsg = invokeCompiler(validationCode);

    if (checkMsg.startsWith("ОШИБКА") || checkMsg.startsWith("ERROR")) {
        result.error = QObject::tr("Ошибка в коде (возможно, в Header):\n") + checkMsg;
    }
    return result;
}

// ===================================================================
// === ДУБЛИРУЮЩАЯ ФУНКЦИЯ (Генерирует один файл) ===
// ===================================================================
QString CppTranslator::generateSourceCode(QGraphicsScene *scene, const QString &manualHeader)
{
    m_autoVariables.clear();
    bool hasDangerousCode = false;
    QRegularExpression dangerRegex("(\\[|\\]|\\->|\\bnew\\b|\\bdelete\\b|\\bmalloc\\b|\\bfree\\b|\\/)");

    for (QGraphicsItem *item : scene->items()) {
        if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
            if (dangerRegex.match(b->getOnEnter()).hasMatch() || dangerRegex.match(b->getOnExit()).hasMatch()) { hasDangerousCode = true; break; }
        } else if (item->data(ConnectionRole).toBool()) {
            if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
                if (conn->transition()) {
                    if (dangerRegex.match(conn->transition()->conditionText()).hasMatch() || dangerRegex.match(conn->transition()->actionText()).hasMatch()) { hasDangerousCode = true; break; }
                }
            }
        }
    }

    if (hasDangerousCode) {
        QSettings settings("VisualBuild", "IDE");
        if (!settings.value("DisableCrashWarnings", false).toBool()) {
            QMessageBox::StandardButton reply = QMessageBox::warning(nullptr, QObject::tr("Внимание: Опасные операции C++"), QObject::tr("Рекомендуется сохранить проект. Хотите продолжить?"), QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No) return QObject::tr("ERROR: Генерация отменена пользователем.");
        }
    }

    struct BlockData {
        DiagramItem* item; int id; QString cleanOnEnter; QString cleanOnExit;
    };
    QList<BlockData> processedBlocks;
    QList<DiagramItem*> rawBlocks, startBlocks;

    for (QGraphicsItem *item : scene->items()) {
        if (DiagramItem *b = dynamic_cast<DiagramItem*>(item)) {
            if (b->getType() == "Стартовый блок" || b->getType() == "Start Block") startBlocks.append(b);
            else rawBlocks.append(b);
        }
    }

    if (startBlocks.isEmpty()) return QObject::tr("ERROR: Нет стартового блока!");
    for (DiagramItem* sb : startBlocks) rawBlocks.prepend(sb);

    bool isMulti = (startBlocks.size() > 1);
    QMap<DiagramItem*, int> blockIds;
    int idCounter = 1;

    for (DiagramItem* b : rawBlocks) {
        blockIds[b] = idCounter;
        QString rawEnter = sanitizeCppCode(b->getOnEnter(), false);
        QString rawExit = sanitizeCppCode(b->getOnExit(), false);
        QString err;
        if (hasPythonSyntax(rawEnter, err)) return QObject::tr("ERROR: Python в onEnter: ") + err;
        if (hasPythonSyntax(rawExit, err)) return QObject::tr("ERROR: Python в onExit: ") + err;
        processedBlocks.append({b, idCounter, extractAndRemoveTypes(rawEnter), extractAndRemoveTypes(rawExit)});
        idCounter++;
    }

    QMap<int, QString> blockToHistoryVar;
    QMap<DiagramItem*, QString> historyTargetVar;
    QString historyDeclarations;

    for (QGraphicsItem *item : scene->items()) {
        if (GroupItem *g = dynamic_cast<GroupItem*>(item)) {
            HistoryItem* hist = nullptr;
            for (QGraphicsItem* child : scene->items()) {
                if (HistoryItem* h = dynamic_cast<HistoryItem*>(child)) {
                    if (g->sceneBoundingRect().contains(h->sceneBoundingRect().center())) { hist = h; break; }
                }
            }
            if (hist) {
                QString varName = "hist_group_" + QString::number(hist->id());
                historyTargetVar[hist] = varName;
                int defaultTargetId = 0;
                for (QGraphicsItem *connItem : scene->items()) {
                    if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(connItem)) {
                        if (conn->startBlock() == hist && conn->endBlock() && !conn->isBlocked()) { defaultTargetId = blockIds[conn->endBlock()]; break; }
                    }
                }

                if (isMulti) {
                                    historyDeclarations += "    std::vector<int> " + varName + " = std::vector<int>(" + QString::number(startBlocks.size()) + ", " + QString::number(defaultTargetId) + ");\n";
                                } else {
                                    historyDeclarations += "    int " + varName + " = " + QString::number(defaultTargetId) + ";\n";
                                }

                for (DiagramItem* b : blockIds.keys()) {
                    if (g->sceneBoundingRect().contains(b->sceneBoundingRect().center()) && b != hist) blockToHistoryVar[blockIds[b]] = varName;
                }
            }
        }
    }



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

        QString code; QTextStream out(&code);
        out << "#include <iostream>\n#include <string>\n#include <cmath>\n#include <vector>\n\n";
        if (!includesCode.trimmed().isEmpty()) out << "// --- USER HEADER TABLE ---\n" << includesCode << "\n// -------------------------\n\n";
        out << "// Helpers\nvoid log(std::string msg) { std::cout << msg << std::endl; }\nvoid send(std::string msg) { std::cout << \"[SEND] \" << msg << std::endl; }\n\n";

        out << "class StateMachine {\npublic:\n";
        if (!membersCode.isEmpty()) {
            out << "    // --- User Variables & Methods ---\n";
            out << membersCode << "\n";
        }
        out << "private:\n";
        if (!m_autoVariables.isEmpty()) {
            for (auto it = m_autoVariables.begin(); it != m_autoVariables.end(); ++it) {
                if (it.value().type == "int" || it.value().type == "double") out << "    " << it.value().type << " " << it.value().name << " = 0;\n";
                else if (it.value().type == "bool") out << "    " << it.value().type << " " << it.value().name << " = false;\n";
                else out << "    " << it.value().type << " " << it.value().name << ";\n";
            }
        }
        out << historyDeclarations;


    if (isMulti) out << "    std::vector<int> active_states;\n    std::vector<int> previous_states;\n    std::vector<bool> state_changed;\n";
    else out << "    int currentState;\n    int previousState;\n    bool stateChanged;\n";
    out << "    bool running;\n\npublic:\n";

    if (isMulti) {
        out << "    StateMachine() : running(true) {\n";
        for (DiagramItem* sb : startBlocks) out << "        active_states.push_back(" << blockIds[sb] << "); previous_states.push_back(-1); state_changed.push_back(true);\n";
        out << "    }\n\n";
    } else {
        out << "    StateMachine() : currentState(" << blockIds[startBlocks.first()] << "), previousState(-1), stateChanged(true), running(true) {}\n\n";
    }

    out << "    void run() {\n";
    if (isMulti) out << "        while (running && !active_states.empty()) {\n            for (size_t i = 0; i < active_states.size(); ) {\n                int currentState = active_states[i];\n                int nextState = currentState;\n                bool token_finished = false;\n                bool transition_taken = false;\n                if (state_changed[i]) {\n";
        else out << "        while (running) {\n            int nextState = currentState;\n            bool transition_taken = false;\n            if (stateChanged) {\n";

    QString ind1 = isMulti ? "                " : "            ";
    QString ind2 = isMulti ? "                    " : "                ";

    out << ind1 << "switch (currentState) {\n";
    for (const auto& pb : processedBlocks) {
        if (dynamic_cast<BackItem*>(pb.item) || dynamic_cast<HistoryItem*>(pb.item)) continue;
        if (!pb.cleanOnEnter.isEmpty() || blockToHistoryVar.contains(pb.id) || !CommentItem::getAssociatedCode(pb.item, "// ").isEmpty()) {
            out << ind1 << "case " << pb.id << ": {\n";

            if (blockToHistoryVar.contains(pb.id)) out << ind2 << blockToHistoryVar[pb.id] << (isMulti ? "[i]" : "") << " = " << pb.id << ";\n";

            out << ind2 << CommentItem::getAssociatedCode(pb.item, "// ").replace("\n", "\n" + ind2);
            if (!pb.cleanOnEnter.isEmpty()) out << ind2 << pb.cleanOnEnter << "\n";
            out << ind1 << "} break;\n";
        }
    }
    out << ind1 << "}\n" << ind1 << (isMulti ? "state_changed[i] = false;\n            }\n" : "stateChanged = false;\n        }\n");

    out << ind1 << "switch (currentState) {\n";
    for (const auto& pb : processedBlocks) {
        if (dynamic_cast<BackItem*>(pb.item) || dynamic_cast<HistoryItem*>(pb.item)) continue;
        out << ind1 << "case " << pb.id << ": {\n";

        if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(pb.item)) {
            int targetId = -1;
            for (auto it = blockIds.begin(); it != blockIds.end(); ++it) if (it.key()->getLabel() == bridge->targetName()) { targetId = it.value(); break; }         
            if (targetId != -1) {
                            out << ind2 << "nextState = " << targetId << ";\n";
                            out << ind2 << "transition_taken = true;\n";
                        }
            else out << ind2 << (isMulti ? "token_finished = true;\n" : "running = false;\n");
            out << ind1 << "} break;\n"; continue;
        }

        QList<ConnectionItem*> transitions;
        for (QGraphicsItem *item : scene->items()) if (item->data(ConnectionRole).toBool()) { ConnectionItem *c = dynamic_cast<ConnectionItem*>(item); if (c && c->startBlock() == pb.item && c->endBlock()) transitions.append(c); }
        std::sort(transitions.begin(), transitions.end(), [](ConnectionItem *a, ConnectionItem *b) { return (a->transition() ? a->transition()->getPriority() : 999) < (b->transition() ? b->transition()->getPriority() : 999); });

        bool hasUnc = false;
        for (ConnectionItem *conn : transitions) {
            QString cAct = extractAndRemoveTypes(sanitizeCppCode(conn->transition()->actionText(), false));
            QString cCond = sanitizeCppCode(conn->transition()->conditionText(), true).trimmed();
            if (cCond.toLower().startsWith("if ")) cCond = cCond.mid(3).trimmed();



            QString tStr = QString::number(blockIds[conn->endBlock()]);
                        if (BackItem* back = dynamic_cast<BackItem*>(conn->endBlock())) {
                            if (back->backMode() != BackItem::Back) {
                                int targetId = pb.id; // Fallback
                                QString tName = back->targetName();
                                for (auto it = blockIds.begin(); it != blockIds.end(); ++it) {
                                    if (it.key()->getLabel() == tName || it.key()->getDescription() == tName) {
                                        targetId = it.value(); break;
                                    }
                                }
                                tStr = QString::number(targetId);
                            } else {
                                tStr = isMulti ? "(previous_states[i] != -1 ? previous_states[i] : currentState)" : "(previousState != -1 ? previousState : currentState)";
                            }
                        } else if (HistoryItem* hist = dynamic_cast<HistoryItem*>(conn->endBlock())) {
                            if (historyTargetVar.contains(hist)) {
                                tStr = historyTargetVar[hist] + (isMulti ? "[i]" : "");
                            } else {
                                int defaultTargetId = 0;
                                for (QGraphicsItem *ci : scene->items()) {
                                    if (ConnectionItem *c = dynamic_cast<ConnectionItem*>(ci)) {
                                        if (c->startBlock() == hist && c->endBlock() && !c->isBlocked()) {
                                            defaultTargetId = blockIds[c->endBlock()]; break;
                                        }
                                    }
                                }
                                tStr = QString::number(defaultTargetId);
                            }
                        }





                        if (cCond.isEmpty() || cCond == "true" || cCond == "1") {
                                        if (!cAct.isEmpty()) out << ind2 << cAct << "\n";
                                        out << ind2 << "nextState = " << tStr << ";\n"; out << ind2 << "transition_taken = true;\n"; hasUnc = true; break;
                                    } else {
                                        out << ind2 << "if (" << cCond << ") {\n";
                                        if (!cAct.isEmpty()) out << ind2 << "    " << cAct << "\n";
                                        out << ind2 << "    nextState = " << tStr << ";\n" << ind2 << "    transition_taken = true;\n" << ind2 << "    break;\n" << ind2 << "}\n";
                                    }
        }

        if (!hasUnc && transitions.isEmpty()) out << ind2 << (isMulti ? "token_finished = true;\n" : "running = false;\n");
        out << ind1 << "} break;\n";
    }
    out << ind1 << "default: " << (isMulti ? "token_finished = true;" : "running = false;") << " break;\n" << ind1 << "}\n";

    if (isMulti) {
        out << "            if (token_finished) { active_states.erase(active_states.begin() + i); previous_states.erase(previous_states.begin() + i); state_changed.erase(state_changed.begin() + i); ";
                for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                    out << it.value() << ".erase(" << it.value() << ".begin() + i); ";
                }
                out << "} else {\n";
                out << "                if (transition_taken) {\n                    if (nextState != currentState) {\n                        switch (currentState) {\n";
                    } else out << "        if (transition_taken) {\n            if (nextState != currentState) {\n                switch (currentState) {\n";

    for (const auto& pb : processedBlocks) {
            if (!pb.cleanOnExit.isEmpty()) out << ind2 << "case " << pb.id << ": { " << pb.cleanOnExit << " } break;\n";
        }

        // --- ИСПРАВЛЕНИЕ 1: Защита от пустого switch (добавляем дефолтный брейк) ---
        out << ind2 << "default: break;\n";

        // --- ИСПРАВЛЕНИЕ 2: Добавлена скобка }\n для закрытия функции run() перед закрытием класса }; ---
        if (isMulti)
            out << "                        }\n                    }\n                    previous_states[i] = currentState;\n                    active_states[i] = nextState;\n                    state_changed[i] = true;\n                }\n                i++;\n            }\n        }\n    }\n}\n};\n\nint main() { StateMachine sm; sm.run(); return 0; }\n";
        else
            out << "                }\n            }\n            previousState = currentState;\n            currentState = nextState;\n            stateChanged = true;\n        }\n    }\n}\n};\n\nint main() { StateMachine sm; sm.run(); return 0; }\n";

        return code;
    }

QString CppTranslator::translateAndCompile(QGraphicsScene *scene, const QString &manualHeader) {
    QString sourceCode = generateSourceCode(scene, manualHeader);
    if (sourceCode.startsWith("ERROR:")) return sourceCode;
    return invokeCompiler(sourceCode);
}

// === УМНЫЙ ПАРСЕР ===
QString CppTranslator::extractAndRemoveTypes(const QString &rawCode) {
    QString code = rawCode;
    QRegularExpression varRegex(R"(\b(int|double|float|bool|std::string|String|char|long|short|unsigned|auto)\s+(\w+)(\s*=[^;]+)?\s*;)");
    QRegularExpressionMatchIterator i = varRegex.globalMatch(code);

    struct MatchInfo { QString fullMatch, type, name, initPart; };
    QList<MatchInfo> matches;

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        matches.append({match.captured(0), match.captured(1), match.captured(2), match.captured(3)});
    }

    for (const auto &m : matches) {
        QString type = m.type;
        if (type == "String") type = "std::string";

        // Явное создание структуры ТОЛЬКО с type и name (защита от ошибок MinGW)
        if (!m_autoVariables.contains(m.name)) {
            VariableInfo vInfo;
            vInfo.type = type;
            vInfo.name = m.name;
            m_autoVariables.insert(m.name, vInfo);
        }

        code.replace(m.fullMatch, !m.initPart.isEmpty() ? m.name + " " + m.initPart + ";" : "");
    }
    return code;
}
// Вспомогательные функции
QString CppTranslator::sanitizeCppCode(const QString &rawCode, bool isCondition) {
    QString code = rawCode.trimmed();
    // Защита от двоеточий
    if (code.endsWith(":")) code.chop(1);
    if (code.endsWith(":;")) code.chop(2);

    code.replace(QRegularExpression("\\bboolean\\b"), "bool");
    code.replace("System.out.println", "log");
    code.replace(QRegularExpression("\\band\\b"), "&&");
    code.replace(QRegularExpression("\\bor\\b"), "||");
    if (!isCondition && !code.isEmpty() && !code.endsWith(';') && !code.endsWith('}')) code += ";";
    return code;
}

bool CppTranslator::hasPythonSyntax(const QString &code, QString &foundFeature) {
    if (code.contains("def ")) { foundFeature = "def"; return true; }
    if (code.contains("import ")) { foundFeature = "import"; return true; }
    return false;
}

QString CppTranslator::invokeCompiler(const QString &sourceCode) {
    QString fileName = "temp_generated.cpp";
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return QObject::tr("Error file");

    QTextStream out(&file); out << sourceCode; file.close();

    QProcess compiler;
    compiler.start("g++", QStringList() << "-fsyntax-only" << fileName);

    if (!compiler.waitForStarted()) return QObject::tr("Ошибка: g++ не найден.");

    compiler.waitForFinished();
    QString err = QString::fromUtf8(compiler.readAllStandardError());

    return (compiler.exitCode() == 0) ? QObject::tr("УСПЕШНО!\nКод валиден.") : QObject::tr("ОШИБКА:\n") + err;
}
