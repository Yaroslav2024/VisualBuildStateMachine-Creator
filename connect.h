/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef CONNECT_H
#define CONNECT_H

// Простой header — подключаем объявление ElementsRegistry.
// Далее реализация методов находится в connect.cpp
#include "elementsregistry.h"
#include <QObject>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QPointer>
#include <QPen>
#include <QGraphicsItemGroup> // <-- Добавляем для группировки
#include "diagramitem.h"
#include <QDebug>
#include "transitiongroup.h"
#include <QUuid>
class QGraphicsScene;
class DiagramView;
class ArrowEditor;

// Лёгкий контроллер для интерактивного рисования соединения "магнитом"
class Connector : public QObject
{
    Q_OBJECT
public:
    // в class Connector (connect.h)
    void createArrowWithLabels(const QPointF &from,
                               const QPointF &to,
                               QGraphicsScene *scene,
                               QList<QGraphicsItem*> &committedItems,
                               QList<QGraphicsItem*> &sessionItems);
    // *** НОВАЯ ФУНКЦИЯ ДЛЯ ПРЯМЫХ ЛИНИЙ в плане точек***
    void createStraightArrowHead(const QPointF &from,
                                     const QPointF &to,
                                     qreal arrowLen,
                                     qreal arrowWidth,
                                     QGraphicsScene *scene,
                                     QList<QGraphicsItem*> &committedItems,
                                     QList<QGraphicsItem*> &sessionItems);
    void createStraightArrowWithLabels(const QPointF &from,
                                           const QPointF &to,
                                           QGraphicsScene *scene,
                                           QList<QGraphicsItem*> &committedItems,
                                           QList<QGraphicsItem*> &sessionItems);     
    // Здесь добавляем объявление нового метода
            // connect.h (Новое объявление)
            // *** ОБЪЯВЛЕНИЕ ДЛЯ Г-ОБРАЗНЫХ ЛИНИЙ (7 АРГУМЕНТОВ) ***
                void createArrowHead(const QPointF &from,
                                     const QPointF &to,
                                     qreal arrowLen,
                                     qreal arrowWidth,
                                     QGraphicsScene *scene,
                                     QList<QGraphicsItem*> &committedItems,
                                     QList<QGraphicsItem*> &sessionItems);

    // Здесь добавляем объявление нового метода

    // возвращает true, если режим только что был отменён (правый клик)
    bool wasJustStopped() const;
    // потребить флаг (очистить) — вызвать из DiagramView после проверки
    void consumeJustStopped();

    explicit Connector(QGraphicsScene *scene, QObject *parent = nullptr);
    ~Connector() override;

    // Включить/выключить режим соединения
    void start();   // включить режим (готов к рисованию)
    void startFrom(DiagramItem* startBlock); // <-- ДОБАВИТЬ ЭТОТ МЕТОД
    void stop();    // выключить режим и очистить превью

    bool isActive() const { return m_active; }

    // События от view (пересылать координаты в сценных координатах)
    void onMouseMove(const QPointF &scenePos);
    void onMousePress(Qt::MouseButton button, const QPointF &scenePos);

signals:
    // Сигнал при создании неизменяемого соединения (место для интеграции в ElementsRegistry)
    void connectionCommitted(const QPointF &fromScene, const QPointF &toScene);

private:
void initConnectionMode(); // <-- ДОБАВИТЬ ЭТОТ МЕТОД
        qreal m_secondarySnapThreshold = 12.0; //
    // Новое поле для временных элементов текущей линии:
       QList<QGraphicsItem*> m_currentSessionItems;
    // preview второго сегмента и маркеры
    QGraphicsEllipseItem *m_previewEndHighlight = nullptr;

    // список зафиксированных (committed) графических объектов (линии + точки),
    // чтобы при отмене (ПКМ) можно было удалить всю построенную линию
    QList<QGraphicsItem*> m_committedItems;

    // Для продолжения от конечной точки: флаг и точка старта следующего сегмента
    bool m_hasStartNextSegment = false;
    QPointF m_nextSegmentStart;

    // Флаг "только что остановлен" (опционально, для подавления контекстного меню)


        // **Новые поля: второй отрезок превью и точка поворота (bend)**
        QGraphicsLineItem *m_previewLine2 = nullptr;
        QGraphicsEllipseItem *m_previewBendHighlight = nullptr;
        // **Новый метод:** обновление превью с возможным Г-образным изгибом
        // *** НОВЫЕ РАЗДЕЛЬНЫЕ ФУНКЦИИ РИСОВАНИЯ ПРЕВЬЮ ***
                void drawStraightPreview(const QPointF &previewStart, const QPointF &scenePos);
                void drawLShapedPreview(const QPointF &previewStart, const QPointF &scenePos);

    bool m_justStopped = false;

    QGraphicsScene *m_scene = nullptr;
    bool m_active = false;

    // Визуальные превью
    QGraphicsLineItem *m_previewLine = nullptr;
    QGraphicsEllipseItem *m_previewStartHighlight = nullptr;

    // Текущие данные
    DiagramItem *m_startBlock = nullptr;
    QPointF m_startLocal;   // локальная точка в блоке
    QPointF m_startScene;   // сцена координата стартовой точки
    bool m_hasStart = false;
    // *** НОВЫЕ ПОЛЯ ДЛЯ ХРАНЕНИЯ НАЧАЛА ЛИНИИ ***
        DiagramItem *m_originBlock = nullptr;
        QPointF m_originLocal;
        // *** КОНЕЦ НОВЫХ ПОЛЕЙ ***
    qreal m_snapThreshold = 24.0; // пиксельная дистанция, внутри которой мы "магнитимся" к блоку
    // (true = горизонтальный, false = вертикальный)
        bool m_lastSegmentWasHorizontal = false;
};

class ConnectionItem : public QGraphicsItemGroup
{
public:
    void resetVisualState();
    // НОВЫЕ МЕТОДЫ БЛОКИРОВКИ
        bool isBlocked() const { return m_isBlocked; }
        void setBlocked(bool blocked);
    void setFalsePath(bool active);
    void setError(bool active);
    void setHighlight(bool active);
    // Геттер для ID
        QString id() const { return m_uid; }

        // Сеттер (нужен при загрузке из файла, чтобы восстановить старый ID)
        void setId(const QString &id) { m_uid = id; }
    // НОВЫЙ МЕТОД:
        // Перестраивает путь по самому короткому ортогональному маршруту
    // (Это нужно, чтобы QGraphicsItemGroup корректно работал с paint())
        enum { Type = UserType + 10 };
        int type() const override { return Type; }
    ConnectionItem(QGraphicsItem *parent = nullptr);
~ConnectionItem(); // Деструктор
    // Методы для регистрации компонентов при создании
    void setStart(DiagramItem *block, const QPointF &localAnchor);
    void setEnd(DiagramItem *block, const QPointF &localAnchor);
    void registerItem(QGraphicsItem *item);
    void finalizeConstruction(); // Вызывается после добавления всех
    // Добавляем метод для принудительного пересчета пути
        void updatePath();
    // Главный метод обновления
    void updatePosition();
    // *** НОВЫЙ МЕТОД ***
        // Позволяет редактору безопасно удалять линии при их слиянии
        void removeInternalLine(QGraphicsLineItem* line) { m_lines.removeAll(line); }
    // Геттеры для удаления
    DiagramItem* startBlock() const { return m_startBlock; }
    DiagramItem* endBlock() const { return m_endBlock; }

    QPointF startAnchorLocal() const { return m_startAnchorLocal; }
        QPointF endAnchorLocal() const { return m_endAnchorLocal; }

    // Для ArrowEditor, чтобы он получил доступ к компонентам
    QList<QGraphicsLineItem*> lines() const { return m_lines; }
    QList<QGraphicsEllipseItem*> nodes() const { return m_nodes; }
    QGraphicsPolygonItem* arrowHead() const { return m_arrowHead; }
    TransitionGroup* transition() const { return m_transition; }
protected:

    // Переопределяем shape() для точного выделения
    // (boundingRect() останется большим, чтобы текст перерисовывался,
    // а shape() будет точным, для кликов мыши)
    QPainterPath shape() const override;
    // Переопределяем paint() для ручной отрисовки выделения
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
private:
        bool m_isBlocked = false; // Флаг: заблокирована стрелка или нет
        bool m_isFalse = false; // Флаг "Условие не выполнено"
        bool m_isError = false; // Флаг ошибки
        bool m_isHighlighted = false; // Флаг: true = рисуем зеленый контур
        QString m_uid; // <-- Уникальный номер
        // НОВЫЕ МЕТОДЫ для инкрементального обновления
            void appendOrthogonalPath(const QPointF &from, const QPointF &to, bool horizontalFirst);
            // Храним флаг, была ли линия уже построена полностью хотя бы раз
                bool m_pathInitialized = false;
            // Храним флаг, была ли линия уже построена полностью хотя бы раз
        // НОВЫЙ вспомогательный метод
            QPointF getOutwardNormal(const QRectF &bound, const QPointF &anchor);
        // Храним предпочтительную ориентацию первого сегмента
            // true = сначала горизонтально, false = сначала вертикально
            bool m_horizontalFirst = true;

            // Вспомогательный метод для обновления стрелок и текста
            void updateDecorations();
    // Приватный метод для пересчета геометрии
    void updateGeometry(const QPointF &oldPos, const QPointF &newPos);

    DiagramItem *m_startBlock = nullptr;
    DiagramItem *m_endBlock = nullptr;
    QPointF m_startAnchorLocal;
    QPointF m_endAnchorLocal;

    // Кэшированные позиции в координатах сцены
    QPointF m_lastStartPos;
    QPointF m_lastEndPos;

    // Компоненты стрелки
    QList<QGraphicsLineItem*> m_lines;
    QList<QGraphicsEllipseItem*> m_nodes;
    QGraphicsPolygonItem* m_arrowHead = nullptr;
    TransitionGroup* m_transition = nullptr;
};


// Роль для QGraphicsItem::setData(), чтобы помечать наши группы стрелок
enum ConnectionData {
    ConnectionRole = Qt::UserRole + 1
};


// ... (существующий класс Connector)

//
// НОВЫЙ КЛАСС: ArrowEditor
//
// Управляет редактированием существующей, сгруппированной стрелки.
// Активируется двойным кликом по группе.
//
class ArrowEditor : public QObject
{
    Q_OBJECT

    // Приватный вспомогательный класс для ручек (хэндлов)
    // Это аналог ResizeHandle, но для точек соединения
    class EditHandle : public QGraphicsEllipseItem
    {
    public:
        // Создаем желтый кружок
        EditHandle(QGraphicsItem *parent = nullptr)
            : QGraphicsEllipseItem(-6, -6, 12, 12, parent)
        {
            setPen(QPen(Qt::black, 1));
            setBrush(QColor(255, 255, 100, 200)); // Полупрозрачный желтый
            setZValue(3000); // Поверх всего
            setFlag(ItemIgnoresTransformations);
        }

        // Мы не обрабатываем мышь здесь,
        // ArrowEditor будет делать это на основе itemAt()
    };


public:
    // В connect.h, внутри class ArrowEditor public:
    bool isDragging() const { return m_draggedHandle != nullptr; }
    void refreshHandles(); // <--- НОВЫЙ МЕТОД
    bool isRepinning() const { return m_isRepinning; } // <-- ДОБАВИТЬ ЭТУ СТРОКУ
    ArrowEditor(QGraphicsScene *scene, QObject *parent = nullptr);
    ~ArrowEditor() override;

    // Включить режим редактирования для этой группы
    void start(ConnectionItem *group);
    // Включить режим отсоединения/пере-привязки
        void startRepin(ConnectionItem *group);
    // Выключить режим редактирования
    void stop();

    bool isActive() const { return m_active; }

    // События от view (пересылать координаты в сценных координатах)
    void onMousePress(const QPointF &scenePos, Qt::MouseButton button);
    void onMouseMove(const QPointF &scenePos);
    void onMouseRelease(const QPointF &scenePos);

private:

    // Обновляет геометрию линий/наконечника при перетаскивании ручки
    void updateConnection(const QPointF &oldPos, const QPointF &newPos);

    // Очищает все ручки со сцены
    void clearHandles();

    QGraphicsScene *m_scene;
    bool m_active = false;

    ConnectionItem *m_editingGroup = nullptr; // Группа, которую мы редактируем
    QList<EditHandle*> m_handles;                 // Наши ручки
    EditHandle *m_draggedHandle = nullptr;        // Ручка, которую тащим

    // Элементы из m_editingGroup, которые мы будем менять
    QList<QGraphicsLineItem*> m_lines;
    QList<QGraphicsEllipseItem*> m_nodes;
    QGraphicsPolygonItem* m_arrowHead = nullptr;
    QList<QGraphicsRectItem*> m_labelBgs;
QList<QGraphicsTextItem*> m_labels;

bool m_isRepinning = false;                 // Флаг, что мы в режиме "Отсоединения"
    DiagramItem *m_originalEndBlock = nullptr;  // Храним старый блок для отмены
    QPointF m_originalEndAnchor;                // Храним старый якорь для отмены

    // Блок и якорь, над которыми сейчас курсор
    DiagramItem *m_hoveredBlock = nullptr;
    QPointF m_hoveredAnchor;
};

#endif // CONNECT_H
