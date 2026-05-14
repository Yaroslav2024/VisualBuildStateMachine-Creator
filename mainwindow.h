/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "State.h"
#include "scxmlcompiler.h"
#include <QMainWindow>     // Базовый класс для главного окна приложения
#include <QGraphicsScene>  // Класс для хранения и управления графических элементов на сцене
#include "Grid.h"   // Подключаем определение класса Grid
#include "statetable.h"
#include "logicstatemachine.h"
#include "cpptranslator.h"
#include "headerdialog.h"
#include "graphmanager.h"
#include "cpptranslator2.h"
#include <QDockWidget>

#include <QComboBox>
#include "microcontrollertranslator.h"
#include "javatranslator.h"

#include "dynamicmicrotranslator.h"
#include "hardwarelistener.h"
#include "dynamicclasstranslator.h"

#include "hardwarewifilistener.h"      // Новый листенер
#include "dynamicwifitranslator.h"     // Генератор .ino с Wi-Fi
#include "dynamicwificlasstranslator.h" // Генератор классов с Wi-Fi

#include <QInputDialog> // Для выбора порта

#include "groupitem.h"

#include "commentitem.h" // Добавить стикеры
#include "bridgeitem.h"
#include "projectsidebar.h"
#include <QTabBar>
#include <QToolBar>
#include <QInputDialog>
#include <QProgressBar>
#include <QLabel>

class Grid;
class State;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; } // Пространство имён для автоматически сгенерированного UI (из .ui файла)
QT_END_NAMESPACE

// Класс MainWindow — главное окно приложения
// Наследуется от QMainWindow, что позволяет использовать меню, тулбары и центральный виджет
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum DebugMode {
            Mode_EMULATOR, // Режим эмуляции на ПК (по умолчанию)
            Mode_DYNAMIC   // Режим работы с реальным железом
        };
    // Перечисление языков
        enum TranslatorLang {
            Lang_JAVA,
            Lang_CPP,
            Lang_PYTHON,
            Lang_MICRO
        };
    // Конструктор класса. parent — родительский виджет (по умолчанию nullptr)
    MainWindow(QWidget *parent = nullptr);

    // Деструктор класса. Освобождает ресурсы, связанные с UI и сценой
    ~MainWindow();
void deleteSelectedItems();

private slots:
void copySelectedItems();
    void pasteCopiedItems();
void showCodePreview(); // Слот для открытия нашего нового окна
void onSectorChanged(int index);
    void onAddSectorClicked();
void performAutoSave();
void on_addHistoryButton_clicked();
void on_addBackButton_clicked();
void on_addBridgeButton_clicked();
void on_addCommentButton_clicked();
void checkForGroupExpansion(); //
void on_createGroupButton_clicked();
    // Слоты для кнопок меню
void on_actionImport_triggered(); // Import
        void on_actionSave_triggered(); // Сохранить
        void on_actionOpen_triggered(); // Открыть
        void on_actionUndo_triggered(); // Ctrl + Z
        void on_actionRedo_triggered(); // Ctrl + Y

void on_debugButton_clicked();
    void on_headerButton_clicked(); // <-- Новый слот для кнопки "Header"
    void on_translateButton_clicked();
    void on_generateButton_clicked();
    void on_checkButton_clicked(); // <-- Новый слот
    // Слот, вызываемый при нажатии кнопки "Добавить элемент"
    // Обычно реализует добавление нового DiagramItem на сцену
    void on_addItemButton_clicked();
void on_startButton_clicked();
    void on_openTableButton_clicked(); //

    void on_actionNew_Project_triggered();

    void on_actionOpen_Project_triggered();

    void on_actionSave_Project_triggered();

private:


    // Функции для удобного вызова
    void showLoading(const QString &text);
    void hideLoading();
    void setProjectUIEnabled(bool enabled);
    QFrame *m_welcomeScreen = nullptr;
    // Переменные для предпросмотра кода

        QVector<QPair<QString, QString>> m_lastGeneratedFiles;
        QString m_lastTranslatorName;
        bool m_lastGenerationStatus = false;

        QList<int> m_lastErrorLines;
                // Функция обновления статуса теперь принимает список файлов и список ошибок
                void updateCodePreviewStatus(const QVector<QPair<QString, QString>> &files, const QString &translator, bool success, QList<int> errorLines = QList<int>());

    void applyDebugFilters(const QStringList &activeSectors, const QStringList &activeGroups);
    QJsonArray serializeSectors(); // Упаковка
        void deserializeSectors(const QJsonArray &sectors); // Распаковка
    QTabBar *m_sectorTabBar = nullptr;
        QList<QPointF> m_sectorCenters; // Здесь храним координаты центров каждого сектора
    QTimer *m_autoSaveTimer;
    void applyProjectLanguage(int langId);
    void loadProject(const QString &projectPath);
    QString m_currentProjectPath; // <--- Запоминает, какой проект сейчас открыт
    QTimer *m_groupCheckTimer; // Таймер
        GroupItem* findGroupOf(DiagramItem* block); // Помощник
    void createGroupFromSelection();
    QList<DiagramItem*> findConnectedComponent(DiagramItem* startNode);

    DebugMode m_currentDebugMode = Mode_EMULATOR;
    HardwareListener *m_hwListener;
    HardwareWifiListener *m_hwWifiListener;


    ProjectSidebar *m_projectSidebar = nullptr;


    void clearDebugVisuals(); // Объявление

    TranslatorLang m_currentLang; // Текущий выбранный язык
    GraphManager *m_graphManager;
    QString m_headerCode;
    LogicStateMachine *m_logicEngine = nullptr;
    Ui::MainWindow *ui;         // Указатель на объект интерфейса, созданный через Qt Designer
    QGraphicsScene *scene;      // Сцена, на которой размещаются элементы диаграммы
    State *state; // объект состояния
CodeExecutor *m_codeExecutor = nullptr;
    Grid *grid;
    StateTable *m_stateTable = nullptr; // <--- Указатель на окно таблицы
    qreal m_currentZoom = 1.0;     // начальный коэффициент масштаба


};

#endif // MAINWINDOW_H
