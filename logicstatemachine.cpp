/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "logicstatemachine.h"
#include <QDebug>
#include <QFileInfo>

LogicStateMachine::LogicStateMachine(QObject *parent)
    : QObject(parent)
{
    // Настраиваем таймер
        m_timer = new QTimer(this);
        m_timer->setInterval(200); // 5 шагов в секунду (можно менять)
        connect(m_timer, &QTimer::timeout, this, &LogicStateMachine::onTimerTick);
}

LogicStateMachine::~LogicStateMachine()
{
    if (m_sm) {
        delete m_sm;
    }
}
void LogicStateMachine::cleanup()
{
    m_timer->stop();
    if (m_sm) {
        if (m_sm->isRunning()) m_sm->stop();
        delete m_sm;
        m_sm = nullptr;
    }
}
bool LogicStateMachine::loadFile(const QString &fileName)
{


    // 1. Очистка старой машины и остановка таймера (Важно!)
    if (m_timer) {
        m_timer->stop();
    }
    if (m_sm) {
        if (m_sm->isRunning()) m_sm->stop();
        delete m_sm;
        m_sm = nullptr;
    }

    // 2. Проверка файла
    if (!QFileInfo::exists(fileName)) {
        emit logMessage("Error: File not found: " + fileName);
        return false;
    }

    // 3. Попытка парсинга
    m_sm = QScxmlStateMachine::fromFile(fileName);

    // 4. Проверка на критические ошибки создания
    if (!m_sm) {
        emit logMessage("CRITICAL ERROR: Failed to create machine object (possibly broken XML).");
        return false;
    }

    // 5. Проверка ошибок синтаксиса внутри XML
    if (m_sm->parseErrors().size() > 0) {
        QString errStr = "SCXML parsing errors:\n";
        for (const auto &err : m_sm->parseErrors()) {
            errStr += QString("[%1:%2] %3\n")
                          .arg(err.line())
                          .arg(err.column())
                          .arg(err.toString());
        }
        emit logMessage(errStr);
        qDebug() << errStr;
        return false;
    }

    // --- ВСТАВКА: Передача Executor внутрь машины ---
    if (m_executor) {
        QVariantMap initialData;
        initialData.insert("_event_dispatcher", QVariant::fromValue(m_executor));
        // Устанавливаем данные только если есть модель данных (обычно null, но на будущее полезно)
        if (m_sm->dataModel()) {
            m_sm->setInitialValues(initialData);
        }
    }
    // ------------------------------------------------

    // 6. Подключение сигналов
    // ВАЖНО: Здесь мы управляем Таймером, чтобы избежать зависания
    connect(m_sm, &QScxmlStateMachine::runningChanged, this, [this](bool running){
        if (running) {
            emit logMessage("The machine is started");
            if (m_timer) m_timer->start(); // <--- ЗАПУСК ТАЙМЕРА
        } else {
            emit logMessage("The car is stopped.");
            if (m_timer) m_timer->stop();  // <--- ОСТАНОВКА ТАЙМЕРА
        }
    });

    // Подключение к состояниям (Визуализация + Выполнение кода)
    const QStringList allStates = m_sm->stateNames();
    for (const QString &state : allStates) {
        m_sm->connectToState(state, [this, state](bool active) {
            if (active) {
                // 1. Выполняем C++ код блока (onEnter)
                if (m_executor) {
                    m_executor->execute(state, "onEnter");
                }

                // 2. Сообщаем интерфейсу
                emit stateEntered(state);
                // emit logMessage("Переход в состояние: " + state); // Можно раскомментировать, если нужно много логов
            } else {
                // 1. Выполняем C++ код выхода (onExit)
                if (m_executor) {
                    m_executor->execute(state, "onExit");
                }

                // 2. Сообщаем интерфейсу
                emit stateExited(state);
            }
        });
    }

    m_sm->setParent(this);
    emit logMessage(tr("File loaded successfully: %1").arg(fileName));
    return true;
}
void LogicStateMachine::start()
{
    if (m_sm) {
        m_sm->start();
    }
}

void LogicStateMachine::stop()
{
    if (m_sm) {
        m_sm->stop();
    }
}

bool LogicStateMachine::isRunning() const
{
    return m_sm && m_sm->isRunning();
}

bool LogicStateMachine::isActiveState(const QString &stateName) const
{
    if (!m_sm) return false;
    return m_sm->isActive(stateName);
}

void LogicStateMachine::submitEvent(const QString &eventName, const QVariant &data)
{
    if (m_sm && m_sm->isRunning()) {
        m_sm->submitEvent(eventName, data);

    }
}

void LogicStateMachine::onStateChanged(bool active)
{
    Q_UNUSED(active);
    // Сюда можно добавить общую логику обработки
}
// ГЛАВНОЕ: Таймер пинает автомат
void LogicStateMachine::onTimerTick()
{
    if (m_sm && m_sm->isRunning()) {
        // Посылаем событие "tick", чтобы разрешить переход
        m_sm->submitEvent("tick");
    }
}
void LogicStateMachine::setTickInterval(int msec)
{
    if (m_timer) {
        m_timer->setInterval(msec);
    }
}
