/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "javatranslator.h"
#include "diagramitem.h"
#include "connect.h"
#include "transitiongroup.h"
#include <QTextStream>
#include <algorithm>
#include "commentitem.h"
#include "bridgeitem.h"
#include "backitem.h"
#include "groupitem.h"
#include "historyitem.h"

JavaTranslator::JavaTranslator(QObject *parent) : QObject(parent) {}

GeneratedJavaCode JavaTranslator::generateClean(QGraphicsScene *scene, const QString &className, const QString &manualHeader)
{
    GeneratedJavaCode result;
    QMap<QString, VariableInfo> variables;

    // --- ЭТАП 1: Сбор блоков ---
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
        result.error = QObject::tr("ERROR: Start block not found (Нет стартового блока)!");
        return result;
    }
    for (DiagramItem* sb : startBlocks) {
        rawBlocks.prepend(sb);
    }

    bool isMulti = (startBlocks.size() > 1);

    // Присваиваем ID и чистим код внутри блоков
    int idCounter = 1;
    QMap<DiagramItem*, int> blockIds;

    for (DiagramItem* b : rawBlocks) {
        blockIds[b] = idCounter;

        QString rawEnter = sanitizeJavaCode(b->getOnEnter(), false);
        QString rawExit = sanitizeJavaCode(b->getOnExit(), false);

        QString cleanEnter = extractAndRemoveTypes(rawEnter, variables);
        QString cleanExit = extractAndRemoveTypes(rawExit, variables);

        processedBlocks.append({b, idCounter, cleanEnter, cleanExit});
        idCounter++;
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
                            defaultTargetId = blockIds[conn->endBlock()];
                            break;
                        }
                    }
                }

                // ВАЖНО ДЛЯ JAVA: добавляем слово static
                                if (isMulti) {
                                    historyDeclarations += "    static List<Integer> " + varName + " = new ArrayList<>(Collections.nCopies(" + QString::number(startBlocks.size()) + ", " + QString::number(defaultTargetId) + "));\n";
                                } else {
                                    historyDeclarations += "    static int " + varName + " = " + QString::number(defaultTargetId) + ";\n";
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
            if (line.trimmed().startsWith("import ")) {
                importsCode += line + "\n";
            } else {
                membersCode += line + "\n";
            }
        }

        // --- ЭТАП 2: Генерация Java Кода ---
        QString code;
        QTextStream out(&code);

        QString finalClassName = className.isEmpty() ? "Robot" : className;
        finalClassName.replace(" ", "");

        out << "// Generated Java Code\n";
        out << "import java.util.*;\n";
        out << "import java.lang.Math;\n";

        if (!importsCode.trimmed().isEmpty()) {
            out << "// --- User Imports ---\n";
            out << importsCode << "\n";
        }

        out << "public class " << finalClassName << " {\n\n";

        if (!membersCode.trimmed().isEmpty()) {
            out << "    // --- User Defined Members ---\n";
            out << membersCode << "\n";
        }

        // --- 2.1 Глобальные переменные (static fields) ---
        out << "    // --- Global Variables ---\n";
        if (isMulti) {
            out << "    static List<Integer> activeStates = new ArrayList<>(Arrays.asList(";
            for (int i = 0; i < startBlocks.size(); ++i) {
                out << blockIds[startBlocks[i]] << (i < startBlocks.size() - 1 ? ", " : "");
            }
            out << "));\n";
            out << "    static List<Integer> previousStates = new ArrayList<>(Collections.nCopies(" << startBlocks.size() << ", -1));\n";
            out << "    static List<Boolean> stateChanged = new ArrayList<>(Collections.nCopies(" << startBlocks.size() << ", true));\n";
        } else {
            out << "    static int currentState = " << blockIds[startBlocks.first()] << ";\n";
            out << "    static int previousState = -1;\n";
            out << "    static boolean stateChanged = true;\n";
        }
        out << "    static boolean isRunning = true;\n\n";

        if (!variables.isEmpty()) {
            for (auto it = variables.begin(); it != variables.end(); ++it) {
                QString type = it.value().type;
                QString name = it.value().name;

                QString initVal;
                if (type == "int" || type == "double" || type == "float" || type == "long") initVal = " = 0";
                else if (type == "boolean") initVal = " = false";
                else if (type == "String") initVal = " = \"\"";
                else initVal = " = null";

                out << "    static " << type << " " << name << initVal << ";\n";
            }
        }
        out << "\n" << historyDeclarations << "\n";



    // --- 2.2 Пользовательский Хедер ---
    if (!manualHeader.trimmed().isEmpty()) {
        out << "    // --- User Defined Members ---\n";
        out << "    " << manualHeader << "\n\n";
    }

    // --- 2.3 Main (Точка входа) ---
    out << "    public static void main(String[] args) {\n";
    out << "        System.out.println(\"--- Java Robot Started ---\");\n";
    out << "        setup();\n";
    out << "        while (isRunning) {\n";
    out << "            loop();\n";
    out << "        }\n";
    out << "        System.out.println(\"--- Execution Finished ---\");\n";
    out << "    }\n\n";

    out << "    static void setup() {\n";
    out << "        // Initialization code here\n";
    out << "    }\n\n";

    // --- 2.5 Loop (Профессиональный 3-фазный конвейер) ---
    out << "    static void loop() {\n";
    if (isMulti) {
        out << "        if (!isRunning || activeStates.isEmpty()) return;\n";
        out << "        for (int i = 0; i < activeStates.size(); ) {\n";
        out << "            int currentState = activeStates.get(i);\n";
        out << "            int nextState = currentState;\n";
        out << "            boolean token_finished = false;\n\n";
        out << "            // --- ON ENTER PHASE ---\n";
        out << "            if (stateChanged.get(i)) {\n";
    } else {
        out << "        if (!isRunning) return;\n";
        out << "        int nextState = currentState;\n\n";
        out << "        // --- ON ENTER PHASE ---\n";
        out << "        if (stateChanged) {\n";
    }

    QString ind1 = isMulti ? "            " : "        ";
    QString ind2 = isMulti ? "                " : "            ";

    out << ind1 << "    switch (currentState) {\n";
    for (const auto& pb : processedBlocks) {
        if (dynamic_cast<BackItem*>(pb.item) || dynamic_cast<HistoryItem*>(pb.item)) continue;

        bool hasCode = !pb.cleanOnEnter.isEmpty() || blockToHistoryVar.contains(pb.id);
        QString comments = CommentItem::getAssociatedCode(pb.item, "// ");

        if (hasCode || !comments.isEmpty()) {
            out << ind1 << "        case " << pb.id << ": {\n";

            if (blockToHistoryVar.contains(pb.id)) {
                            if (isMulti) {
                                out << ind2 << "    " << blockToHistoryVar[pb.id] << ".set(i, " << pb.id << "); // Update History\n";
                            } else {
                                out << ind2 << "    " << blockToHistoryVar[pb.id] << " = " << pb.id << "; // Update History\n";
                            }
                        }

            if (!comments.isEmpty()) out << ind2 << "    " << comments.replace("\n", "\n" + ind2 + "    ");
            if (!pb.cleanOnEnter.isEmpty()) out << ind2 << "    " << pb.cleanOnEnter << "\n";
            out << ind1 << "        } break;\n";
        }
    }
    out << ind1 << "    }\n";

    if (isMulti) out << ind1 << "    stateChanged.set(i, false);\n" << ind1 << "}\n\n";
    else out << ind1 << "    stateChanged = false;\n" << ind1 << "}\n\n";

    out << ind1 << "// --- TRANSITIONS PHASE ---\n";
    out << ind1 << "switch (currentState) {\n";

    for (const auto& pb : processedBlocks) {
        if (dynamic_cast<BackItem*>(pb.item) || dynamic_cast<HistoryItem*>(pb.item)) continue;
        out << ind1 << "    case " << pb.id << ": {\n";

        if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(pb.item)) {
            QString targetName = bridge->targetName();
            int targetId = -1;
            for (auto it = blockIds.begin(); it != blockIds.end(); ++it) {
                if (it.key()->getLabel() == targetName) { targetId = it.value(); break; }
            }
            if (targetId != -1) out << ind2 << "nextState = " << targetId << ";\n";
            else out << ind2 << (isMulti ? "token_finished = true;\n" : "isRunning = false;\n");
            out << ind1 << "    } break;\n";
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
            if (!arrowComm.isEmpty()) out << ind2 << arrowComm.replace("\n", "\n" + ind2);

            QString cond = sanitizeJavaCode(conn->transition()->conditionText(), true).trimmed();
            if (cond.toLower() == "if") cond = "";
            if (cond.toLower().startsWith("if ")) cond = cond.mid(3).trimmed();

            QString cleanAct = extractAndRemoveTypes(sanitizeJavaCode(conn->transition()->actionText(), false), variables);


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
                                targetStr = isMulti ? "(previousStates.get(i) != -1 ? previousStates.get(i) : currentState)" : "(previousState != -1 ? previousState : currentState)";
                            }
                        } else if (HistoryItem* hist = dynamic_cast<HistoryItem*>(conn->endBlock())) {
                            if (historyTargetVar.contains(hist)) {
                                targetStr = historyTargetVar[hist] + (isMulti ? ".get(i)" : "");
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


            bool isAlwaysTrue = (cond.isEmpty() || cond == "true" || cond == "1");

            if (isAlwaysTrue) {
                if (!cleanAct.isEmpty()) out << ind2 << cleanAct << "\n";
                out << ind2 << "nextState = " << targetStr << ";\n";
                hasUnconditional = true;
                break;
            } else {
                out << ind2 << "if (" << cond << ") {\n";
                if (!cleanAct.isEmpty()) out << ind2 << "    " << cleanAct << "\n";
                out << ind2 << "    nextState = " << targetStr << ";\n";
                out << ind2 << "    break;\n";
                out << ind2 << "}\n";
            }
        }

        if (!hasUnconditional && transitions.isEmpty()) {
             out << ind2 << (isMulti ? "token_finished = true;\n" : "isRunning = false;\n");
        }
        out << ind1 << "    } break;\n";
    }

    out << ind1 << "    default: " << (isMulti ? "token_finished = true;" : "isRunning = false;") << " break;\n";
    out << ind1 << "}\n\n";

    // --- ON EXIT & UPDATE PHASE ---
    if (isMulti) {
            out << "            // --- ON EXIT & UPDATE PHASE ---\n";
            out << "            if (token_finished) {\n";
            out << "                activeStates.remove(i);\n";
            out << "                previousStates.remove(i);\n";
            out << "                stateChanged.remove(i);\n";
            for (auto it = historyTargetVar.begin(); it != historyTargetVar.end(); ++it) {
                out << "                " << it.value() << ".remove(i);\n";
            }
            out << "            } else {\n";

        out << "                if (nextState != currentState) {\n";
        out << "                    switch (currentState) {\n";
    } else {
        out << "        // --- ON EXIT & UPDATE PHASE ---\n";
        out << "        if (nextState != currentState) {\n";
        out << "            switch (currentState) {\n";
    }

    for (const auto& pb : processedBlocks) {
        if (!pb.cleanOnExit.isEmpty()) {
            out << ind2 << "case " << pb.id << ": {\n";
            out << ind2 << "    " << pb.cleanOnExit << "\n";
            out << ind2 << "} break;\n";
        }
    }

    if (isMulti) {
        out << "                    }\n";
        out << "                    previousStates.set(i, currentState);\n";
        out << "                    activeStates.set(i, nextState);\n";
        out << "                    stateChanged.set(i, true);\n";
        out << "                }\n";
        out << "                i++;\n";
        out << "            }\n";
        out << "        }\n";
        out << "        try { Thread.sleep(10); } catch (Exception e) {}\n";
    } else {
        out << "            }\n";
        out << "            previousState = currentState;\n";
        out << "            currentState = nextState;\n";
        out << "            stateChanged = true;\n";
        out << "        }\n";
        out << "        try { Thread.sleep(10); } catch (Exception e) {}\n";
    }

    out << "    }\n\n"; // Конец loop()

    out << "    // Helper for printing\n";
    out << "    static void print(Object o) { System.out.println(o); }\n";
    out << "    static void log(Object o) { System.out.println(o); }\n";
    out << "}\n";

    result.javaCode = code;
    return result;
}

// === ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===
QString JavaTranslator::extractAndRemoveTypes(const QString &rawCode, QMap<QString, VariableInfo> &variables) {
    QString code = rawCode;
    QRegularExpression varRegex(R"(\b(int|double|float|boolean|String|char|long)\s+(\w+)(\s*=[^;]+)?\s*;)");

    QRegularExpressionMatchIterator i = varRegex.globalMatch(code);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString type = match.captured(1);
        QString name = match.captured(2);
        QString val = match.captured(3);

        // Идеально безопасное заполнение QMap
        if (!variables.contains(name)) {
            VariableInfo vInfo;
            vInfo.type = type;
            vInfo.name = name;
            variables.insert(name, vInfo);
        }

        if (!val.isEmpty()) code.replace(match.captured(0), name + " " + val + ";");
        else code.replace(match.captured(0), "");
    }
    return code;
}

QString JavaTranslator::sanitizeJavaCode(const QString &rawCode, bool isCondition) {
    QString code = rawCode.trimmed();

    // Защита от синтаксических ошибок (по профессиональному стандарту)
    if (code.endsWith(":")) code.chop(1);
    if (code.endsWith(":;")) code.chop(2);

    code.replace(QRegularExpression("\\bbool\\b"), "boolean");
    code.replace(QRegularExpression("\\bstring\\b"), "String");
    code.replace(QRegularExpression("\\bconst char\\*\\b"), "String");
    code.replace("std::to_string", "String.valueOf");
    code.replace("Serial.println", "System.out.println");
    code.replace("std::cout", "System.out.print");
    code.replace("<<", "+");

    if (!isCondition && !code.isEmpty() && !code.endsWith(';') && !code.endsWith('}')) {
        code += ";";
    }
    return code;
}
