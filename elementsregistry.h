/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef ELEMENTSREGISTRY_H
#define ELEMENTSREGISTRY_H

#include <QObject>
#include <QList>
#include <QPointer>
#include <QPointF>
#include <QLineF>
#include <QVariantMap>
#include <QUuid>

class QGraphicsItem;
class QGraphicsLineItem;
class QGraphicsPolygonItem;
class QGraphicsEllipseItem;
class DiagramItem; // ваш блок (предположительно DiagramItem)

// Класс ElementsRegistry — централизованный реестр и хранитель "независимых" данных
// для якорей, линий, стрел, мнимых (phantom) элементов, регистрации и связей с блоками.
// Характеристики:
//  - содержит структуры описания каждого типа сущности,
//  - предоставляет публичные API для регистрации/удаления/запроса,
//  - содержит вложенный State (RegistryState) для сохранения и восстановления состояния.
// Реализация методов оставлена на .cpp (здесь только объявления).
class ElementsRegistry : public QObject
{
    Q_OBJECT
public:
    explicit ElementsRegistry(QObject *parent = nullptr);
    ~ElementsRegistry() override;

    // ---- Идентификаторы ----
    // Каждый элемент в реестре получает уникальный QUuid id.
    using Id = QUuid;

    // ---- Структуры описания сущностей ----

    // Описание якорной точки (anchor)
    struct AnchorInfo {
        Id id;                                   // уникальный идентификатор якоря
        QPointer<QGraphicsEllipseItem> item;     // визуальный QGraphicsItem (если есть)
        QPointF pos;                             // позиция в координатах сцены (логические координаты)
        QVariantMap meta;                        // дополнительные метаданные (ключ-значение)
        DiagramItem* attachedBlock = nullptr;    // если якорь привязан к блоку — обычный указатель на блок
        QString role;                            // роль/тип точки (например "in", "out", "connection")
    };

    // Описание линии (edge)
    struct LineInfo {
        Id id;                                   // уникальный идентификатор линии
        QPointer<QGraphicsLineItem> item;        // визуальный QGraphicsLineItem (если есть)
        Id anchorA;                              // id первой опорной точки (может быть пустой)
        Id anchorB;                              // id второй опорной точки (может быть пустой)
        QList<QPointF> path;                     // дополнительные узлы пути (если ломаная)
        QVariantMap meta;                        // дополнительные метаданные (толщина, стиль и т.п.)
    };

    // Описание наконечника стрелы (arrow head)
    struct ArrowHeadInfo {
        Id id;
        QPointer<QGraphicsPolygonItem> item;     // визуальный полигон/треугольник
        Id attachedLine;                         // id линии, к которой прикреплён наконечник
        QVariantMap meta;                        // направление, размер, стиль и т.п.
    };

    // Описание "мнимой" (phantom) сущности — используется для временных операций (drag, preview)
    struct PhantomLineInfo {
        Id id;
        QPointer<QGraphicsLineItem> item;        // временная линия (не постоянная)
        QPointF from;
        QPointF to;
        QVariantMap meta;                        // например: isTemporary=true, style=...
    };

    struct PhantomPointInfo {
        Id id;
        QPointer<QGraphicsEllipseItem> item;     // временная точка
        QPointF pos;
        QVariantMap meta;
    };

    struct PhantomArrowInfo {
        Id id;
        QPointer<QGraphicsPolygonItem> item;     // временная стрелка
        QPointF pos;
        QVariantMap meta;
    };

    // Запись регистрации — связь между логической сущностью и визуальным элементом
    struct RegistrationEntry {
        Id id;                                   // id зарегистрированной сущности
        QString type;                            // "anchor", "line", "arrow", "phantom", ...
        QVariantMap meta;                        // доп. инфо: источник, время, user data
    };

    // ---- State: отдельный под-структ state внутри класса (без реализации) ----
    // Aggregated state snapshot. В дальнейшем можно сериализовать/десериализовать.
    struct RegistryState {
        QList<AnchorInfo> anchors;               // все зарегистрированные якори
        QList<LineInfo>   lines;                 // все зарегистрированные линии
        QList<ArrowHeadInfo> arrows;             // зарегистрированные наконечники стрел
        QList<PhantomLineInfo> phantomLines;     // временные линии
        QList<PhantomPointInfo> phantomPoints;   // временные точки
        QList<PhantomArrowInfo> phantomArrows;   // временные стрелы
        QList<RegistrationEntry> registrations;  // история/реестр регистраций
        QVariantMap globalMeta;                  // общие метаданные (версия, timestamp и т.п.)

        // Примечание: реализация сериализации/копирования делается в .cpp при необходимости.
    };

    // ---- Публичный API (объявления, реализация в .cpp) ----

    // --- Anchors ---
    Id registerAnchor(const AnchorInfo &info);         // добавить/зарегистрировать якорь, вернуть id
    bool unregisterAnchor(const Id &anchorId);        // удалить якорь по id
    AnchorInfo findAnchor(const Id &anchorId) const;  // найти якорь (если не найден — вернуть пустой AnchorInfo или бросить)
    QList<AnchorInfo> anchors() const;                // получить все якори

    // --- Lines ---
    Id registerLine(const LineInfo &info);
    bool unregisterLine(const Id &lineId);
    LineInfo findLine(const Id &lineId) const;
    QList<LineInfo> lines() const;

    // --- Arrow heads ---
    Id registerArrowHead(const ArrowHeadInfo &info);
    bool unregisterArrowHead(const Id &arrowId);
    ArrowHeadInfo findArrowHead(const Id &arrowId) const;
    QList<ArrowHeadInfo> arrowHeads() const;

    // --- Phantom (temporary preview) ---
    Id createPhantomLine(const PhantomLineInfo &info);
    bool removePhantomLine(const Id &phantomId);
    QList<PhantomLineInfo> phantomLines() const;

    Id createPhantomPoint(const PhantomPointInfo &info);
    bool removePhantomPoint(const Id &phantomId);
    QList<PhantomPointInfo> phantomPoints() const;

    Id createPhantomArrow(const PhantomArrowInfo &info);
    bool removePhantomArrow(const Id &phantomId);
    QList<PhantomArrowInfo> phantomArrows() const;

    // --- Registration management ---
    void addRegistration(const RegistrationEntry &entry);
    bool removeRegistration(const Id &regId);
    QList<RegistrationEntry> registrations() const;

    // --- Connections / attaching anchors to blocks ---
    bool attachAnchorToBlock(const Id &anchorId, DiagramItem *block); // связывает якорь с блоком
    bool detachAnchorFromBlock(const Id &anchorId);                   // отвязать
    QList<Id> anchorsForBlock(DiagramItem *block) const;              // получить якори, связанные с блоком

    // --- State save/restore (без реализации здесь) ---
    RegistryState saveState() const;           // сделать снимок текущего состояния (вернуть RegistryState)
    void restoreState(const RegistryState &state); // восстановить из RegistryState

signals:
    // Сигналы об изменениях — позволяют подписываться на события реестра
    void anchorRegistered(const ElementsRegistry::AnchorInfo &info);
    void anchorUnregistered(const ElementsRegistry::Id &anchorId);

    void lineRegistered(const ElementsRegistry::LineInfo &info);
    void lineUnregistered(const ElementsRegistry::Id &lineId);

    void arrowRegistered(const ElementsRegistry::ArrowHeadInfo &info);
    void arrowUnregistered(const ElementsRegistry::Id &arrowId);

    void phantomCreated(const ElementsRegistry::Id &phantomId);
    void phantomRemoved(const ElementsRegistry::Id &phantomId);

    void registrationAdded(const ElementsRegistry::RegistrationEntry &entry);
    void registrationRemoved(const ElementsRegistry::Id &regId);

public slots:
    // Небольшие удобные слоты-обёртки (реализация в .cpp)
    void clearAllPhantoms();                    // удалить все временные объекты (phantom)
    void clearAll();                            // очистить реестр полностью

private:
    // ---- Внутренние контейнеры хранения (не экспонируем напрямую) ----
    // В .cpp будет реализована безопасная работа с ними (по id, мьютексы/блокировки если нужно).
    QList<AnchorInfo> m_anchors;
    QList<LineInfo>   m_lines;
    QList<ArrowHeadInfo> m_arrowHeads;

    QList<PhantomLineInfo> m_phantomLines;
    QList<PhantomPointInfo> m_phantomPoints;
    QList<PhantomArrowInfo> m_phantomArrows;

    QList<RegistrationEntry> m_registrations;

    // Дополнительные быстрые индексы можно добавить в .cpp (QHash<Id, index>) для ускорения поиска.
};

#endif // ELEMENTSREGISTRY_H
