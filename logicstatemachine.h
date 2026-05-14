/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef LOGICSTATEMACHINE_H
#define LOGICSTATEMACHINE_H

#include <QObject>
#include <QScxmlStateMachine>
#include <QVariantMap>
#include "codeexecutor.h"
#include <QTimer> // <--- Добавляем таймер
// Класс-обертка для управления SCXML машиной состояний
class LogicStateMachine : public QObject
{
    Q_OBJECT
public:
    // Изменить скорость симуляции (интервал таймера)
        void setTickInterval(int msec);
    explicit LogicStateMachine(QObject *parent = nullptr);
    ~LogicStateMachine();
     void setExecutor(CodeExecutor *exec) { m_executor = exec; }
    // Загрузить SCXML из файла
    bool loadFile(const QString &fileName);

    // Запустить машину
    void start();

    // Остановить машину
    void stop();

    // Проверить, активна ли машина
    bool isRunning() const;

    // Проверить, находится ли машина в конкретном состоянии
    bool isActiveState(const QString &stateName) const;

    // Отправить событие в машину (например "click", "timeout")
    void submitEvent(const QString &eventName, const QVariant &data = QVariant());
void cleanup();
signals:
    // Сигналы для внешнего мира (например, чтобы MainWindow обновил UI)
    void stateEntered(const QString &stateName);
    void stateExited(const QString &stateName);
    void logMessage(const QString &msg);

private slots:
    // Внутренние слоты для перехвата изменений состояний
    void onStateChanged(bool active);
void onTimerTick();
private:
    QScxmlStateMachine *m_sm = nullptr;
    // *** ВОТ ЭТОЙ ПЕРЕМЕННОЙ НЕ ХВАТАЛО ***
        CodeExecutor *m_executor = nullptr;
        QTimer *m_timer; // <--- Таймер тактирования
};

#endif // LOGICSTATEMACHINE_H
