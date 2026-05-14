/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef HARDWARELISTENER_H
#define HARDWARELISTENER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QRegularExpression>

class HardwareListener : public QObject
{
    Q_OBJECT
public:

    explicit HardwareListener(QObject *parent = nullptr);
    ~HardwareListener();

    // Управление подключением
    bool connectPort(const QString &portName, int baudRate = 115200);
    void disconnectPort();
    bool isConnected() const;

    // Список доступных портов для UI
    static QStringList getAvailablePorts();

signals:
    // Сигналы для DebuggerWindow
    void onBlockEntered(int id);      // [B:1]
    void onTransitionTaken(int id);   // [T:10]
    void onTransitionFalse(int id);   // [F:10]
    void onError(QString msg);        // Ошибки порта
    void onLogMessage(QString msg);   // Обычный Serial.println (для консоли)

private slots:
    void handleReadyRead(); // Чтение данных

private:
    QSerialPort *m_serial;
    QByteArray m_buffer; // Буфер для склейки разорванных строк

    // Регулярка для парсинга: [Type:ID]
    // Группа 1: Тип (B, T, F)
    // Группа 2: ID (число)
    QRegularExpression m_protocolRegex;
};

#endif // HARDWARELISTENER_H
