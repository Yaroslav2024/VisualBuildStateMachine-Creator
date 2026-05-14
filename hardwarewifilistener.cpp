/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "hardwarewifilistener.h"
#include <QHostAddress>

HardwareWifiListener::HardwareWifiListener(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this))
{
    // Та же регулярка: [Type:ID]
    m_protocolRegex.setPattern(R"(\[([BTF]):(\d+)\])");

    connect(m_socket, &QTcpSocket::readyRead, this, &HardwareWifiListener::handleReadyRead);

    // Обработка ошибок сокета
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QTcpSocket::SocketError socketError){
        emit onError("Socket Error: " + m_socket->errorString());
    });
}

HardwareWifiListener::~HardwareWifiListener()
{
    disconnectDevice();
}

bool HardwareWifiListener::connectToDevice(const QString &ip, quint16 port)
{
    disconnectDevice();

    emit onLogMessage(QString("Connecting to %1:%2...").arg(ip).arg(port));

    m_socket->connectToHost(ip, port);

    // Ждем 3 секунды подключения
    if (m_socket->waitForConnected(3000)) {
        emit onLogMessage("Connected to Wi-Fi Device!");
        m_buffer.clear();
        return true;
    } else {
        emit onError("Connection failed: " + m_socket->errorString());
        return false;
    }
}

void HardwareWifiListener::disconnectDevice()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState)
            m_socket->waitForDisconnected(1000);
    }
}

bool HardwareWifiListener::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void HardwareWifiListener::handleReadyRead()
{
    // Читаем всё, что прилетело по Wi-Fi
    QByteArray data = m_socket->readAll();
    m_buffer.append(data);

    // Парсим так же, как и Serial (по строкам)
    while (m_buffer.contains('\n')) {
        int lineEndIndex = m_buffer.indexOf('\n');
        QByteArray lineBytes = m_buffer.left(lineEndIndex).trimmed();
        QString line = QString::fromLatin1(lineBytes);
        m_buffer.remove(0, lineEndIndex + 1);

        if (line.isEmpty()) continue;

        QRegularExpressionMatch match = m_protocolRegex.match(line);

        if (match.hasMatch()) {
            QString type = match.captured(1);
            int id = match.captured(2).toInt();

            if (type == "B") emit onBlockEntered(id);
            else if (type == "T") emit onTransitionTaken(id);
            else if (type == "F") emit onTransitionFalse(id);
        } else {
            emit onLogMessage(line);
        }
    }
}

