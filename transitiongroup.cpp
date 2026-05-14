/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "transitiongroup.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QtMath>

#include <QGraphicsLineItem>
#include <QCursor>
#include <QTextDocument>
#include <QInputDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QDialogButtonBox>

// Подключаем наш хайлайтер и реестр
#include "codehighlighter.h"
#include "syntaxregistry.h"

#include "utils.h"
TransitionGroup::TransitionGroup(QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    setFlag(ItemIsMovable, false);
    setFlag(ItemIsSelectable, true);
    setFlag(ItemSendsGeometryChanges, true);

    // --- 1. Желтые блоки (Скрываем, так как рисуем сами в paint) ---
    m_condBg = new QGraphicsRectItem(this);
    m_condBg->setVisible(false);

    m_actionBg = new QGraphicsRectItem(this);
    m_actionBg->setVisible(false);

    m_conditionText = new QGraphicsTextItem("if", this);
    m_conditionText->setVisible(false);

    m_actionText = new QGraphicsTextItem("action", this);
    m_actionText->setVisible(false);

    // --- 2. Элементы Приоритета ---
    m_priorityBg = new QGraphicsEllipseItem(-10, -10, 20, 20, this);
    m_priorityBg->setBrush(Qt::white);
    m_priorityBg->setPen(QPen(Qt::black, 1));
    m_priorityBg->setZValue(10); // Поверх желтых блоков

    m_priorityText = new QGraphicsTextItem("1", this);
    m_priorityText->setDefaultTextColor(Qt::black);
    QFont f = m_priorityText->font();
    f.setBold(true);
    f.setPixelSize(12);
    m_priorityText->setFont(f);
    m_priorityText->document()->setDocumentMargin(0);
    m_priorityText->setZValue(11); // Поверх кружка
    m_priorityText->setParentItem(m_priorityBg);

    setPriority(1);
    updateLayout();
}

QRectF TransitionGroup::boundingRect() const
{
    return childrenBoundingRect();
}

QPainterPath TransitionGroup::shape() const
{
    QPainterPath path;
    if (m_condBg) path.addPath(mapFromItem(m_condBg, m_condBg->shape()));
    if (m_actionBg) path.addPath(mapFromItem(m_actionBg, m_actionBg->shape()));
    if (m_priorityBg) path.addPath(mapFromItem(m_priorityBg, m_priorityBg->shape()));
    return path;
}

// --- ОТРИСОВКА С ПОДСВЕТКОЙ ---
void TransitionGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    auto drawHighlighted = [&](QString text, QPointF pos, QRectF bgRect) {
            // 1. Фон (Рисуем, только если включена галочка в меню Edit)
            if (Utils::showYellowLabels()) {
                painter->save();
                painter->setBrush(QColor(255, 255, 0));
                painter->setPen(Qt::NoPen);
                painter->drawRect(bgRect);
                painter->restore();
            }

        // 2. Текст
        painter->save();
        painter->translate(pos);

        QTextDocument doc;
        doc.setDefaultFont(painter->font());
        doc.setDocumentMargin(0);
        doc.setTextWidth(bgRect.width());

        if (text.trimmed().isEmpty()) {
            // Пусто
        } else {
            doc.setPlainText(text);
            QTextCursor cursor(&doc);

            // Цвета
            QTextCharFormat keywordFmt; keywordFmt.setForeground(Qt::darkBlue); keywordFmt.setFontWeight(QFont::Bold);
            QTextCharFormat numberFmt;  numberFmt.setForeground(Qt::magenta);
            QTextCharFormat stringFmt;  stringFmt.setForeground(Qt::darkRed);
            QTextCharFormat commentFmt; commentFmt.setForeground(Qt::darkGreen);
            QTextCharFormat errorFmt;   errorFmt.setForeground(Qt::red); errorFmt.setFontWeight(QFont::Bold);

            bool isCpp = (SyntaxRegistry::instance().language() == SyntaxRegistry::Lang_Cpp);

            // Числа
            QRegularExpression numbers("\\b\\d+\\b");
            auto i = numbers.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                cursor.setPosition(match.capturedStart());
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                cursor.mergeCharFormat(numberFmt);
            }

            // Ключевые слова
            QString patternStr;
            if (isCpp) patternStr = "\\b(if|else|while|for|return|true|false|int|void|bool|double|nullptr)\\b";
            else       patternStr = "\\b(and|as|def|if|else|elif|for|import|in|is|not|or|return|while|None|True|False)\\b";
            QRegularExpression keywords(patternStr);
            i = keywords.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                cursor.setPosition(match.capturedStart());
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                cursor.mergeCharFormat(keywordFmt);
            }

            // Словарь
            QStringList extKeywords = SyntaxRegistry::instance().allKeywords();
            QRegularExpression words("([a-zA-Z_][\\w\\.]*)");
            i = words.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                if (extKeywords.contains(match.captured(1).toLower())) {
                    cursor.setPosition(match.capturedStart());
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                    cursor.mergeCharFormat(keywordFmt);
                }
            }

            // Строки
            QRegularExpression strings(isCpp ? "\".*\"" : "(\".*\"|\'.*\')");
            i = strings.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                cursor.setPosition(match.capturedStart());
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                cursor.mergeCharFormat(stringFmt);
            }

            // Комментарии
            QRegularExpression comments(isCpp ? "//.*" : "#.*");
            i = comments.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                cursor.setPosition(match.capturedStart());
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                cursor.mergeCharFormat(commentFmt);
            }

            // Скобки
            QList<int> openBrackets;
            for (int j = 0; j < text.length(); ++j) {
                QChar c = text[j];
                if (c == '(' || c == '[' || c == '{') openBrackets.append(j);
                else if (c == ')' || c == ']' || c == '}') {
                    if (openBrackets.isEmpty()) {
                        cursor.setPosition(j);
                        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                        cursor.mergeCharFormat(errorFmt);
                    } else {
                        openBrackets.removeLast();
                    }
                }
            }
            for (int idx : openBrackets) {
                cursor.setPosition(idx);
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 1);
                cursor.mergeCharFormat(errorFmt);
            }
        }

        doc.drawContents(painter);
        painter->restore();
    };

    if (m_conditionText && m_condBg) {
        drawHighlighted(m_conditionText->toPlainText(), m_conditionText->pos(), m_condBg->rect());
    }
    if (m_actionText && m_actionBg) {
        drawHighlighted(m_actionText->toPlainText(), m_actionText->pos(), m_actionBg->rect());
    }
}

void TransitionGroup::updateLayout()
{
    const qreal paddingX = 4.0;
    const qreal paddingY = 1.0;
    const qreal lineGap = 16.0;

    QRectF condR = m_conditionText->boundingRect();
    qreal condW = condR.width() + 2 * paddingX;
    qreal condH = condR.height() + 2 * paddingY;

    QRectF actR = m_actionText->boundingRect();
    qreal actW = actR.width() + 2 * paddingX;
    qreal actH = actR.height() + 2 * paddingY;

    if (m_isVertical) {
        qreal condX = -(lineGap / 2.0) - condW;
        qreal condY = -condH / 2.0;
        m_condBg->setRect(condX, condY, condW, condH);
        m_conditionText->setPos(condX + paddingX, condY + paddingY);

        qreal actX = (lineGap / 2.0);
        qreal actY = -actH / 2.0;
        m_actionBg->setRect(actX, actY, actW, actH);
        m_actionText->setPos(actX + paddingX, actY + paddingY);
    } else {
        qreal condX = -condW / 2.0;
        qreal condY = -(lineGap / 2.0) - condH;
        m_condBg->setRect(condX, condY, condW, condH);
        m_conditionText->setPos(condX + paddingX, condY + paddingY);

        qreal actX = -actW / 2.0;
        qreal actY = (lineGap / 2.0);
        m_actionBg->setRect(actX, actY, actW, actH);
        m_actionText->setPos(actX + paddingX, actY + paddingY);
    }
    updatePriorityPosition();
}

// --- ОБНОВЛЕНИЕ ПОЗИЦИЙ ---
void TransitionGroup::updatePriorityPosition()
{
    if (m_cachedLines.isEmpty()) return;

    int idx = m_prioSegmentIndex;
    if (idx < 0 || idx >= m_cachedLines.size()) {
        idx = m_cachedLines.size() - 1;
    }

    QLineF line = m_cachedLines[idx];
    QPointF basePos = line.pointAt(m_prioSliderPos);

    QLineF normal(0, 0, -line.dy(), line.dx());
    normal.setLength(m_prioSideOffset);

    QPointF prioScenePos = basePos + normal.p2();
    QPointF localPrioPos = mapFromParent(prioScenePos);

    m_priorityBg->setPos(localPrioPos);
}
void TransitionGroup::updateAnchor(const QList<QGraphicsLineItem *> &segments)
{
    if (segments.isEmpty()) return;

    // Оставляем переменную для сохранения старой структуры (хоть мы её и не используем для сброса позиции)
    bool segmentsCountChanged = (m_cachedLines.count() != segments.count());
    bool isFirstRun = m_cachedLines.isEmpty();

    qreal absoluteDist = 0.0; // ВМЕСТО currentRatio
    bool hasValidDist = false; // ВМЕСТО hasValidRatio

    // Убрали !segmentsCountChanged, чтобы позиция сохранялась при изломе стрелки
    if (!isFirstRun && m_currentSegmentIndex >= 0 && m_currentSegmentIndex < m_cachedLines.size()) {
        qreal totalLen = 0.0;
        qreal currentDist = 0.0;
        for (int i = 0; i < m_cachedLines.size(); ++i) {
            qreal segLen = m_cachedLines[i].length();
            if (i < m_currentSegmentIndex) currentDist += segLen;
            else if (i == m_currentSegmentIndex) currentDist += segLen * m_sliderPos;
            totalLen += segLen;
        }
        absoluteDist = currentDist; // Запоминаем в ПИКСЕЛЯХ
        hasValidDist = true;
    }

    qreal prioAbsoluteDist = 0.0; // ВМЕСТО prioRatio
    // Убрали !segmentsCountChanged
    bool prioValid = (!isFirstRun && m_prioSegmentIndex >= 0 && m_prioSegmentIndex < m_cachedLines.size());

    if (prioValid) {
        qreal totalLen = 0.0;
        qreal currentDist = 0.0;
        for (int i = 0; i < m_cachedLines.size(); ++i) {
            qreal segLen = m_cachedLines[i].length();
            if (i < m_prioSegmentIndex) currentDist += segLen;
            else if (i == m_prioSegmentIndex) currentDist += segLen * m_prioSliderPos;
            totalLen += segLen;
        }
        prioAbsoluteDist = currentDist; // Запоминаем в ПИКСЕЛЯХ
    }

    m_cachedLines.clear();
    for (auto item : segments) m_cachedLines.append(item->line());

    if (m_isDragging || m_isDraggingPrio) return;

    // --- ПРИМЕНЯЕМ НОВЫЕ ПОЗИЦИИ С ОГРАНИЧЕНИЕМ 50% ---
    if (prioValid) {
        qreal newTotalLen = 0.0;
        for (const QLineF &l : m_cachedLines) newTotalLen += l.length();

        // Ограничиваем: сохраненная дистанция ИЛИ половина новой длины
        qreal targetDist = qMin(prioAbsoluteDist, newTotalLen / 2.0);

        for (int i = 0; i < m_cachedLines.size(); ++i) {
            qreal segLen = m_cachedLines[i].length();
            if (targetDist <= segLen || i == m_cachedLines.size() - 1) {
                m_prioSegmentIndex = i;
                m_prioSliderPos = (segLen > 0.001) ? (targetDist / segLen) : 0.0;
                m_prioSliderPos = qBound(0.0, m_prioSliderPos, 1.0);
                break;
            }
            targetDist -= segLen;
        }
    } else {
        // Поведение по умолчанию для кружка приоритета (эквивалент старых 0.15)
        qreal newTotalLen = 0.0;
        for (const QLineF &l : m_cachedLines) newTotalLen += l.length();
        qreal targetDist = qMin(15.0, newTotalLen / 2.0); // 15 пикселей от начала

        for (int i = 0; i < m_cachedLines.size(); ++i) {
            qreal segLen = m_cachedLines[i].length();
            if (targetDist <= segLen || i == m_cachedLines.size() - 1) {
                m_prioSegmentIndex = i;
                m_prioSliderPos = (segLen > 0.001) ? (targetDist / segLen) : 0.0;
                m_prioSliderPos = qBound(0.0, m_prioSliderPos, 1.0);
                break;
            }
            targetDist -= segLen;
        }
    }

    if (hasValidDist) {
        qreal newTotalLen = 0.0;
        for (const QLineF &l : m_cachedLines) newTotalLen += l.length();

        // Ограничиваем: сохраненная дистанция ИЛИ половина новой длины
        qreal targetDist = qMin(absoluteDist, newTotalLen / 2.0);

        for (int i = 0; i < m_cachedLines.size(); ++i) {
            qreal segLen = m_cachedLines[i].length();
            if (targetDist <= segLen || i == m_cachedLines.size() - 1) {
                m_currentSegmentIndex = i;
                m_sliderPos = (segLen > 0.001) ? (targetDist / segLen) : 0.0;
                m_sliderPos = qBound(0.0, m_sliderPos, 1.0);
                break;
            }
            targetDist -= segLen;
        }
    } else {
        // Поведение по умолчанию для желтых блоков (эквивалент старых 0.4)
        qreal newTotalLen = 0.0;
        for (const QLineF &l : m_cachedLines) newTotalLen += l.length();
        qreal targetDist = qMin(40.0, newTotalLen / 2.0); // 40 пикселей от начала

        for (int i = 0; i < m_cachedLines.size(); ++i) {
            qreal segLen = m_cachedLines[i].length();
            if (targetDist <= segLen || i == m_cachedLines.size() - 1) {
                m_currentSegmentIndex = i;
                m_sliderPos = (segLen > 0.001) ? (targetDist / segLen) : 0.0;
                m_sliderPos = qBound(0.0, m_sliderPos, 1.0);
                break;
            }
            targetDist -= segLen;
        }
    }

    QLineF line = m_cachedLines[m_currentSegmentIndex];
    m_sliderPos = qBound(0.0, m_sliderPos, 1.0);
    QPointF posOnLine = line.pointAt(m_sliderPos);
    setPos(posOnLine);

    bool newIsVertical = (qAbs(line.dx()) < qAbs(line.dy()));
    if (m_isVertical != newIsVertical) {
        m_isVertical = newIsVertical;
        updateLayout();
    } else {
        updatePriorityPosition();
    }

    setRotation(0);
}



void TransitionGroup::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF localPos = event->pos();

        // 1. Приоритет
        if (m_priorityBg->contains(m_priorityBg->mapFromParent(localPos))) {
            m_isDraggingPrio = true;
            m_prioBackup.segmentIndex = m_prioSegmentIndex;
            m_prioBackup.sliderPos = m_prioSliderPos;
            m_prioBackup.sideOffset = m_prioSideOffset;
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }

        // 2. Желтые блоки
        m_isDragging = true;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QGraphicsObject::mousePressEvent(event);
    }
}


void TransitionGroup::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF parentPos = mapToParent(event->pos());

    if (m_isDraggingPrio && !m_cachedLines.isEmpty()) {
        int bestIdx = -1;
        qreal minDist = std::numeric_limits<qreal>::max();
        qreal bestT = 0.0;
        qreal bestOffset = 0.0;

        for (int i = 0; i < m_cachedLines.size(); ++i) {
            const QLineF &line = m_cachedLines[i];
            QPointF p1 = line.p1();
            QPointF vec = line.p2() - p1;
            qreal len2 = vec.x()*vec.x() + vec.y()*vec.y();
            if (qFuzzyIsNull(len2)) continue;

            QPointF diff = parentPos - p1;
            qreal t = (diff.x()*vec.x() + diff.y()*vec.y()) / len2;
            qreal tClamped = qBound(0.0, t, 1.0);

            QPointF proj = p1 + vec * tClamped;
            qreal dist = QLineF(parentPos, proj).length();

            if (dist < minDist) {
                minDist = dist;
                bestIdx = i;
                bestT = tClamped;
                qreal cross = vec.x() * (parentPos.y() - p1.y()) - vec.y() * (parentPos.x() - p1.x());
                bestOffset = (cross >= 0) ? 25.0 : -25.0;
            }
        }

        if (bestIdx != -1) {
            // =================================================================
            // --- НОВОЕ: ПРАВИЛО 50% ДЛЯ ПРИОРИТЕТА ---
            qreal totalLen = 0.0;
            qreal currentDist = 0.0;
            for (int i = 0; i < m_cachedLines.size(); ++i) {
                qreal segLen = m_cachedLines[i].length();
                totalLen += segLen;
                if (i < bestIdx) currentDist += segLen;
                else if (i == bestIdx) currentDist += segLen * bestT;
            }

            qreal maxDist = totalLen / 2.0;
            if (currentDist > maxDist) {
                currentDist = maxDist; // Ограничиваем половиной длины стрелки
                qreal remaining = currentDist;
                for (int i = 0; i < m_cachedLines.size(); ++i) {
                    qreal segLen = m_cachedLines[i].length();
                    if (remaining <= segLen || i == m_cachedLines.size() - 1) {
                        bestIdx = i;
                        bestT = (segLen > 0.001) ? (remaining / segLen) : 0.0;
                        break;
                    }
                    remaining -= segLen;
                }
            }
            // --- КОНЕЦ НОВОГО ПРАВИЛА ---
            // =================================================================

            m_prioSegmentIndex = bestIdx;
            m_prioSliderPos = bestT;
            m_prioSideOffset = bestOffset;
            updatePriorityPosition();
        }
        event->accept();
        return;
    }

    if (m_isDragging && !m_cachedLines.isEmpty()) {
        int bestIdx = -1;
        qreal minDist = std::numeric_limits<qreal>::max();
        qreal bestT = 0.0;

        for (int i = 0; i < m_cachedLines.size(); ++i) {
            const QLineF &line = m_cachedLines[i];
            QPointF vec = line.p2() - line.p1();
            qreal len2 = vec.x()*vec.x() + vec.y()*vec.y();

            if (qFuzzyIsNull(len2)) continue;

            QPointF diff = parentPos - line.p1();
            qreal t = (diff.x()*vec.x() + diff.y()*vec.y()) / len2;
            qreal tClamped = qBound(0.0, t, 1.0);

            QPointF proj = line.p1() + vec * tClamped;
            qreal dist = QLineF(parentPos, proj).length();

            if (dist < minDist) {
                minDist = dist;
                bestIdx = i;
                bestT = tClamped;
            }
        }

        if (bestIdx != -1) {
            // =================================================================
            // --- НОВОЕ: ПРАВИЛО 50% ДЛЯ ЖЕЛТЫХ БЛОКОВ ---
            qreal totalLen = 0.0;
            qreal currentDist = 0.0;
            for (int i = 0; i < m_cachedLines.size(); ++i) {
                qreal segLen = m_cachedLines[i].length();
                totalLen += segLen;
                if (i < bestIdx) currentDist += segLen;
                else if (i == bestIdx) currentDist += segLen * bestT;
            }

            qreal maxDist = totalLen / 2.0;
            if (currentDist > maxDist) {
                currentDist = maxDist; // Ограничиваем половиной длины стрелки
                qreal remaining = currentDist;
                for (int i = 0; i < m_cachedLines.size(); ++i) {
                    qreal segLen = m_cachedLines[i].length();
                    if (remaining <= segLen || i == m_cachedLines.size() - 1) {
                        bestIdx = i;
                        bestT = (segLen > 0.001) ? (remaining / segLen) : 0.0;
                        break;
                    }
                    remaining -= segLen;
                }
            }
            // --- КОНЕЦ НОВОГО ПРАВИЛА ---
            // =================================================================

            m_currentSegmentIndex = bestIdx;
            m_sliderPos = bestT;
            setPos(m_cachedLines[bestIdx].pointAt(bestT));

            QLineF newLine = m_cachedLines[bestIdx];
            bool newIsVertical = (qAbs(newLine.dx()) < qAbs(newLine.dy()));
            if (m_isVertical != newIsVertical) {
                m_isVertical = newIsVertical;
                updateLayout();
            } else {
                updatePriorityPosition();
            }
        }
        event->accept();
        return;
    }
    QGraphicsObject::mouseMoveEvent(event);
}

void TransitionGroup::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_isDraggingPrio) {
        m_isDraggingPrio = false;
        setCursor(Qt::ArrowCursor);
        if (m_priorityBg->collidesWithItem(m_condBg) || m_priorityBg->collidesWithItem(m_actionBg)) {
            m_prioSegmentIndex = m_prioBackup.segmentIndex;
            m_prioSliderPos = m_prioBackup.sliderPos;
            m_prioSideOffset = m_prioBackup.sideOffset;
            updatePriorityPosition();
        }
        event->accept();
        return;
    }

    if (m_isDragging) {
        m_isDragging = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsObject::mouseReleaseEvent(event);
}

void TransitionGroup::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF clickPos = event->pos();
    QWidget *parentWidget = (scene() && !scene()->views().isEmpty()) ? scene()->views().first() : nullptr;
    // =========================================================================
        // УМНАЯ ФУНКЦИЯ ДЛЯ ПОДСВЕТКИ СКОБОК
        // =========================================================================
        auto setupBracketMatching = [](QTextEdit *editor) {
            QObject::connect(editor, &QTextEdit::cursorPositionChanged, editor, [editor]() {
                QList<QTextEdit::ExtraSelection> selections;
                editor->setExtraSelections(selections);

                QTextCursor cursor = editor->textCursor();
                int pos = cursor.position();
                QString text = editor->toPlainText();
                if (text.isEmpty()) return;

                auto findMatch = [&](int startPos, QChar openChar, QChar closeChar, int direction) -> int {
                    int count = 1;
                    int curr = startPos + direction;
                    while (curr >= 0 && curr < text.length()) {
                        if (text[curr] == openChar) count++;
                        else if (text[curr] == closeChar) count--;
                        if (count == 0) return curr;
                        curr += direction;
                    }
                    return -1;
                };

                QChar charRight = (pos < text.length()) ? text[pos] : QChar();
                QChar charLeft = (pos > 0) ? text[pos - 1] : QChar();

                int matchPos = -1, bracketPos = -1;

                if (charRight == '{')      { matchPos = findMatch(pos, '{', '}', 1); bracketPos = pos; }
                else if (charRight == '(') { matchPos = findMatch(pos, '(', ')', 1); bracketPos = pos; }
                else if (charRight == '[') { matchPos = findMatch(pos, '[', ']', 1); bracketPos = pos; }
                else if (charLeft == '}')  { matchPos = findMatch(pos - 1, '}', '{', -1); bracketPos = pos - 1; }
                else if (charLeft == ')')  { matchPos = findMatch(pos - 1, ')', '(', -1); bracketPos = pos - 1; }
                else if (charLeft == ']')  { matchPos = findMatch(pos - 1, ']', '[', -1); bracketPos = pos - 1; }

                if (matchPos != -1) {
                    QTextCharFormat format;
                    format.setBackground(QColor(180, 220, 255));
                    format.setFontWeight(QFont::Bold);

                    QTextEdit::ExtraSelection sel1, sel2;
                    QTextCursor cur1 = editor->textCursor();
                    cur1.setPosition(bracketPos); cur1.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    sel1.cursor = cur1; sel1.format = format;

                    QTextCursor cur2 = editor->textCursor();
                    cur2.setPosition(matchPos); cur2.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    sel2.cursor = cur2; sel2.format = format;

                    selections.append(sel1); selections.append(sel2);
                    editor->setExtraSelections(selections);
                }
            });
        };

    if (m_condBg->contains(m_condBg->mapFromParent(clickPos))) {
        QDialog dlg(parentWidget);
        dlg.setWindowTitle("Condition");
        dlg.setMinimumWidth(300);
        QVBoxLayout *layout = new QVBoxLayout(&dlg);
        layout->addWidget(new QLabel("Enter condition:", &dlg));

        QTextEdit *edit = new QTextEdit(&dlg);
        edit->setPlainText(m_conditionText->toPlainText());
        edit->setFixedHeight(60);
        new CodeHighlighter(edit->document()); // Подсветка
        setupBracketMatching(edit);
        layout->addWidget(edit);

        QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        layout->addWidget(btns);

        if (dlg.exec() == QDialog::Accepted) {
            QString text = edit->toPlainText();
            if (text.trimmed().isEmpty()) setConditionText(" ");
            else setConditionText(text);
        }
        event->accept();
        return;
    }

    if (m_actionBg->contains(m_actionBg->mapFromParent(clickPos))) {
        QDialog dlg(parentWidget);
        dlg.setWindowTitle("Action");
        dlg.setMinimumWidth(300);
        QVBoxLayout *layout = new QVBoxLayout(&dlg);
        layout->addWidget(new QLabel("Enter action:", &dlg));

        QTextEdit *edit = new QTextEdit(&dlg);
        edit->setPlainText(m_actionText->toPlainText());
        edit->setFixedHeight(60);
        new CodeHighlighter(edit->document()); // Подсветка
        setupBracketMatching(edit);
        layout->addWidget(edit);

        QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        layout->addWidget(btns);

        if (dlg.exec() == QDialog::Accepted) {
            QString text = edit->toPlainText();
            if (text.trimmed().isEmpty()) setActionText(" ");
            else setActionText(text);
        }
        event->accept();
        return;
    }

    if (m_priorityBg->contains(m_priorityBg->mapFromParent(clickPos))) {
        bool ok;
        int val = QInputDialog::getInt(parentWidget, "Priority", "Enter priority:", m_priorityValue, 0, 100, 1, &ok);
        if (ok) setPriority(val);
        event->accept();
        return;
    }
    event->ignore();
}

void TransitionGroup::setPriority(int priority) {
    m_priorityValue = priority;
    m_priorityText->setPlainText(QString::number(priority));
    // Центрирование цифры
    QRectF br = m_priorityText->boundingRect();
    m_priorityText->setPos(-br.width()/2, -br.height()/2);
}

int TransitionGroup::getPriority() const {
    return m_priorityValue;
}

void TransitionGroup::setConditionText(const QString &text) {
    m_conditionText->setPlainText(text);
    updateLayout();
}

QString TransitionGroup::conditionText() const {
    return m_conditionText->toPlainText();
}

void TransitionGroup::setActionText(const QString &text) {
    m_actionText->setPlainText(text);
    updateLayout();
}

QString TransitionGroup::actionText() const {
    return m_actionText->toPlainText();
}
