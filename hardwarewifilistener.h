/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef HARDWAREWIFILISTENER_H
#define HARDWAREWIFILISTENER_H

#include <QObject>
#include <QTcpSocket> // <-- Используем сокеты вместо SerialPort
#include <QRegularExpression>

class HardwareWifiListener : public QObject
{
    Q_OBJECT
public:

    explicit HardwareWifiListener(QObject *parent = nullptr);
    ~HardwareWifiListener();

    // Подключение не по имени порта, а по IP и Порту
    bool connectToDevice(const QString &ip, quint16 port = 8080);
    void disconnectDevice();
    bool isConnected() const;

signals:
    // Те же сигналы, чтобы DebuggerWindow не заметил подмены
    void onBlockEntered(int id);      // [B:1]
    void onTransitionTaken(int id);   // [T:10]
    void onTransitionFalse(int id);   // [F:10]
    void onError(QString msg);
    void onLogMessage(QString msg);

private slots:
    void handleReadyRead(); // Чтение пакетов из сети

private:
    QTcpSocket *m_socket;
    QByteArray m_buffer;
    QRegularExpression m_protocolRegex;
};

#endif // HARDWAREWIFILISTENER_H
