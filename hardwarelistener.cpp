/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "hardwarelistener.h"
#include <QDebug>
#include <QThread> // <--- ДОБАВЛЕНО (Исправляет ошибку incomplete type)

HardwareListener::HardwareListener(QObject *parent)
    : QObject(parent), m_serial(new QSerialPort(this))
{
    // Компилируем регулярку один раз для скорости
    // Ищем паттерн: [Буква:Число] -> например [B:12]
    m_protocolRegex.setPattern(R"(\[([BTF]):(\d+)\])");

    connect(m_serial, &QSerialPort::readyRead, this, &HardwareListener::handleReadyRead);
}

HardwareListener::~HardwareListener()
{
    if (m_serial->isOpen()) {
        m_serial->close();
    }
}

QStringList HardwareListener::getAvailablePorts()
{
    QStringList ports;
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : serialPortInfos) {
        ports << portInfo.portName();
    }
    return ports;
}

bool HardwareListener::connectPort(const QString &portName, int baudRate)
{
    disconnectPort(); // Сначала отключаем старое

    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);

    // Стандартные настройки Arduino
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);

    // ---  OneStop вместо OneStopBit ---
    m_serial->setStopBits(QSerialPort::OneStop);
    // -----------------------------------------------

    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadOnly)) { // Нам нужно только читать
        // Сбрасываем DTR, чтобы перезагрузить Arduino (удобно для старта дебага)
        m_serial->setDataTerminalReady(true);

        // ---  работает, т.к. есть <QThread> ---
        QThread::msleep(100);
        // -----------------------------------------------------------

        m_serial->setDataTerminalReady(false);

        m_buffer.clear();
        return true;
    } else {
        emit onError("Failed to open port: " + m_serial->errorString());
        return false;
    }
}

void HardwareListener::disconnectPort()
{
    if (m_serial->isOpen()) {
        m_serial->close();
    }
}

bool HardwareListener::isConnected() const
{
    return m_serial->isOpen();
}

void HardwareListener::handleReadyRead()
{
    // Читаем все, что пришло
    QByteArray data = m_serial->readAll();
    m_buffer.append(data);

    // Обрабатываем построчно
    while (m_buffer.contains('\n')) {
        int lineEndIndex = m_buffer.indexOf('\n');

        // Извлекаем одну строку (без \n и \r)
        QByteArray lineBytes = m_buffer.left(lineEndIndex).trimmed();
        QString line = QString::fromLatin1(lineBytes);

        // Удаляем обработанную часть из буфера
        m_buffer.remove(0, lineEndIndex + 1);

        if (line.isEmpty()) continue;

        // --- ПАРСИНГ ---
        QRegularExpressionMatch match = m_protocolRegex.match(line);

        if (match.hasMatch()) {
            // Это НАШЕ сообщение протокола!
            QString type = match.captured(1); // B, T или F
            int id = match.captured(2).toInt();

            if (type == "B") {
                emit onBlockEntered(id);
            } else if (type == "T") {
                emit onTransitionTaken(id);
            } else if (type == "F") {
                emit onTransitionFalse(id);
            }
        } else {
            // Это обычный лог (Serial.println("Hello"))
            emit onLogMessage(line);
        }
    }
}

