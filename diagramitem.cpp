/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "diagramitem.h"
#include "editabletextitem.h" // Больше не используется, но можно оставить для совместимости
#include <QGraphicsSceneMouseEvent>
#include <QFontMetrics>
#include "resizehandle.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QGraphicsScene>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include "grid.h"
#include <cmath>
#include <QGraphicsView>
#include "connect.h"
#include <limits>
#include "propertiesdialog.h"
#include <QTextDocument>      // <-- Для сложного текста
#include <QTextCursor>
#include <QTextBlockFormat>
#include "syntaxregistry.h"
#include <QRegularExpression>
#include "codehighlighter.h"
#include <algorithm> // <--- ОБЯЗАТЕЛЬНО  для std::sort
#include "diagramview.h"
int DiagramItem::s_counter = 0;

DiagramItem::DiagramItem(QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    m_id = ++s_counter;

    // Начальный размер чуть больше для трех секций
    setRect(0, 0, 160, 96);

    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
setBrush(Qt::white); // Устанавливаем базовый цвет (который можно менять)
    // Инициализация ручек
    for (int i = 0; i < 8; ++i) {
        ResizeHandle* handle = new ResizeHandle(static_cast<ResizeHandle::Position>(i), this, this);
        m_handles.append(handle);
        handle->hide();
    }

    // Первичная генерация якорей
    // (grid может быть еще не установлен, но точки нужны)
    generateAnchorPoints();
}

// --- ОТРИСОВКА БЛОКА (ТРИ СЕКЦИИ) ---
void DiagramItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    QRectF r = rect();

    // 1. Фон и рамка
    // --- СТАЛО (Используем текущий цвет кисти): ---
        painter->setBrush(brush());
    QPen pen(Qt::black, 1);
    if (option->state & QStyle::State_Selected) {
        pen.setColor(Qt::blue);
        pen.setWidth(2);
    }
    painter->setPen(pen);
    painter->drawRect(r);

    // 2. Расчет высоты секций
    qreal headerH = 30.0;
    qreal remainingH = r.height() - headerH;
    if (remainingH < 0) remainingH = 0;
    qreal section2H = remainingH / 2.0;
    if (section2H < 15.0 && remainingH > 0) section2H = remainingH / 2.0;
    qreal y1 = headerH;
    qreal y2 = headerH + section2H;

    painter->setPen(QPen(Qt::black, 1));
    if (r.height() > headerH) painter->drawLine(0, y1, r.width(), y1);
    if (r.height() > y2) painter->drawLine(0, y2, r.width(), y2);

    // 3. Заголовок
    QRectF headerRect(0, 0, r.width(), headerH);
    QString titleText = m_description;
    if (titleText.isEmpty()) {
        painter->setPen(Qt::gray);
        painter->drawText(headerRect, Qt::AlignCenter, QObject::tr("Настройте блок"));
    } else {
        painter->setPen(Qt::black);
        QFont f = painter->font();
        f.setBold(true);
        painter->setFont(f);
        painter->drawText(headerRect, Qt::AlignCenter, titleText);
        f.setBold(false);
        painter->setFont(f);
    }

    // --- ФУНКЦИЯ ОТРИСОВКИ С ДИНАМИЧЕСКОЙ ПОДСВЕТКОЙ (C++/PYTHON) ---
    auto drawColoredText = [&](QString text, QRectF rect, QString placeholder) {
        painter->save();
        painter->translate(rect.topLeft());

        QTextDocument doc;
        doc.setDefaultFont(painter->font());
        doc.setDocumentMargin(0);
        doc.setTextWidth(rect.width());

        if (text.isEmpty()) {
            QTextCharFormat fmt;
            fmt.setForeground(Qt::lightGray);
            QTextCursor cursor(&doc);
            cursor.insertText(placeholder, fmt);
            QTextBlockFormat blockFmt;
            blockFmt.setAlignment(Qt::AlignCenter);
            cursor.select(QTextCursor::Document);
            cursor.mergeBlockFormat(blockFmt);
        } else {
            doc.setPlainText(text);
            QTextCursor cursor(&doc);

            // Цвета
            QTextCharFormat numberFmt; numberFmt.setForeground(Qt::magenta);
            QTextCharFormat keywordFmt; keywordFmt.setForeground(Qt::darkBlue); keywordFmt.setFontWeight(QFont::Bold);
            QTextCharFormat stringFmt; stringFmt.setForeground(Qt::darkRed);
            QTextCharFormat commentFmt; commentFmt.setForeground(Qt::darkGreen);
            QTextCharFormat errorFmt; errorFmt.setForeground(Qt::red); errorFmt.setFontWeight(QFont::Bold);

            // *** ОПРЕДЕЛЯЕМ ТЕКУЩИЙ ЯЗЫК ***
            bool isCpp = (SyntaxRegistry::instance().language() == SyntaxRegistry::Lang_Cpp);

            // 1. ЧИСЛА
            QRegularExpression numbers("\\b\\d+\\b");
            auto i = numbers.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                cursor.setPosition(match.capturedStart());
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                cursor.mergeCharFormat(numberFmt);
            }

            // 2. КЛЮЧЕВЫЕ СЛОВА (В ЗАВИСИМОСТИ ОТ ЯЗЫКА)
            QString patternStr;
            if (isCpp) {
                patternStr = "\\b(if|else|while|for|return|true|false|int|void|bool|double|float|char|class|struct|public|private|protected|const|static|new|delete|nullptr|this|switch|case|break|continue)\\b";
            } else {
                patternStr = "\\b(and|as|assert|break|class|continue|def|del|elif|else|except|finally|for|from|global|if|import|in|is|lambda|not|or|pass|raise|return|try|while|with|yield|None|True|False)\\b";
            }
            QRegularExpression keywords(patternStr);
            i = keywords.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                cursor.setPosition(match.capturedStart());
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                cursor.mergeCharFormat(keywordFmt);
            }

            // 3. СЛОВА ИЗ СЛОВАРЯ (functions.txt)
            QStringList extKeywords = SyntaxRegistry::instance().allKeywords();
            QRegularExpression words("([a-zA-Z_][\\w\\.]*)");
            i = words.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                QString word = match.captured(1);
                if (extKeywords.contains(word.toLower())) {
                    cursor.setPosition(match.capturedStart());
                    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                    cursor.mergeCharFormat(keywordFmt);
                }
            }

            // 4. СТРОКИ (C++: "...", Python: "..." и '...')
            QRegularExpression strings(isCpp ? "\".*\"" : "(\".*\"|\'.*\')");
            i = strings.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                cursor.setPosition(match.capturedStart());
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                cursor.mergeCharFormat(stringFmt);
            }

            // 5. КОММЕНТАРИИ (C++: //, Python: #)
            QRegularExpression comments(isCpp ? "//.*" : "#.*");
            i = comments.globalMatch(text);
            while (i.hasNext()) {
                auto match = i.next();
                cursor.setPosition(match.capturedStart());
                cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, match.capturedLength());
                cursor.mergeCharFormat(commentFmt);
            }

            // 6. ПРОВЕРКА СКОБОК
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

        doc.drawContents(painter, QRectF(0, 0, rect.width(), rect.height()));
        painter->restore();
    };

    // Секция 2: onEnter
    if (r.height() > headerH) {
        QRectF enterRect(4, y1 + 2, r.width() - 8, section2H - 4);
        drawColoredText(m_onEnter, enterRect, "onEnter()");
    }

    // Секция 3: onExit
    if (r.height() > y2) {
        QRectF exitRect(4, y2 + 2, r.width() - 8, (r.height() - y2) - 4);
        drawColoredText(m_onExit, exitRect, "onExit()");
    }

    // 5. Якоря
    if (m_showAnchorPoints && !m_anchorPoints.isEmpty()) {
        painter->save();
        painter->setPen(Qt::NoPen);
        qreal radius = 3.0;
        const qreal eps = 1e-3;
        for (const QPointF &pt : m_anchorPoints) {
            if (m_hasHighlightedAnchor && QLineF(pt, m_highlightedAnchor).length() < eps) {
                painter->setBrush(Qt::red);
            } else {
                painter->setBrush(Qt::blue);
            }
            painter->drawEllipse(pt, radius, radius);
        }
        painter->restore();
    }
    if (m_isError) {
            // Рисуем Желтый Треугольник в правом верхнем углу
            painter->save();

            // Настраиваем желтую кисть и черную обводку
            painter->setBrush(Qt::yellow);
            painter->setPen(QPen(Qt::black, 2));

            // Координаты треугольника (справа сверху)
            qreal size = 24;
            qreal margin = 4;
            QPointF p1(rect().width() - margin - size/2, margin);        // Верх
            QPointF p2(rect().width() - margin, margin + size);          // Право-Низ
            QPointF p3(rect().width() - margin - size, margin + size);   // Лево-Низ

            QPolygonF triangle;
            triangle << p1 << p2 << p3;
            painter->drawPolygon(triangle);

            // Рисуем Восклицательный знак "!"
            painter->setPen(Qt::black);
            QFont f = painter->font();
            f.setBold(true);
            f.setPixelSize(16);
            painter->setFont(f);

            // Центрируем знак внутри треугольника
            QRectF textRect(p3.x(), p1.y(), size, size);
            painter->drawText(textRect, Qt::AlignCenter, "!");

            painter->restore();
        }
}

// --- ОБРАБОТКА СОБЫТИЙ ---

void DiagramItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    // 1. Координата клика по вертикали (относительно верхнего края блока)
    qreal y = event->pos().y();
    int section = 0; // По умолчанию (0) - считаем, что клик в Заголовок или это спецблок

    // 2. Берем безопасную математику из вашей функции paint()
    qreal headerH = 30.0;
    qreal remainingH = rect().height() - headerH;

    // ЗАЩИТА ДЛЯ СПЕЦБЛОКОВ: Если блок слишком маленький (меньше 30px),
    // у него нет нижних секций.
    if (remainingH < 0) {
        remainingH = 0;
    }

    qreal section2H = remainingH / 2.0;
    if (section2H < 15.0 && remainingH > 0) {
        section2H = remainingH / 2.0;
    }

    // 3. Вычисляем зону (только если у блока РЕАЛЬНО есть пространство под секции)
    if (remainingH > 0) {
        if (y > headerH && y <= headerH + section2H) {
            section = 1; // Кликнули в onEnter
        } else if (y > headerH + section2H) {
            section = 2; // Кликнули в onExit
        }
    }

    // 4. Открываем окно с учетом зоны (для спецблоков всегда передастся 0)
    openPropertiesDialog(section);
    event->accept();
}

void DiagramItem::openPropertiesDialog(int focusSection)
{
    PropertiesDialog::Data data;
    data.type = m_type;
    data.description = m_description;
    data.label = m_label;
    data.onEnter = m_onEnter;
    data.onExit = m_onExit;
    data.clickedSection = focusSection;

    QWidget *parentWidget = nullptr;
    if (scene() && !scene()->views().isEmpty()) {
        parentWidget = scene()->views().first();
    }

    PropertiesDialog dialog(data, parentWidget);
    if (dialog.exec() == QDialog::Accepted) {
        PropertiesDialog::Data res = dialog.getData();

        // Применяем изменения
        prepareGeometryChange(); // На всякий случай
        m_type = res.type;
        m_description = res.description;
        m_label = res.label;
        m_onEnter = res.onEnter;
        m_onExit = res.onExit;

        // Подгоняем размер и привязываем к сетке
        adjustSizeToContent();
        finalizeResize();

        update();
        // --- ДАТЧИК 4: Изменили текст в блоке ---
                if (scene() && !scene()->views().isEmpty()) {
                    if (DiagramView *view = dynamic_cast<DiagramView*>(scene()->views().first())) {
                        emit view->sceneChanged(); // Искусственно дергаем сигнал холста!
                    }
                }
    }
}

void DiagramItem::adjustSizeToContent()
{
    // Простой расчет высоты (как у вас было)
    qreal hHeader = 30;

    // Если текста много, увеличиваем
    qreal hEnter = (m_onEnter.length() > 25) ? 50 : 30;
    qreal hExit = (m_onExit.length() > 25) ? 50 : 30;

    qreal totalH = hHeader + hEnter + hExit;

    // Округляем высоту до ближайшего шага сетки (16.0).
    const qreal step = 16.0;
    qreal newH = std::ceil(totalH / step) * step;

    QRectF oldRect = rect(); // <-- СОХРАНЯЕМ СТАРЫЙ РАЗМЕР
    QRectF newRect(0, 0, rect().width(), newH);

    // Защита от лишних перерисовок
    if (oldRect == newRect) return;

    // 1. СНАЧАЛА меняем размер самого блока
    prepareGeometryChange();
    setRect(newRect);

    // 2. И ТОЛЬКО ПОТОМ обновляем стрелки (они увидят уже новый размер блока)
    updateConnectionsAfterResize(oldRect, newRect);

}

// --- СОХРАНЕНИЕ СОСТОЯНИЯ ---

void DiagramItem::saveState() {
    m_state.rect = rect();
    m_state.pos = pos();
    // Сохраняем новые поля
    m_state.type = m_type;
    m_state.description = m_description;
    m_state.label = m_label;
    m_state.onEnter = m_onEnter;
    m_state.onExit = m_onExit;
}

void DiagramItem::restoreState() {
    prepareGeometryChange();
    setRect(m_state.rect);
    setPos(m_state.pos);
    // Восстанавливаем новые поля
    m_type = m_state.type;
    m_description = m_state.description;
    m_label = m_state.label;
    m_onEnter = m_state.onEnter;
    m_onExit = m_state.onExit;

    updateHandlesPosition();
    update();
}

// --- ОСТАЛЬНЫЕ МЕТОДЫ (Без изменений логики) ---

QVariant DiagramItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionChange && scene())
    {

        if (m_showAnchorPoints) {
           if (!m_updatingAnchors) {
               m_updatingAnchors = true;
               updateAnchorPoints();
               m_updatingAnchors = false;
           }
        }
        for (ConnectionItem *conn : m_connections) {
            if (conn) conn->updatePosition();
        }
    }

    if (change == QGraphicsItem::ItemTransformChange) {
        updateResizeHandles();
    }

    if (change == ItemPositionHasChanged && scene()) {
        for (ConnectionItem *conn : m_connections) {
            if (conn) conn->updatePosition();
        }
        generateAnchorPoints();
        update();
    }

    return QGraphicsRectItem::itemChange(change, value);
}

QPointF DiagramItem::snapToGridLocal(const QPointF& pt) const {
    if (!m_grid || !scene()) return pt;
    QPointF scenePt = mapToScene(pt);
    QPointF snapped = m_grid->snapToGrid(scenePt);
    return mapFromScene(snapped);
}

void DiagramItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsRectItem::mouseReleaseEvent(event);

    if (m_grid) {
        qreal step = m_grid->getStep();
        if (step > 0.0) {
            QRectF r = rect();
            qreal newW = std::round(r.width() / step) * step;
            qreal newH = std::round(r.height() / step) * step;

            const qreal minW = 40.0;
            const qreal minH = 24.0;
            if (newW < minW) newW = qMax(minW, r.width());
            if (newH < minH) newH = qMax(minH, r.height());

            prepareResize();
            QRectF oldRect = rect();
            setRect(0, 0, newW, newH);

            updateConnectionsAfterResize(oldRect, rect());
            QPointF snappedTopLeft = m_grid->snapToGrid(pos());
            setPos(snappedTopLeft);

            updateHandlesPosition();
            updateResizeHandles();
        }
    }
}


void DiagramItem::onResizedByHandle() {
    // Можно вызвать adjustSizeToContent(), но при ручном ресайзе лучше не менять размер автоматом
    // updateHandlesPosition() уже вызывается в ResizeHandle
}

void DiagramItem::showResizeHandles()
{
    if (m_handlesVisible) return;
    if (m_handles.isEmpty()) {
        for(int i=0; i<8; ++i)
            m_handles << new ResizeHandle(static_cast<ResizeHandle::Position>(i), this, this);
    }
    for (ResizeHandle *h : m_handles) {
        h->show();
        h->setVisible(true);
    }
    updateHandlesPosition();
    m_handlesVisible = true;
}

void DiagramItem::hideResizeHandles()
{
    if (!m_handlesVisible) return;
    for (ResizeHandle *h : m_handles) h->setVisible(false);
    m_handlesVisible = false;
}

void DiagramItem::updateHandlesPosition()
{
    QRectF r = rect();
    qreal w = r.width();
    qreal h = r.height();
    if (m_handles.size() < 8) return;

    m_handles[0]->setPos(0, h/2);
    m_handles[1]->setPos(w, h/2);
    m_handles[2]->setPos(w/2, 0);
    m_handles[3]->setPos(w/2, h);
    m_handles[4]->setPos(0, 0);
    m_handles[5]->setPos(w, 0);
    m_handles[6]->setPos(0, h);
    m_handles[7]->setPos(w, h);
}

void DiagramItem::updateResizeHandles()
{
    for (ResizeHandle* handle : m_handles) {
        handle->updatePosition();
    }
}

void DiagramItem::resizeFromHandle(Qt::Corner corner, QPointF delta)
{
    prepareGeometryChange();
    QRectF oldRect = rect(); // <-- СОХРАНЯЕМ СТАРЫЙ РАЗМЕР
    QRectF r = rect();
    switch (corner) {
            case Qt::TopLeftCorner: r.setTopLeft(r.topLeft() + delta); break;
            case Qt::TopRightCorner: r.setTopRight(r.topRight() + delta); break;
            case Qt::BottomLeftCorner: r.setBottomLeft(r.bottomLeft() + delta); break;
            case Qt::BottomRightCorner: r.setBottomRight(r.bottomRight() + delta); break;
    }
    setRect(r);
    updateConnectionsAfterResize(oldRect, r);
    updateHandlesPosition();
    updateResizeHandles();
}

void DiagramItem::finalizeResize()
{
    if (!m_grid) {
        updateHandlesPosition();
        updateResizeHandles();
        return;
    }
    qreal step = m_grid->getStep();
    if (step <= 0.0) {
        updateHandlesPosition();
        updateResizeHandles();
        return;
    }
    if (m_showAnchorPoints) updateAnchorPoints();

    QPointF snappedPos = m_grid->snapToGrid(pos());
    prepareGeometryChange();
    setPos(snappedPos);

    updateHandlesPosition();
    updateResizeHandles();
    generateAnchorPoints();
    update();
}

// --- РАБОТА С ЯКОРЯМИ И СВЯЗЯМИ (Без изменений) ---

void DiagramItem::generateAnchorPoints()
{
    m_anchorPoints.clear();
    QRectF r = rect();
    qreal step = m_grid ? m_grid->getStep() : 16.0;

    for (qreal x = step; x < r.width(); x += step) {
        m_anchorPoints.append(snapToGridLocal(QPointF(x, 0)));
        m_anchorPoints.append(snapToGridLocal(QPointF(x, r.height())));
    }
    for (qreal y = step; y < r.height(); y += step) {
        m_anchorPoints.append(snapToGridLocal(QPointF(0, y)));
        m_anchorPoints.append(snapToGridLocal(QPointF(r.width(), y)));
    }
}

void DiagramItem::setShowAnchorPoints(bool show)
{
    m_showAnchorPoints = show;
    update();
}

void DiagramItem::updateAnchorPoints()
{
    generateAnchorPoints();
    update();
}

QPointF DiagramItem::nearestAnchorToScenePoint(const QPointF &scenePos, qreal *outDist) const
{
    QPointF bestLocal;
    qreal bestDist = std::numeric_limits<qreal>::infinity();

    if (m_anchorPoints.isEmpty()) {
        if (outDist) *outDist = bestDist;
        return bestLocal;
    }

    for (const QPointF &localPt : m_anchorPoints) {
        QPointF scenePt = mapToScene(localPt);
        qreal d = QLineF(scenePt, scenePos).length();
        if (d < bestDist) {
            bestDist = d;
            bestLocal = localPt;
        }
    }
    if (outDist) *outDist = bestDist;
    return bestLocal;
}

void DiagramItem::setAnchorHighlighted(const QPointF &localAnchor, bool highlighted)
{
    if (highlighted) {
        if (!m_hasHighlightedAnchor || m_highlightedAnchor != localAnchor) {
            m_hasHighlightedAnchor = true;
            m_highlightedAnchor = localAnchor;
            update();
        }
    } else {
        if (m_hasHighlightedAnchor) {
            m_hasHighlightedAnchor = false;
            update();
        }
    }
}

void DiagramItem::addConnection(ConnectionItem *conn)
{
    if (conn && !m_connections.contains(conn)) {
        m_connections.append(conn);
    }
}

void DiagramItem::removeConnection(ConnectionItem *conn)
{
    if (m_connections.removeOne(conn)) {
        updateAllConnections();
    }
}

void DiagramItem::removeAllConnections()
{
    QList<ConnectionItem*> conns = m_connections;
    m_connections.clear();

    for (ConnectionItem *conn : conns) {
        if (!conn) continue;
        DiagramItem* start = conn->startBlock();
        DiagramItem* end = conn->endBlock();
        if (start && start != this) start->removeConnection(conn);
        if (end && end != this) end->removeConnection(conn);
        conn->setStart(nullptr, QPointF());
        conn->setEnd(nullptr, QPointF());
        if (conn->scene()) conn->scene()->removeItem(conn);
        delete conn;
    }
}

bool DiagramItem::isAnchorFree(const QPointF &localAnchor) const
{
    auto aeq = [](const QPointF& p1, const QPointF& p2) {
        return qFuzzyCompare(p1.x(), p2.x()) && qFuzzyCompare(p1.y(), p2.y());
    };
    for (const ConnectionItem *conn : m_connections) {
        if (!conn) continue;
        if (conn->startBlock() == this && aeq(conn->startAnchorLocal(), localAnchor)) return false;
        if (conn->endBlock() == this && aeq(conn->endAnchorLocal(), localAnchor)) return false;
    }
    return true;
}

QRectF DiagramItem::getOccupiedAnchorRect() const
{
    if (m_connections.isEmpty()) return QRectF();
    qreal minX = std::numeric_limits<qreal>::max();
    qreal minY = std::numeric_limits<qreal>::max();
    qreal maxX = std::numeric_limits<qreal>::lowest();
    qreal maxY = std::numeric_limits<qreal>::lowest();
    bool foundAny = false;

    for (const ConnectionItem *conn : m_connections) {
        if (!conn) continue;
        QPointF pt;
        bool found = false;
        if (conn->startBlock() == this) { pt = conn->startAnchorLocal(); found = true; }
        else if (conn->endBlock() == this) { pt = conn->endAnchorLocal(); found = true; }

        if (found) {
            if (pt.x() < minX) minX = pt.x();
            if (pt.x() > maxX) maxX = pt.x();
            if (pt.y() < minY) minY = pt.y();
            if (pt.y() > maxY) maxY = pt.y();
            foundAny = true;
        }
    }
    if (!foundAny) return QRectF();
    QRectF r(QPointF(minX, minY), QPointF(maxX, maxY));
    return r.adjusted(-16.0, -16.0, 16.0, 16.0);
}

void DiagramItem::shiftAnchors(qreal dx, qreal dy) {
    if (qFuzzyIsNull(dx) && qFuzzyIsNull(dy)) return;
    for (ConnectionItem *conn : m_connections) {
        if (!conn) continue;
        if (conn->startBlock() == this) conn->setStart(this, conn->startAnchorLocal() - QPointF(dx, dy));
        if (conn->endBlock() == this) conn->setEnd(this, conn->endAnchorLocal() - QPointF(dx, dy));
        conn->updatePosition();
    }
}

int DiagramItem::getSideForAnchor(const QPointF &localAnchor) const
{
    QRectF r = rect();
    qreal eps = 8.0;
    if (qAbs(localAnchor.y() - r.top()) <= eps) return 0;
    if (qAbs(localAnchor.x() - r.right()) <= eps) return 1;
    if (qAbs(localAnchor.y() - r.bottom()) <= eps) return 2;
    if (qAbs(localAnchor.x() - r.left()) <= eps) return 3;
    return -1;
}

struct ConnectionSortEntry {
    ConnectionItem* conn;
    int side;
    qreal pos;
    static bool compare(const ConnectionSortEntry& a, const ConnectionSortEntry& b) {
        if (a.side != b.side) return a.side < b.side;
        if (!qFuzzyCompare(a.pos, b.pos)) return a.pos < b.pos;
        return a.conn < b.conn;
    }
};

int DiagramItem::getConnectionRank(ConnectionItem* targetConn) const
{
    QPointF targetAnchor;
    if (targetConn->startBlock() == this) targetAnchor = targetConn->startAnchorLocal();
    else if (targetConn->endBlock() == this) targetAnchor = targetConn->endAnchorLocal();
    else return 0;

    int targetSide = getSideForAnchor(targetAnchor);
    if (targetSide == -1) return 0;

    QList<ConnectionSortEntry> sideConnections;
    bool targetFound = false;

    for (ConnectionItem* conn : m_connections) {
        if (conn == targetConn) targetFound = true;
        QPointF otherAnchor;
        if (conn->startBlock() == this) otherAnchor = conn->startAnchorLocal();
        else if (conn->endBlock() == this) otherAnchor = conn->endAnchorLocal();
        else continue;

        int side = getSideForAnchor(otherAnchor);
        if (side == targetSide) {
            ConnectionSortEntry entry;
            entry.conn = conn;
            entry.side = side;
            entry.pos = (side == 0 || side == 2) ? otherAnchor.x() : otherAnchor.y();
            sideConnections.append(entry);
        }
    }

    if (!targetFound) {
        ConnectionSortEntry entry;
        entry.conn = targetConn;
        entry.side = targetSide;
        entry.pos = (targetSide == 0 || targetSide == 2) ? targetAnchor.x() : targetAnchor.y();
        sideConnections.append(entry);
    }

    std::sort(sideConnections.begin(), sideConnections.end(), ConnectionSortEntry::compare);

    for (int i = 0; i < sideConnections.count(); ++i) {
        if (sideConnections[i].conn == targetConn) return i;
    }
    return 0;
}

void DiagramItem::updateAllConnections()
{
    QList<ConnectionItem*> conns = m_connections;
    for (ConnectionItem* conn : conns) {
        if (conn) conn->updatePosition();
    }
}

int DiagramItem::getGlobalRank(ConnectionItem* targetConn) const
{
    QList<ConnectionSortEntry> allConnections;
    bool targetFound = false;

    for (ConnectionItem* conn : m_connections) {
        if (conn == targetConn) targetFound = true;
        QPointF anchor;
        if (conn->startBlock() == this) anchor = conn->startAnchorLocal();
        else if (conn->endBlock() == this) anchor = conn->endAnchorLocal();
        else continue;

        int side = getSideForAnchor(anchor);
        allConnections.append({conn, side, (side == 0 || side == 2) ? anchor.x() : anchor.y()});
    }

    if (!targetFound) {
        QPointF targetAnchor;
        if (targetConn->startBlock() == this) targetAnchor = targetConn->startAnchorLocal();
        else targetAnchor = targetConn->endAnchorLocal();
        int side = getSideForAnchor(targetAnchor);
        allConnections.append({targetConn, side, (side == 0 || side == 2) ? targetAnchor.x() : targetAnchor.y()});
    }

    std::sort(allConnections.begin(), allConnections.end(), ConnectionSortEntry::compare);

    for (int i = 0; i < allConnections.count(); ++i) {
        if (allConnections[i].conn == targetConn) return i;
    }
    return 0;
}
int DiagramItem::getNextPriority() const
{
    int maxPrio = 0;

    // Проходим по всем связям, подключенным к этому блоку
    for (ConnectionItem *conn : m_connections) {
        if (!conn) continue;

        // Нас интересуют только ИСХОДЯЩИЕ связи (где старт == этот блок)
        // Это покрывает и А->Б, и петлю А->А
        if (conn->startBlock() == this) {

            // Если у связи есть TransitionGroup (желтые блоки/приоритет)
            if (TransitionGroup *tg = conn->transition()) {
                int p = tg->getPriority();
                if (p > maxPrio) {
                    maxPrio = p;
                }
            }
        }
    }

    // Возвращаем (максимальный найденный + 1).
    // Если связей нет, maxPrio=0, вернем 1.
    return maxPrio + 1;
}
void DiagramItem::setLabel(const QString &text)
{
    m_label = text; // Сохраняем текст в переменную
    update();       // Перерисовываем блок, чтобы текст отобразился
}
void DiagramItem::setError(const QString &message)
{
    m_isError = true;
    setBrush(Qt::red);             // Красим фон в красный
    setToolTip(QObject::tr("ОШИБКА: %1").arg(message)); // Всплывающая подсказка
    update();                      // Перерисовать
}

void DiagramItem::clearError()
{
    m_isError = false;
    setBrush(Qt::white);           // Возвращаем белый цвет
    setToolTip("");                // Убираем подсказку
    update();
}
// =======================================================================
// УМНОЕ ОБНОВЛЕНИЕ СТРЕЛОК ПРИ ИЗМЕНЕНИИ РАЗМЕРА БЛОКА
// =======================================================================
void DiagramItem::updateConnectionsAfterResize(const QRectF &oldRect, const QRectF &newRect, const QPointF &posShift)
{
    if (oldRect == newRect && posShift.isNull()) return;

    for (ConnectionItem *conn : m_connections) {
        if (!conn) continue;

        auto processAnchor = [&](QPointF oldLocal) -> QPointF {
            QPointF newLocal = oldLocal;
            const qreal eps = 2.0;

            // 1. Компенсируем сдвиг всего блока при растягивании за левый/верхний край
            newLocal -= posShift;

            // 2. Липнем к границам по старым координатам
            if (qAbs(oldLocal.x() - oldRect.right()) < eps) newLocal.setX(newRect.right());
            else if (qAbs(oldLocal.x() - oldRect.left()) < eps) newLocal.setX(newRect.left());

            if (qAbs(oldLocal.y() - oldRect.bottom()) < eps) newLocal.setY(newRect.bottom());
            else if (qAbs(oldLocal.y() - oldRect.top()) < eps) newLocal.setY(newRect.top());

            return newLocal;
        };

        bool changed = false;

        if (conn->startBlock() == this) {
            QPointF newA = processAnchor(conn->startAnchorLocal());
            if (newA != conn->startAnchorLocal()) {
                conn->setStart(this, newA);
                changed = true;
            }
        }
        if (conn->endBlock() == this) {
            QPointF newA = processAnchor(conn->endAnchorLocal());
            if (newA != conn->endAnchorLocal()) {
                conn->setEnd(this, newA);
                changed = true;
            }
        }

        if (changed) conn->updatePosition();
    }
}
