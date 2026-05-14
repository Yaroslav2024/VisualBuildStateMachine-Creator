/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "connect.h"
#include "diagramitem.h"
#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <cmath>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPainter>                     // <-- Для рисования
#include <QStyleOptionGraphicsItem>     // <-- Для проверки 'state' (выбран/не выбран)
#include <limits> // <--- ДОБАВЬТЕ ЭТОТ INCLUDE
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include "bridgeitem.h"
#include <QMessageBox>
#include "historyitem.h"
#include "backitem.h"
#include <QApplication>
#include "utils.h"
static QPointF snapToGrid(const QPointF &pos, qreal step)
{
    if (step <= 0.0) return pos;
    qreal x = std::round(pos.x() / step) * step;
    qreal y = std::round(pos.y() / step) * step;
    return QPointF(x, y);
}
// Реализация методов ElementsRegistry
ElementsRegistry::ElementsRegistry(QObject *parent)
    : QObject(parent)
{
    // Инициализация при необходимости
}

ElementsRegistry::~ElementsRegistry()
{
    // Очистка: удалим ссылки на QGraphicsItems (но не их сами — владельцы сцены управляют жизненным циклом)
    m_anchors.clear();
    m_lines.clear();
    m_arrowHeads.clear();
    m_phantomLines.clear();
    m_phantomPoints.clear();
    m_phantomArrows.clear();
    m_registrations.clear();
}

// -------------------- Helpers --------------------
static ElementsRegistry::Id makeId()
{
    return QUuid::createUuid();
}

static int indexOfId(const QList<ElementsRegistry::AnchorInfo> &list, const ElementsRegistry::Id &id)
{
    for (int i = 0; i < list.size(); ++i) if (list[i].id == id) return i;
    return -1;
}
static int indexOfId(const QList<ElementsRegistry::LineInfo> &list, const ElementsRegistry::Id &id)
{
    for (int i = 0; i < list.size(); ++i) if (list[i].id == id) return i;
    return -1;
}
static int indexOfId(const QList<ElementsRegistry::ArrowHeadInfo> &list, const ElementsRegistry::Id &id)
{
    for (int i = 0; i < list.size(); ++i) if (list[i].id == id) return i;
    return -1;
}

// -------------------- Anchors --------------------
ElementsRegistry::Id ElementsRegistry::registerAnchor(const AnchorInfo &info)
{
    AnchorInfo copy = info;
    if (copy.id.isNull()) copy.id = makeId();
    m_anchors.append(copy);
    emit anchorRegistered(copy);
    return copy.id;
}

bool ElementsRegistry::unregisterAnchor(const Id &anchorId)
{
    int idx = indexOfId(m_anchors, anchorId);
    if (idx == -1) return false;
    m_anchors.removeAt(idx);
    emit anchorUnregistered(anchorId);
    return true;
}

ElementsRegistry::AnchorInfo ElementsRegistry::findAnchor(const Id &anchorId) const
{
    for (const AnchorInfo &a : m_anchors) {
        if (a.id == anchorId) return a;
    }
    return AnchorInfo(); // пустая структура
}

QList<ElementsRegistry::AnchorInfo> ElementsRegistry::anchors() const
{
    return m_anchors;
}

// -------------------- Lines --------------------
ElementsRegistry::Id ElementsRegistry::registerLine(const LineInfo &info)
{
    LineInfo copy = info;
    if (copy.id.isNull()) copy.id = makeId();
    m_lines.append(copy);
    emit lineRegistered(copy);
    return copy.id;
}

bool ElementsRegistry::unregisterLine(const Id &lineId)
{
    int idx = indexOfId(m_lines, lineId);
    if (idx == -1) return false;
    m_lines.removeAt(idx);
    emit lineUnregistered(lineId);
    return true;
}

ElementsRegistry::LineInfo ElementsRegistry::findLine(const Id &lineId) const
{
    for (const LineInfo &l : m_lines) {
        if (l.id == lineId) return l;
    }
    return LineInfo();
}

QList<ElementsRegistry::LineInfo> ElementsRegistry::lines() const
{
    return m_lines;
}

// -------------------- ArrowHeads --------------------
ElementsRegistry::Id ElementsRegistry::registerArrowHead(const ArrowHeadInfo &info)
{
    ArrowHeadInfo copy = info;
    if (copy.id.isNull()) copy.id = makeId();
    m_arrowHeads.append(copy);
    emit arrowRegistered(copy);
    return copy.id;
}

bool ElementsRegistry::unregisterArrowHead(const Id &arrowId)
{
    int idx = indexOfId(m_arrowHeads, arrowId);
    if (idx == -1) return false;
    m_arrowHeads.removeAt(idx);
    emit arrowUnregistered(arrowId);
    return true;
}

ElementsRegistry::ArrowHeadInfo ElementsRegistry::findArrowHead(const Id &arrowId) const
{
    for (const ArrowHeadInfo &a : m_arrowHeads) {
        if (a.id == arrowId) return a;
    }
    return ArrowHeadInfo();
}

QList<ElementsRegistry::ArrowHeadInfo> ElementsRegistry::arrowHeads() const
{
    return m_arrowHeads;
}

// -------------------- Phantom objects --------------------
ElementsRegistry::Id ElementsRegistry::createPhantomLine(const PhantomLineInfo &info)
{
    PhantomLineInfo copy = info;
    if (copy.id.isNull()) copy.id = makeId();
    m_phantomLines.append(copy);
    emit phantomCreated(copy.id);
    return copy.id;
}

bool ElementsRegistry::removePhantomLine(const Id &phantomId)
{
    for (int i = 0; i < m_phantomLines.size(); ++i) {
        if (m_phantomLines[i].id == phantomId) {
            m_phantomLines.removeAt(i);
            emit phantomRemoved(phantomId);
            return true;
        }
    }
    return false;
}

QList<ElementsRegistry::PhantomLineInfo> ElementsRegistry::phantomLines() const
{
    return m_phantomLines;
}

ElementsRegistry::Id ElementsRegistry::createPhantomPoint(const PhantomPointInfo &info)
{
    PhantomPointInfo copy = info;
    if (copy.id.isNull()) copy.id = makeId();
    m_phantomPoints.append(copy);
    emit phantomCreated(copy.id);
    return copy.id;
}

bool ElementsRegistry::removePhantomPoint(const Id &phantomId)
{
    for (int i = 0; i < m_phantomPoints.size(); ++i) {
        if (m_phantomPoints[i].id == phantomId) {
            m_phantomPoints.removeAt(i);
            emit phantomRemoved(phantomId);
            return true;
        }
    }
    return false;
}

QList<ElementsRegistry::PhantomPointInfo> ElementsRegistry::phantomPoints() const
{
    return m_phantomPoints;
}

ElementsRegistry::Id ElementsRegistry::createPhantomArrow(const PhantomArrowInfo &info)
{
    PhantomArrowInfo copy = info;
    if (copy.id.isNull()) copy.id = makeId();
    m_phantomArrows.append(copy);
    emit phantomCreated(copy.id);
    return copy.id;
}

bool ElementsRegistry::removePhantomArrow(const Id &phantomId)
{
    for (int i = 0; i < m_phantomArrows.size(); ++i) {
        if (m_phantomArrows[i].id == phantomId) {
            m_phantomArrows.removeAt(i);
            emit phantomRemoved(phantomId);
            return true;
        }
    }
    return false;
}

QList<ElementsRegistry::PhantomArrowInfo> ElementsRegistry::phantomArrows() const
{
    return m_phantomArrows;
}

// -------------------- Registrations --------------------
void ElementsRegistry::addRegistration(const RegistrationEntry &entry)
{
    m_registrations.append(entry);
    emit registrationAdded(entry);
}

bool ElementsRegistry::removeRegistration(const Id &regId)
{
    for (int i = 0; i < m_registrations.size(); ++i) {
        if (m_registrations[i].id == regId) {
            m_registrations.removeAt(i);
            emit registrationRemoved(regId);
            return true;
        }
    }
    return false;
}

QList<ElementsRegistry::RegistrationEntry> ElementsRegistry::registrations() const
{
    return m_registrations;
}

// -------------------- Attach / Detach anchors to blocks --------------------
bool ElementsRegistry::attachAnchorToBlock(const Id &anchorId, DiagramItem *block)
{
    for (AnchorInfo &a : m_anchors) {
        if (a.id == anchorId) {
            a.attachedBlock = block;
            return true;
        }
    }
    return false;
}

bool ElementsRegistry::detachAnchorFromBlock(const Id &anchorId)
{
    for (AnchorInfo &a : m_anchors) {
        if (a.id == anchorId) {
            a.attachedBlock = nullptr; // <-- было a.attachedBlock.clear();
            return true;
        }
    }
    return false;
}


QList<ElementsRegistry::Id> ElementsRegistry::anchorsForBlock(DiagramItem *block) const
{
    QList<Id> out;
    for (const AnchorInfo &a : m_anchors) {
        if (a.attachedBlock == block) out.append(a.id); // убрали лишнюю проверку a.attachedBlock &&
    }
    return out;
}

// -------------------- State save / restore --------------------
ElementsRegistry::RegistryState ElementsRegistry::saveState() const
{
    RegistryState st;
    st.anchors = m_anchors;
    st.lines = m_lines;
    st.arrows = m_arrowHeads;
    st.phantomLines = m_phantomLines;
    st.phantomPoints = m_phantomPoints;
    st.phantomArrows = m_phantomArrows;
    st.registrations = m_registrations;
    st.globalMeta = QVariantMap();
    return st;
}

void ElementsRegistry::restoreState(const RegistryState &state)
{
    m_anchors = state.anchors;
    m_lines = state.lines;
    m_arrowHeads = state.arrows;
    m_phantomLines = state.phantomLines;
    m_phantomPoints = state.phantomPoints;
    m_phantomArrows = state.phantomArrows;
    m_registrations = state.registrations;
    // не посылаем сигналы массово — можно добавить при необходимости
}

// -------------------- Clear helpers --------------------
void ElementsRegistry::clearAllPhantoms()
{
    // удаляем только временные объекты
    m_phantomLines.clear();
    m_phantomPoints.clear();
    m_phantomArrows.clear();
    // оповестим слушателей (можно отправить отдельные сигналы, но здесь простой вариант)
    // посылаем сигнал с пустым id как уведомление — или добавить новый сигнал при необходимости
    // emit phantomRemoved(...);
}

void ElementsRegistry::clearAll()
{
    // Полная очистка (включая постоянные записи)
    m_anchors.clear();
    m_lines.clear();
    m_arrowHeads.clear();
    m_phantomLines.clear();
    m_phantomPoints.clear();
    m_phantomArrows.clear();
    m_registrations.clear();
    // Можно посылать события, если нужно
}


// Реализация метода Connector::createArrowHead
// Реализация метода Connector::createArrowHead

Connector::Connector(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), m_scene(scene)
{
}

Connector::~Connector()
{
    stop();
}
// *** ДОБАВИТЬ ЭТОТ НОВЫЙ ПРИВАТНЫЙ МЕТОД ***
void Connector::initConnectionMode()
{
    m_active = true;
    m_justStopped = false;

    m_hasStartNextSegment = false;
    m_nextSegmentStart = QPointF();

    m_hasStart = false;
    m_startScene = QPointF();
    m_startLocal = QPointF();
    m_startBlock = nullptr; // По умолчанию null
m_lastSegmentWasHorizontal = false; // Сбрасываем ориентацию
    // *** ДОБАВИТЬ ЭТИ СТРОКИ ***
        m_originBlock = nullptr;
        m_originLocal = QPointF();
        // *** КОНЕЦ ***

    // ensure preview items exist lazily
    if (!m_previewLine2) {
        m_previewLine2 = nullptr;
    }
    if (!m_previewBendHighlight) {
        m_previewBendHighlight = nullptr;
    }
    if (!m_previewEndHighlight) {
        m_previewEndHighlight = nullptr;
    }
}

// *** ДОБАВИТЬ ЭТОТ НОВЫЙ ПУБЛИЧНЫЙ МЕТОД ***
void Connector::startFrom(DiagramItem *startBlock)
{
    initConnectionMode(); // Вызываем общую инициализацию

    // *** Ключевое отличие ***
    if (startBlock) {
        // --- НОВАЯ ПРОВЕРКА ---
                if (dynamic_cast<BridgeItem*>(startBlock)) {
                    QMessageBox::warning(nullptr, "Запрет", "Мост не может иметь исходящих связей.");
                    stop(); // Останавливаем режим
                    return;
                }
                // ----------------------
                // ==========================================
                        // *** НОВАЯ ПРОВЕРКА ДЛЯ ИСТОРИИ ***
                        // Запрещаем рисовать вторую стрелку, если одна уже есть
                        if (HistoryItem* hist = dynamic_cast<HistoryItem*>(startBlock)) {
                            int outCount = 0;
                            if (m_scene) {
                                for (QGraphicsItem *item : m_scene->items()) {
                                    if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
                                        if (conn->startBlock() == hist) outCount++;
                                    }
                                }
                            }
                            if (outCount >= 1) {
                                QMessageBox::warning(nullptr, "Запрет", "Блок 'История' может иметь только ОДНУ исходящую стрелку!");
                                stop();
                                return;
                            }
                        }
                        // ==========================================
        m_startBlock = startBlock; // Сразу устанавливаем стартовый блок
        m_originBlock = startBlock;
        m_hasStart = true; // Сообщаем, что блок у нас уже есть

        // Показываем якоря (старая логика)
        if (m_scene) {
            for (QGraphicsItem *it : m_scene->items()) {
                if (DiagramItem *d = dynamic_cast<DiagramItem*>(it)) {
                    d->generateAnchorPoints();
                    d->setShowAnchorPoints(true);
                }
            }
        }
    } else {
        // Если вдруг передали null, ведем себя как обычно
        start();
    }
}

void Connector::start()
{
    initConnectionMode(); // <-- ЗАМЕНИТЬ все тело метода на эту строку
    // Показываем anchor-поинты (синие точки) на всех DiagramItem в сцене,
    // чтобы пользователь видел куда можно прилипнуть при построении стрелки.
    if (m_scene) {
        for (QGraphicsItem *it : m_scene->items()) {
            if (DiagramItem *d = dynamic_cast<DiagramItem*>(it)) {
                d->generateAnchorPoints();
                d->setShowAnchorPoints(true);
            }
        }
    }
}




void Connector::stop()
{
    m_active = false;
    m_hasStart = false;
    m_startBlock = nullptr;
    m_lastSegmentWasHorizontal = false; // <--- ДОБАВЬТЕ ЭТУ СТРОКУ
    // Спрячем anchor-поинты у всех блоков (выйдя из режима рисования)
        if (m_scene) {
            for (QGraphicsItem *it : m_scene->items()) {
                if (DiagramItem *d = dynamic_cast<DiagramItem*>(it)) {
                    d->setShowAnchorPoints(false);
                }
            }
        }
    // удалить превью-объекты (если есть)
    if (m_previewLine && m_scene) {
        m_scene->removeItem(m_previewLine);
        delete m_previewLine;
        m_previewLine = nullptr;
    }
    if (m_previewLine2 && m_scene) {
        m_scene->removeItem(m_previewLine2);
        delete m_previewLine2;
        m_previewLine2 = nullptr;
    }
    if (m_previewStartHighlight && m_scene) {
        m_scene->removeItem(m_previewStartHighlight);
        delete m_previewStartHighlight;
        m_previewStartHighlight = nullptr;
    }
    if (m_previewBendHighlight && m_scene) {
        m_scene->removeItem(m_previewBendHighlight);
        delete m_previewBendHighlight;
        m_previewBendHighlight = nullptr;
    }
    if (m_previewEndHighlight && m_scene) {
        m_scene->removeItem(m_previewEndHighlight);
        delete m_previewEndHighlight;
        m_previewEndHighlight = nullptr;
    }

    // не удаляем m_committedItems здесь — удаление выполняется при ПКМ (полностью)
}


// --- НАЧАЛО ФРАГМЕНТА ---

void Connector::onMouseMove(const QPointF &scenePos)
{
    if (!m_active || !m_scene) return;

    // --- ИЗМЕНЕНИЕ ЛОГИКИ ВЫБОРА СТАРТА ---
    // Если m_startBlock УЖЕ УСТАНОВЛЕН (через startFrom),
    // нам не нужно искать его по selection.
    if (!m_hasStart && !m_hasStartNextSegment)
    {
        // m_startBlock ЕЩЕ НЕ УСТАНОВЛЕН (вызвали start(), а не startFrom)
        // Ищем его по selection (старая логика)
        DiagramItem *selectedBlock = nullptr;
        for (QGraphicsItem *it : m_scene->items()) {
            if (DiagramItem *d = dynamic_cast<DiagramItem*>(it)) {
                if (d->isSelected()) { selectedBlock = d; break; }
            }
        }

        if (!selectedBlock) {
            // (старая логика)
            if (m_previewLine) {
                m_scene->removeItem(m_previewLine);
                delete m_previewLine; m_previewLine = nullptr;
            }
            if (m_previewStartHighlight) {
                m_scene->removeItem(m_previewStartHighlight);
                delete m_previewStartHighlight; m_previewStartHighlight = nullptr;
            }
            m_startBlock = nullptr;
            m_hasStart = false;
            return;
        }
        m_startBlock = selectedBlock;
        // m_hasStart будет установлен в true чуть ниже
    }
    // --- КОНЕЦ ИЗМЕНЕНИЯ ---


    // --- ВЫБОР ТОЧКИ НАЧАЛА ---
    QPointF startScene;
    if (m_hasStartNextSegment) {
        // если уже есть предыдущий конец — продолжаем от него
        startScene = m_nextSegmentStart;
    } else if (m_startBlock) {
        qreal dist;
        QPointF nearestLocal = m_startBlock->nearestAnchorToScenePoint(scenePos, &dist);
        startScene = m_startBlock->mapToScene(nearestLocal);

        // *** НАЧАЛО ИЗМЕНЕНИЯ (Проверка, свободен ли стартовый якорь) ***
        if (!m_startBlock->isAnchorFree(nearestLocal)) {
            // Якорь занят.
            m_startBlock->setAnchorHighlighted(nearestLocal, false); // Убедимся, что подсветки нет

            // Прячем все элементы превью
            if (m_previewStartHighlight) m_previewStartHighlight->hide();
            if (m_previewLine) m_previewLine->hide();
            if (m_previewLine2) m_previewLine2->hide();
            if (m_previewBendHighlight) m_previewBendHighlight->hide();
            if (m_previewEndHighlight) m_previewEndHighlight->hide();

            m_hasStart = false; // Не разрешаем старт
            return; // Выходим из onMouseMove
        }
        // *** КОНЕЦ ИЗМЕНЕНИЯ ***

        // подсветка активного якоря
        if (m_startBlock) m_startBlock->setAnchorHighlighted(nearestLocal, true);

        // подсветка точки старта на сцене
        if (!m_previewStartHighlight) {
            m_previewStartHighlight = new QGraphicsEllipseItem(-5, -5, 10, 10);
            m_previewStartHighlight->setBrush(QBrush(Qt::red));
            m_previewStartHighlight->setPen(Qt::NoPen);
            m_previewStartHighlight->setZValue(2000);
            m_scene->addItem(m_previewStartHighlight);
        }
        m_previewStartHighlight->show(); // Убедимся, что он видим
        m_previewStartHighlight->setPos(startScene);
        m_startLocal = nearestLocal;
        m_startScene = startScene;
        // Если мы еще не сохранили origin (например, в startFrom), делаем это сейчас
        // (НОВЫЙ КОД в onMouseMove)
                // Если мы еще не сохранили origin (например, в startFrom), делаем это сейчас
                        if (!m_originBlock) {
                            m_originBlock = m_startBlock;
                        }
                        // ОБНОВЛЯЕМ m_originLocal КАЖДЫЙ РАЗ, когда меняется m_startLocal
                        m_originLocal = m_startLocal;
        m_hasStart = true;
    } else {
        return;
    }

    // --- (!!!) УДАЛЕНЫ БЛОКИ snapToGrid, РАСЧЁТ orthoEnd, m_previewLine->setLine() ---

    // --- РАСЧЁТ НАПРАВЛЕНИЯ (только для определения needBend) ---
    qreal dx = scenePos.x() - startScene.x();
    qreal dy = scenePos.y() - startScene.y();
    qreal adx = qAbs(dx);
    qreal ady = qAbs(dy);

    // --- L-образная подсказка (РЕШЕНИЕ О ТИПЕ ПРЕВЬЮ) ---

        // *** ИСПРАВЛЕННАЯ ЛОГИКА ОПРЕДЕЛЕНИЯ ИЗГИБА ***
        bool needBend = false;

        // Проверяем, ушла ли мышь достаточно далеко
        if (adx > m_snapThreshold || ady > m_snapThreshold) {
            if (adx >= ady) {
                // Основное направление - ГОРИЗОНТАЛЬНОЕ
                if (ady > m_secondarySnapThreshold) {
                    // Отклонение по Y слишком большое, нужен изгиб
                    needBend = true;
                }
            } else {
                // Основное направление - ВЕРТИКАЛЬНОЕ
                if (adx > m_secondarySnapThreshold) {
                    // Отклонение по X слишком большое, нужен изгиб
                    needBend = true;
                }
            }
        }



        // (На случай, если они были спрятаны проверкой на занятый якорь)
        if (m_previewLine) m_previewLine->show();
        if (m_previewLine2) m_previewLine2->show();
        if (m_previewBendHighlight) m_previewBendHighlight->show();
        if (m_previewEndHighlight) m_previewEndHighlight->show();


        // --- ВЫЗОВ НУЖНОЙ ФУНКЦИИ РИСОВАНИЯ ---
        if (needBend) {
            drawLShapedPreview(startScene, scenePos);
        } else {
            drawStraightPreview(startScene, scenePos);
        }


}

// --- КОНЕЦ ФРАГМЕНТА ---

void Connector::onMousePress(Qt::MouseButton button, const QPointF &scenePos)
{
    if (!m_active) return;

    // ЛКМ: фиксируем сегмент (если есть старт)
    if (button == Qt::LeftButton) {
        if (m_hasStart && m_scene) {
            // --- НАЧАЛО НОВОГО КОДА (ПРОВЕРКА НА ЗАНЯТУЮ ТОЧКУ ФИНИША) ---
                        //
                        // Проверяем, не находится ли курсор мыши (scenePos) над занятым якорем
                        // ПЕРЕД тем, как создавать какие-либо сегменты.

                        const qreal anchorThreshold = 8.0; // Дистанция "прилипания" (такая же, как при поиске targetBlock)
                        bool endPointIsBlocked = false;

                        if (m_scene) {
                                                    for (QGraphicsItem* it : m_scene->items()) {
                                                        if (DiagramItem* d = dynamic_cast<DiagramItem*>(it)) {

                                                            // 1. Пропускаем, если это тот же блок, с которого начали (для старта петли)
                                                            if (d == m_originBlock && !m_hasStartNextSegment) {
                                                                continue;
                                                            }

                                                            // 2. СНАЧАЛА проверяем дистанцию!
                                                            // Если мышь далеко от этого блока, переходим к следующему.
                                                            qreal anchorDist;
                                                            QPointF nearestLocal = d->nearestAnchorToScenePoint(scenePos, &anchorDist);

                                                            if (anchorDist > anchorThreshold) {
                                                                continue; // Мышь не над этим блоком
                                                            }

                                                            // === Если дошли сюда, значит мышь НАД блоком d ===
                                                            // ==========================================
                                                                    // *** НОВЫЕ ПРОВЕРКИ ДЛЯ ИСТОРИИ (Входящие связи) ***
                                                                    if (dynamic_cast<HistoryItem*>(m_originBlock)) {
                                                                        // 1. Запрет рекурсии (указывать на саму себя)
                                                                        if (d == m_originBlock) {

                                                                            endPointIsBlocked = true;
                                                                            break;
                                                                        }
                                                                        // 2. Запрет указывать на Мост или Возврат
                                                                        if (dynamic_cast<BridgeItem*>(d) || dynamic_cast<BackItem*>(d)) {

                                                                            endPointIsBlocked = true;
                                                                            break;
                                                                        }
                                                                    }
                                                                    // ==========================================
                                                            // 3. Проверка на РЕКУРСИЮ (только если d — это Мост)
                                                            if (BridgeItem* bridge = dynamic_cast<BridgeItem*>(d)) {
                                                                if (m_originBlock) {
                                                                    QString startName = m_originBlock->getDescription(); // Или getLabel()
                                                                    if (bridge->targetName() == startName) {

                                                                        endPointIsBlocked = true;
                                                                        break; // Блокируем и выходим из цикла
                                                                    }
                                                                }
                                                            }

                                                            // 4. Проверка на ЗАНЯТЫЙ ЯКОРЬ
                                                            if (!d->isAnchorFree(nearestLocal)) {

                                                                endPointIsBlocked = true;
                                                                break; // Блокируем и выходим из цикла
                                                            }
                                                        }
                                                    }
                                                }

                        // Если конечная точка заблокирована (мышь над занятым якорем)
                        if (endPointIsBlocked) {
                            return; // Игнорируем нажатие, сегмент не будет создан
                        }
                        // --- КОНЕЦ НОВОГО КОДА ---
            // Соберём список сегментов для фиксации: из превью, если они есть, иначе вычислим один сегмент
            QList<QLineF> linesToCommit;
            QPointF finalEndPoint;

            const bool hadPreview1 = (m_previewLine != nullptr);
            const bool hadPreview2 = (m_previewLine2 != nullptr);

            if (hadPreview1 || hadPreview2) {
                if (hadPreview1) linesToCommit.append(m_previewLine->line());
                if (hadPreview2) linesToCommit.append(m_previewLine2->line());
                if (!linesToCommit.isEmpty()) finalEndPoint = linesToCommit.last().p2();
                else finalEndPoint = scenePos;
            } else {
                // никаких превью — вычислим один сегмент как раньше
                QPointF endPos = scenePos;
                if (m_startBlock) {
                    qreal dist;
                    QPointF nearestLocal = m_startBlock->nearestAnchorToScenePoint(scenePos, &dist);
                    QPointF nearestScene = m_startBlock->mapToScene(nearestLocal);

                    qreal dx = qAbs(scenePos.x() - nearestScene.x());
                    qreal dy = qAbs(scenePos.y() - nearestScene.y());
                    if (dx >= dy) {
                        endPos.setX(scenePos.x());
                        endPos.setY(nearestScene.y());
                    } else {
                        endPos.setX(nearestScene.x());
                        endPos.setY(scenePos.y());
                    }

                    auto snapToGrid = [](qreal value, qreal step) -> qreal {
                        if (step <= 0.0) return value;
                        qreal t = value / step;
                        qreal f = std::floor(t);
                        qreal frac = t - f;
                        if (frac < 0.5) return f * step;
                        else return (f + 1.0) * step;
                    };
                    const qreal gridStep = 16.0;
                    qreal ddx = qAbs(scenePos.x() - nearestScene.x());
                    qreal ddy = qAbs(scenePos.y() - nearestScene.y());
                    if (ddx >= ddy) {
                        endPos.setY(snapToGrid(nearestScene.y(), gridStep));
                        endPos.setX(snapToGrid(endPos.x(), gridStep));
                    } else {
                        endPos.setX(snapToGrid(nearestScene.x(), gridStep));
                        endPos.setY(snapToGrid(endPos.y(), gridStep));
                    }
                }
                QPointF lineStart = m_hasStartNextSegment ? m_nextSegmentStart : m_startScene;
                linesToCommit.append(QLineF(lineStart, endPos));
                finalEndPoint = endPos;
            }

            // Если нет сегментов — выходим
            if (linesToCommit.isEmpty()) return;
            // *** 1. ДОБАВЬТЕ ЭТОТ БЛОК (ЗАПОМИНАЕМ ОРИЕНТАЦИЮ) ***
                        QLineF lastLine = linesToCommit.last();
                        // Сравниваем dx и dy, чтобы определить ориентацию
                        m_lastSegmentWasHorizontal = (qAbs(lastLine.dx()) > qAbs(lastLine.dy()));
            // --- Создаём постоянные элементы для каждой линии в linesToCommit
            for (const QLineF &ln : linesToCommit) {
                QGraphicsLineItem *commLine = new QGraphicsLineItem(ln);
                QPen p(Qt::black);
                p.setWidth(2);
                commLine->setPen(p);
                commLine->setZValue(1000);
                if (m_scene) m_scene->addItem(commLine);

                // сохраняем в общий список и в текущую сессию
                m_committedItems.append(commLine);
                m_currentSessionItems.append(commLine);
            }

            // --- Если был маркер сгиба (previewBendHighlight) — создадим постоянную точку в той же позиции
            if (m_previewBendHighlight && m_scene) {
                QPointF bendPos = m_previewBendHighlight->pos();
                constexpr qreal size = 3.0;
                QGraphicsEllipseItem *bendMarker = new QGraphicsEllipseItem(-size/2, -size/2, size, size);
                bendMarker->setBrush(QBrush(Qt::black));
                bendMarker->setPen(Qt::NoPen);
                bendMarker->setZValue(1000);
                bendMarker->setPos(bendPos);
                m_scene->addItem(bendMarker);

                m_committedItems.append(bendMarker);
                m_currentSessionItems.append(bendMarker);
            }

            // --- Создаём постоянный маркер конца (finalEndPoint)
            {
                constexpr qreal size = 3.0;
                QGraphicsEllipseItem *endMarker = new QGraphicsEllipseItem(-size/2, -size/2, size, size);
                endMarker->setBrush(QBrush(Qt::black));
                endMarker->setPen(Qt::NoPen);
                endMarker->setZValue(1001);
                endMarker->setPos(finalEndPoint);
                if (m_scene) m_scene->addItem(endMarker);

                m_committedItems.append(endMarker);
                m_currentSessionItems.append(endMarker);
            }

            // уведомляем слушателей (используем координаты первого начала и финальной точки)
            QPointF committedStart = linesToCommit.first().p1();
            emit connectionCommitted(committedStart, finalEndPoint);

            // --- Попытка добавить наконечник стрелы к концу последнего зафиксированного сегмента ---
            // --- Попытка добавить наконечник стрелы к концу последнего зафиксированного сегмента ---
            // --- Попытка добавить наконечник стрелы к концу последнего зафиксированного сегмента ---
            // --- Попытка добавить наконечник стрелы к концу последнего зафиксированного сегмента ---
            {
                            QLineF lastLn = linesToCommit.last();
                            QPointF arrowPos = lastLn.p2();
                            QPointF arrowFrom = lastLn.p1(); // Будет переопределена в ветках

                            const qreal anchorThreshold = 8.0;

                            // Проверим попали ли в anchor какого-то блока — только тогда добавляем наконечник
                            DiagramItem* targetBlock = nullptr;
                            QPointF targetLocal;
                            qreal anchorDist = std::numeric_limits<qreal>::infinity();
                            QPointF finalArrowPos = arrowPos; // По умолчанию: конечная точка линии

                            // [НОВЫЙ КОД]
                                                        if (m_scene) {
                                                            for (QGraphicsItem* it : m_scene->items()) {
                                                                if (DiagramItem* d = dynamic_cast<DiagramItem*>(it)) {

                                                                    QPointF local = d->nearestAnchorToScenePoint(arrowPos, &anchorDist);
                                                                    QPointF sceneAnchor = d->mapToScene(local);

                                                                    if (QLineF(sceneAnchor, arrowPos).length() <= anchorThreshold) {
                                                                        // *** НОВОЕ ИСПРАВЛЕНИЕ: Проверяем на петлю в ту же точку ***
                                                                                                                                                auto aeq = [](const QPointF& p1, const QPointF& p2) {
                                                                                                                                                    return qFuzzyCompare(p1.x(), p2.x()) && qFuzzyCompare(p1.y(), p2.y());
                                                                                                                                                };
                                                                                                                                                bool isLoopOnSameAnchor = (d == m_originBlock && aeq(local, m_originLocal));
                                                                                                                                                // *** КОНЕЦ НОВОГО ИСПРАВЛЕНИЯ ***
                                                                        // *** НАЧАЛО ИЗМЕНЕНИЯ (Проверка, свободен ли целевой якорь) ***
                                                                        if (d->isAnchorFree(local)) {
                                                                            // Якорь свободен
                                                                            targetBlock = d;
                                                                            targetLocal = local;
                                                                            finalArrowPos = sceneAnchor; // Переносим на точный anchor
                                                                            break; // Нашли свободный якорь, выходим из цикла
                                                                        } else {
                                                                            // Якорь занят, игнорируем этот блок
                                                                            // и продолжаем цикл (вдруг курсор на границе двух блоков)
                                                                            continue;
                                                                        }
                                                                        // *** КОНЕЦ ИЗМЕНЕНИЯ ***
                                                                    }
                                                                }
                                                            }
                                                        }

                            // *** НАЧАЛО ЯВНОГО РАЗДЕЛЕНИЯ ЛОГИКИ ***
                                                        if (targetBlock) {
                                                                                        // *** 2. ДОБАВЬТЕ ЭТУ СТРОКУ (СБРОС ФЛАГА) ***
                                                                                                        m_lastSegmentWasHorizontal = false; // Линия завершена, сбрасываем
                                                                                        const qreal arrowLen = 10.0;
                                                                                        const qreal arrowWidth = 6.0;

                                                                                        // --------------------------------------------------------------------------
                                                                                        // *** ИСПРАВЛЕННАЯ ЛОГИКА ***
                                                                                        // СЦЕНАРИЙ A: Это НАСТОЯЩАЯ ПРЯМАЯ ЛИНИЯ
                                                                                        // (Только 1 сегмент превью И мы НЕ продолжаем другую линию)
                                                                                        // --------------------------------------------------------------------------
                                                                                        if (linesToCommit.count() == 1 && !m_hasStartNextSegment)
                                                                                        {
                                                                                            QLineF straightLn = linesToCommit.first();
                                                                                            arrowFrom = straightLn.p1(); // Начало прямой линии

                                                                                            // 1. Создание стрелки (вызов функции для ПРЯМЫХ)
                                                                                            createStraightArrowHead(arrowFrom, finalArrowPos,
                                                                                                                    arrowLen, arrowWidth, m_scene,
                                                                                                                    m_committedItems, m_currentSessionItems);

                                                                                        }
                                                                                        // --------------------------------------------------------------------------
                                                                                        // СЦЕНАРИЙ B: Г-ОБРАЗНАЯ ЛИНИЯ
                                                                                        // (Больше 1 сегмента ИЛИ это продолжение предыдущего сегмента)
                                                                                        // --------------------------------------------------------------------------
                                                                                        else // (linesToCommit.count() > 1 || m_hasStartNextSegment)
                                                                                        {
                                                                                            // 'arrowFrom' уже равен lastLn.p1() (точка изгиба ИЛИ начало последнего сегмента)
                                                                                            arrowFrom = lastLn.p1();

                                                                                            // 1. Создание стрелки (вызов функции для Г-ОБРАЗНЫХ)
                                                                                            // (БЕЗ +180 градусов, т.к. вектор от изгиба/начала сегмента уже правильный)
                                                                                            createArrowHead(arrowFrom, finalArrowPos,
                                                                                                            arrowLen, arrowWidth, m_scene,
                                                                                                            m_committedItems, m_currentSessionItems);


                                                                                        }

                                                                                        // *** НОВАЯ ЛОГИКА: Установка возрастающего приоритета ***
                                                                                        if (m_originBlock) {
                                                                                            // Вычисляем следующий номер приоритета для блока-источника
                                                                                            int nextPrio = m_originBlock->getNextPriority();

                                                                                            // Ищем TransitionGroup среди только что созданных элементов сессии
                                                                                            for (QGraphicsItem *it : m_currentSessionItems) {
                                                                                                if (TransitionGroup *tg = dynamic_cast<TransitionGroup*>(it)) {
                                                                                                    tg->setPriority(nextPrio);
                                                                                                    break; // Нашли и установили — выходим
                                                                                                }
                                                                                            }
                                                                                        }
                                                                                        // *** КОНЕЦ ЛОГИКИ ***

                                                                                        // --------------------------------------------------------------------------
                                                                                        // ОБЩИЙ СБРОС СОСТОЯНИЯ (Выполняется после успешного коммита)
                                                                                        // --------------------------------------------------------------------------

                                                                                        // *** НАЧАЛО ИЗМЕНЕНИЯ: Группировка элементов ***
                                                                                        ConnectionItem* group = new ConnectionItem();
                                                                                        // *** Передаем блоки и якоря в ConnectionItem ***
                                                                                        group->setStart(m_originBlock, m_originLocal);
                                                                                        group->setEnd(targetBlock, targetLocal);
                                                                                        // Помечаем группу, чтобы мы могли найти ее по двойному клику
                                                                                        group->setData(ConnectionRole, true); // Убедитесь, что ConnectionRole определен в connect.h

                                                                                        // Добавляем все временные элементы в группу
                                                                                        for (QGraphicsItem* item : m_currentSessionItems) {
                                                                                            // item->scene() вернет null, т.к. addToGroup() забирает его
                                                                                            // *** НОВОЕ: Регистрируем каждый элемент в ConnectionItem ***
                                                                                                group->registerItem(item);
                                                                                            group->addToGroup(item);
                                                                                        }
                                                                                        // ***  Вызываем finalize, чтобы ConnectionItem нашел свои компоненты ***
                                                                                        group->finalizeConstruction();
                                                                                        // Добавляем группу на сцену (это вернет все элементы)
                                                                                        if (m_scene) m_scene->addItem(group);
                                                                                        // *** ВСТАВИТЬ ЭТОТ БЛОК ЗДЕСЬ ***
                                                                                                                        // --- ПРИНУДИТЕЛЬНОЕ ОБНОВЛЕНИЕ ПОЗИЦИИ ТЕКСТА ---
                                                                                                                        // Сейчас у нас есть полная линия (от А до Б).
                                                                                                                        // Мы передаем её в TransitionGroup.
                                                                                                                        // Так как раньше там был только 1 сегмент (хвост),
                                                                                                                        // сработает флаг structureChanged и текст прыгнет в НАЧАЛО (индекс 0).
                                                                                                                        if (group->transition()) {
                                                                                                                            group->transition()->updateAnchor(group->lines());
                                                                                                                        }
                                                                                        // *** Сообщаем блокам, что они связаны ***
                                                                                        if (m_originBlock) {

                                                                                                                            m_originBlock->addConnection(group);
                                                                                                                        } else {

                                                                                                                        }


                                                                                        if (targetBlock) {

                                                                                            targetBlock->addConnection(group);
                                                                                        } else {
                                                                                            // Этого не должно случиться, так как мы внутри 'if (targetBlock)'

                                                                                        }

                                                                                        // Теперь в m_committedItems мы сохраняем ОДНУ группу, а не россыпь элементов
                                                                                        m_committedItems.append(group);

                                                                                        // Очищаем сессию, т.к. элементы теперь в группе
                                                                                        m_currentSessionItems.clear();
                                                                                        // ==========================================================
                                                                                            // *** НОВОЕ: АВТОМАТИЧЕСКАЯ МАРШРУТИЗАЦИЯ (2 КЛИКА) ***
                                                                                            // Если включен режим, мы заставляем стрелку мгновенно пересчитать
                                                                                            // свой авто-путь, игнорируя то, как криво вел мышку пользователь.
                                                                                            // ==========================================================
                                                                                            if (Utils::isAutoRoutingEnabled()) {
                                                                                                group->updatePath();
                                                                                            }


                                                                                                        // Заставляем стрелку сразу пересчитать свой путь, чтобы учесть
                                                                                                        // ранги (отступы) и не слипаться с другими стрелками.
                                                                                        m_active = false;
                                                                                        m_justStopped = true;

                                                                                        // Эти строки НУЖНО ОСТАВИТЬ, они сбрасывают состояние для *следующей* линии
                                                                                        m_hasStart = false;
                                                                                        m_startBlock = nullptr;
                                                                                        m_hasStartNextSegment = false;
                                                                                        m_nextSegmentStart = QPointF();
                                                                                        m_originBlock = nullptr;
                                                                                        m_originLocal = QPointF();
                                                                                    }
                // *** КОНЕЦ ЯВНОГО РАЗДЕЛЕНИЯ ЛОГИКИ ***

            // --------------------------------------------------------------------------
                                        // *** КОНЕЦ ЯВНОГО РАЗДЕЛЕНИЯ ЛОГИКИ ***
                                    }
                        // --------------------------------------------------------------------------
                        // --------------------------------------------------------------------------
            // --------------------------------------------------------------------------

            // Очистим временные превью (но не удаляем только что созданные committed элементы)
            if (m_previewLine && m_scene) {
                m_scene->removeItem(m_previewLine);
                delete m_previewLine;
                m_previewLine = nullptr;
            }
            if (m_previewLine2 && m_scene) {
                m_scene->removeItem(m_previewLine2);
                delete m_previewLine2;
                m_previewLine2 = nullptr;
            }
            if (m_previewStartHighlight && m_scene) {
                m_scene->removeItem(m_previewStartHighlight);
                delete m_previewStartHighlight;
                m_previewStartHighlight = nullptr;
            }
            if (m_previewBendHighlight && m_scene) {
                m_scene->removeItem(m_previewBendHighlight);
                delete m_previewBendHighlight;
                m_previewBendHighlight = nullptr;
            }
            if (m_previewEndHighlight && m_scene) {
                m_scene->removeItem(m_previewEndHighlight);
                delete m_previewEndHighlight;
                m_previewEndHighlight = nullptr;
            }

            // Сбрасываем подсветку стартового блока (если осталась)
            if (m_startBlock) m_startBlock->setAnchorHighlighted(QPointF(), false);

            // Подготовим старт для следующего сегмента — от finalEndPoint (если режим всё ещё активен)
            if (m_active) {
                m_hasStartNextSegment = true;
                m_nextSegmentStart = finalEndPoint;
                m_hasStart = true;
                // *** ДОБАВЬТЕ ЭТОТ ЛОГ ДЛЯ ДИАГНОСТИКИ ***

                                // *** КОНЕЦ ЛОГА ***
                                m_startBlock = nullptr;
            } else {
                // если режим завершён — очистим временный сессионный список,
                // т.к. элементы уже перенесены в m_committedItems (и текущая сессия закрыта)
                m_currentSessionItems.clear();
            }

            // если режим всё ещё активен — продолжаем, иначе функция завершена
            return;
        }
    }
    // ПКМ: отмена/удаление текущей линии (только текущей сессии)
    else if (button == Qt::RightButton) {
        // Если нет зафиксированных сегментов и нет превью — просто выключаем режим
        if (m_committedItems.isEmpty() && !m_hasStart && !m_previewLine && !m_previewLine2 && !m_previewStartHighlight) {
            m_active = false;
            m_justStopped = true;
            return;
        }

        // Если есть элементы текущей линии (текущая сессия) — удаляем только их
        if (!m_currentSessionItems.isEmpty()) {
            for (QGraphicsItem* it : m_currentSessionItems) {
                if (it && m_scene) m_scene->removeItem(it);
                delete it;
            }
            m_currentSessionItems.clear();
        }

        // Теперь удаляем превью текущей линии и сбрасываем состояние
        stop();

        // Сброс состояния продолжения текущей линии
        m_hasStartNextSegment = false;
        m_nextSegmentStart = QPointF();

        m_hasStart = false;
        m_startBlock = nullptr;

        m_originBlock = nullptr;
        m_originLocal = QPointF();

        m_startScene = QPointF();
        m_startLocal = QPointF();

        m_active = false;
        m_justStopped = true;

        return;
    }
}

// подпись: принимает дополнительные sessionItems, чтобы можно было чистить текущую сессию
// подпись: принимает дополнительные sessionItems, чтобы можно было чистить текущую сессию
//для вызова функций Г образных линий
void Connector::createArrowHead(const QPointF &from,
                                const QPointF &to,
                                qreal arrowLen,
                                qreal arrowWidth,
                                QGraphicsScene *scene,
                                QList<QGraphicsItem*> &committedItems,
                                QList<QGraphicsItem*> &sessionItems)
{
    if (!scene) return;

    // 1. Создание наконечника (Геометрия: вершина в 0,0, хвост влево)
    QPolygonF tri;
    tri << QPointF(0, 0)
        << QPointF(-arrowLen, -arrowWidth/2.0)
        << QPointF(-arrowLen,  arrowWidth/2.0);

    QGraphicsPolygonItem *arrow = new QGraphicsPolygonItem(tri);
    arrow->setBrush(QBrush(Qt::black));
    arrow->setPen(Qt::NoPen);
    arrow->setZValue(1001);
    arrow->setTransformOriginPoint(QPointF(0, 0));

    // Расчет угла: от колена (from) к острию (to)
    QPointF vec = to - from;
    qreal angleDeg = 0.0;
    if (!qFuzzyIsNull(vec.x()) || !qFuzzyIsNull(vec.y())) {
        angleDeg = std::atan2(vec.y(), vec.x()) * 180.0 / M_PI;
    }

    // *** ИСПРАВЛЕНИЕ: Убрали +180. Чистый atan2 дает верный угол. ***

    arrow->setRotation(angleDeg);
    arrow->setPos(to);

    scene->addItem(arrow);
    committedItems.append(arrow);
    sessionItems.append(arrow);

    // 2. Создание TransitionGroup
    TransitionGroup *transGroup = new TransitionGroup();
    scene->addItem(transGroup);
    transGroup->setZValue(1002);

    committedItems.append(transGroup);
    sessionItems.append(transGroup);

    QGraphicsLineItem tempLine(QLineF(from, to));
    QList<QGraphicsLineItem*> tempLines;
    tempLines.append(&tempLine);
    transGroup->updateAnchor(tempLines);
}
// *** НОВАЯ ФУНКЦИЯ ДЛЯ ПРЯМЫХ ЛИНИЙ ***
// (Эта функция идентична, но включает компенсацию +180 градусов)
void Connector::createStraightArrowHead(const QPointF &from,
                                        const QPointF &to,
                                        qreal arrowLen,
                                        qreal arrowWidth,
                                        QGraphicsScene *scene,
                                        QList<QGraphicsItem*> &committedItems,
                                        QList<QGraphicsItem*> &sessionItems)
{
    if (!scene) return;

    // 1. Создание наконечника
    QPolygonF tri;
    tri << QPointF(0, 0)
        << QPointF(-arrowLen, -arrowWidth/2.0)
        << QPointF(-arrowLen,  arrowWidth/2.0);

    QGraphicsPolygonItem *arrow = new QGraphicsPolygonItem(tri);
    arrow->setBrush(QBrush(Qt::black));
    arrow->setPen(Qt::NoPen);
    arrow->setZValue(1001);
    arrow->setTransformOriginPoint(QPointF(0, 0));

    QPointF vec = to - from;
    qreal angleDeg = 0.0;
    if (!qFuzzyIsNull(vec.x()) || !qFuzzyIsNull(vec.y())) {
        angleDeg = std::atan2(vec.y(), vec.x()) * 180.0 / M_PI;
    }

    // *** ИСПРАВЛЕНИЕ: Убрали +180. ***

    arrow->setRotation(angleDeg);
    arrow->setPos(to);

    scene->addItem(arrow);
    committedItems.append(arrow);
    sessionItems.append(arrow);

    // 2. Создание TransitionGroup
    TransitionGroup *transGroup = new TransitionGroup();
    scene->addItem(transGroup);
    transGroup->setZValue(1002);

    committedItems.append(transGroup);
    sessionItems.append(transGroup);

    QGraphicsLineItem tempLine(QLineF(from, to));
    QList<QGraphicsLineItem*> tempLines;
    tempLines.append(&tempLine);
    transGroup->updateAnchor(tempLines);
}
// Реализация: создаёт два редактируемых текстовых поля рядом с линией (по её нормали)
// в connect.cpp
// В connect.cpp
// в connect.cpp
// Должен быть в .cpp файле класса Connector (или где он у вас)




// -------------------------------------------
// в connect.h, внутри class Connector
void Connector::createArrowWithLabels(const QPointF &from,
                                      const QPointF &to,
                                      QGraphicsScene *scene,
                                      QList<QGraphicsItem*> &committedItems,
                                      QList<QGraphicsItem*> &sessionItems)
{
    if (!scene) return;

    // ============================================================
    // ЧАСТЬ 1: Визуальный маркер (Наконечник стрелки)
    // ============================================================
    const qreal arrowLen = 10.0;
    const qreal arrowWidth = 6.0;
    QPolygonF tri;
    tri << QPointF(0, 0)
        << QPointF(-arrowLen, -arrowWidth/2.0)
        << QPointF(-arrowLen,  arrowWidth/2.0);

    QGraphicsPolygonItem *arrow = new QGraphicsPolygonItem(tri);
    arrow->setBrush(Qt::black);
    arrow->setPen(Qt::NoPen);
    arrow->setZValue(1001); // Наконечник лежит высоко
    arrow->setTransformOriginPoint(0,0);

    // Рассчитываем угол для наконечника
    QPointF vec = to - from;
    qreal angleDeg = std::atan2(vec.y(), vec.x()) * 180.0 / M_PI;
    arrow->setRotation(angleDeg);
    arrow->setPos(to);

    scene->addItem(arrow);
    committedItems.append(arrow);
    sessionItems.append(arrow);

    // ============================================================
    // ЧАСТЬ 2: Логический блок (Условия перехода)
    // Используем НОВЫЙ класс TransitionGroup вместо старых прямоугольников
    // ============================================================

    TransitionGroup *transGroup = new TransitionGroup();

    // Добавляем на сцену
    scene->addItem(transGroup);
    transGroup->setZValue(1002); // Поверх линий

    // Добавляем в списки.
    // ConnectionItem::registerItem() распознает тип TransitionGroup* // и заберет его под свое управление.
    committedItems.append(transGroup);
    sessionItems.append(transGroup);

    // Первичное позиционирование.
    // Создаем временную линию, чтобы группа знала, где встать
    QGraphicsLineItem tempLine(QLineF(from, to));
    QList<QGraphicsLineItem*> tempLines;
    tempLines.append(&tempLine);

    transGroup->updateAnchor(tempLines);
}
// Файл: connect.cpp (добавьте эту новую функцию)

// *** НОВАЯ ФУНКЦИЯ ДЛЯ ПРЯМЫХ ЛИНИЙ ***
void Connector::createStraightArrowWithLabels(const QPointF &from,
                                              const QPointF &to,
                                              QGraphicsScene *scene,
                                              QList<QGraphicsItem*> &committedItems,
                                              QList<QGraphicsItem*> &sessionItems)
{
    if (!scene) return;

    // --- 1. Создание стрелки (кончик в to) ---
    const qreal arrowLen = 10.0;
    const qreal arrowWidth = 6.0;
    QPolygonF tri;
    tri << QPointF(0, 0)
        << QPointF(-arrowLen, -arrowWidth/2.0)
        << QPointF(-arrowLen,  arrowWidth/2.0);

    QGraphicsPolygonItem *arrow = new QGraphicsPolygonItem(tri);
    arrow->setBrush(Qt::black);
    arrow->setPen(Qt::NoPen);
    arrow->setZValue(1001);
    arrow->setTransformOriginPoint(0,0);

    QPointF vec = to - from;
    qreal angleDeg = std::atan2(vec.y(), vec.x()) * 180.0 / M_PI;

    // Компенсация на 180 градусов для прямых линий
    angleDeg += 180.0;
    while (angleDeg < 0.0) angleDeg += 360.0;
    while (angleDeg >= 360.0) angleDeg -= 360.0;

    arrow->setRotation(angleDeg);
    arrow->setPos(to);

    scene->addItem(arrow);
    committedItems.append(arrow);
    sessionItems.append(arrow);

    // --- 2. Создание TransitionGroup ---

    TransitionGroup *transGroup = new TransitionGroup();
    scene->addItem(transGroup);
    transGroup->setZValue(1002);

    committedItems.append(transGroup);
    sessionItems.append(transGroup);

    // Первичное позиционирование
    QGraphicsLineItem tempLine(QLineF(from, to));
    QList<QGraphicsLineItem*> tempLines;
    tempLines.append(&tempLine);

    transGroup->updateAnchor(tempLines);
}


// --- Флаги подавления контекстного меню при отмене рисования ---
bool Connector::wasJustStopped() const
{
    return m_justStopped;
}

void Connector::consumeJustStopped()
{
    m_justStopped = false;
}
/**
 * @brief Рисует ОДИН ортогональный сегмент превью (прямая линия).
 * Эта функция вызывается, когда onMouseMove решил, что изгиб НЕ нужен.
 */
void Connector::drawStraightPreview(const QPointF &previewStart, const QPointF &scenePos)
{
    // Удаляем элементы Г-образного превью, если они остались с прошлого кадра
    if (m_previewLine2) { if (m_scene) m_scene->removeItem(m_previewLine2); delete m_previewLine2; m_previewLine2 = nullptr; }
    if (m_previewBendHighlight) { if (m_scene) m_scene->removeItem(m_previewBendHighlight); delete m_previewBendHighlight; m_previewBendHighlight = nullptr; }

    // Вспомогательная функция snapToGrid
    auto snapToGridMid = [](qreal value, qreal step) -> qreal {
        if (step <= 0.0) return value;
        qreal t = value / step;
        qreal f = std::floor(t);
        qreal frac = t - f;
        if (frac < 0.5) return f * step;
        else return (f + 1.0) * step;
    };
    const qreal gridStep = 16.0;

    // Вектор от старта до курсора
    qreal dx = scenePos.x() - previewStart.x();
    qreal dy = scenePos.y() - previewStart.y();

    // Выбираем ориентацию (горизонтальная если dx>=dy, иначе вертикальная)
    QPointF snappedEnd = scenePos;
    if (qAbs(dx) >= qAbs(dy)) {
        // Горизонтальная основная: Y = start.y(), X = курсор.x() (с привязкой)
        snappedEnd.setY(snapToGridMid(previewStart.y(), gridStep));
        snappedEnd.setX(snapToGridMid(scenePos.x(), gridStep));
    } else {
        // Вертикальная основная: X = start.x(), Y = курсор.y()
        snappedEnd.setX(snapToGridMid(previewStart.x(), gridStep));
        snappedEnd.setY(snapToGridMid(scenePos.y(), gridStep));
    }

    // Создаём/обновляем основной превью-отрезок
    if (!m_previewLine) {
        m_previewLine = new QGraphicsLineItem;
        QPen p(Qt::black);
        p.setWidth(2);
        m_previewLine->setPen(p);
        m_previewLine->setZValue(1999);
        m_scene->addItem(m_previewLine);
    }
    m_previewLine->setLine(QLineF(previewStart, snappedEnd));

    // Обновим/создадим маркер конца (точку)
    if (!m_previewEndHighlight) {
        constexpr qreal size = 6.0;
        m_previewEndHighlight = new QGraphicsEllipseItem(-size/2, -size/2, size, size);
        m_previewEndHighlight->setBrush(QBrush(Qt::black));
        m_previewEndHighlight->setPen(Qt::NoPen);
        m_previewEndHighlight->setZValue(2001);
        m_scene->addItem(m_previewEndHighlight);
    }
    m_previewEndHighlight->setPos(snappedEnd);
}


/**
 * @brief Рисует ДВА ортогональных сегмента превью (Г-образная линия).
 * Эта функция вызывается, когда onMouseMove решил, что изгиб НУЖЕН.
 */
// --- НАЧАЛО ФРАГМЕНТА ---

/**
 * @brief Рисует ДВА ортогональных сегмента превью (Г-образная линия).
 * Эта функция вызывается, когда onMouseMove решил, что изгиб НУЖЕН.
 */
void Connector::drawLShapedPreview(const QPointF &previewStart, const QPointF &scenePos)
{
    // Вспомогательная функция snapToGrid
    auto snapToGridMid = [](qreal value, qreal step) -> qreal {
        if (step <= 0.0) return value;
        qreal t = value / step;
        qreal f = std::floor(t);
        qreal frac = t - f;
        if (frac < 0.5) return f * step;
        else return (f + 1.0) * step;
    };
    const qreal gridStep = 16.0;

    // Вектор от старта до курсора
    qreal dx = scenePos.x() - previewStart.x();
    qreal dy = scenePos.y() - previewStart.y();
    qreal adx = qAbs(dx);
    qreal ady = qAbs(dy);

    // Вычисляем bend и конечную точку и привязываем
    QPointF bend;
    QPointF endPoint = scenePos;

    // *** 1. НАЧАЛО ИСПРАВЛЕНИЯ ***
    bool forceHorizontalFirst = (adx >= ady); // Стандартная логика

    // Если мы продолжаем линию, ИНВЕРТИРУЕМ логику,
    // чтобы первый сегмент был перпендикулярен предыдущему.
    if (m_hasStartNextSegment) {
        forceHorizontalFirst = !m_lastSegmentWasHorizontal;
    }

    // Ориентация:
    if (forceHorizontalFirst) {
        // Сначала горизонталь, затем вертикаль
        bend.setX(scenePos.x());
        bend.setY(previewStart.y());
    } else {
        // Сначала вертикаль, затем горизонталь
        bend.setX(previewStart.x());
        bend.setY(scenePos.y());
    }
    // *** КОНЕЦ ИСПРАВЛЕНИЯ ***

    // Привязка точки поворота и конечной точки к сетке
    bend.setX(snapToGridMid(bend.x(), gridStep));
    bend.setY(snapToGridMid(bend.y(), gridStep));
    endPoint.setX(snapToGridMid(endPoint.x(), gridStep));
    endPoint.setY(snapToGridMid(endPoint.y(), gridStep));

    // Первый сегмент: previewStart -> bend
    if (!m_previewLine) {
        m_previewLine = new QGraphicsLineItem;
        QPen p(Qt::black);
        p.setWidth(2);
        m_previewLine->setPen(p);
        m_previewLine->setZValue(1999);
        m_scene->addItem(m_previewLine);
    }
    m_previewLine->setLine(QLineF(previewStart, bend));

    // Второй сегмент: bend -> endPoint
    if (!m_previewLine2) {
        m_previewLine2 = new QGraphicsLineItem;
        QPen p2(Qt::black);
        p2.setWidth(2);
        m_previewLine2->setPen(p2);
        m_previewLine2->setZValue(1999);
        m_scene->addItem(m_previewLine2);
    }
    m_previewLine2->setLine(QLineF(bend, endPoint));

    // Точка в bend (маркер поворота)
    if (!m_previewBendHighlight) {
        constexpr qreal size = 6.0;
        m_previewBendHighlight = new QGraphicsEllipseItem(-size/2, -size/2, size, size);
        m_previewBendHighlight->setBrush(QBrush(Qt::black));
        m_previewBendHighlight->setPen(Qt::NoPen);
        m_previewBendHighlight->setZValue(2000);
        m_scene->addItem(m_previewBendHighlight);
    }
    m_previewBendHighlight->setPos(bend);

    // Точка на конце для продолжения
    if (!m_previewEndHighlight) {
        constexpr qreal size = 6.0;
        m_previewEndHighlight = new QGraphicsEllipseItem(-size/2, -size/2, size, size);
        m_previewEndHighlight->setBrush(QBrush(Qt::black));
        m_previewEndHighlight->setPen(Qt::NoPen);
        m_previewEndHighlight->setZValue(2001);
        m_scene->addItem(m_previewEndHighlight);
    }
    m_previewEndHighlight->setPos(endPoint);
}

// --- КОНЕЦ ФРАГМЕНТА ---
// =================================================================
//
// Реализация ArrowEditor
//
// =================================================================

ArrowEditor::ArrowEditor(QGraphicsScene *scene, QObject *parent)
    : QObject(parent), m_scene(scene)
{
}

ArrowEditor::~ArrowEditor()
{
    stop();
}

void ArrowEditor::clearHandles()
{
    // Удаляем все ручки со сцены
    for (EditHandle *h : m_handles) {
        if (h && h->scene()) {
            m_scene->removeItem(h);
        }
        delete h;
    }
    m_handles.clear();
}

void ArrowEditor::start(ConnectionItem *group)
{
    if (m_active) {
        stop();
    }
    if (!group || !m_scene) {
        return;
    }

    m_active = true;
    m_isRepinning = false;
    m_editingGroup = group;
    // Отключаем выделение самой группы, чтобы не мешала кликам по ручкам
    m_editingGroup->setFlag(QGraphicsItem::ItemIsSelectable, false);

    // В этом режиме нам нужны ВСЕ ручки для изменения формы
    refreshHandles();
}
// --- Режим ОТСОЕДИНЕНИЯ КОНЦА (Контекстное меню) ---
void ArrowEditor::startRepin(ConnectionItem *group)
{
    if (m_active) {
        stop();
    }
    if (!group || !m_scene) {
        return;
    }

    m_active = true;
    m_isRepinning = true; // <--- Включаем режим отсоединения
    m_editingGroup = group;
    m_editingGroup->setFlag(QGraphicsItem::ItemIsSelectable, false);

    // 1. Инициализируем компоненты
    m_lines = m_editingGroup->lines();
    m_nodes = m_editingGroup->nodes();
    m_arrowHead = m_editingGroup->arrowHead();


    // 2. Запоминаем для отмены
    m_originalEndBlock = m_editingGroup->endBlock();
    m_originalEndAnchor = m_editingGroup->endAnchorLocal();

    if (!m_originalEndBlock || !m_arrowHead) {
        stop();
        return;
    }

    // 3. Логически отсоединяем
    m_originalEndBlock->removeConnection(m_editingGroup);
    m_editingGroup->setEnd(nullptr, QPointF());

    // 4. Показываем якоря на других блоках
    for (QGraphicsItem *it : m_scene->items()) {
        if (DiagramItem *d = dynamic_cast<DiagramItem*>(it)) {
            if (d == m_editingGroup->startBlock()) continue; // Кроме стартового
            d->generateAnchorPoints();
            d->setShowAnchorPoints(true);
        }
    }

    // 5. Создаем ТОЛЬКО ОДНУ ручку на кончике стрелки
    clearHandles();
    QPointF tipPos = m_arrowHead->pos();
    EditHandle *handle = new EditHandle();
    m_scene->addItem(handle);
    handle->setPos(tipPos);
    m_handles.append(handle);
}
// --- НАЧАЛО ФРАГМЕНТА ---
// *** НОВЫЙ МЕТОД: Полное обновление ручек ***
// Вызывайте его, если геометрия стрелки сильно изменилась (например, после updatePath)
// *** НОВЫЙ МЕТОД: Полное обновление ручек ***
// Вызывайте его, если геометрия стрелки сильно изменилась (например, после updatePath)
void ArrowEditor::refreshHandles()
{
    clearHandles(); // Удаляем старые ручки перед созданием новых

    if (!m_editingGroup || !m_scene) return;

    // 1. Получаем АКТУАЛЬНЫЙ список линий из группы
    m_lines = m_editingGroup->lines();
    m_nodes = m_editingGroup->nodes(); // <--- Сюда попадают обновленные "колени"
    m_arrowHead = m_editingGroup->arrowHead();

    if (!m_arrowHead) return;

    // 2. Определяем начальную и конечную точки (для логики СЛУЧАЯ 1)
    const QPointF endPoint = m_arrowHead->pos();
    QPointF startPoint;
    if (!m_lines.isEmpty()) {
         // Ищем точку, которая является началом линии, но не является концом никакой другой
         QList<QPointF> allP2s;
         for (QGraphicsLineItem *line : m_lines) allP2s.append(line->line().p2());
         for (QGraphicsLineItem *line : m_lines) {
             if (!allP2s.contains(line->line().p1())) {
                 startPoint = line->line().p1();
                 break;
             }
         }
         // Если замкнутый цикл или не нашли, берем первую попавшуюся
         if (startPoint.isNull() && !m_lines.isEmpty()) startPoint = m_lines.first()->line().p1();
    }

    // --- ГЛАВНОЕ ИСПРАВЛЕНИЕ ---
    if (m_lines.count() == 1)
    {
        // === СЛУЧАЙ 1: ПРЯМАЯ ЛИНИЯ ===
        // (Эта логика у вас уже была правильной, она "разрезает" линию)

        QGraphicsLineItem* firstLine = m_lines.first();
        QLineF lineGeom = firstLine->line();
        QPointF midPoint = lineGeom.pointAt(0.5); // Находим середину

        QPointF snappedMid = snapToGrid(midPoint, 16.0);

        // 1. Укорачиваем существующую первую линию до середины
        firstLine->setLine(QLineF(lineGeom.p1(), snappedMid));

        // 2. Создаем вторую линию от середины до конца
        QGraphicsLineItem* secondLine = new QGraphicsLineItem(QLineF(snappedMid, lineGeom.p2()));
        secondLine->setPen(firstLine->pen());
        secondLine->setZValue(firstLine->zValue());

        // 3. Регистрируем новую линию в группе
        m_editingGroup->addToGroup(secondLine);
        m_editingGroup->registerItem(secondLine);

        // 4. Обновляем локальный список линий редактора
        m_lines.append(secondLine);

        // 5. Создаем желтую ручку в точке разреза
        EditHandle *handle = new EditHandle();
        m_scene->addItem(handle);
        handle->setPos(snappedMid);
        m_handles.append(handle);
    }
    else
    {
        // === СЛУЧАЙ 2: УЖЕ ИЗОГНУТАЯ ЛИНИЯ (НОВАЯ ЛОГИКА) ===
        // updatePath() уже создал m_nodes (черные точки) в нужных местах.
        // Просто создаем желтые ручки в тех же позициях.

        for (QGraphicsEllipseItem* node : m_nodes)
        {
            // (updatePath позаботился о том, чтобы m_nodes
            // не создавались на start/end, а только на изгибах (i > 1))

            EditHandle *handle = new EditHandle();
            m_scene->addItem(handle);
            handle->setPos(node->pos()); // Ставим ручку на позицию черной точки
            m_handles.append(handle);
        }
    }
}


// --- КОНЕЦ ФРАГМЕНТА ---
void ArrowEditor::stop()
{
    if (!m_active) return;

    // *** НОВОЕ: Автоматическое восстановление при сбое отсоединения ***
    // Если режим Repin все еще включен (значит, мы не завершили его успешно),
    // а нас принудительно останавливают -> восстанавливаем связь.
    if (m_isRepinning && m_originalEndBlock && m_editingGroup) {

         m_editingGroup->setEnd(m_originalEndBlock, m_originalEndAnchor);
         m_originalEndBlock->addConnection(m_editingGroup);
         m_editingGroup->updatePath();
    }
    m_isRepinning = false; // Сбрасываем флаг
    // *****************************************************************

    if (m_editingGroup) {
        m_editingGroup->setFlag(QGraphicsItem::ItemIsSelectable, true); // Возвращаем возможность выделения
    }

    clearHandles(); // Удаляем ручки

    // Прячем все точки привязки на сцене
    if (m_scene) {
        for (QGraphicsItem *it : m_scene->items()) {
            if (DiagramItem *d = dynamic_cast<DiagramItem*>(it)) {
                d->setShowAnchorPoints(false);
                d->setAnchorHighlighted(QPointF(), false);
            }
        }
    }

    // Сбрасываем все указатели
    m_lines.clear();
    m_nodes.clear();
    m_arrowHead = nullptr;
    m_labelBgs.clear();
    m_labels.clear();
    m_editingGroup = nullptr;
    m_draggedHandle = nullptr;
    m_originalEndBlock = nullptr; // Тоже сбрасываем
    m_hoveredBlock = nullptr;
    m_hoveredAnchor = QPointF();
    m_active = false;
}

// --- НАЧАЛО ФРАГМЕНТА ---

// *** ИЗМЕНИТЕ СИГНАТУРУ МЕТОДА (Шаг 5.3) ***
void ArrowEditor::onMousePress(const QPointF &scenePos, Qt::MouseButton button)
{
    if (!m_active || !m_scene) return;

    // *** 1. ЛОГИКА ДЛЯ РЕЖИМА "Отсоединения" (Repin) ***
    if (m_isRepinning) {
        if (button == Qt::RightButton) {
            // Отмена (ПКМ) -> восстанавливаем исходное состояние
            if (m_originalEndBlock && m_arrowHead && m_editingGroup) {
                 QPointF originalPos = m_originalEndBlock->mapToScene(m_originalEndAnchor);
                 updateConnection(m_arrowHead->pos(), originalPos);
                 m_editingGroup->setEnd(m_originalEndBlock, m_originalEndAnchor);
                 m_originalEndBlock->addConnection(m_editingGroup);
                 m_editingGroup->updatePath();
            }
            m_isRepinning = false;
            stop();
            return;
        }
        if (button == Qt::LeftButton) {
            QGraphicsItem *item = m_scene->itemAt(scenePos, QTransform());
            if (EditHandle *handle = dynamic_cast<EditHandle*>(item)) {
                m_draggedHandle = handle;
            }
        }
        return;
    }

    // *** 2. ОБЫЧНЫЙ РЕЖИМ РЕДАКТИРОВАНИЯ ***
    if (button == Qt::LeftButton) {
        QGraphicsItem *item = m_scene->itemAt(scenePos, QTransform());
        EditHandle *handle = dynamic_cast<EditHandle*>(item);

        if (handle) {
            // Попали в ручку -> начинаем тащить
            m_draggedHandle = handle;
        } else {
            // Попали НЕ в ручку. Проверяем, попали ли мы в ЛЮБУЮ часть стрелки.
            bool isArrowPart = false;
            if (item && m_editingGroup) {
                // Проходим вверх по всем родителям элемента, чтобы найти нашу группу.
                QGraphicsItem* curr = item;
                while (curr) {
                    if (curr == m_editingGroup || curr->group() == m_editingGroup) {
                        isArrowPart = true;
                        break;
                    }
                    curr = curr->parentItem();
                }
            }

            if (isArrowPart) {
                // Кликнули по телу стрелки -> остаемся в режиме редактирования
                return;
            } else {
                // Кликнули совсем мимо -> завершаем редактирование
                stop();
            }
        }
    }
}
void ArrowEditor::onMouseMove(const QPointF &scenePos)
{
    // Если ручка не захвачена, выйти
    if (!m_draggedHandle) return;

    // Определяем шаг сетки (в идеале нужно брать из Grid, пока 16.0)
    const qreal gridStep = 16.0;
    QPointF oldPos = m_draggedHandle->pos();
    QPointF newPos = snapToGrid(scenePos, gridStep); // Используем привязку

    // Если позиция не изменилась, ничего не делаем
    if (oldPos == newPos) return;

    // Перемещаем ручку
    m_draggedHandle->setPos(newPos);

    // Обновляем геометрию (линия + наконечник)
    updateConnection(oldPos, newPos);

    // *** 1. ЛОГИКА ДЛЯ РЕЖИМА "Отсоединения" (Шаг 4) ***
    if (m_isRepinning) {

        // 5. Логика подсветки якорей
        const qreal snapThreshold = 16.0; // Дистанция "прилипания"
        DiagramItem* bestBlock = nullptr;
        QPointF bestAnchor;
        qreal bestDist = std::numeric_limits<qreal>::infinity();

        // Ищем ближайший якорь среди ВСЕХ блоков
        if (m_scene) {
            for (QGraphicsItem* it : m_scene->items()) {
                DiagramItem* d = dynamic_cast<DiagramItem*>(it);

                // Не прилипаем к блоку, от которого УЖЕ идет стрелка
                if (!d || !m_editingGroup || d == m_editingGroup->startBlock()) {
                    continue;
                }

                qreal dist;
                QPointF local = d->nearestAnchorToScenePoint(newPos, &dist);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestBlock = d;
                    bestAnchor = local;
                }
            }
        }

        // Определяем, ЧТО сейчас должно быть подсвечено
        DiagramItem* blockToHighlight = nullptr;
        QPointF anchorToHighlight;
        if (bestBlock && bestDist < snapThreshold) {
            blockToHighlight = bestBlock;
            anchorToHighlight = bestAnchor;
        }

        // Синхронизируем состояние (снимаем старую, ставим новую)
        if (m_hoveredBlock != blockToHighlight || m_hoveredAnchor != anchorToHighlight) {
            // Снимаем старую подсветку (если она была)
            if (m_hoveredBlock) {
                m_hoveredBlock->setAnchorHighlighted(m_hoveredAnchor, false);
            }

            // Ставим новую подсветку (если она есть)
            if (blockToHighlight) {
                blockToHighlight->setAnchorHighlighted(anchorToHighlight, true);
            }

            // Обновляем кэш
            m_hoveredBlock = blockToHighlight;
            m_hoveredAnchor = anchorToHighlight;
        }
    }
    // *** КОНЕЦ ЛОГИКИ (Шаг 4) ***
}

// --- КОНЕЦ ФРАГМЕНТА ---

// --- НАЧАЛО ФРАГМЕНТА (connect.cpp, ArrowEditor::onMouseRelease) ---

void ArrowEditor::onMouseRelease(const QPointF &scenePos)
{
    Q_UNUSED(scenePos); // scenePos нам не нужен, мы используем m_hoveredBlock

    // *** 1. ЛОГИКА ДЛЯ РЕЖИМА "Отсоединения" (Шаг 7) ***
    if (m_isRepinning) {

        // Проверяем, был ли захвачен хэндл и отпустили ли мы его
        // над подсвеченным и свободным якорем
        if (m_draggedHandle && m_hoveredBlock) {

            // Проверяем, свободен ли якорь (Шаг 6)
            if (m_hoveredBlock->isAnchorFree(m_hoveredAnchor)) {


                // 1. Перемещаем наконечник точно на якорь
                QPointF finalPos = m_hoveredBlock->mapToScene(m_hoveredAnchor);
                QPointF oldPos = m_draggedHandle->pos();
                updateConnection(oldPos, finalPos);

                // 2. Логически привязываем стрелку
                m_editingGroup->setEnd(m_hoveredBlock, m_hoveredAnchor);

                // 3. Уведомляем новый блок
                m_hoveredBlock->addConnection(m_editingGroup);
                m_editingGroup->updatePath();

                // *** ВАЖНОЕ ИСПРАВЛЕНИЕ ***
                // Сообщаем, что перенос завершен успешно, ПЕРЕД вызовом stop().
                // Это предотвратит автоматическое восстановление старой связи.
                m_isRepinning = false;

                // 4. Завершаем режим
                stop();

            } else {
                // Якорь занят

                // (Ничего не делаем, остаемся в режиме открепления, ждем следующей попытки)
            }

        } else {
            // Отпустили ЛКМ в пустом месте

            // (Ничего не делаем, остаемся в режиме открепления)
        }

        m_draggedHandle = nullptr; // В любом случае отпускаем ручку
        return; // Выходим, т.к. m_isRepinning = true
    }
    // *** КОНЕЦ ЛОГИКИ (Шаг 7) ***


    // --- Старая логика (для режима редактирования сегментов) ---
    // (Этот код выполнится, только если m_isRepinning = false)
    m_draggedHandle = nullptr; // Перестаем тащить
}
// --- КОНЕЦ ФРАГМЕНТА ---

void ArrowEditor::updateConnection(const QPointF &oldPos, const QPointF &newPos)
{
    if (!m_editingGroup) return;

    // 1. Обновляем линии
    for (QGraphicsLineItem *line : m_lines) {
        QLineF lf = line->line();
        bool changed = false;

        // Используем приближенное сравнение для float
        auto aeq = [](const QPointF& p1, const QPointF& p2) {
            return qFuzzyCompare(p1.x(), p2.x()) && qFuzzyCompare(p1.y(), p2.y());
        };

        if (aeq(lf.p1(), oldPos)) {
            lf.setP1(newPos);
            changed = true;
        }
        if (aeq(lf.p2(), oldPos)) {
            lf.setP2(newPos);
            changed = true;
        }

        if (changed) {
            line->setLine(lf);
        }
    }

    // 2. Обновляем узлы (точки-кружки)
    for (QGraphicsEllipseItem *node : m_nodes) {
        if (qFuzzyCompare(node->pos().x(), oldPos.x()) && qFuzzyCompare(node->pos().y(), oldPos.y())) {
            node->setPos(newPos);
        }
    }

    // 3. Обновляем наконечник стрелы
    // 3. Обновляем наконечник стрелы
        if (m_arrowHead) {
            // Синхронизируем позицию
            if (qFuzzyCompare(m_arrowHead->pos().x(), oldPos.x()) && qFuzzyCompare(m_arrowHead->pos().y(), oldPos.y())) {
                m_arrowHead->setPos(newPos);
            }

            QGraphicsLineItem *attachedLine = nullptr;
            QPointF arrowCurrentPos = m_arrowHead->pos();

            // Ищем линию, которая заканчивается в точке наконечника
            for (QGraphicsLineItem *line : m_lines) {
                if (qFuzzyCompare(line->line().p2().x(), arrowCurrentPos.x()) &&
                    qFuzzyCompare(line->line().p2().y(), arrowCurrentPos.y())) {
                    attachedLine = line;
                    break;
                }
            }

            if (attachedLine) {
                // Вектор: от НАЧАЛА последнего сегмента (колено) -> к КОНЦУ (наконечник)
                QPointF vec = arrowCurrentPos - attachedLine->line().p1();

                qreal angleDeg = 0.0;
                if (!qFuzzyIsNull(vec.x()) || !qFuzzyIsNull(vec.y())) {
                    angleDeg = std::atan2(vec.y(), vec.x()) * 180.0 / M_PI;
                }

                // *** ИСПРАВЛЕНИЕ: Устанавливаем чистый угол без добавок ***
                m_arrowHead->setRotation(angleDeg);

                // 4. Обновляем позицию текстовых полей
                if (m_editingGroup->transition()) {
                    m_editingGroup->transition()->updateAnchor(m_lines);
                }
            }
        }

    // 5. Обновляем другие ручки, чтобы они следовали за линиями
    for (EditHandle *handle : m_handles) {
        if (handle == m_draggedHandle) continue;
        if (qFuzzyCompare(handle->pos().x(), oldPos.x()) && qFuzzyCompare(handle->pos().y(), oldPos.y())) {
            handle->setPos(newPos);
        }
    }

    // 6. ФИНАЛЬНАЯ ЗАЧИСТКА (Слияние "гармошки")
    auto isHorizontal = [&](const QLineF &line) { return qFuzzyCompare(line.p1().y(), line.p2().y()); };
    auto isVertical = [&](const QLineF &line) { return qFuzzyCompare(line.p1().x(), line.p2().x()); };

    for (int i = 0; i < m_lines.count() - 1; ++i) {
        QLineF line1 = m_lines[i]->line();
        QLineF line2 = m_lines[i+1]->line();

        bool merge = false;
        // Если два соседних сегмента коллинеарны (оба горизонтальны или оба вертикальны)
        if (isHorizontal(line1) && isHorizontal(line2) && qFuzzyCompare(line1.p1().y(), line2.p1().y())) {
             merge = true;
        }
        else if (isVertical(line1) && isVertical(line2) && qFuzzyCompare(line1.p1().x(), line2.p1().x())) {
             merge = true;
        }

        if (merge) {
            // Сливаем: удлиняем [i] до конца [i+1]
            m_lines[i]->setLine(QLineF(line1.p1(), line2.p2()));

            // Удаляем [i+1]
            QGraphicsLineItem* toDelete = m_lines.takeAt(i+1);

            // *** КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: сообщаем группе об удалении ***
            m_editingGroup->removeInternalLine(toDelete);
            // ************************************************************

            if (toDelete && toDelete->scene()) {
                toDelete->scene()->removeItem(toDelete);
            }
            delete toDelete;

            // Удаляем ручку, которая была на месте стыка
            for (int h = 0; h < m_handles.count(); ++h) {
                // Используем нечеткое сравнение для надежности
                if (qFuzzyCompare(m_handles[h]->pos().x(), line1.p2().x()) &&
                    qFuzzyCompare(m_handles[h]->pos().y(), line1.p2().y())) {
                    EditHandle* handleToDelete = m_handles.takeAt(h);
                    if (handleToDelete->scene()) {
                        handleToDelete->scene()->removeItem(handleToDelete);
                    }
                    delete handleToDelete;
                    break;
                }
            }

            i--; // Проверяем этот же индекс заново (вдруг нужно слить еще раз)
        }
    }
}

ConnectionItem::ConnectionItem(QGraphicsItem *parent)
    : QGraphicsItemGroup(parent)
{
    // Генерируем уникальный UUID (например: "{67C8770B-44F1-410A-AB9A-F9B5446F13EE}")
        m_uid = QUuid::createUuid().toString();
    // *** ГЛАВНОЕ ИСПРАВЛЕНИЕ: ***
        // Мы должны разрешить детям (TransitionGroup) получать свои события мыши.
        // По умолчанию QGraphicsItemGroup перехватывает их (ставит true).
        // Мы ставим false, чтобы клик доходил до TransitionGroup.
        setHandlesChildEvents(false);
    //
        // *** ДОБАВЬТЕ ЭТУ СТРОКУ ***
        //
        // Говорим группе, чтобы она сама обрабатывала события мыши,
        // используя свой собственный shape(), а не передавала их дочерним элементам.
      //  setHandlesChildEvents(false);
        // --- ДОБАВИТЬ ЭТУ СТРОКУ (ОБЯЗАТЕЛЬНО!) ---
            // Это заставляет группу вызывать метод paint(). Без этого не заработает.
            setFlag(QGraphicsItem::ItemHasNoContents, false);

    // Устанавливаем флаг, чтобы группа не наследовала трансформации (если нужно)
    // setFlag(QGraphicsItem::ItemIgnoresTransformations); // (Опционально)
        // 2. Эта строка заставляет Qt РИСОВАТЬ ВЫДЕЛЕНИЕ по shape(),
            //    а не по прямоугольному boundingRect()
        // Убедимся, что саму группу можно выбрать
            setFlag(QGraphicsItem::ItemIsSelectable, true);
            // Инициализация флага
                m_isHighlighted = false;
                m_isError = false;
                    m_isFalse = false;
}
// Деструктор ConnectionItem
ConnectionItem::~ConnectionItem()
{


    // 1. Уведомляем начальный блок, что мы от него отсоединяемся
    if (m_startBlock) {
        m_startBlock->removeConnection(this);
    }

    // 2. Уведомляем конечный блок
    if (m_endBlock) {
        m_endBlock->removeConnection(this);
    }

    // (Qt автоматически удалит всех детей (линии, узлы, текст),
    // так как они были добавлены в QGraphicsItemGroup)
}
// Переключение состояния блокировки

void ConnectionItem::resetVisualState()
{
    // --- ИСПРАВЛЕНИЕ: Сначала гасим все "лампочки" дебаггера ---
        m_isHighlighted = false;
        m_isError = false;
        m_isFalse = false;

    // Возвращаем обычный черный цвет
    QPen pen(Qt::black, 2, Qt::SolidLine);
    QBrush brush(Qt::black);

    // Применяем перо ко всем линиям
    for (QGraphicsLineItem *line : m_lines) {
        line->setPen(pen);
    }

    // Применяем кисть к наконечнику и узлам
    if (m_arrowHead) m_arrowHead->setBrush(brush);
    for (QGraphicsEllipseItem *node : m_nodes) node->setBrush(brush);

    // Обновляем (чтобы убрать возможные цветные ореолы, если они были)
    update();
}
// Контекстное меню по ПКМ

void ConnectionItem::setHighlight(bool active)
{
    if (m_isHighlighted != active) {
        m_isHighlighted = active;
        update(); // Перерисовать стрелку
    }
}
void ConnectionItem::setError(bool active)
{
    if (m_isError != active) {
        m_isError = active;
        update(); // Перерисовать
    }
}
void ConnectionItem::setFalsePath(bool active)
{
    if (m_isFalse != active) {
        m_isFalse = active;
        update();
    }
}
void ConnectionItem::setStart(DiagramItem *block, const QPointF &localAnchor)
{
    m_startBlock = block;
    m_startAnchorLocal = localAnchor;
}

void ConnectionItem::setEnd(DiagramItem *block, const QPointF &localAnchor)
{
    m_endBlock = block;
    m_endAnchorLocal = localAnchor;
}

// Собирает все "запчасти" стрелки
void ConnectionItem::registerItem(QGraphicsItem *item)
{
    if (!item) return;

    if (QGraphicsLineItem *line = dynamic_cast<QGraphicsLineItem*>(item)) {
        m_lines.append(line);
    } else if (QGraphicsEllipseItem *node = dynamic_cast<QGraphicsEllipseItem*>(item)) {
        m_nodes.append(node);
    } else if (QGraphicsPolygonItem *arrow = dynamic_cast<QGraphicsPolygonItem*>(item)) {
        m_arrowHead = arrow;
    } // --- ИЗМЕНЕНИЕ НАЧАЛО ---
    else if (TransitionGroup *tg = dynamic_cast<TransitionGroup*>(item)) {
            m_transition = tg;





            m_transition->setParentItem(this);
            // (Теперь это просто дочерний элемент. Он движется со стрелкой,
            // но обрабатывает свои клики САМ).
        }

}

// Вызывается после того, как все registerItem() завершены
void ConnectionItem::finalizeConstruction()
{
    if (!m_startBlock || !m_endBlock) return;

    // Кэшируем начальные позиции
    m_lastStartPos = m_startBlock->mapToScene(m_startAnchorLocal);
    m_lastEndPos = m_endBlock->mapToScene(m_endAnchorLocal);
}

// Главный метод, вызываемый из DiagramItem::itemChange
// В connect.cpp

// Главный метод, вызываемый из DiagramItem::itemChange
// В файле connect.cpp

// В connect.cpp

void ConnectionItem::updatePosition()
{
    if (!m_startBlock || !m_endBlock) return;

    // 1. Получаем новые позиции якорей
    QPointF newStartPos = m_startBlock->mapToScene(m_startAnchorLocal);
    QPointF newEndPos = m_endBlock->mapToScene(m_endAnchorLocal);

    // 2. Проверяем, изменилось ли что-то (сравниваем с кэшем из updatePath)
    bool startMoved = !qFuzzyCompare(m_lastStartPos.x(), newStartPos.x()) ||
                      !qFuzzyCompare(m_lastStartPos.y(), newStartPos.y());
    bool endMoved = !qFuzzyCompare(m_lastEndPos.x(), newEndPos.x()) ||
                    !qFuzzyCompare(m_lastEndPos.y(), newEndPos.y());

    if (!startMoved && !endMoved) {
        return; // Ничего не изменилось
    }

    // 3. Всегда вызываем ПОЛНУЮ перестройку пути
    // (Это вернет поведение, где вся линия перестраивается с учетом рангов)
    updatePath();
}


// --- НОВЫЕ МЕТОДЫ, КОТОРЫЕ НУЖНО ДОБАВИТЬ СЛЕДОМ ---

// В файле connect.cpp

// --------------------------------------------------------------------
// Вспомогательный метод: определяет вектор нормали от грани блока
// (куда должна выходить стрелка, чтобы не налезть на блок)
// --------------------------------------------------------------------
QPointF ConnectionItem::getOutwardNormal(const QRectF &bound, const QPointF &anchor)
{
    qreal distLeft = qAbs(anchor.x() - bound.left());
    qreal distRight = qAbs(anchor.x() - bound.right());
    qreal distTop = qAbs(anchor.y() - bound.top());
    qreal distBottom = qAbs(anchor.y() - bound.bottom());

    qreal minDist = qMin(qMin(distLeft, distRight), qMin(distTop, distBottom));

    if (qFuzzyCompare(minDist, distLeft))   return QPointF(-1, 0);
    if (qFuzzyCompare(minDist, distRight))  return QPointF(1, 0);
    if (qFuzzyCompare(minDist, distTop))    return QPointF(0, -1);
    if (qFuzzyCompare(minDist, distBottom)) return QPointF(0, 1);
    return QPointF(0, 0);
}

// --------------------------------------------------------------------
// ПОЛНОСТЬЮ ОБНОВЛЕННЫЙ метод построения ортогонального пути
// --------------------------------------------------------------------
// В файле connect.cpp

// ... (метод getOutwardNormal оставляем без изменений) ...

// В файле connect.cpp

// В файле connect.cpp

// В файле connect.cpp

// В файле connect.cpp

// В connect.cpp
// В connect.cpp
// Добавьте эту новую функцию (можно после updatePath)

// В connect.cpp


// В connect.cpp


void ConnectionItem::updatePath()
{
    prepareGeometryChange();

    // --- ИЗМЕНЕНИЕ: Правильное удаление из группы ---
    for (auto line : m_lines) {
        removeFromGroup(line); // Важно для пересчета границ группы
        delete line;
    }
    m_lines.clear();

    for (auto node : m_nodes) {
        removeFromGroup(node); // Важно для пересчета границ группы
        delete node;
    }
    m_nodes.clear();
    // ------------------------------------------------

    if (!m_startBlock || !m_endBlock) return;

    // 1. Получаем координаты и геометрию
    QPointF start = m_startBlock->mapToScene(m_startAnchorLocal);
    QPointF end = m_endBlock->mapToScene(m_endAnchorLocal);
    QRectF startRect = m_startBlock->sceneBoundingRect();
    QRectF endRect = m_endBlock->sceneBoundingRect();

    // 2. Настройки
    const qreal gridStep = 16.0;
    const qreal baseMargin = 16.0;

    auto snap = [gridStep](qreal val) -> qreal {
        return std::round(val / gridStep) * gridStep;
    };

    // 3. РАСЧЕТ ДИНАМИЧЕСКОГО ОТСТУПА
    int startRank = m_startBlock->getConnectionRank(this);
    int endRank = m_endBlock->getConnectionRank(this);

    // Индивидуальные отступы для первых сегментов (сохраняют локальную "лесенку")
    qreal startMargin = baseMargin + (startRank * gridStep);
    qreal endMargin = baseMargin + (endRank * gridStep);

    QPointF startNormal = getOutwardNormal(startRect, start);
    QPointF endNormal = getOutwardNormal(endRect, end);

    QPointF startStandoff = start + startNormal * startMargin;
    QPointF endStandoff = end + endNormal * endMargin;

    if (!qFuzzyIsNull(startNormal.x())) startStandoff.setX(snap(startStandoff.x()));
    else startStandoff.setY(snap(startStandoff.y()));

    if (!qFuzzyIsNull(endNormal.x())) endStandoff.setX(snap(endStandoff.x()));
    else endStandoff.setY(snap(endStandoff.y()));

    // 4. Строим базовый путь
    QList<QPointF> points;
    points.append(start);
    points.append(startStandoff);

    // --- ЛОГИКА МАРШРУТИЗАЦИИ ---
    bool startIsHoriz = !qFuzzyIsNull(startNormal.x());
    bool endIsHoriz = !qFuzzyIsNull(endNormal.x());
    QRectF unionRect = startRect.united(endRect);

    // *** УЛУЧШЕННАЯ ФОРМУЛА ОТСТУПА ДЛЯ ОБХОДА ***
    // (startRank + endRank) * gridStep -> основной отступ, зависит от загруженности обоих концов.
    // + (startRank * 8.0) -> "тай-брейкер", чтобы развести симметричные пары (например, 0->1 и 1->0).
    qreal goAroundMargin = baseMargin + (startRank + endRank) * gridStep + (startRank * 8.0);

    if (startIsHoriz != endIsHoriz)
    {
        // === L-SHAPE ===
        QPointF corner(startIsHoriz ? startStandoff.x() : endStandoff.x(),
                       startIsHoriz ? endStandoff.y() : startStandoff.y());

        if (unionRect.adjusted(-gridStep, -gridStep, gridStep, gridStep).contains(corner)) {
             // Z-образный обход
             if (startIsHoriz) {
                 points.append(QPointF(startStandoff.x(), endStandoff.y()));
             } else {
                 points.append(QPointF(endStandoff.x(), startStandoff.y()));
             }
        } else {
             // Простой L-путь
             points.append(corner);
        }
    }
    else
    {
        // === Z-SHAPE или U-SHAPE ===
        if (startIsHoriz) {
            // --- Горизонтальные ---
            if (qFuzzyCompare(startNormal.x(), endNormal.x())) {
                // U-shape (полный обход) -> используем goAroundMargin
                qreal goAroundY = (startStandoff.y() > endStandoff.y())
                        ? unionRect.bottom() + goAroundMargin
                        : unionRect.top() - goAroundMargin;
                goAroundY = snap(goAroundY);
                points.append(QPointF(startStandoff.x(), goAroundY));
                points.append(QPointF(endStandoff.x(), goAroundY));
            } else {
                // Z-shape (навстречу)
                bool facingEachOther = (startNormal.x() > 0 && endStandoff.x() > startStandoff.x()) ||
                                       (startNormal.x() < 0 && endStandoff.x() < startStandoff.x());
                if (facingEachOther) {
                    points.append(QPointF(startStandoff.x(), endStandoff.y()));
                } else {
                    // G-shape обход -> используем goAroundMargin
                    qreal goAroundY = (startStandoff.y() > endStandoff.y())
                            ? unionRect.bottom() + goAroundMargin
                            : unionRect.top() - goAroundMargin;
                    goAroundY = snap(goAroundY);
                    points.append(QPointF(startStandoff.x(), goAroundY));
                    points.append(QPointF(endStandoff.x(), goAroundY));
                }
            }
        } else {
            // --- Вертикальные ---
            if (qFuzzyCompare(startNormal.y(), endNormal.y())) {
                // U-shape (полный обход) -> используем goAroundMargin
                qreal goAroundX = (startStandoff.x() > endStandoff.x())
                        ? unionRect.right() + goAroundMargin
                        : unionRect.left() - goAroundMargin;
                goAroundX = snap(goAroundX);
                points.append(QPointF(goAroundX, startStandoff.y()));
                points.append(QPointF(goAroundX, endStandoff.y()));
            } else {
                // Z-shape
                bool facingEachOther = (startNormal.y() > 0 && endStandoff.y() > startStandoff.y()) ||
                                       (startNormal.y() < 0 && endStandoff.y() < startStandoff.y());
                if (facingEachOther) {
                    points.append(QPointF(endStandoff.x(), startStandoff.y()));
                } else {
                    // G-shape обход -> используем goAroundMargin
                    qreal goAroundX = (startStandoff.x() > endStandoff.x())
                            ? unionRect.right() + goAroundMargin
                            : unionRect.left() - goAroundMargin;
                    goAroundX = snap(goAroundX);
                    points.append(QPointF(goAroundX, startStandoff.y()));
                    points.append(QPointF(goAroundX, endStandoff.y()));
                }
            }
        }
    }

    points.append(endStandoff);
    points.append(end);

    // 5. Упрощение пути
    for (int i = points.count() - 2; i > 0; --i) {
        bool sameX = qFuzzyCompare(points[i-1].x(), points[i].x()) && qFuzzyCompare(points[i].x(), points[i+1].x());
        bool sameY = qFuzzyCompare(points[i-1].y(), points[i].y()) && qFuzzyCompare(points[i].y(), points[i+1].y());
        if (points[i] == points[i-1] || sameX || sameY) {
            points.removeAt(i);
        }
    }

    // 6. Создаем графические сегменты
    for (int i = 0; i < points.count() - 1; ++i) {
        QGraphicsLineItem* line = new QGraphicsLineItem(this);
        QPointF p1 = points[i];
        QPointF p2 = points[i+1];
        line->setLine(QLineF(p1, p2));
        QPen pen(Qt::black, 2);
        line->setPen(pen);

        // --- ИЗМЕНЕНИЕ: Явно добавляем в группу ---
        addToGroup(line);
        // ------------------------------------------

        m_lines.append(line);

        if (i > 1) {
            constexpr qreal size = 3.0;
            QGraphicsEllipseItem *bendMarker = new QGraphicsEllipseItem(-size/2, -size/2, size, size, this);
            bendMarker->setBrush(Qt::black);
            bendMarker->setPen(Qt::NoPen);
            bendMarker->setZValue(1000);
            bendMarker->setPos(p1);

            // --- ИЗМЕНЕНИЕ: Явно добавляем в группу ---
            addToGroup(bendMarker);
            // ------------------------------------------

            m_nodes.append(bendMarker);
        }
    }

    m_lastStartPos = start;
    m_lastEndPos = end;
    m_pathInitialized = true;

    updateDecorations();

    // --- ИЗМЕНЕНИЕ: Убедимся, что наконечник в группе ---
    if (m_arrowHead && m_arrowHead->group() != this) {
        addToGroup(m_arrowHead);
    }
    // ----------------------------------------------------
}
void ConnectionItem::updateDecorations()
{
    if (m_lines.isEmpty()) return;

    // Ищем последний сегмент с ненулевой длиной
    QLineF lastSeg;
    bool found = false;
    for (int i = m_lines.count() - 1; i >= 0; --i) {
        lastSeg = m_lines[i]->line();
        // ИСПРАВЛЕНИЕ: используем qFuzzyIsNull для длины,
        // так как qFuzzyCompare не работает с QPointF напрямую.
        if (!qFuzzyIsNull(lastSeg.length())) {
            found = true;
            break;
        }
    }

    // Если все сегменты нулевой длины (странно, но бывает), берем самый последний как есть
    if (!found) {
        lastSeg = m_lines.last()->line();
    }

    QPointF vec = lastSeg.p2() - lastSeg.p1();
    qreal angleDeg = std::atan2(vec.y(), vec.x()) * 180.0 / M_PI;

    // 1. Обновляем наконечник
    if (m_arrowHead) {
        // Важно: позицию берем от САМОГО последнего сегмента, даже если он нулевой,
        // чтобы стрелка всегда была точно в конце пути.
        m_arrowHead->setPos(m_lines.last()->line().p2());
        m_arrowHead->setRotation(angleDeg);
    }

    // 2. Обновляем метки (Action / If)
    // Теперь делегируем позиционирование классу TransitionGroup
    if (m_transition) {
        // Передаем ВСЕ линии, чтобы группа могла "скользить" по ним и обновлять свою позицию
        m_transition->updateAnchor(m_lines);
    }
}

// ПРИВАТНЫЙ МЕТОД:
// Этот код - почти полная копия ArrowEditor::updateConnection
// Он обновляет все дочерние элементы, когда точка oldPos переместилась в newPos
void ConnectionItem::updateGeometry(const QPointF &oldPos, const QPointF &newPos)
{
    // 1. Обновляем линии
    for (QGraphicsLineItem *line : m_lines) {
        QLineF lf = line->line();
        bool changed = false;

        // Используем приближенное сравнение для float
        auto aeq = [](const QPointF& p1, const QPointF& p2) {
            return qFuzzyCompare(p1.x(), p2.x()) && qFuzzyCompare(p1.y(), p2.y());
        };

        if (aeq(lf.p1(), oldPos)) {
            lf.setP1(newPos);
            changed = true;
        }
        if (aeq(lf.p2(), oldPos)) {
            lf.setP2(newPos);
            changed = true;
        }

        if (changed) {
            line->setLine(lf);
        }
    }

    // 2. Обновляем узлы (точки-кружки)
    for (QGraphicsEllipseItem *node : m_nodes) {
        if (qFuzzyCompare(node->pos().x(), oldPos.x()) && qFuzzyCompare(node->pos().y(), oldPos.y())) {
            node->setPos(newPos);
        }
    }

    // 3. Обновляем наконечник стрелы
    if (m_arrowHead) {
        // 3a. Сначала двигаем наконечник, ЕСЛИ мы двигаем конечную точку
        if (qFuzzyCompare(m_arrowHead->pos().x(), oldPos.x()) && qFuzzyCompare(m_arrowHead->pos().y(), oldPos.y())) {
            m_arrowHead->setPos(newPos);
        }

        // 3b. Обновляем вращение наконечника
        // Найдем линию, к которой прикреплен наконечник
        QGraphicsLineItem *attachedLine = nullptr;
        QPointF arrowCurrentPos = m_arrowHead->pos(); // Берем текущую позицию

        for (QGraphicsLineItem *line : m_lines) {
            if (qFuzzyCompare(line->line().p2().x(), arrowCurrentPos.x()) &&
                qFuzzyCompare(line->line().p2().y(), arrowCurrentPos.y())) {
                attachedLine = line;
                break;
            }
        }

        // Если нашли (это должен быть m_lines.last())
        if (attachedLine) {
            QPointF vec = arrowCurrentPos - attachedLine->line().p1();
            qreal angleDeg = 0.0;
            if (!qFuzzyIsNull(vec.x()) || !qFuzzyIsNull(vec.y())) {
                angleDeg = std::atan2(vec.y(), vec.x()) * 180.0 / M_PI;
            }
            m_arrowHead->setRotation(angleDeg);

            // 4. Обновляем позицию TransitionGroup
            // Передаем обновленный список линий, группа сама найдет нужный сегмент
            if (m_transition) {
                m_transition->updateAnchor(m_lines);
            }
        }
    }
}
// Реализация ConnectionItem::shape()
QPainterPath ConnectionItem::shape() const
{
    QPainterPath path;

    // --- 1. Добавляем "утолщенные" линии ---
    if (!m_lines.isEmpty())
    {
        QPainterPathStroker stroker;

        // Настраиваем "утолщение" (hit-box)
        // Возьмем перо из первой линии
        QPen linePen = m_lines.first()->pen();

        // Устанавливаем ширину для "попадания" (hit-box)
        // (например, 8 пикселей "подушка" + реальная ширина линии)
        const qreal hitTestWidth = 8.0;
        stroker.setWidth(linePen.widthF() + hitTestWidth);
        stroker.setCapStyle(Qt::FlatCap);
        stroker.setJoinStyle(Qt::MiterJoin); // Важно для Г-образных углов

        // Сначала строим "скелет" (тонкий путь из всех линий)
        QPainterPath skeletonPath;
        for (const QGraphicsLineItem* line : m_lines)
        {
            skeletonPath.moveTo(line->line().p1());
            skeletonPath.lineTo(line->line().p2());
        }

        // Добавляем "утолщенный" скелет к итоговой форме
        path.addPath(stroker.createStroke(skeletonPath));
    }

    // --- 2. Добавляем узлы (точки-кружки) ---
    for (const QGraphicsEllipseItem *node : m_nodes)
    {
        // mapToParent() преобразует локальную форму (эллипс)
        // в координаты группы (учитывая pos() узла).
        path.addPath(node->mapToParent(node->shape()));
    }

    // --- 3. Добавляем наконечник стрелы ---
    if (m_arrowHead)
    {
        // mapToParent() учтет и pos(), и rotation() наконечника.
        path.addPath(m_arrowHead->mapToParent(m_arrowHead->shape()));
    }

    // Мы НАМЕРЕННО НЕ добавляем формы m_labelBgs и m_labels

    return path;
}
void ConnectionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    // --- УМНАЯ СМЕНА ЦВЕТА СТРЕЛОК (СВЕТЛАЯ/ТЁМНАЯ ТЕМА) ---
        // Поскольку тёмную тему мы задаем через глобальный стиль qApp,
        // проверяем: если стиль не пустой, значит включена Тёмная тема.
        bool isDarkTheme = !qApp->styleSheet().isEmpty();
        QColor arrowColor = isDarkTheme ? Qt::white : Qt::black;

        // Если цвет линий не совпадает с темой — перекрашиваем их
        // (Проверка != arrowColor спасает программу от зависания и бесконечного цикла отрисовки)
        if (!m_lines.isEmpty() && m_lines.first() && m_lines.first()->pen().color() != arrowColor) {
            for (QGraphicsLineItem* line : m_lines) {
                if (line) {
                    QPen p = line->pen();
                    p.setColor(arrowColor);
                    line->setPen(p);
                }
            }
            if (m_arrowHead) {
                m_arrowHead->setBrush(arrowColor);
                m_arrowHead->setPen(QPen(arrowColor, 1));
            }
        }
    // 1. ПРИОРИТЕТ 1: ОШИБКА (КРАСНАЯ + ТРЕУГОЛЬНИК)
    if (m_isError) {
        painter->save();
        QPen errorPen(Qt::red);
        errorPen.setWidth(6);
        errorPen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(errorPen);
        painter->setBrush(Qt::NoBrush);

        QPainterPath path;
        for (const QGraphicsLineItem* line : m_lines) {
            if (line) {
                path.moveTo(line->line().p1());
                path.lineTo(line->line().p2());
            }
        }
        painter->drawPath(path);

        // ТРЕУГОЛЬНИК "!"
        if (!m_lines.isEmpty()) {
                    int midIndex = m_lines.count() / 2;
                    if (m_lines[midIndex]) {
                        QLineF lineGeom = m_lines[midIndex]->line();
                        QPointF center = lineGeom.pointAt(0.5);

                        // --- НОВОЕ: СМЕЩЕНИЕ ЗНАЧКА ВБОК ---
                        // Вычисляем вектор линии
                        qreal dx = lineGeom.dx();
                        qreal dy = lineGeom.dy();
                        qreal length = std::sqrt(dx * dx + dy * dy);

                        // Сдвиг (в пикселях)
                        qreal offset = 25.0;

                        if (length > 0) {
                            // Нормализуем и поворачиваем на 90 градусов (-dy, dx)
                            // Это создает перпендикуляр к линии движения
                            qreal nx = -dy / length;
                            qreal ny = dx / length;

                            // Применяем смещение к центру
                            center.setX(center.x() + nx * offset);
                            center.setY(center.y() + ny * offset);
                        }
                        // -----------------------------------

                        painter->setBrush(Qt::yellow);
                        painter->setPen(QPen(Qt::black, 2));

                        qreal size = 24;
                        QPointF p1(center.x(), center.y() - size/2);
                        QPointF p2(center.x() + size/2, center.y() + size/2);
                        QPointF p3(center.x() - size/2, center.y() + size/2);

                        QPolygonF tri;
                        tri << p1 << p2 << p3;
                        painter->drawPolygon(tri);

                        painter->setPen(Qt::black);
                        QFont f = painter->font();
                        f.setBold(true);
                        f.setPixelSize(16);
                        painter->setFont(f);

                        QRectF textRect(center.x() - size/2, center.y() - size/2, size, size);
                        painter->drawText(textRect, Qt::AlignCenter, "!");
                    }
                }
        painter->restore();
    }
    // 2. ПРИОРИТЕТ 2: ЛОЖНЫЙ ПУТЬ (ПРОСТО КРАСНЫЙ)
    else if (m_isFalse) {
        painter->save();
        QPen falsePen(Qt::red);
        falsePen.setWidth(6);
        falsePen.setJoinStyle(Qt::RoundJoin);

        painter->setPen(falsePen);
        painter->setBrush(Qt::NoBrush);

        QPainterPath path;
        for (const QGraphicsLineItem* line : m_lines) {
            if (line) {
                path.moveTo(line->line().p1());
                path.lineTo(line->line().p2());
            }
        }
        painter->drawPath(path);
        painter->restore();
    }
    // 3. ПРИОРИТЕТ 3: ТРАССИРОВКА (ЗЕЛЕНАЯ)
    else if (m_isHighlighted) {
        painter->save();
        QPen highlightPen(Qt::green);
        highlightPen.setWidth(6);
        highlightPen.setJoinStyle(Qt::RoundJoin);
        highlightPen.setCapStyle(Qt::RoundCap);

        painter->setPen(highlightPen);
        painter->setBrush(Qt::NoBrush);

        QPainterPath path;
        for (const QGraphicsLineItem* line : m_lines) {
            if (line) {
                path.moveTo(line->line().p1());
                path.lineTo(line->line().p2());
            }
        }
        painter->drawPath(path);
        painter->restore();
    }

    // 4. ВЫДЕЛЕНИЕ (ПУНКТИР)
    if (option->state & QStyle::State_Selected) {
        painter->save();
        QPen selectionPen;
        selectionPen.setColor(Qt::black);
        selectionPen.setStyle(Qt::DashLine);
        selectionPen.setWidth(0);
        painter->setPen(selectionPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(shape());
        painter->restore();
    }

}
