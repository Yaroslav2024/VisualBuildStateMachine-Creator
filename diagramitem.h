/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef DIAGRAMITEM_H
#define DIAGRAMITEM_H

#include <QGraphicsTextItem>
#include "resizehandle.h"
#include <QGraphicsRectItem> // Подключаем базовый класс QGraphicsRectItem для создания прямоугольных графических объектов на сцене
#include "grid.h"  // чтобы компилятор знал про класс Grid
// --- ФЛАГ ДЛЯ ВЫБОРОЧНОЙ ОТЛАДКИ ---
#define ROLE_DEBUG_ENABLED (Qt::UserRole + 100)
// Удаляем EditableTextItem, так как редактирование теперь через диалог
class ResizeHandle;
class ConnectionItem;

struct DiagramItemState {
    QRectF rect;
    QPointF pos;
    // Данные блока
    QString type;
    QString description;
    QString label;
    QString onEnter;
    QString onExit;
};
// Это уникальный ключ, по которому мы будем искать ID внутри объекта
const int ItemIdRole = Qt::UserRole + 1337;
// Класс DiagramItem — представляет элемент диаграммы, который можно размещать на сцене
// Наследуется от QGraphicsRectItem, то есть является прямоугольным графическим объектом
class DiagramItem : public QGraphicsRectItem
{
public:
    void updateConnectionsAfterResize(const QRectF &oldRect, const QRectF &newRect, const QPointF &posShift = QPointF(0,0));
    int id() const { return m_id; }
    // --- ДОБАВИТЬ ЭТУ СЕКЦИЮ ---
        void setError(const QString &message); // Включить режим ошибки
        void clearError();                     // Выключить (починить)
        bool isError() const { return m_isError; }
    void setType(const QString &type) { m_type = type; update(); }
        void setDescription(const QString &desc) { m_description = desc; update(); }
    void setLabel(const QString &text);
    void setOnEnter(const QString &code) { m_onEnter = code; }
        void setOnExit(const QString &code) { m_onExit = code; }
        // Геттеры для компилятора
        QString getType() const { return m_type; }

        QString getOnEnter() const { return m_onEnter; }
        QString getOnExit() const { return m_onExit; }

    // НОВЫЙ МЕТОД: Чтобы таблица могла узнать имя блока
        QString getLabel() const { return m_label; }

        // Можно также добавить геттер для типа, если нужно
        QString getDescription() const { return m_description; }
    // НОВЫЙ МЕТОД: Вычисляет следующий приоритет для исходящих линий
        int getNextPriority() const;
    // --- Работа с данными блока ---
    // Открыть редактор свойств
    void openPropertiesDialog(int focusSection = 0);

    // НОВЫЙ МЕТОД:
    int getGlobalRank(ConnectionItem* targetConn) const;
    // Определяет сторону блока для локального якоря: 0=Top, 1=Right, 2=Bottom, 3=Left, -1=Error
    int getSideForAnchor(const QPointF &localAnchor) const;

    // Возвращает порядковый номер (ранг) указанного соединения на той стороне,
    // к которой относится localAnchor.
    // Ранг 0 означает, что это первая стрелка на этой стороне.
    int getConnectionRank(ConnectionItem* targetConn) const;
    // Уведомляет все присоединенные стрелки о необходимости обновить свои пути.
    // Нужно вызывать при удалении связей, чтобы остальные "сомкнули ряды".
    void updateAllConnections();
    // НОВЫЙ МЕТОД: Возвращает прямоугольник, охватывающий все занятые якоря
    // Если занятых якорей нет, возвращает невалидный (null) прямоугольник.
    QRectF getOccupiedAnchorRect() const;
    void shiftAnchors(qreal dx, qreal dy);
    // Вернуть локальные координаты всех anchor (используется внешним кодом)
    QList<QPointF> getAnchorPoints() const { return m_anchorPoints; }

    // Найти локальную anchor точку, ближайшую к указанной позиции в координатах сцены.
    // Возвращает локальную точку; если outDist != nullptr, в него запишется расстояние (в сценических координатах).
    QPointF nearestAnchorToScenePoint(const QPointF &scenePos, qreal *outDist = nullptr) const;

    // Подсветить/снять подсветку для одной локальной anchor точки (рисуется в paint как красная)
    void setAnchorHighlighted(const QPointF &localAnchor, bool highlighted);


    void setView(QGraphicsView* view) { m_view = view; }
    void updateAnchorPoints();

    void generateAnchorPoints();   // генерирует точки с шагом 1
    void setShowAnchorPoints(bool show);
    // Вызывается после завершения операции изменения размера (ручкой)
    // Выполняет привязку размеров и позиции к сетке (snap) один раз при отпускании.
    void finalizeResize();

    void setGrid(Grid* grid) { m_grid = grid; }

    // Методы для работы с состоянием
    void saveState();
    void restoreState();
    void resizeFromHandle(Qt::Corner corner, QPointF delta); // реальное изменение размеров
    // уведомление ручек и текста, что размер изменился
    void updateResizeHandles();
    void updateHandlesPosition();

    // Показать/скрыть ручки масштабирования
    void showResizeHandles();
    void hideResizeHandles();

    // Вызывается из ResizeHandle после изменения размера
    void onResizedByHandle();

    // Конструктор класса. parent — указатель на родительский элемент сцены (по умолчанию nullptr)
    DiagramItem(QGraphicsItem *parent = nullptr);
    void prepareResize() { prepareGeometryChange(); }

    // Проверяет, используется ли уже этот якорь (в m_connections)
    bool isAnchorFree(const QPointF &localAnchor) const;
    // *** НОВЫЕ МЕТОДЫ ДЛЯ СВЯЗЕЙ ***
    void addConnection(ConnectionItem *conn);
    void removeConnection(ConnectionItem *conn);
    void removeAllConnections(); // Вызывается при удалении блока
    bool hasConnections() const { return !m_connections.isEmpty(); } // Проверка наличия стрелок
protected:
    // Авто-подгонка размера под контент
        void adjustSizeToContent();

        // Эти поля нужны наследникам (BridgeItem)
        bool m_isError = false;
        QList<ConnectionItem*> m_connections;
        bool m_updatingAnchors = false;
        QPointF m_highlightedAnchor;
        bool m_hasHighlightedAnchor = false;

        // Точки привязки
        bool m_showAnchorPoints = false;
        QList<QPointF> m_anchorPoints;

    // Обработчик двойного клика мыши на элементе
    // Переопределяем виртуальный метод из QGraphicsItem
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:


    QPointF snapToGridLocal(const QPointF& pt) const;

    QGraphicsView* m_view = nullptr;  // указатель на view для масштабирования
    DiagramItemState m_state;
    Grid* m_grid = nullptr; // указатель на сетку
    static int s_counter; // Статическая переменная-счётчик для присвоения уникальных ID элементам
    int m_id;             // Уникальный идентификатор данного элемента
    QList<ResizeHandle*> m_resizeHandles; // ручки для масштабирования блока

    // --- Данные блока (НОВОЕ) ---
    QString m_type = "Вершина";
    QString m_description = ""; // Пустое по умолчанию (будет мнимый текст)
    QString m_label = "";
    QString m_onEnter = "";
    QString m_onExit = "";

    // const qreal padding = 10; // (Уже не используется, так как рисуем сами)

    // resize handles
    QList<ResizeHandle*> m_handles;
    bool m_handlesVisible = false;
};

#endif // DIAGRAMITEM_H
