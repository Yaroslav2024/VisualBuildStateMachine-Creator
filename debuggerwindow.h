/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef DEBUGGERWINDOW_H
#define DEBUGGERWINDOW_H

#include <QMap>
#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QProcess>
#include "diagramitem.h"
#include <QHBoxLayout>
#include <QToolButton>
#include "groupitem.h"
#include "hardwarelistener.h"
#include "hardwarewifilistener.h"

#include <QTabBar>
#include <QList>
#include <QPointF>
#include <QSet>
namespace Ui {
class DebuggerWindow;
}

class DebuggerWindow : public QDialog
{
    Q_OBJECT

public:
    // Передаем имена вкладок и их координаты
        void setSectors(const QStringList &names, const QList<QPointF> &centers);


    void setHardwareWifiListener(HardwareWifiListener* hwWifiListener, const QString& ip = "", quint16 port = 8080);
    void setHardwareListener(HardwareListener* hwListener, const QString& portName = "");
    // --- ДОБАВЛЯЕМ ПЕРЕЧИСЛЕНИЕ ЯЗЫКОВ ---
        enum DebugLang {
            Lang_CPP,
            Lang_PYTHON,
            Lang_JAVA
        };
    explicit DebuggerWindow(QWidget *parent = nullptr);
    ~DebuggerWindow();
    // Сюда мы передадим "Карту", где записано: 1 -> БлокА, 2 -> СтрелкаБ
        void setItemMap(const QMap<int, QGraphicsItem*> &map);
    // Главная функция: получить сцену из главного окна
    void setDebugScene(QGraphicsScene *scene);
    // --- НОВАЯ ФУНКЦИЯ: Установить язык ---
        void setLanguage(DebugLang lang);

        // --- ОБНОВЛЯЕМ ЗАПУСК: Теперь он принимает код ---
        void startDebugSession(const QString &code);
protected:
    // --- ДОБАВИТЬ ВОТ ЭТУ СТРОКУ: ---
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
public slots:
    void onSectorChanged(int index);
    // Методы для управления извне (от HardwareListener)
        void externalSetCurrentBlock(int id);
        void externalHighlightArrow(int id, bool success);
        void appendLog(const QString &message); // Вывод текста в лог
private slots:

    void on_stopButton_clicked();
    void on_btnTogglePort_clicked();
    void on_restartButton_clicked();
    void on_btnToggleWifi_clicked();
    void onProcessOutputReady();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
private:
    QByteArray m_processBuffer;
    QTabBar *m_sectorTabBar = nullptr;
        QList<QPointF> m_sectorCenters;
    HardwareWifiListener* m_hwWifiListener = nullptr;
    HardwareListener* m_hwListener = nullptr;
        QString m_lastPortName;                   // <--- ИМЯ ПОРТА
        QString m_lastWifiIp;
            quint16 m_lastWifiPort;
    void setupGroupNavigation();
    QWidget *m_navBar; // Панелька внизу
    DiagramItem *m_currentBlockItem = nullptr;
    // Здесь мы храним базу данных всех объектов
        QMap<int, QGraphicsItem*> m_itemMap;
    Ui::DebuggerWindow *ui;
    QProcess *m_process;

        QSet<DiagramItem*> m_activeBlocks;

        // --- ПЕРЕМЕННАЯ ДЛЯ ХРАНЕНИЯ ЯЗЫКА ---
        DebugLang m_currentLang;
         QString m_lastCode; // код для перезапуска
        void highlightItem(int id);
        void showError(int id, const QString &message);
};

#endif // DEBUGGERWINDOW_H
