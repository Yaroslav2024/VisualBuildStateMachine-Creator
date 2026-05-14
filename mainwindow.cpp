/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "diagramitem.h"
#include "State.h"
#include "grid.h"
#include "statetable.h"
#include "syntaxregistry.h"
#include <QFileDialog>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QActionGroup>
#include "scxmlcompiler.h"
#include "cpptranslator.h"
#include <QShortcut>
#include "pythontranslator.h"
#include "debuggerwindow.h"
#include "connect.h"
#include "pythontranslator2.h"
#include <QProcess>
#include "microtranslator2.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QStyle>
#include "javatranslator2.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QTimer>
#include "backitem.h"
#include "hardwarelistener.h"
#include "historyitem.h"
#include "projectsidebar.h"
#include "utils.h"
#include "librarymanagerdialog.h"
#include "networkmanager.h"
#include <QLabel>
#include <QFileSystemWatcher>
#include <QToolButton>
#include <QMenu>
#include <QHBoxLayout>
#include <QLabel>
#include <QStandardPaths>
#include <QSettings>
#include <QDialog>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QFormLayout>  // Для выравнивания текста и полей ввода (form.addRow)
#include <QLineEdit>    // Для самих текстовых полей, где будет путь к файлу
#include "codepreviewdialog.h"
#include <QToolButton>
#include <QTextBrowser>
#include <QRadioButton>
#include <QGroupBox>
#include <QClipboard>
// -------------------------------------------

// Конструктор главного окна
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      scene(new QGraphicsScene(this)),
      state(new State(this))
{
    ui->setupUi(this);
    // Устанавливаем иконку окна
        // (Замените "icon.png" на точное название вашего файла)
        this->setWindowIcon(QIcon(":/StateMachineicon.png"));
    QLabel *m_netStatusLabel = new QLabel(tr("⚪ Проверка сети..."), this);
    ui->statusbar->addPermanentWidget(m_netStatusLabel); // Добавляем в правый нижний угол

    // Слушаем ответ от NetworkManager
    connect(&NetworkManager::instance(), &NetworkManager::connectionStatus, this, [this, m_netStatusLabel](bool isOnline) {
        if (isOnline) {
            m_netStatusLabel->setText(tr("🟢 Online (Catalog available)"));
        } else {
            m_netStatusLabel->setText(tr("🔴 Offline (Local libraries only)"));
        }
    });

    // Запускаем проверку сети при старте программы
    NetworkManager::instance().checkConnection();
    // 1. Устанавливаем стандартное значение в комбобоксе (200 мс - это индекс 2)
    ui->speedComboBox->setCurrentIndex(2);
    // --- ДОБАВЛЯЕМ ПЕРЕКЛЮЧАТЕЛЬ СЕТКИ В МЕНЮ EDIT ---
        // =========================================================
        QAction *toggleGridAction = new QAction(tr("Show grid"), this);
        toggleGridAction->setCheckable(true);
        toggleGridAction->setChecked(true); // По умолчанию сетка включена
        ui->menuedit->addAction(toggleGridAction);

        // Подключаем НАШУ созданную кнопку, а не ту, что в ui
        connect(toggleGridAction, &QAction::toggled, this, [this](bool checked) {
            Utils::toggleGrid(this->grid, checked);
        });
        // =========================================================
    // 1. Создаем шторку
        m_projectSidebar = new ProjectSidebar(tr("Project Workspace"), this);
        addDockWidget(Qt::LeftDockWidgetArea, m_projectSidebar);
        resizeDocks({m_projectSidebar}, {220}, Qt::Horizontal);
        // ДВОЙНОЙ КЛИК / ОТКРЫТИЕ ПРОЕКТА
            connect(m_projectSidebar, &ProjectSidebar::projectDoubleClicked, this, [this](const QString &path) {
                // Железобетонная проверка пути
                if (QDir(path).exists("graph.json")) {
                    if (QDir::cleanPath(m_currentProjectPath) != QDir::cleanPath(path)) {
                        loadProject(path);
                    }
                }
            });
        // =============================================================
            // --- ДОБАВЛЯЕМ ПЕРЕКЛЮЧАТЕЛЬ ТЕМЫ В МЕНЮ EDIT ---
                // =========================================================
                QAction *toggleThemeAction = new QAction(tr("Dark theme"), this);
                toggleThemeAction->setCheckable(true);
                toggleThemeAction->setChecked(false); // По умолчанию светлая
                ui->menuedit->addAction(toggleThemeAction);

                connect(toggleThemeAction, &QAction::toggled, this, [](bool checked) {
                    Utils::applyTheme(checked);
                });
                // =========================================================



                // =========================================================
                    // --- ПЕРЕКЛЮЧАТЕЛЬ ЖЕЛТОГО ФОНА ТЕКСТА (С ПАМЯТЬЮ) ---
                    // =========================================================
                    QAction *toggleYellowBgAction = new QAction(tr("Yellow text background"), this);
                    toggleYellowBgAction->setCheckable(true);

                    // 1. Читаем сохраненную настройку (если её нет, ставим true по умолчанию)
                    QSettings yellowBgSettings("VisualBuild", "IDE");
                    bool isYellowBgEnabled = yellowBgSettings.value("showYellowBg", true).toBool();

                    // 2. Применяем прочитанную настройку к галочке и к самому Utils
                    toggleYellowBgAction->setChecked(isYellowBgEnabled);
                    Utils::setShowYellowLabels(isYellowBgEnabled);

                    ui->menuedit->addAction(toggleYellowBgAction);

                    connect(toggleYellowBgAction, &QAction::toggled, this, [this](bool checked) {
                        // 3. Сохраняем выбор пользователя навсегда в память ПК
                        QSettings s("VisualBuild", "IDE");
                        s.setValue("showYellowBg", checked);

                        // Применяем визуал
                        Utils::setShowYellowLabels(checked);
                        if (scene) scene->update(); // Мгновенно перерисовываем холст
                    });
                    // =========================================================

                    // =========================================================
                        // --- ПЕРЕКЛЮЧАТЕЛЬ АВТОМАРШРУТИЗАЦИИ СТРЕЛОК ---
                        // =========================================================
                        QAction *toggleAutoRouteAction = new QAction(tr("Auto-routing of arrows (2 clicks)"), this);
                        toggleAutoRouteAction->setCheckable(true);

                        // Читаем сохраненную настройку (по умолчанию false - ручное)
                        QSettings routeSettings("VisualBuild", "IDE");
                        bool isAutoRouteEnabled = routeSettings.value("autoRouting", false).toBool();

                        toggleAutoRouteAction->setChecked(isAutoRouteEnabled);
                        Utils::setAutoRoutingEnabled(isAutoRouteEnabled);

                        ui->menuedit->addAction(toggleAutoRouteAction);

                        connect(toggleAutoRouteAction, &QAction::toggled, this, [](bool checked) {
                            QSettings s("VisualBuild", "IDE");
                            s.setValue("autoRouting", checked);
                            Utils::setAutoRoutingEnabled(checked);
                        });
                        // =========================================================
                        // =========================================================
                                                // --- ПЕРЕКЛЮЧАТЕЛЬ ОТКЛЮЧЕНИЯ ПРЕДУПРЕЖДЕНИЙ О ВЫЛЕТАХ ---
                                                // =========================================================
                                                QAction *disableWarningsAction = new QAction(tr("Disable Crash Warnings"), this);
                                                disableWarningsAction->setCheckable(true);

                                                // Читаем сохраненное состояние
                                                QSettings warnSettings("VisualBuild", "IDE");
                                                disableWarningsAction->setChecked(warnSettings.value("DisableCrashWarnings", false).toBool());

                                                ui->menuedit->addAction(disableWarningsAction);

                                                connect(disableWarningsAction, &QAction::toggled, this, [](bool checked) {
                                                    QSettings s("VisualBuild", "IDE");
                                                    s.setValue("DisableCrashWarnings", checked);
                                                });
                                                // =========================================================
                                                // =========================================================
                                                    // --- ПЕРЕКЛЮЧАТЕЛЬ ПОДСВЕТКИ ОШИБОК (ОДНА ИЛИ ВСЕ) ---
                                                    // =========================================================
                                                    QAction *toggleErrorsAction = new QAction(tr("Highlight ALL errors"), this);
                                                    toggleErrorsAction->setCheckable(true);

                                                    QSettings errSettings("VisualBuild", "IDE");
                                                    bool isAllErrorsEnabled = errSettings.value("highlightAllErrors", false).toBool();

                                                    toggleErrorsAction->setChecked(isAllErrorsEnabled);
                                                    Utils::setHighlightAllErrors(isAllErrorsEnabled);

                                                    ui->menuedit->addAction(toggleErrorsAction);

                                                    connect(toggleErrorsAction, &QAction::toggled, this, [](bool checked) {
                                                        QSettings s("VisualBuild", "IDE");
                                                        s.setValue("highlightAllErrors", checked);
                                                        Utils::setHighlightAllErrors(checked);
                                                    });
                                                    // =========================================================
            //  ЛОВИМ СИГНАЛ: Открыть онлайн-менеджер С УЛУЧШЕННОЙ ЗАЩИТОЙ
                connect(m_projectSidebar, &ProjectSidebar::requestLibraryManager, this, [this]() {
                    // Узнаем, какая папка сейчас выделена в шторке
                    QString targetDir = m_projectSidebar->selectedFolder();
                    QFileInfo fi(targetDir);
                    QString folderName = fi.fileName(); // Получаем имя самой папки

                    // --- ЖЕСТКАЯ ЗАЩИТА ОТ ДУРАКА ---
                    // Разрешаем открывать менеджер ТОЛЬКО если выделена одна из папок библиотек!
                    if (!fi.isDir() ||
                       (folderName != "CppLibraries" &&
                        folderName != "PythonLibraries" &&
                        folderName != "JavaLibraries")) {

                        QMessageBox::warning(this, tr("Неверная папка"),
                            tr("Скачивать модули можно только в специальные папки библиотек!\n\n"
                               "Пожалуйста, выделите в дереве одну из этих папок:\n"
                               "📁 CppLibraries\n"
                               "📁 PythonLibraries\n"
                               "📁 JavaLibraries\n\n"
                               "А затем снова нажмите 'Library Manager'."));
                        return; // Прерываем выполнение, диалог не откроется
                    }

                    // Если проверка прошла (выбрана правильная папка) — открываем менеджер
                    LibraryManagerDialog dialog(targetDir, this);
                    dialog.exec();
                });

            // 4. ЛОВИМ СИГНАЛ: Добавить локальные файлы (Умное добавление)
            connect(m_projectSidebar, &ProjectSidebar::requestAddFile, this, [this]() {
                // Узнаем, какая папка сейчас выделена в шторке
                QString targetDir = m_projectSidebar->selectedFolder();

                // Заголовок окна теперь подсказывает, куда мы добавляем файлы
                QStringList fileNames = QFileDialog::getOpenFileNames(this,
                                                                      tr("Выберите файлы для добавления в папку: %1").arg(QFileInfo(targetDir).fileName()),
                                                                          "",
                                                                          tr("All Files (*.*)"));

                if (fileNames.isEmpty()) return;

                // Копируем выбранные файлы в выделенную папку
                for (const QString &filePath : fileNames) {
                    QFileInfo fi(filePath);
                    QString destPath = targetDir + "/" + fi.fileName();

                    // Защита от дубликатов: удаляем старый файл, если он уже существует
                    if (QFile::exists(destPath)) {
                        QFile::remove(destPath);
                    }

                    QFile::copy(filePath, destPath);
                }
                // Дерево (QFileSystemModel) обновится абсолютно само!
            });
            // 5. ЛОВИМ СИГНАЛ: ЗАКРЫТЬ ПРОЕКТ
                connect(m_projectSidebar, &ProjectSidebar::requestCloseProject, this, [this]() {
                    // Если ничего не открыто - игнорируем
                    if (m_currentProjectPath.isEmpty()) return;

                    // 1. Тихо сохраняем текущий проект перед закрытием (защита данных)
                    m_graphManager->saveToFile(m_currentProjectPath + "/graph.json", m_headerCode, m_currentLang, serializeSectors());
                    grid = nullptr;
                    // 2. Очищаем холст и память кода
                    // 2. БЕЗОПАСНАЯ ОЧИСТКА ХОЛСТА (вместо scene->clear())
                                        QList<QGraphicsItem*> allItems = scene->items();
                                        // Сначала аккуратно удаляем все стрелки
                                        for (QGraphicsItem *item : allItems) {
                                            if (dynamic_cast<ConnectionItem*>(item)) {
                                                scene->removeItem(item);
                                                delete item;
                                            }
                                        }
                                        // Теперь безопасно удаляем блоки, рамки и всё остальное
                                        allItems = scene->items();
                                        for (QGraphicsItem *item : allItems) {
                                            scene->removeItem(item);
                                            delete item;
                                        }
                                        scene->clear(); // Контрольный выстрел по пустой сцене
                                        m_headerCode.clear();
                                        if (m_graphManager) m_graphManager->clearHistory();

                    ui->headerButton->setStyleSheet(""); // Убираем зеленую рамку с кнопки Header

                    // 3. Возвращаем чистую сетку на пустой холст
                    grid = new Grid(16);
                    scene->addItem(grid);


                    grid->setData(0, 1);
                    grid->setScaleFactor(m_currentZoom);
                    grid->updateSceneRect();

                    // 4. Очищаем визуальную таблицу состояний
                    if (m_stateTable) m_stateTable->updateFromScene(scene);

                    // 5. Сбрасываем память "последнего проекта", чтобы он не автозагружался при следующем старте!
                    m_currentProjectPath = "";
                    QSettings settings("VisualBuild", "IDE");
                    settings.setValue("lastProject", "");

                    // 6. Возвращаем шторку в домашнюю директорию (показываем все проекты)
                    QString defaultWorkspace = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/VisualBuildProjects";
                    m_projectSidebar->setWorkspace(defaultWorkspace);

                    ui->statusbar->showMessage(tr("Проект закрыт. Холст очищен."), 3000);
                    if (m_welcomeScreen) m_welcomeScreen->show();
                    setProjectUIEnabled(false); // <-- Снова блокируем!
                });
                // --- СОЗДАЕМ КНОПКУ ПРЕДПРОСМОТРА КОДА ---
                // --- НАСТРАИВАЕМ НАШУ КНОПКУ ПРЕДПРОСМОТРА ИЗ ДИЗАЙНЕРА ---
                    ui->previewButton->setIcon(Utils::createPreviewIcon());
                    ui->previewButton->setToolTip(tr("Предпросмотр сгенерированного кода"));
                    ui->previewButton->setIconSize(QSize(24, 24));
                    ui->previewButton->setCursor(Qt::PointingHandCursor);

                    // Подключаем клик
                    connect(ui->previewButton, &QToolButton::clicked, this, &MainWindow::showCodePreview);

    // --- НАСТРОЙКА ИКОНОК ТУЛБАРА ---
    ui->addItemButton->setIcon(Utils::createBlockIcon());
    ui->addBridgeButton->setIcon(Utils::createBridgeIcon());
    ui->addBackButton->setIcon(Utils::createBackIcon());
    ui->addCommentButton->setIcon(Utils::createCommentIcon());
    ui->createGroupButton->setIcon(Utils::createGroupIcon());
    ui->addHistoryButton->setIcon(Utils::createHistoryIcon());
    // Горячая клавиша для удаления (Delete)

    // Горячая клавиша для удаления (Delete)
        QShortcut *deleteShortcut = new QShortcut(QKeySequence::Delete, this);
        deleteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(deleteShortcut, &QShortcut::activated, this, &MainWindow::deleteSelectedItems);

        // Горячие клавиши для копирования и вставки (Ctrl+C, Ctrl+V)
        QShortcut *copyShortcut = new QShortcut(QKeySequence::Copy, this);
        copyShortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(copyShortcut, &QShortcut::activated, this, &MainWindow::copySelectedItems);

        QShortcut *pasteShortcut = new QShortcut(QKeySequence::Paste, this);
        pasteShortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(pasteShortcut, &QShortcut::activated, this, &MainWindow::pasteCopiedItems);


    // Задаем одинаковый размер иконок для всех кнопок, сохраняя их текст из Qt Designer
    QList<QPushButton*> tools = {
        ui->addItemButton, ui->addBridgeButton, ui->addBackButton,
        ui->addCommentButton, ui->createGroupButton, ui->addHistoryButton
    };

    for (QPushButton* btn : tools) {
        if (btn) {
            btn->setIconSize(QSize(24, 24));



        }
    }
    // ---------------------------------
    // 2. Связываем выбор в комбобоксе с изменением скорости машины через Utils
        connect(ui->speedComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            if (m_logicEngine) {
                m_logicEngine->setTickInterval(Utils::getSpeedInterval(index));
            }
        });

        // Связываем кнопку из дизайнера с нашей функцией
        connect(ui->actionImport_JSON, &QAction::triggered, this, &MainWindow::on_actionImport_triggered);
    // =============================================================
// =============================================================
    // --- НАСТРОЙКА КНОПКИ DEBUG (ВЫПАДАЮЩЕЕ МЕНЮ) ---
    // =============================================================

    // 1. Создаем меню
    QMenu *debugMenu = new QMenu(this);

    // 2. Создаем действия (Actions)
    QAction *actEmulator = debugMenu->addAction(tr("Emulator"));
    QAction *actDynamic = debugMenu->addAction(tr("Dynamic"));

    // 3. Делаем их переключаемыми (галочки)
    actEmulator->setCheckable(true);
    actDynamic->setCheckable(true);

    // 4. Группа, чтобы можно было выбрать только одно
    QActionGroup *dbgGroup = new QActionGroup(this);
    dbgGroup->addAction(actEmulator);
    dbgGroup->addAction(actDynamic);

    // 5. Устанавливаем значение по умолчанию
    actEmulator->setChecked(true);
    m_currentDebugMode = Mode_EMULATOR;

    // 6. Логика переключения
    connect(actEmulator, &QAction::triggered, this, [this](){
        m_currentDebugMode = Mode_EMULATOR;
        ui->debugButton->setText(tr("Debug (Emulator)")); // Меняем текст для наглядности
        ui->statusbar->showMessage(tr("Режим отладки: Эмулятор ПК"), 3000);
    });

    connect(actDynamic, &QAction::triggered, this, [this](){
        m_currentDebugMode = Mode_DYNAMIC;
        ui->debugButton->setText(tr("Debug (Dynamic)"));
        ui->statusbar->showMessage(tr("Режим отладки: Динамический (На железе)"), 3000);
    });

    // 7. Прикрепляем меню к кнопке
    ui->debugButton->setMenu(debugMenu);
    // =============================================================
// --- МЕНЮ СИНТАКСИСА
QMenu *syntaxMenu = menuBar()->addMenu(tr("Syntax"));
syntaxMenu->menuAction()->setObjectName("menuSyntax");
    QActionGroup *langGroup = new QActionGroup(this);
    QAction *javaAction = syntaxMenu->addAction("Java");
    javaAction->setCheckable(true);
    langGroup->addAction(javaAction);

QAction *cppAction = syntaxMenu->addAction("C++");
cppAction->setCheckable(true);
cppAction->setChecked(true); // По умолчанию
langGroup->addAction(cppAction);

QAction *pyAction = syntaxMenu->addAction("Python");
pyAction->setCheckable(true);
langGroup->addAction(pyAction);

// Подключаем переключение синтаксиса
connect(javaAction, &QAction::triggered, this, [this](){
        SyntaxRegistry::instance().setLanguage(SyntaxRegistry::Lang_Java);
        scene->update();
        ui->statusbar->showMessage(tr("Синтаксис: %1").arg("Java"), 2000);
    });

connect(cppAction, &QAction::triggered, this, [this](){
    SyntaxRegistry::instance().setLanguage(SyntaxRegistry::Lang_Cpp);
    // Принудительно обновляем сцену, чтобы перерисовались блоки
    scene->update();
});

connect(pyAction, &QAction::triggered, this, [this](){
    SyntaxRegistry::instance().setLanguage(SyntaxRegistry::Lang_Python);
    scene->update();
});
// --- КНОПКА КОММЕНТАРИЯ (В коде, если нет в UI) ---
    // Если вы добавили кнопку в .ui c именем addCommentButton, этот код настроит иконку
    if (ui->addCommentButton) {        
        ui->addCommentButton->setToolTip(tr("Добавить комментарий/стикер. Выделите блок, чтобы привязать."));
    }
    // МЕНЮ "НАСТРОЙКИ" (Выбор транслятора)
    // 1. Создаем меню "Настройки" в верхней строке
QMenu *settingsMenu = menuBar()->addMenu(tr("Project Settings"));
// ---  АВТОЗАГРУЗКА ГРАФА ---
    QAction *actAutoLoad = settingsMenu->addAction(tr("Auto-load last project graph"));
    actAutoLoad->setCheckable(true);
    actAutoLoad->setObjectName("settingAutoLoad");

    // Считываем сохраненное значение (по умолчанию false - не загружать)
    QSettings startupSettings("VisualBuild", "IDE");
    actAutoLoad->setChecked(startupSettings.value("autoLoadGraph", false).toBool());

    // При клике сразу сохраняем выбор в память
    connect(actAutoLoad, &QAction::triggered, this, [](bool checked){
        QSettings s("VisualBuild", "IDE");
        s.setValue("autoLoadGraph", checked);
    });
    // --- ГЕНЕРАЦИЯ ПО ПАПКАМ (STRUCTURED FOLDERS) ---
            QAction *actStructured = settingsMenu->addAction(tr("Generate code in structured folders (%1, %2)").arg("src").arg("include"));
            actStructured->setCheckable(true);
            actStructured->setObjectName("settingStructured");

            // Читаем настройку (по умолчанию выключено)
            QSettings folderSettings("VisualBuild", "IDE");
            actStructured->setChecked(folderSettings.value("structuredFolders", false).toBool());

            connect(actStructured, &QAction::triggered, this, [](bool checked){
                QSettings s("VisualBuild", "IDE");
                s.setValue("structuredFolders", checked);
            });
    // --- НАСТРОЙКИ АВТОСОХРАНЕНИЯ (AUTOSAVE) ---
        QMenu *autoSaveMenu = settingsMenu->addMenu(tr("Auto-Save Settings"));
        autoSaveMenu->menuAction()->setObjectName("settingAutoSave");

        QAction *actEnableAutosave = autoSaveMenu->addAction(tr("Enable Auto-Save (1 min)"));
        actEnableAutosave->setCheckable(true);

        QAction *actShowAutosaveMsg = autoSaveMenu->addAction(tr("Show 'Autosave...' notification"));
        actShowAutosaveMsg->setCheckable(true);

        // Читаем сохраненные настройки (по умолчанию: автосохранение включено, уведомления выключены)
        QSettings asSettings("VisualBuild", "IDE");
        bool isAutosaveEnabled = asSettings.value("enableAutosave", true).toBool();
        bool showAutosaveMsg = asSettings.value("showAutosaveMsg", false).toBool();

        actEnableAutosave->setChecked(isAutosaveEnabled);
        actShowAutosaveMsg->setChecked(showAutosaveMsg);

        // Логика включения/отключения таймера
        connect(actEnableAutosave, &QAction::triggered, this, [this](bool checked){
            QSettings s("VisualBuild", "IDE");
            s.setValue("enableAutosave", checked);
            if (checked) {
                m_autoSaveTimer->start(60000); // Запускаем
                ui->statusbar->showMessage(tr("Автосохранение ВКЛЮЧЕНО"), 3000);
            } else {
                m_autoSaveTimer->stop(); // Останавливаем
                ui->statusbar->showMessage(tr("Автосохранение ВЫКЛЮЧЕНО"), 3000);
            }
        });
        settingsMenu->addSeparator(); // Добавляем линию-разделитель для красоты

        // Логика уведомлений
        connect(actShowAutosaveMsg, &QAction::triggered, this, [](bool checked){
            QSettings s("VisualBuild", "IDE");
            s.setValue("showAutosaveMsg", checked);
        });

    // --- НАСТРОЙКА СКОРОСТИ АВТО-РАСШИРЕНИЯ РАМОК ---
        // Читаем настройку прямо здесь, чтобы меню знало, куда ставить галочку
        QSettings speedSettings("VisualBuild", "IDE");
        int menuSpeed = speedSettings.value("groupExpandSpeed", 500).toInt();

        QMenu *groupSpeedMenu = settingsMenu->addMenu(tr("Group Auto-Expand Speed"));
        groupSpeedMenu->menuAction()->setObjectName("settingGroupSpeed");
        QActionGroup *speedGroup = new QActionGroup(this);
        speedGroup->setExclusive(true);

        QAction *speedOff = groupSpeedMenu->addAction(tr("Off (Best Performance)"));
        QAction *speedFast = groupSpeedMenu->addAction(tr("Fast (200 ms)"));
        QAction *speedNormal = groupSpeedMenu->addAction(tr("Normal (500 ms)"));
        QAction *speedSlow = groupSpeedMenu->addAction(tr("Slow (1.5 sec)"));

        speedOff->setCheckable(true);
        speedFast->setCheckable(true);
        speedNormal->setCheckable(true);
        speedSlow->setCheckable(true);

        speedGroup->addAction(speedOff);
        speedGroup->addAction(speedFast);
        speedGroup->addAction(speedNormal);
        speedGroup->addAction(speedSlow);

        // Устанавливаем галочку на сохраненном значении
        if (menuSpeed == 0) speedOff->setChecked(true);
        else if (menuSpeed == 200) speedFast->setChecked(true);
        else if (menuSpeed == 1500) speedSlow->setChecked(true);
        else speedNormal->setChecked(true); // 500 по умолчанию

        // Универсальная лямбда-функция для смены скорости
        auto setGroupSpeed = [this](int speed) {
            QSettings s("VisualBuild", "IDE");
            s.setValue("groupExpandSpeed", speed); // Сохраняем навсегда

            if (speed == 0) {
                m_groupCheckTimer->stop(); // Полностью отключаем таймер
                ui->statusbar->showMessage(tr("Авто-расширение рамок ОТКЛЮЧЕНО"), 3000);
            } else {
                m_groupCheckTimer->setInterval(speed);
                if (!m_groupCheckTimer->isActive()) m_groupCheckTimer->start();
                ui->statusbar->showMessage(tr("Скорость рамок: %1 мс").arg(speed), 3000);
            }
        };

        // Подключаем кнопки к функции
        connect(speedOff, &QAction::triggered, this, [setGroupSpeed](){ setGroupSpeed(0); });
        connect(speedFast, &QAction::triggered, this, [setGroupSpeed](){ setGroupSpeed(200); });
        connect(speedNormal, &QAction::triggered, this, [setGroupSpeed](){ setGroupSpeed(500); });
        connect(speedSlow, &QAction::triggered, this, [setGroupSpeed](){ setGroupSpeed(1500); });


        // ----------------------------------------
    // ----------------------------------------
        // =========================================================
        // --- ОКНО НАСТРОЕК КОМПИЛЯТОРОВ (TOOLCHAINS) ---
        // =========================================================
        QAction *toolchainsAction = new QAction(tr("Compiler Settings (Toolchains)..."), this);


        // Добавляем подменю "Project" прямо внутрь "Project Settings"
                QMenu *projectMenu = settingsMenu->addMenu(tr("Project"));
                projectMenu->addAction(toolchainsAction);

                // =========================================================
                                        // --- ОКНО НАСТРОЕК ПУТЕЙ И ИМЕН (ТИХАЯ ГЕНЕРАЦИЯ) ---
                                        // =========================================================
                                        QAction *pathsAction = new QAction(tr("Path Settings (Silent Export)..."), this);
                                        projectMenu->addAction(pathsAction);

                                        connect(pathsAction, &QAction::triggered, this, [this]() {
                                            QDialog dialog(this);
                                            dialog.setWindowTitle(tr("Настройки путей и файлов (Тихий экспорт)"));
                                            dialog.resize(750, 450); // Увеличили размер для новых элементов

                                            QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
                                            QSettings settings("VisualBuild", "IDE");

                                            // Рабочая область (Workspace)
                                            QHBoxLayout *wsRow = new QHBoxLayout();
                                            QLineEdit *wsEdit = new QLineEdit(settings.value("defaultWorkspace", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/VisualBuildProjects").toString());
                                            QPushButton *wsBtn = new QPushButton(tr("Обзор..."));
                                            connect(wsBtn, &QPushButton::clicked, [&dialog, wsEdit]() {
                                                QString path = QFileDialog::getExistingDirectory(&dialog, tr("Select Folder"), wsEdit->text().isEmpty() ? "C:/" : wsEdit->text());
                                                if (!path.isEmpty()) wsEdit->setText(path);
                                            });
                                            wsRow->addWidget(new QLabel(tr("Workspace (Папка проектов):")));
                                            wsRow->addWidget(wsEdit);
                                            wsRow->addWidget(wsBtn);
                                            mainLayout->addLayout(wsRow);

                                            QFrame* line1 = new QFrame();
                                            line1->setFrameShape(QFrame::HLine);
                                            line1->setFrameShadow(QFrame::Sunken);
                                            mainLayout->addWidget(line1);

                                            QFormLayout *form = new QFormLayout();

                                            // Улучшенная функция для создания строк экспорта с ИМЕНЕМ ФАЙЛА
                                            auto createExportRow = [&](const QString& label, const QString& folderKey, const QString& fileKey, const QString& defaultFile) -> QPair<QLineEdit*, QLineEdit*> {
                                                QHBoxLayout *row = new QHBoxLayout();
                                                QLineEdit *folderEdit = new QLineEdit(settings.value(folderKey, "").toString());
                                                folderEdit->setPlaceholderText(tr("Папка (пусто = окно сохранения)"));

                                                QPushButton *btnBrowse = new QPushButton(tr("Обзор..."));
                                                connect(btnBrowse, &QPushButton::clicked, [&dialog, folderEdit]() {
                                                    QString path = QFileDialog::getExistingDirectory(&dialog, tr("Select Folder"), folderEdit->text().isEmpty() ? "C:/" : folderEdit->text());
                                                    if (!path.isEmpty()) folderEdit->setText(path);
                                                });

                                                QLineEdit *fileEdit = new QLineEdit(settings.value(fileKey, defaultFile).toString());
                                                fileEdit->setPlaceholderText(tr("Имя файла"));
                                                fileEdit->setFixedWidth(180); // Делаем поле для имени файла компактным

                                                row->addWidget(folderEdit);
                                                row->addWidget(btnBrowse);
                                                row->addWidget(new QLabel(tr("Файл:")));
                                                row->addWidget(fileEdit);

                                                form->addRow(label, row);
                                                return {folderEdit, fileEdit};
                                            };

                                            // --- УБРАНЫ РАСШИРЕНИЯ ИЗ ДЕФОЛТНЫХ ИМЕН ---
                                            auto cppPair   = createExportRow(tr("C++:"), "exportPathCpp", "exportFileCpp", "StateMachine");
                                            auto pyPair    = createExportRow(tr("Python:"), "exportPathPython", "exportFilePython", "robot_script");
                                            auto javaPair  = createExportRow(tr("Java:"), "exportPathJava", "exportFileJava", "RobotApp");
                                            auto microPair = createExportRow(tr("Arduino/ESP:"), "exportPathMicro", "exportFileMicro", "Firmware");

                                            mainLayout->addLayout(form);

                                            // --- НОВАЯ СЕКЦИЯ: РЕЖИМ МИКРОКОНТРОЛЛЕРА ---
                                            QGroupBox *microGroup = new QGroupBox(tr("Формат тихого экспорта для Микроконтроллеров"), &dialog);
                                            QVBoxLayout *rbLayout = new QVBoxLayout(microGroup);
                                            QRadioButton *rbIno = new QRadioButton(tr("Простой скетч (.ino)"), microGroup);
                                            QRadioButton *rbCpp = new QRadioButton(tr("Прошивка PlatformIO (.cpp + .h)"), microGroup);
                                            QRadioButton *rbSim = new QRadioButton(tr("Симулятор на ПК (.cpp)"), microGroup);
                                            rbLayout->addWidget(rbIno);
                                            rbLayout->addWidget(rbCpp);
                                            rbLayout->addWidget(rbSim);
                                            mainLayout->addWidget(microGroup);

                                            // Считываем сохраненный режим
                                            int mode = settings.value("microExportMode", 0).toInt();
                                            if (mode == 1) rbCpp->setChecked(true);
                                            else if (mode == 2) rbSim->setChecked(true);
                                            else rbIno->setChecked(true);

                                            QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
                                            mainLayout->addWidget(buttons);
                                            connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
                                            connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

                                            if (dialog.exec() == QDialog::Accepted) {
                                                settings.setValue("defaultWorkspace", wsEdit->text());

                                                settings.setValue("exportPathCpp", cppPair.first->text());
                                                // --- УБРАНЫ РАСШИРЕНИЯ ПРИ СОХРАНЕНИИ ПУСТЫХ СТРОК ---
                                                settings.setValue("exportFileCpp", cppPair.second->text().isEmpty() ? "StateMachine" : cppPair.second->text());

                                                settings.setValue("exportPathPython", pyPair.first->text());
                                                settings.setValue("exportFilePython", pyPair.second->text().isEmpty() ? "robot_script" : pyPair.second->text());

                                                settings.setValue("exportPathJava", javaPair.first->text());
                                                settings.setValue("exportFileJava", javaPair.second->text().isEmpty() ? "RobotApp" : javaPair.second->text());

                                                settings.setValue("exportPathMicro", microPair.first->text());
                                                settings.setValue("exportFileMicro", microPair.second->text().isEmpty() ? "Firmware" : microPair.second->text());

                                                // Сохраняем выбранный режим микроконтроллера
                                                int newMode = 0;
                                                if (rbCpp->isChecked()) newMode = 1;
                                                else if (rbSim->isChecked()) newMode = 2;
                                                settings.setValue("microExportMode", newMode);

                                                QMessageBox::information(this, tr("Saved"), tr("Пути и форматы успешно сохранены!"));
                                            }
                                        });
                                        // =========================================================



        // 4. Логика открытия окна настроек
        connect(toolchainsAction, &QAction::triggered, this, [this]() {
            QDialog dialog(this);
            dialog.setWindowTitle(tr("Toolchains & Compilers"));
            dialog.resize(600, 250); // Немного увеличил ширину для новых кнопок

            QFormLayout form(&dialog);
            QSettings settings("VisualBuild", "IDE");

            // Обновленная лямбда-функция с поддержкой Автопоиска
            // Обновленная лямбда-функция с поддержкой Автопоиска
                        auto createRow = [&](const QString& label, const QString& key, const QString& defaultVal, const QString& toolName) {
                            QHBoxLayout *row = new QHBoxLayout();
                            QLineEdit *edit = new QLineEdit(settings.value(key, defaultVal).toString());

                            QPushButton *btnAuto = new QPushButton(tr("Автопоиск"));
                            QPushButton *btnBrowse = new QPushButton(tr("Browse..."));

                            // Логика автоматического поиска пути
                            connect(btnAuto, &QPushButton::clicked, [&dialog, edit, toolName]() {
                                QString foundPath;
                                if (toolName == "javac") {
                                    // Используем продвинутый поиск для Java В ТИХОМ РЕЖИМЕ (true)
                                    foundPath = Utils::findJavaTool("javac", &dialog, true);
                                } else {
                                    // Используем стандартный поиск для g++ и python
                                    foundPath = QStandardPaths::findExecutable(toolName);
                                }

                                if (!foundPath.isEmpty()) {
                                    edit->setText(foundPath);
                                    QMessageBox::information(&dialog,
                                                             tr("Найдено"),
                                                             tr("Инструмент успешно найден и путь обновлен:\n\n%1").arg(foundPath));
                                } else {
                                    QMessageBox::warning(&dialog,
                                                         tr("Не найдено"),
                                                         tr("Не удалось найти '%1' автоматически. Укажите путь вручную.").arg(toolName));
                                }
                            });

                            // Логика ручного выбора файла
                            connect(btnBrowse, &QPushButton::clicked, [&dialog, edit]() {
                                QString path = QFileDialog::getOpenFileName(&dialog, tr("Select Executable"), "C:/", tr("Executables (*.exe);;All Files (*)"));
                                if (!path.isEmpty()) edit->setText(path);
                            });

                            row->addWidget(edit);
                            row->addWidget(btnAuto);
                            row->addWidget(btnBrowse);
                            form.addRow(label, row);
                            return edit;
                        };

            // Создаем строки, передавая системное имя инструмента для поиска
            QLineEdit *cppEdit  = createRow("C++ (g++):", "compilerPathCpp", "g++", "g++");
            QLineEdit *pyEdit   = createRow("Python:", "compilerPathPython", "python", "python");
            QLineEdit *javaEdit = createRow("Java (javac):", "compilerPathJava", "javac", "javac");

            QDialogButtonBox buttons(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
            form.addRow(&buttons);
            connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

            if (dialog.exec() == QDialog::Accepted) {
                settings.setValue("compilerPathCpp", cppEdit->text());
                settings.setValue("compilerPathPython", pyEdit->text());
                settings.setValue("compilerPathJava", javaEdit->text());

                QMessageBox::information(this, tr("Saved"), tr("Compiler paths updated successfully!"));
            }
        });

            // =========================================================
    // 2. Создаем подменю "Транслятор"
    QMenu *translatorMenu = settingsMenu->addMenu(tr("State Machine Compiler"));
    translatorMenu->menuAction()->setObjectName("menuCompiler");
    // 3. Создаем группу (чтобы галочка была только одна)
    QActionGroup *transGroup = new QActionGroup(this);
    transGroup->setExclusive(true);

    // --- [НОВОЕ] Пункт Java (Самый первый!) ---
    QAction *actJava = translatorMenu->addAction("Java");
    actJava->setCheckable(true);
    actJava->setObjectName("actTransJava");
    transGroup->addAction(actJava);

    // --- Пункт C++ ---
    QAction *actCpp = translatorMenu->addAction("C++");
    actCpp->setCheckable(true);
    actCpp->setChecked(true); // Выбран по умолчанию
    actCpp->setObjectName("actTransCpp");
    transGroup->addAction(actCpp);

    // --- Пункт Python ---
    QAction *actPy = translatorMenu->addAction("Python");
    actPy->setCheckable(true);
    actPy->setObjectName("actTransPy");
    transGroup->addAction(actPy);

    // Пункт Microcontroller ---
    QAction *actMicro = translatorMenu->addAction("Arduino / ESP32/ STM32");
    actMicro->setCheckable(true);
    actMicro->setObjectName("actTransMicro");
    transGroup->addAction(actMicro);

    // 4. Логика переключения переменной m_currentLang
    m_currentLang = Lang_CPP; // Значение по умолчанию

    // --- СВЯЗКА (ТРАНСЛЯТОР + СИНТАКСИС) ---

    // --- ЛОГИКА ПЕРЕКЛЮЧЕНИЯ ТРАНСЛЯТОРА + АВТО-СМЕНА СИНТАКСИСА ---

        // ВАЖНО: В квадратных скобках [this, javaAction] мы "захватываем" кнопку синтаксиса,
        // чтобы внутри функции нажать на неё программно.

        connect(actJava, &QAction::triggered, this, [this, javaAction](){
            m_currentLang = Lang_JAVA;

            // 1. Меняем мозги
            SyntaxRegistry::instance().setLanguage(SyntaxRegistry::Lang_Java);
            // 2. Меняем галочку в меню Синтаксис (Визуально)
            javaAction->setChecked(true);
            // 3. Обновляем сцену
            if(scene) scene->update();

            ui->statusbar->showMessage(tr("Режим: %1").arg("Java"), 3000);
        });

        connect(actCpp, &QAction::triggered, this, [this, cppAction](){
            m_currentLang = Lang_CPP;
            SyntaxRegistry::instance().setLanguage(SyntaxRegistry::Lang_Cpp);
            cppAction->setChecked(true); // Ставим галочку на C++
            if(scene) scene->update();
            ui->statusbar->showMessage(tr("Режим: %1").arg("C++"), 3000);
        });

        connect(actPy, &QAction::triggered, this, [this, pyAction](){
            m_currentLang = Lang_PYTHON;
            SyntaxRegistry::instance().setLanguage(SyntaxRegistry::Lang_Python);
            pyAction->setChecked(true); // Ставим галочку на Python
            if(scene) scene->update();
            ui->statusbar->showMessage(tr("Режим: %1").arg("Python"), 3000);
        });

        connect(actMicro, &QAction::triggered, this, [this, cppAction](){
            m_currentLang = Lang_MICRO;
            // Для Ардуино включаем C++
            SyntaxRegistry::instance().setLanguage(SyntaxRegistry::Lang_Cpp);
            cppAction->setChecked(true); // Ставим галочку на C++
            if(scene) scene->update();
            ui->statusbar->showMessage(tr("Режим: Микроконтроллер (%1)").arg("Arduino/ESP/STM"), 3000);
        });
    // =============================================================
        // =============================================================
                    // --- МЕНЮ "СПРАВКА" (HELP & TIPS) ---
                    // =============================================================
                    QMenu *helpMenu = settingsMenu->addMenu(tr("Help"));

            QAction *actTips = helpMenu->addAction(tr("Tips & Shortcuts"));
            actTips->setObjectName("actionHelpTips");

            connect(actTips, &QAction::triggered, this, [this]() {
                QMessageBox::information(this,
                    // 2. Оборачиваем заголовок окна
                    tr("Полезные советы"),

                    // 3. Оборачиваем весь огромный блок текста с HTML-тегами
                    tr("<h3>🛠 Горячие клавиши:</h3>"
                       "<ul>"
                       "<li><b>Ctrl + C</b> : Скопировать выделенные элементы</li>"
                       "<li><b>Ctrl + V</b> : Вставить скопированные элементы</li>"
                       "<li><b>Ctrl + Z</b> : Отменить последнее действие (Undo)</li>"
                       "<li><b>Ctrl + Y</b> : Вернуть отмененное действие (Redo)</li>"
                       "<li><b>Delete</b> : Удалить выделенные элементы</li>"
                       "</ul>"
                       "<h3>💡 Важно знать:</h3>"
                       "<p>История ваших шагов (Undo/Redo) сохраняется только для текущего активного проекта. "
                       "При создании нового проекта, закрытии текущего или переключении на другой файл в шторке, "
                       "<b>история действий полностью стирается</b>. <br><br>"
                       "Это сделано для защиты ваших проектов от случайного смешивания данных и для экономии оперативной памяти ПК.</p>")
                );
            });
            helpMenu->addSeparator(); // Добавляем красивую линию-разделитель
            QAction *actAbout = helpMenu->addAction(tr("About %1").arg("VisualBuild..."));
            actAbout->setObjectName("actionHelpAbout");

            connect(actAbout, &QAction::triggered, this, [this]() {
                // Создаем свое собственное окно нужного размера
                QDialog aboutDialog(this);
                aboutDialog.setWindowTitle(tr("About").arg("VisualBuildStateMachine Creator v 1.0.0"));
                aboutDialog.setMinimumSize(750, 650);

                // Используем только ВЕРТИКАЛЬНЫЙ слой
                QVBoxLayout *mainLayout = new QVBoxLayout(&aboutDialog);

                // ТЕКСТОВОЕ ПОЛЕ СО СКРОЛЛОМ (теперь картинка будет прямо внутри него)
                QTextBrowser *textBrowser = new QTextBrowser(&aboutDialog);
                textBrowser->setFrameShape(QFrame::NoFrame); // Убираем некрасивую рамку
                textBrowser->viewport()->setAutoFillBackground(false); // Прозрачный фон
                textBrowser->setOpenExternalLinks(true); // Разрешаем кликать на email

                // HTML текст (Увеличили размер картинки до 480x480 для четкости)
                QString html = tr(
                    "<div align='center'>"
                    "<img src=':/StateMachineicon.png' width='480' height='480'>"
                    "</div>"
                    "<h2 style='text-align: center;'>VisualBuildStateMachine Creator</h2>"
                    "<p><b>Version:</b> 1.0.0</p>"
                    "<p>A tool for the visual design of finite state machines and logic generation<br>"
                    "for C++, Python, Java, and microcontrollers. <b>Features a core SCXML translator.</b></p>"
                    "<hr>"
                    "<p><b>Authors:</b></p>"
                    "<p style='font-size: 13px; line-height: 1.4;'>"
                    "<b>Yaroslav Yevheniyovych Donchenko</b><br>"
                    "<i>Student at the Department of Automation of Production Processes, Donbas State Engineering Academy.</i><br>"
                    "ORCID: 0009-0006-2746-0359<br><br>"
                    "<b>Donchenko Yevhenii Ivanovych</b><br>"
                    "<i>Ph.D. in Technical Sciences, Senior Lecturer at the Department of Automation of Production Processes, Donbas State Engineering Academy.</i><br>"
                    "E-mail: donchenko.egen@gmail.com | ORCID: 0000-0001-6941-9019"
                    "</p>"
                    "<hr>"
                    "<p style='font-size: 12px;'><b>License: GNU GPL v3.0</b><br>"
                    "This program is open-source and free software. You are welcome to <b>fork</b>, redistribute, and/or modify it<br>"
                    "under the terms of the GNU General Public License as published by the Free Software Foundation.</p>"
                    "<p style='font-size: 12px; color: #2c3e50;'><b>Note on Generated Code:</b><br>"
                    "The source code generated by this tool is the property of the user. Using VisualBuild "
                    "does not impose the GPL license on your generated output. You are completely free to use, "
                    "modify, and distribute the generated code in any open-source or proprietary commercial projects.</p>"
                    "<p style='font-size: 12px; color: #8b0000;'><b>Disclaimer of Liability:</b><br>"
                    "This software is provided \"as is\", without warranty of any kind, express or implied. "
                    "The authors assume no responsibility for any damage, loss of data, or equipment malfunction "
                    "resulting from the use of this software or the code it generates.</p>"
                );

                textBrowser->setHtml(html);

                // Добавляем текстовое поле со всем содержимым на форму
                mainLayout->addWidget(textBrowser, 1);

                // КНОПКА ОК
                QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, &aboutDialog);
                connect(buttonBox, &QDialogButtonBox::accepted, &aboutDialog, &QDialog::accept);
                mainLayout->addWidget(buttonBox);

                aboutDialog.exec(); // Показываем окно
            });
    ui->graphicsView->setScene(scene);

    // --- ИНТЕГРАЦИЯ GRAPH MANAGER ---

        // 1. Создаем менеджер
        m_graphManager = new GraphManager(scene, this);
        // --- ЛОВИМ СИГНАЛ С ХОЛСТА ДЛЯ ИСТОРИИ ---
            connect(ui->graphicsView, &DiagramView::sceneChanged, this, [this]() {
                if (m_graphManager) {
                    m_graphManager->createSnapshot(); // Сохраняем шаг!
                }
            });
        // 2. Настраиваем горячие клавиши (Ctrl+Z, Ctrl+Y)
        // Undo
        QShortcut *undoShortcut = new QShortcut(QKeySequence("Ctrl+Z"), this);
        connect(undoShortcut, &QShortcut::activated, this, &MainWindow::on_actionUndo_triggered);

        // Redo (обычно Ctrl+Y или Ctrl+Shift+Z)
        QShortcut *redoShortcut = new QShortcut(QKeySequence("Ctrl+Y"), this);
        connect(redoShortcut, &QShortcut::activated, this, &MainWindow::on_actionRedo_triggered);

    ui->startButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    // --- ИСПРАВЛЕНИЕ: ЖЕСТКИЕ ГРАНИЦЫ ХОЛСТА ---
        // Теперь холст начинается строго с 0,0 и растет только вправо и вниз (на 500 000 пикселей)
    // --- ИСПРАВЛЕНИЕ: МЯГКИЕ ГРАНИЦЫ ХОЛСТА ---
        // Даем камере запас в -5000 пикселей, чтобы она могла выводить
        // крайние блоки ровно в центр экрана, не упираясь в стену.
        scene->setSceneRect(-5000, -5000, 500000, 500000);
        ui->graphicsView->setAlignment(Qt::AlignCenter);
        // =========================================================
                // --- СОЗДАНИЕ СТАРТОВОГО ЭКРАНА (WELCOME SCREEN) ---
                // =========================================================
                m_welcomeScreen = new QFrame(ui->graphicsView);
                m_welcomeScreen->setObjectName("welcomeScreen");

                // Красивый CSS для панели (белый фон, скругления, пунктирная рамка)
                m_welcomeScreen->setStyleSheet(
                    "QFrame#welcomeScreen { "
                    "background-color: #fcfcfc; "
                    "border: 2px dashed #b0b0b0; "
                    "border-radius: 12px; "
                    "}"
                );

                QVBoxLayout *welcomeLayout = new QVBoxLayout(m_welcomeScreen);
                welcomeLayout->setContentsMargins(50, 50, 50, 50);
                welcomeLayout->setSpacing(20);

                QLabel *welcomeTitle = new QLabel(tr("No Project Opened"), m_welcomeScreen);
                welcomeTitle->setAlignment(Qt::AlignCenter);
                welcomeTitle->setStyleSheet("font-size: 26px; font-weight: bold; color: #444; border: none;");

                QLabel *welcomeSub = new QLabel(tr("Create a new project or open an existing one to start."), m_welcomeScreen);
                welcomeSub->setAlignment(Qt::AlignCenter);
                welcomeSub->setStyleSheet("font-size: 14px; color: #777; border: none;");

                QPushButton *btnCreateProj = new QPushButton(tr("➕ Create New Project"), m_welcomeScreen);
                QPushButton *btnOpenProj = new QPushButton(tr("📁 Open Existing Project"), m_welcomeScreen);
                btnCreateProj->setMinimumHeight(45);
                btnOpenProj->setMinimumHeight(45);
                btnCreateProj->setCursor(Qt::PointingHandCursor);
                btnOpenProj->setCursor(Qt::PointingHandCursor);

                // Стили кнопок
                btnCreateProj->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-size: 16px; font-weight: bold; border-radius: 6px; } QPushButton:hover { background-color: #45a049; }");
                btnOpenProj->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-size: 16px; font-weight: bold; border-radius: 6px; } QPushButton:hover { background-color: #1e88e5; }");

                welcomeLayout->addWidget(welcomeTitle);
                welcomeLayout->addWidget(welcomeSub);
                welcomeLayout->addSpacing(15);
                welcomeLayout->addWidget(btnCreateProj);
                welcomeLayout->addWidget(btnOpenProj);

                // Центрируем панель прямо поверх холста
                QGridLayout *viewLayout = new QGridLayout(ui->graphicsView);
                viewLayout->addWidget(m_welcomeScreen, 0, 0, Qt::AlignCenter);

                // Привязываем новые кнопки к уже существующим функциям меню!
                connect(btnCreateProj, &QPushButton::clicked, this, &MainWindow::on_actionNew_Project_triggered);
                connect(btnOpenProj, &QPushButton::clicked, this, &MainWindow::on_actionOpen_Project_triggered);

                // =========================================================
    // Инициализируем окно таблицы
    m_stateTable = new StateTable(this);
    m_codeExecutor = new CodeExecutor(this);
    // --- Инициализация движка SCXML ---
    m_logicEngine = new LogicStateMachine(this);

    // Связываем кнопки с нашими функциями
    ui->checkButton->setToolTip(tr("Проверить схему на ошибки без сохранения"));
    ui->generateButton->setToolTip(tr("Проверить и сохранить в SCXML"));

    // ------------------ ДОБАВЛЯЕМ СЕТКУ ------------------
    grid = new Grid(16);
    scene->addItem(grid);
    grid->setData(0, 1);

    grid->setScaleFactor(m_currentZoom);
    grid->updateSceneRect();

    connect(scene, &QGraphicsScene::sceneRectChanged, this, [this](const QRectF &){
        if (grid) grid->updateSceneRect();
    });

    ui->graphicsView->initConnector(scene);
    ui->graphicsView->initArrowEditor(scene);
    m_hwListener = new HardwareListener(this);
    m_hwWifiListener = new HardwareWifiListener(this);

    // =============================================================
        // --- ТАЙМЕР ДЛЯ РАМОК (ГРУПП) ---
        // =============================================================
        m_groupCheckTimer = new QTimer(this);
        connect(m_groupCheckTimer, &QTimer::timeout, this, &MainWindow::checkForGroupExpansion);

        // Читаем сохраненную скорость (по умолчанию 500 мс)
        QSettings timerSettings("VisualBuild", "IDE");
        int savedSpeed = timerSettings.value("groupExpandSpeed", 500).toInt();

        // Если не 0 (не выключено) — запускаем
        if (savedSpeed > 0) {
            m_groupCheckTimer->setInterval(savedSpeed);
            m_groupCheckTimer->start();
        }
        // =============================================================

        // =============================================================
        // --- УМНАЯ АВТОЗАГРУЗКА ПРИ СТАРТЕ (ТЕПЕРЬ НА ПРАВИЛЬНОМ МЕСТЕ!) ---
        // =============================================================
        QSettings settings("VisualBuild", "IDE");
        QString lastProject = settings.value("lastProject", "").toString();
        QString defaultWorkspace = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/VisualBuildProjects";

        // Читаем, включил ли пользователь нашу новую галочку в настройках
        bool autoLoadGraph = settings.value("autoLoadGraph", false).toBool();

        if (!lastProject.isEmpty() && QDir(lastProject).exists("graph.json")) {
            // 1. ПРОЕКТ НАЙДЕН: Всегда настраиваем шторку, чтобы показать дерево файлов
            QFileInfo fi(lastProject);
            m_projectSidebar->setWorkspace(fi.absolutePath());

            // 2. Проверяем галочку: загружать ли сам граф на холст?
            if (autoLoadGraph) {
                // Используем singleShot 0, чтобы загрузка началась строго после полного завершения конструктора
                QTimer::singleShot(0, this, [this, lastProject]() {
                    loadProject(lastProject);
                });
            } else {
                // Если галочка выключена, холст остается чистым.
                m_currentProjectPath = "";
            }
        } else {
            // ПРОЕКТА НЕТ (первый запуск): Открываем стандартную папку
            QDir().mkpath(defaultWorkspace);
            m_projectSidebar->setWorkspace(defaultWorkspace);
        }
        // --- ТАЙМЕР АВАРИЙНОГО АВТОСОХРАНЕНИЯ (КАЖДУЮ МИНУТУ) ---
            // =============================================================
        m_autoSaveTimer = new QTimer(this);
            connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::performAutoSave);

            // Запускаем только если настройка включена
            if (QSettings("VisualBuild", "IDE").value("enableAutosave", true).toBool()) {
                m_autoSaveTimer->start(60000); // 60 000 мс = 1 минута
            }
            // =============================================================

            // --- ПАНЕЛЬ СЕКТОРОВ (ВКЛАДКИ ВНИЗУ ЭКРАНА) ---
                // =============================================================
                m_sectorTabBar = new QTabBar(this);
                m_sectorTabBar->setDrawBase(false); // Убираем лишнюю линию снизу
                m_sectorTabBar->setExpanding(false);

                QToolButton *addSectorBtn = new QToolButton(this);
                addSectorBtn->setText("+");
                addSectorBtn->setToolTip(tr("Создать сектор (запомнить текущее положение камеры)"));
                addSectorBtn->setObjectName("addSectorBtn");

                // Создаем нижний тулбар и кладем туда вкладки и кнопку
                QToolBar *sectorToolBar = new QToolBar("Sectors", this);
                sectorToolBar->setMovable(false); // Запрещаем отрывать панель
                sectorToolBar->addWidget(m_sectorTabBar);
                sectorToolBar->addWidget(addSectorBtn);

                // Добавляем тулбар в самый низ окна (над статус-баром)
                addToolBar(Qt::BottomToolBarArea, sectorToolBar);

                // Подключаем сигналы
                connect(m_sectorTabBar, &QTabBar::currentChanged, this, &MainWindow::onSectorChanged);
                connect(addSectorBtn, &QToolButton::clicked, this, &MainWindow::onAddSectorClicked);

                // ==========================================================
                // --- КОНТЕКСТНОЕ МЕНЮ ДЛЯ СЕКТОРОВ (ПЕРЕИМЕНОВАНИЕ И УДАЛЕНИЕ) ---
                // ==========================================================
                m_sectorTabBar->setContextMenuPolicy(Qt::CustomContextMenu);
                connect(m_sectorTabBar, &QTabBar::customContextMenuRequested, this, [this](const QPoint &pos) {
                    int index = m_sectorTabBar->tabAt(pos);
                    if (index < 0) return; // Если кликнули мимо вкладки

                    QMenu menu(this);
                    QAction *renameAct = menu.addAction(tr("✏️ Переименовать сектор"));
                    QAction *deleteAct = menu.addAction(tr("🗑️ Удалить сектор"));

                    // Показываем меню ровно под курсором
                    QAction *selected = menu.exec(m_sectorTabBar->mapToGlobal(pos));

                    if (selected == renameAct) {
                        bool ok;
                        QString oldName = m_sectorTabBar->tabText(index);
                        QString newName = QInputDialog::getText(this,
                                                                tr("Переименование"),
                                                                tr("Новое имя сектора:"),
                                                                QLineEdit::Normal, oldName, &ok);
                        // Если ввели новое имя и нажали ОК
                        if (ok && !newName.trimmed().isEmpty()) {
                            m_sectorTabBar->setTabText(index, newName.trimmed());
                        }
                    }
                    else if (selected == deleteAct) {
                        // Спрашиваем подтверждение, чтобы не удалить случайно
                        QMessageBox::StandardButton reply;
                        reply = QMessageBox::question(this,
                                                      tr("Удаление сектора"),
                                                      tr("Удалить сектор \"%1\"?\n(Сами блоки останутся на холсте)").arg(m_sectorTabBar->tabText(index)),
                                                      QMessageBox::Yes | QMessageBox::No);
                        if (reply == QMessageBox::Yes) {
                            m_sectorTabBar->removeTab(index);            // Удаляем визуал (вкладку)
                            if (index < m_sectorCenters.size()) {
                                m_sectorCenters.removeAt(index);         // Удаляем математику (координаты)
                            }
                        }
                    }
                });
                // ==========================================================

                // --- СОЗДАЕМ БАЗОВЫЙ СЕКТОР A1 ПО УМОЛЧАНИЮ ---
                m_sectorTabBar->addTab("A1");
                // ТЕПЕРЬ СОХРАНЯЕМ ЛЕВЫЙ ВЕРХНИЙ УГОЛ (0, 0), а не центр!
                m_sectorCenters.append(QPointF(0.0, 0.0));

                // Рисуем рамку для первого поля (2500x1500)
                QGraphicsRectItem* bgRect = new QGraphicsRectItem(0, 0, 5000, 3000);
                bgRect->setPen(QPen(Qt::gray, 2, Qt::DashLine));
                bgRect->setZValue(-1000);
                scene->addItem(bgRect);

                // Текст прижимаем в самый угол
                QGraphicsTextItem* label = new QGraphicsTextItem("A1");
                label->setPos(10, 10); // Сдвиг всего на 10 пикселей от левого края
                label->setDefaultTextColor(QColor(150, 150, 150, 150));
                QFont f = label->font(); f.setPointSize(48); f.setBold(true);
                label->setFont(f);
                label->setZValue(-1000);
                scene->addItem(label);

                // --- ДОБАВЛЯЕМ ПОДМЕНЮ "ВИД" (Скрытие панелей) ---
                // ==========================================================
                // Так как переменная settingsMenu уже существует в конструкторе,
                // мы просто добавляем подменю напрямую в неё, без поиска!

                if (settingsMenu) {
                    settingsMenu->addSeparator(); // Красивая линия-разделитель
                    QMenu *viewMenu = settingsMenu->addMenu(tr("View (Show/Hide)"));

                    // Добавляем галочку для левой шторки
                    QAction *toggleSidebar = m_projectSidebar->toggleViewAction();
                    toggleSidebar->setText(tr("Project Curtain"));
                    viewMenu->addAction(toggleSidebar);

                    // Добавляем галочку для нижней панели секторов
                    QAction *toggleSectors = sectorToolBar->toggleViewAction();
                    toggleSectors->setText(tr("Sector panel"));
                    viewMenu->addAction(toggleSectors);
                }
                // ==========================================================
                                // ==========================================================
                                // ==========================================================

                                // Запускаем телепорт на А1 через 100мс (чтобы окно успело отрисоваться и получить правильные размеры)
                                QTimer::singleShot(100, this, [this]() {
                                    onSectorChanged(0);
                                });
                                // ========================
                                // --- ПРАВИЛЬНОЕ МЕСТО ДЛЯ БЛОКИРОВКИ ПРИ СТАРТЕ ---
                                                setProjectUIEnabled(false);
                                                // =========================================================
                                                // =========================================================
                                                        // --- НАСТРОЙКА ПОЛЗУНКА ЗАГРУЗКИ ИЗ UI ---
                                                        // =========================================================
                                                        ui->progressBar->setRange(0, 0); // Делаем анимацию бесконечной (бегающий кубик)
                                                        ui->progressBar->hide();         // Прячем его по умолчанию при запуске
                                                        // =========================================================
                // =============================================================
                                                        // ==========================================================                                                      
                                                                                                                // --- ДОБАВЛЯЕМ МЕНЮ ЯЗЫКОВ СЮДА ---
                                                                                                                // ==========================================================
                                                                                                                QMenu *langMenu = settingsMenu->addMenu(tr("Language"));

                                                        // ИСПРАВЛЕНИЕ: Переименовали langGroup в uiLangGroup
                                                        QActionGroup *uiLangGroup = new QActionGroup(this);
                                                        uiLangGroup->setExclusive(true);

                                                        QAction *actionRu = langMenu->addAction("Русский (Russian)");
                                                        actionRu->setCheckable(true);
                                                        uiLangGroup->addAction(actionRu);

                                                        QAction *actionEn = langMenu->addAction("English (Английский)");
                                                        actionEn->setCheckable(true);
                                                        uiLangGroup->addAction(actionEn);

                                                        // Читаем текущий язык из памяти ПК и ставим галочку
                                                        QSettings langSettings("VisualBuild", "IDE");
                                                        QString currentLangStr = langSettings.value("language", "en_US").toString();

                                                        if (currentLangStr == "en_US") {
                                                            actionEn->setChecked(true);
                                                        } else {
                                                            actionRu->setChecked(true);
                                                        }

                                                        // Обработка нажатия на "Русский"
                                                        connect(actionRu, &QAction::triggered, this, [this]() {
                                                            QSettings settings("VisualBuild", "IDE");
                                                            if (settings.value("language").toString() != "ru_RU") {
                                                                settings.setValue("language", "ru_RU");

                                                                QMessageBox::StandardButton reply;
                                                                reply = QMessageBox::question(this, tr("Смена языка"),
                                                                                              tr("Язык изменен на Русский.\nПерезапустить программу сейчас, чтобы применить изменения?"),
                                                                                              QMessageBox::Yes | QMessageBox::No);

                                                                if (reply == QMessageBox::Yes) {
                                                                    qApp->quit();
                                                                    QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
                                                                }
                                                            }
                                                        });

                                                        // Обработка нажатия на "Английский"
                                                        connect(actionEn, &QAction::triggered, this, [this]() {
                                                            QSettings settings("VisualBuild", "IDE");
                                                            if (settings.value("language").toString() != "en_US") {
                                                                settings.setValue("language", "en_US");

                                                                QMessageBox::StandardButton reply;
                                                                reply = QMessageBox::question(this, tr("Смена языка"),
                                                                                              tr("Язык изменен на Английский.\nПерезапустить программу сейчас, чтобы применить изменения?"),
                                                                                              QMessageBox::Yes | QMessageBox::No);

                                                                if (reply == QMessageBox::Yes) {
                                                                    qApp->quit();
                                                                    QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList());
                                                                }
                                                            }
                                                        });
                                                        // ==========================================================
                                                            // ==========================================================
                                                            // ==========================================================
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_addItemButton_clicked()
{
    DiagramItem *item = new DiagramItem();
    item->setGrid(grid);

    QPointF center = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect().center());

    QRectF rect = item->rect();
    qreal origW = rect.width();
    qreal origH = rect.height();

    qreal step = 0.0;
    if (grid) step = grid->getStep();

    qreal w = origW;
    qreal h = origH;
    QPointF topLeft;

    if (grid && step > 0.0) {
        w = std::round(origW / step) * step;
        h = std::round(origH / step) * step;

        const qreal minW = 40.0;
        const qreal minH = 24.0;
        if (w < minW) w = minW;
        if (h < minH) h = minH;

        QPointF desiredTopLeft = center - QPointF(w / 2.0, h / 2.0);
        topLeft = grid->snapToGrid(desiredTopLeft);
    } else {
        topLeft = center - QPointF(w / 2.0, h / 2.0);
    }

    item->prepareResize();
    item->setRect(0, 0, w, h);
    item->setPos(topLeft);

    scene->addItem(item);
    state->addItem(item);
    // <--- ВСТАВИТЬ СЮДА: Делаем снимок истории после добавления
    if (m_graphManager) m_graphManager->createSnapshot();
}

void MainWindow::on_openTableButton_clicked()
{
    // Вся логика открытия и обновления теперь в Utils
    Utils::openTransitionTable(m_stateTable, scene);
}

// ИСПРАВЛЕНИЕ 3: Старую функцию onSyntaxCPlusClicked мы удалили,
// КНОПКА 1: Просто проверка (для отладки)
// ---------------------------------------------------------
void MainWindow::on_checkButton_clicked()
{
    // 1. Запускаем только валидацию
    QList<ValidationError> errors = ScxmlCompiler::validate(scene);

    if (errors.isEmpty()) {
        QMessageBox::information(this,
                                 tr("Проверка"),
                                 tr("Ошибок нет! Схема корректна."));
    } else {
        // Собираем текст ошибок
        QString msg = tr("Найдены ошибки:\n");
        for (const auto &err : errors) {
            // Формируем пункт списка через шаблон
            msg += tr("- %1\n").arg(err.message);

            // Подсвечиваем ошибки на сцене (выделяем их)
            if (err.block) err.block->setSelected(true);
            if (err.arrow) err.arrow->setSelected(true);
        }
        QMessageBox::warning(this, tr("Ошибка проверки"), msg);
    }
}

// ---------------------------------------------------------
// КНОПКА 2: Генерация (с защитой)
// ---------------------------------------------------------
void MainWindow::on_generateButton_clicked()
{
    // ====================================================================
        // --- ПРЕДВЫЧИСЛИТЕЛЬНАЯ ПРОВЕРКА: ЗАЩИТА ОТ ИСТОРИИ ВНЕ РАМОК ---
        bool hasHistoryError = false;
        for (QGraphicsItem *item : scene->items()) {
            if (HistoryItem *hist = dynamic_cast<HistoryItem*>(item)) {
                bool inGroup = false;
                for (QGraphicsItem *possibleGroup : scene->items()) {
                    if (GroupItem *g = dynamic_cast<GroupItem*>(possibleGroup)) {
                        if (g->sceneBoundingRect().contains(hist->sceneBoundingRect().center())) {
                            inGroup = true; break;
                        }
                    }
                }
                if (!inGroup) {
                    hasHistoryError = true;
                    scene->clearSelection();
                    hist->setSelected(true); // Подсвечиваем ошибочный блок
                    break;
                }
            }
        }
        if (hasHistoryError) {
            QMessageBox::critical(this, tr("Генерация заблокирована"), tr("Ошибка: Найден блок 'История', который находится вне рамки!\n\nПожалуйста, поместите выделенный блок внутрь рамки или удалите его."));
            return;
        }
        // ====================================================================
    // 1. Проверка ошибок
    QList<ValidationError> errors = ScxmlCompiler::validate(scene);
    if (!errors.isEmpty()) {
        QString msg = tr("Генерация остановлена! Найдены ошибки:\n");
        for (const auto &err : errors) {
            msg += tr("- %1\n").arg(err.message);
            if (err.block) err.block->setSelected(true);
            if (err.arrow) err.arrow->setSelected(true);
        }
        QMessageBox::warning(this, tr("Ошибка проверки"), msg);
        return;
    }




    // 2. Выбор файла (Умный путь: Папка проекта ИЛИ Рабочий стол)
        QString defaultDir = m_currentProjectPath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) : m_currentProjectPath;
        QString path = QFileDialog::getSaveFileName(this, tr("Сохранить SCXML"), defaultDir + "/machine.scxml", tr("SCXML Files (*.scxml)"));
    if (path.isEmpty()) return;



    // 3. Генерация файла
    bool success = ScxmlCompiler::compileToScxml(scene, path);

    if (success) {
        QMessageBox::information(this, tr("Успех"),
            tr("Код успешно сгенерирован и сохранен в:\n%1").arg(path));

        // --- ИНТЕГРАЦИЯ И ЗАПУСК (ОДИН РАЗ!) ---

        // А. Собираем весь код C++/Python в память исполнителя
        ScxmlCompiler::collectCode(scene, m_codeExecutor);

        // Б. Передаем исполнителя в движок
        // (Важно сделать это ДО загрузки файла, чтобы он попал в setInitialValues)
        m_logicEngine->setExecutor(m_codeExecutor);

        // В. Загружаем файл и запускаем

        if (m_logicEngine->loadFile(path)) {
            m_logicEngine->start();

        } else {
            QMessageBox::warning(this, tr("Ошибка запуска"),
                                 tr("Не удалось загрузить SCXML файл в движок."));
        }
        // ---------------------------------------

    } else {
        QMessageBox::critical(this, tr("Ошибка записи"), tr("Не удалось сохранить файл."));
    }
}
// В файле mainwindow.cpp




void MainWindow::on_startButton_clicked()
{
    // ====================================================================
        // --- ПРЕДВЫЧИСЛИТЕЛЬНАЯ ПРОВЕРКА: ЗАЩИТА ОТ ИСТОРИИ ВНЕ РАМОК ---
        bool hasHistoryError = false;
        for (QGraphicsItem *item : scene->items()) {
            if (HistoryItem *hist = dynamic_cast<HistoryItem*>(item)) {
                bool inGroup = false;
                for (QGraphicsItem *possibleGroup : scene->items()) {
                    if (GroupItem *g = dynamic_cast<GroupItem*>(possibleGroup)) {
                        if (g->sceneBoundingRect().contains(hist->sceneBoundingRect().center())) {
                            inGroup = true; break;
                        }
                    }
                }
                if (!inGroup) {
                    hasHistoryError = true;
                    scene->clearSelection();
                    hist->setSelected(true); // Подсвечиваем ошибочный блок
                    break;
                }
            }
        }
        if (hasHistoryError) {
            QMessageBox::critical(this, tr("Запуск заблокирован"), tr("Ошибка: Найден блок 'История', который находится вне рамки!\n\nПожалуйста, поместите выделенный блок внутрь рамки или удалите его."));
            return;
        }
        // ====================================================================
    // --- ЭТАП 1: ПРОВЕРКА ---
    QList<ValidationError> errors = ScxmlCompiler::validate(scene);
    if (!errors.isEmpty()) {
        QString msg = tr("Запуск невозможен! Ошибки:\n");
        for (const auto &err : errors) {
            msg += tr("- %1\n").arg(err.message);
            if (err.block) err.block->setSelected(true);
        }
        QMessageBox::warning(this, tr("Ошибка"), msg);
        return;
    }

    // --- ЭТАП 2: СИМУЛЯЦИЯ (SCXML) ---
    ScxmlCompiler::collectCode(scene, m_codeExecutor);
    if (m_logicEngine) m_logicEngine->setExecutor(m_codeExecutor);
    QString autoPath = QDir::tempPath() + "/autorun_temp.scxml";

    if (ScxmlCompiler::compileToScxml(scene, autoPath)) {
        if (m_logicEngine && m_logicEngine->loadFile(autoPath)) {
            m_logicEngine->start();            
        }
    } else {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось создать файл симуляции."));
        return;
    }

    // --- ЭТАП 3: ГЕНЕРАЦИЯ КОДА ---
    bool useFolders = QSettings("VisualBuild", "IDE").value("structuredFolders", false).toBool();
    QString defaultDir = m_currentProjectPath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) : m_currentProjectPath;

    // --- УМНАЯ ФУНКЦИЯ ТИХОЙ ГЕНЕРАЦИИ С АВТОРАСШИРЕНИЕМ ---
    QSettings settings("VisualBuild", "IDE");
    auto getSmartPath = [&](const QString& folderKey, const QString& fileKey, const QString& defaultFileName, const QString& filter, const QString& requiredExt) -> QString {
        QString silentFolder = settings.value(folderKey, "").toString();
        QString silentFile = settings.value(fileKey, defaultFileName).toString();
        if (silentFile.isEmpty()) silentFile = defaultFileName; // Защита от пустой строки

        // Принудительно добавляем или исправляем расширение
        QFileInfo fi(silentFile);
        if (fi.suffix().isEmpty()) {
            silentFile += "." + requiredExt;
        } else if (fi.suffix().toLower() != requiredExt.toLower()) {
            silentFile = fi.completeBaseName() + "." + requiredExt;
        }

        if (!silentFolder.isEmpty() && QDir(silentFolder).exists()) {
            return silentFolder + "/" + silentFile; // Тихий режим
        } else {
            return QFileDialog::getSaveFileName(this, tr("Сохранить"), defaultDir + "/" + silentFile, filter);
        }
    };

    // === ВЕТКА PYTHON ===
    if (m_currentLang == Lang_PYTHON)
    {
        QString pyPath = getSmartPath("exportPathPython", "exportFilePython", "robot_script", tr("Python Script (*.py)"), "py");
        if (pyPath.isEmpty()) return;

        QFileInfo fileInfo(pyPath);
        if (useFolders) {
            QDir().mkpath(fileInfo.absolutePath() + "/src");
            pyPath = fileInfo.absolutePath() + "/src/" + fileInfo.fileName();
        }

        PythonTranslator2::GeneratedPyCode code = PythonTranslator2::generate(scene, m_headerCode);
        updateCodePreviewStatus({{"script.py", code.code}}, "Python", code.error.isEmpty());

        if (!code.error.isEmpty()) {
            QMessageBox::warning(this, tr("Ошибка трансляции Python"), code.error);
            return;
        }

        QFile filePy(pyPath);
        if (filePy.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&filePy);
            out << code.code;
            filePy.close();

            if (!Utils::checkPythonSyntax(pyPath)) {
                updateCodePreviewStatus({{"script.py", code.code}}, "Python", false, Utils::getLastErrorLines());
                return;
            }
            updateCodePreviewStatus({{"script.py", code.code}}, "Python", true);

            QMessageBox::information(this, tr("Готово"), tr("Скрипт успешно сохранен:\n%1").arg(pyPath));
        } else {
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось записать .py файл"));
        }
        return;
    }

    // === ВЕТКА ARDUINO / ESP32 / STM32 ===
    else if (m_currentLang == Lang_MICRO)
    {
        QString silentFolder = settings.value("exportPathMicro", "").toString();
        QString silentFile = settings.value("exportFileMicro", "Firmware").toString();
        if (silentFile.isEmpty()) silentFile = "Firmware";
        int exportMode = settings.value("microExportMode", 0).toInt();

        // 1. АВТОМАТИЧЕСКАЯ ПОДМЕНА РАСШИРЕНИЯ ПОД ВЫБРАННЫЙ РЕЖИМ
        QString requiredExt = (exportMode == 0) ? "ino" : "cpp";
        QFileInfo fi(silentFile);
        if (fi.suffix().isEmpty()) {
            silentFile += "." + requiredExt;
        } else if (fi.suffix().toLower() != requiredExt.toLower()) {
            silentFile = fi.completeBaseName() + "." + requiredExt;
        }

        QString savePath;
        QString selectedFilter;
        bool isSilent = (!silentFolder.isEmpty() && QDir(silentFolder).exists());

        if (isSilent) {
            savePath = silentFolder + "/" + silentFile;
        } else {
            QString filter = tr("Arduino Sketch (*.ino);;C++ Firmware (PlatformIO) (*.cpp);;PC Simulation (Windows/Linux) (*.cpp)");
            savePath = QFileDialog::getSaveFileName(this, tr("Сохранить код"), defaultDir + "/" + silentFile, filter, &selectedFilter);
            if (savePath.isEmpty()) return;

            if (selectedFilter.contains("PlatformIO")) exportMode = 1;
            else if (selectedFilter.contains("PC Simulation")) exportMode = 2;
            else exportMode = 0;
        }

        QFileInfo fileInfo(savePath);
        QString baseName = fileInfo.baseName();
        QString headerName = baseName + ".h";
        QString headerPath = fileInfo.path() + "/" + headerName;
        bool splitHeader = (exportMode == 1 || exportMode == 2);

        if (useFolders) {
            if (!splitHeader) {
                QString sketchDir = fileInfo.absolutePath() + "/" + baseName;
                QDir().mkpath(sketchDir);
                savePath = sketchDir + "/" + fileInfo.fileName();
            } else {
                QDir().mkpath(fileInfo.absolutePath() + "/src");
                QDir().mkpath(fileInfo.absolutePath() + "/include");
                savePath = fileInfo.absolutePath() + "/src/" + fileInfo.fileName();
                headerPath = fileInfo.absolutePath() + "/include/" + headerName;
            }
        }

        QString finalCode;
        QString error;
        QString resultMsg;
        QString customHeaderContent;

        if (exportMode == 2)
        {
            MicroDebugResult res = MicroTranslator2::generateDebugSource(scene, splitHeader ? "" : m_headerCode);
            finalCode = res.sourceCode;
            error = res.error;

            if (splitHeader) {
                QString inc = "#include \"" + headerName + "\"\n";
                if (finalCode.contains("class StateMachine")) {
                    finalCode.replace("class StateMachine", inc + "\nclass StateMachine");
                } else {
                    finalCode.prepend(inc);
                }
            }
            resultMsg = tr("Файлы симуляции (Source + Header) созданы!");
        }
        else if (exportMode == 1)
        {
            MicroCode res = MicroTranslator::generate(scene, m_headerCode);
            finalCode = res.source;
            customHeaderContent = res.header;
            error = res.error;

            finalCode.replace("#include \"FsmCore.h\"", "#include \"" + headerName + "\"");
            customHeaderContent.replace("FSM_CORE_H", baseName.toUpper() + "_H");

            resultMsg = tr("Файлы прошивки (Source + Header) созданы!");
        }
        else
        {
            MicroCode res = MicroTranslator::generate(scene, m_headerCode);
            finalCode = res.code;
            error = res.error;
            resultMsg = tr("Скетч Arduino (.ino) создан!");
        }

        if (splitHeader) {
            QString previewHeader = customHeaderContent.isEmpty() ? ("// User Header Code\n" + m_headerCode) : customHeaderContent;
            updateCodePreviewStatus({{headerName, previewHeader}, {baseName + ".cpp", finalCode}}, tr("Микроконтроллер (C++)"), error.isEmpty());
        } else {
            updateCodePreviewStatus({{baseName + ".ino", finalCode}}, tr("Arduino / ESP32"), error.isEmpty());
        }

        if (!error.isEmpty()) {
            QMessageBox::warning(this, tr("Ошибка генерации"), error);
            return;
        }

        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << finalCode;
            file.close();
        } else {
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось записать файл: %1").arg(savePath));
            return;
        }

        if (splitHeader) {
            QFile fileH(headerPath);
            if (fileH.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream outH(&fileH);
                if (!customHeaderContent.isEmpty()) {
                    outH << customHeaderContent;
                } else {
                    QString guard = baseName.toUpper() + "_H";
                    outH << "#ifndef " << guard << "\n#define " << guard << "\n\n// User Header Code\n" << m_headerCode << "\n\n#endif // " << guard;
                }
                fileH.close();
            } else {
                 QMessageBox::warning(this, tr("Внимание"), tr("Не удалось создать .h файл!"));
            }
        }

        QMessageBox::information(this, tr("Успех"), tr("%1\n\nФайл: %2").arg(resultMsg).arg(savePath));
        return;
    }

    // === ВЕТКА JAVA ===
    else if (m_currentLang == Lang_JAVA)
    {
        QString savePath = getSmartPath("exportPathJava", "exportFileJava", "RobotApp", tr("Java Class (*.java)"), "java");
        if (savePath.isEmpty()) return;

        QFileInfo fileInfo(savePath);
        QString className = fileInfo.baseName();

        if (useFolders) {
            QString javaDir = fileInfo.absolutePath() + "/src/main/java";
            QDir().mkpath(javaDir);
            savePath = javaDir + "/" + fileInfo.fileName();
        }

        GeneratedJavaCode result = JavaTranslator::generateClean(scene, className, m_headerCode);

        if (!result.error.isEmpty()) {
            updateCodePreviewStatus({{className + ".java", result.javaCode}}, "Java", false);
            QMessageBox::warning(this, tr("Ошибка генерации Java"), result.error);
            return;
        }

        ui->statusbar->showMessage(tr("Проверка синтаксиса Java..."), 2000);
        QApplication::processEvents();

        bool isOk = Utils::checkJavaSyntax(result.javaCode, className, this);
        updateCodePreviewStatus({{className + ".java", result.javaCode}}, "Java", isOk, Utils::getLastErrorLines());

        if (!isOk) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Найдены ошибки"),
                tr("В сгенерированном Java-коде есть синтаксические ошибки (см. красную строку в предпросмотре).\n\nВсё равно сохранить файл?"),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::No) {
                ui->statusbar->showMessage(tr("Сохранение отменено"), 2000);
                return;
            }
        }

        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << result.javaCode;
            file.close();

            if (isOk) {
                QMessageBox::information(this, tr("Успех"), tr("Java класс создан!\nИмя класса: %1\nФайл: %2").arg(className).arg(savePath));
            } else {
                QMessageBox::warning(this, tr("Сохранено с предупреждениями"), tr("Файл сохранен, но содержит синтаксические ошибки.\nОткройте его в IDE, чтобы исправить."));
            }
        } else {
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось записать файл."));
        }
        return;
    }

    // === ВЕТКА C++ ===
    else
    {
        QString cppPath = getSmartPath("exportPathCpp", "exportFileCpp", "StateMachine", tr("C++ Source (*.cpp)"), "cpp");
        if (cppPath.isEmpty()) return;

        QFileInfo fileInfo(cppPath);
        QString baseDir = fileInfo.absolutePath();
        QString headerFileName = fileInfo.baseName() + ".h";
        QString headerPath = baseDir + "/" + headerFileName;

        if (useFolders) {
            QDir().mkpath(baseDir + "/src");
            QDir().mkpath(baseDir + "/include");
            cppPath = baseDir + "/src/" + fileInfo.fileName();
            headerPath = baseDir + "/include/" + headerFileName;
        }

        CppTranslator translator;
        GeneratedCode code = translator.generateCodePair(scene, m_headerCode, headerFileName);

        if (!code.error.isEmpty()) {
            updateCodePreviewStatus({{headerFileName, code.headerContent}, {fileInfo.baseName() + ".cpp", code.sourceContent}}, "C++ (MinGW)", false);
            QMessageBox::warning(this, tr("Ошибка трансляции"), code.error);
            return;
        }

        ui->statusbar->showMessage(tr("Проверка синтаксиса C++..."), 2000);
        QApplication::processEvents();

        bool isOk = Utils::checkCppSyntax(code.sourceContent, code.headerContent, headerFileName, this);
        updateCodePreviewStatus({{headerFileName, code.headerContent}, {fileInfo.baseName() + ".cpp", code.sourceContent}}, "C++ (MinGW)", isOk, Utils::getLastErrorLines());

        if (!isOk) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Найдены ошибки"),
                tr("В сгенерированном C++ коде есть ошибки синтаксиса (см. предпросмотр).\n\nВсё равно сохранить файлы?"),
                QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::No) {
                ui->statusbar->showMessage(tr("Сохранение отменено"), 2000);
                return;
            }
        }

        QFile fileCpp(cppPath);
        if (fileCpp.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&fileCpp);
            out << code.sourceContent;
            fileCpp.close();
        } else {
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось записать .cpp файл"));
            return;
        }

        QFile fileH(headerPath);
        if (fileH.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream outH(&fileH);
            outH << code.headerContent;
            fileH.close();
        } else {
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось записать .h файл"));
            return;
        }

        if (isOk) {
            QMessageBox::information(this, tr("Готово"), tr("Сгенерированы файлы:\n%1\n%2").arg(cppPath).arg(headerPath));
        } else {
            QMessageBox::warning(this, tr("Сохранено с предупреждениями"), tr("Файлы сохранены, но содержат синтаксические ошибки.\nОткройте их в вашей IDE (например, VS Code) для исправления."));
        }
    }
}

// 2. ОБНОВЛЕННАЯ кнопка Translate
void MainWindow::on_translateButton_clicked()
{
    // ====================================================================
        // --- ПРЕДВЫЧИСЛИТЕЛЬНАЯ ПРОВЕРКА: ЗАЩИТА ОТ ИСТОРИИ ВНЕ РАМОК ---
        bool hasHistoryError = false;
        for (QGraphicsItem *item : scene->items()) {
            if (HistoryItem *hist = dynamic_cast<HistoryItem*>(item)) {
                bool inGroup = false;
                for (QGraphicsItem *possibleGroup : scene->items()) {
                    if (GroupItem *g = dynamic_cast<GroupItem*>(possibleGroup)) {
                        if (g->sceneBoundingRect().contains(hist->sceneBoundingRect().center())) {
                            inGroup = true; break;
                        }
                    }
                }
                if (!inGroup) {
                    hasHistoryError = true;
                    scene->clearSelection();
                    hist->setSelected(true); // Подсвечиваем ошибочный блок
                    break;
                }
            }
        }
        if (hasHistoryError) {
            QMessageBox::critical(this, tr("Генерация заблокирована"), tr("Ошибка: Найден блок 'История', который находится вне рамки!\n\nПожалуйста, поместите выделенный блок внутрь рамки или удалите его."));
            return;
        }
        // ====================================================================
    QSettings settings("VisualBuild", "IDE");
    bool useFolders = settings.value("structuredFolders", false).toBool();
    QString defaultDir = m_currentProjectPath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) : m_currentProjectPath;

    // --- УМНАЯ ФУНКЦИЯ ТИХОЙ ГЕНЕРАЦИИ С АВТОРАСШИРЕНИЕМ ---
    auto getSmartPath = [&](const QString& folderKey, const QString& fileKey, const QString& defaultFileName, const QString& filter, const QString& requiredExt) -> QString {
        QString silentFolder = settings.value(folderKey, "").toString();
        QString silentFile = settings.value(fileKey, defaultFileName).toString();
        if (silentFile.isEmpty()) silentFile = defaultFileName; // Защита от пустой строки

        // Принудительно добавляем или исправляем расширение
        QFileInfo fi(silentFile);
        if (fi.suffix().isEmpty()) {
            silentFile += "." + requiredExt;
        } else if (fi.suffix().toLower() != requiredExt.toLower()) {
            silentFile = fi.completeBaseName() + "." + requiredExt;
        }

        if (!silentFolder.isEmpty() && QDir(silentFolder).exists()) {
            return silentFolder + "/" + silentFile; // Тихий режим с кастомным именем файла
        } else {
            return QFileDialog::getSaveFileName(this, tr("Сохранить"), defaultDir + "/" + silentFile, filter); // Обычный режим
        }
    };

    // === ВЕТКА PYTHON ===
    if (m_currentLang == Lang_PYTHON)
    {
        QString pyPath = getSmartPath("exportPathPython", "exportFilePython", "robot_script", tr("Python Script (*.py)"), "py");

        if (pyPath.isEmpty()) return;

        QFileInfo fileInfo(pyPath);
        if (useFolders) {
            QDir().mkpath(fileInfo.absolutePath() + "/src");
            pyPath = fileInfo.absolutePath() + "/src/" + fileInfo.fileName();
        }
        showLoading(tr("Генерация Python кода..."));
        PythonTranslator2::GeneratedPyCode code = PythonTranslator2::generate(scene, m_headerCode);

        if (!code.error.isEmpty()) {
            updateCodePreviewStatus({{"script.py", code.code}}, "Python", false);
            hideLoading();
            QMessageBox::warning(this, tr("Ошибка трансляции"), code.error);
            return;
        }

        QFile filePy(pyPath);
        if (filePy.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&filePy);
            out << code.code;
            filePy.close();

            if (!Utils::checkPythonSyntax(pyPath)) {
                updateCodePreviewStatus({{"script.py", code.code}}, "Python", false, Utils::getLastErrorLines());
                hideLoading();
                return;
            }
            updateCodePreviewStatus({{"script.py", code.code}}, "Python", true);
            hideLoading();
            QMessageBox::information(this, tr("Успех"), tr("Сгенерирован скрипт Python:\n%1").arg(pyPath));
        } else {
            hideLoading();
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось записать .py файл"));
        }
    }
    // === ВЕТКА ARDUINO / ESP32 ===
    else if (m_currentLang == Lang_MICRO)
    {
        QString silentFolder = settings.value("exportPathMicro", "").toString();
        QString silentFile = settings.value("exportFileMicro", "Firmware").toString();
        if (silentFile.isEmpty()) silentFile = "Firmware";
        int exportMode = settings.value("microExportMode", 0).toInt();

        // 1. АВТОМАТИЧЕСКАЯ ПОДМЕНА РАСШИРЕНИЯ ПОД ВЫБРАННЫЙ ФОРМАТ
        QString requiredExt = (exportMode == 0) ? "ino" : "cpp";
        QFileInfo fi(silentFile);
        if (fi.suffix().isEmpty()) {
            silentFile += "." + requiredExt;
        } else if (fi.suffix().toLower() != requiredExt.toLower()) {
            silentFile = fi.completeBaseName() + "." + requiredExt;
        }

        QString savePath;
        bool isSilent = (!silentFolder.isEmpty() && QDir(silentFolder).exists());

        if (isSilent) {
            savePath = silentFolder + "/" + silentFile;
        } else {
            QString filter = tr("Arduino Sketch (*.ino);;C++ Firmware (PlatformIO) (*.cpp);;PC Simulation (Windows/Linux) (*.cpp)");
            QString selectedFilter;
            savePath = QFileDialog::getSaveFileName(this, tr("Сохранить код"), defaultDir + "/" + silentFile, filter, &selectedFilter);
            if (savePath.isEmpty()) return;

            if (selectedFilter.contains("PlatformIO")) exportMode = 1;
            else if (selectedFilter.contains("PC Simulation")) exportMode = 2;
            else exportMode = 0;
        }

        QFileInfo fileInfo(savePath);
        QString baseName = fileInfo.baseName();
        QString headerName = baseName + ".h";
        QString headerPath = fileInfo.absolutePath() + "/" + headerName;
        bool splitHeader = (exportMode == 1 || exportMode == 2);

        if (useFolders) {
            if (!splitHeader) {
                QString sketchDir = fileInfo.absolutePath() + "/" + baseName;
                QDir().mkpath(sketchDir);
                savePath = sketchDir + "/" + fileInfo.fileName();
            } else {
                QDir().mkpath(fileInfo.absolutePath() + "/src");
                QDir().mkpath(fileInfo.absolutePath() + "/include");
                savePath = fileInfo.absolutePath() + "/src/" + fileInfo.fileName();
                headerPath = fileInfo.absolutePath() + "/include/" + headerName;
            }
        }

        showLoading(tr("Генерация прошивки Arduino..."));

        QString finalCode, error, resultMsg, customHeaderContent;

        if (exportMode == 2) {
            MicroDebugResult res = MicroTranslator2::generateDebugSource(scene, splitHeader ? "" : m_headerCode);
            finalCode = res.sourceCode;
            error = res.error;
            if (splitHeader) {
                QString inc = "#include \"" + headerName + "\"\n";
                if (finalCode.contains("class StateMachine")) {
                    finalCode.replace("class StateMachine", inc + "\nclass StateMachine");
                } else {
                    finalCode.prepend(inc);
                }
            }
            resultMsg = tr("Файлы симуляции (Source + Header) созданы!");
        } else if (exportMode == 1) {
            MicroCode res = MicroTranslator::generate(scene, m_headerCode);
            finalCode = res.source;
            customHeaderContent = res.header;
            error = res.error;
            finalCode.replace("#include \"FsmCore.h\"", "#include \"" + headerName + "\"");
            customHeaderContent.replace("FSM_CORE_H", baseName.toUpper() + "_H");
            resultMsg = tr("Файлы прошивки (Source + Header) созданы!");
        } else {
            MicroCode res = MicroTranslator::generate(scene, m_headerCode);
            finalCode = res.code;
            error = res.error;
            resultMsg = tr("Скетч Arduino (.ino) создан!");
        }

        if (splitHeader) {
            QString previewHeader = customHeaderContent.isEmpty() ? ("// User Header Code\n" + m_headerCode) : customHeaderContent;
            updateCodePreviewStatus({{headerName, previewHeader}, {baseName + ".cpp", finalCode}}, tr("Микроконтроллер (C++)"), error.isEmpty());
        } else {
            updateCodePreviewStatus({{baseName + ".ino", finalCode}}, tr("Arduino / ESP32"), error.isEmpty());
        }

        if (!error.isEmpty()) {
            hideLoading();
            QMessageBox::warning(this, tr("Ошибка генерации Arduino"), error);
            return;
        }

        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file); out << finalCode; file.close();
        } else {
            hideLoading();
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось записать файл."));
            return;
        }

        if (splitHeader) {
            QFile fileH(headerPath);
            if (fileH.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream outH(&fileH);
                if (!customHeaderContent.isEmpty()) {
                    outH << customHeaderContent;
                } else {
                    QString guard = baseName.toUpper() + "_H";
                    outH << "#ifndef " << guard << "\n#define " << guard << "\n\n// User Header Code\n" << m_headerCode << "\n\n#endif // " << guard;
                }
                fileH.close();
            } else {
                hideLoading();
                QMessageBox::warning(this, tr("Внимание"), tr("Не удалось создать .h файл!"));
                return;
            }
        }

        hideLoading();
        QMessageBox::information(this, tr("Успех"), tr("%1\nМожно открывать в IDE:\n%2").arg(resultMsg).arg(savePath));
    }
    // === ВЕТКА JAVA ===
    else if (m_currentLang == Lang_JAVA)
    {
        QString savePath = getSmartPath("exportPathJava", "exportFileJava", "RobotApp", tr("Java Class (*.java)"), "java");

        if (savePath.isEmpty()) return;

        QFileInfo fileInfo(savePath);
        QString className = fileInfo.baseName();

        if (useFolders) {
            QString javaDir = fileInfo.absolutePath() + "/src/main/java";
            QDir().mkpath(javaDir);
            savePath = javaDir + "/" + fileInfo.fileName();
        }
        showLoading(tr("Генерация Java класса..."));
        GeneratedJavaCode result = JavaTranslator::generateClean(scene, className, m_headerCode);

        if (!result.error.isEmpty()) {
            updateCodePreviewStatus({{className + ".java", result.javaCode}}, "Java", false);
            hideLoading();
            QMessageBox::warning(this, tr("Ошибка генерации Java"), result.error);
            return;
        }

        ui->statusbar->showMessage(tr("Проверка синтаксиса Java..."), 2000);
        QApplication::processEvents();

        bool isOk = Utils::checkJavaSyntax(result.javaCode, className, this);
        updateCodePreviewStatus({{className + ".java", result.javaCode}}, "Java", isOk, Utils::getLastErrorLines());

        if (!isOk) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Найдены ошибки"),
                tr("В сгенерированном Java-коде есть синтаксические ошибки.\n\nВсё равно сохранить файл?"), QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No) {
                ui->statusbar->showMessage(tr("Сохранение отменено"), 2000);
                hideLoading();
                return;
            }
        }

        QFile file(savePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file); out << result.javaCode; file.close();
            hideLoading();
            if (isOk) QMessageBox::information(this, tr("Успех"), tr("Java класс успешно создан!"));
            else QMessageBox::warning(this, tr("Внимание"), tr("Файл сохранен, но содержит синтаксические ошибки."));
        } else {
            hideLoading();
            QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось записать файл."));
        }
    }
    // === ВЕТКА C++ (По умолчанию) ===
    else
    {
        QString cppPath = getSmartPath("exportPathCpp", "exportFileCpp", "StateMachine", tr("C++ Source (*.cpp)"), "cpp");

        if (cppPath.isEmpty()) return;

        QFileInfo fileInfo(cppPath);
        QString baseDir = fileInfo.absolutePath();
        QString headerFileName = fileInfo.baseName() + ".h";
        QString headerPath = baseDir + "/" + headerFileName;

        if (useFolders) {
            QDir().mkpath(baseDir + "/src");
            QDir().mkpath(baseDir + "/include");
            cppPath = baseDir + "/src/" + fileInfo.fileName();
            headerPath = baseDir + "/include/" + headerFileName;
        }
        showLoading(tr("Генерация C++ файлов..."));
        CppTranslator translator;
        GeneratedCode code = translator.generateCodePair(scene, m_headerCode, headerFileName);

        if (!code.error.isEmpty()) {
            updateCodePreviewStatus({{headerFileName, code.headerContent}, {fileInfo.baseName() + ".cpp", code.sourceContent}}, "C++", false);
            hideLoading();
            QMessageBox::warning(this, tr("Ошибка трансляции"), code.error);
            return;
        }

        ui->statusbar->showMessage(tr("Проверка синтаксиса C++..."), 2000);
        QApplication::processEvents();

        bool isOk = Utils::checkCppSyntax(code.sourceContent, code.headerContent, headerFileName, this);
        updateCodePreviewStatus({{headerFileName, code.headerContent}, {fileInfo.baseName() + ".cpp", code.sourceContent}}, "C++ (MinGW)", isOk, Utils::getLastErrorLines());

        if (!isOk) {
            QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Найдены ошибки"),
                tr("В сгенерированном C++ коде есть ошибки синтаксиса.\n\nВсё равно сохранить файлы?"), QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::No) {
                ui->statusbar->showMessage(tr("Сохранение отменено"), 2000);
                hideLoading();
                return;
            }
        }

        QFile fileCpp(cppPath);
        if (fileCpp.open(QIODevice::WriteOnly | QIODevice::Text)) { QTextStream out(&fileCpp); out << code.sourceContent; fileCpp.close(); }

        QFile fileH(headerPath);
        if (fileH.open(QIODevice::WriteOnly | QIODevice::Text)) { QTextStream out(&fileH); out << code.headerContent; fileH.close(); }

        hideLoading();
        if (isOk) QMessageBox::information(this, tr("Готово"), tr("Сгенерированы файлы:\n%1\n%2").arg(cppPath).arg(headerPath));
        else QMessageBox::warning(this, tr("Сохранено с предупреждениями"), tr("Файлы сохранены, но содержат синтаксические ошибки.\nОткройте их в вашей IDE для исправления."));
    }
}

void MainWindow::on_headerButton_clicked()
{
    // Открываем диалог с тем текстом, который мы сохранили ранее
    HeaderDialog dlg(m_headerCode, this);

    // ===================================================================
    // --- НОВАЯ ЛОГИКА: МНИМЫЙ ТЕКСТ (ПОДСКАЗКА) ---
    // ===================================================================
    if (m_currentLang == Lang_MICRO) {
        dlg.setPlaceholderText(tr(
            "// --- PC SIMULATOR WARNING ---\n"
            "// If you plan to use the visual PC Emulator, do NOT include\n"
            "// hardware libraries (like <Servo.h> or <FastLED.h>) here!\n"
            "// The PC C++ compiler cannot compile Arduino hardware code.\n"
            "// \n"
            "// Note: For internal simulation, basic C++ libraries (<string>,\n"
            "// <cmath>, <vector>, etc.) and a 'Serial' mock are already built-in.\n"
            "// \n"
            "// You CAN include hardware libraries here if you only want to\n"
            "// generate a real .ino sketch. However, you MUST remove them\n"
            "// before turning on the PC Simulation!\n"
            "// ---------------------------------------------------------"
        ));
    } else {
        dlg.setPlaceholderText(tr("// Put your #include, #define, and global variables here..."));
    }
    // ===================================================================

    if (dlg.exec() == QDialog::Accepted) {
        // Если нажали OK — сохраняем текст в переменную
        m_headerCode = dlg.getCode();

        // Визуальное подтверждение (опционально, можно убрать)
        if (!m_headerCode.trimmed().isEmpty()) {
            ui->headerButton->setStyleSheet("font-weight: bold; border: 2px solid green;");
        } else {
            ui->headerButton->setStyleSheet("");
        }
    }
}
// --- СОХРАНЕНИЕ ---


void MainWindow::deleteSelectedItems()
{
    if (!scene) return;
    QList<QGraphicsItem *> selected = scene->selectedItems();
    if (selected.isEmpty()) return;

    // ====================================================================
    // ====================================================================
        // --- ПРЕДВАРИТЕЛЬНАЯ ПРОВЕРКА НА ПРИВЯЗАННЫЕ СТИКЕРЫ (БЕЗОПАСНОСТЬ) ---
        for (QGraphicsItem *item : selected) {

            // 1. Если пытаются удалить сам СТИКЕР (нажав Delete)
            if (CommentItem *comment = dynamic_cast<CommentItem*>(item)) {
                if (comment->linkedItem()) {
                    QMessageBox::warning(this, tr("Удаление заблокировано"),
                        tr("Этот стикер привязан к объекту!\n\nСначала отвяжите его через меню (✂ Отвязать)."));
                    return;
                }
            }
            // 2. Если пытаются удалить ОБЪЕКТ, к которому привязан стикер
            else {
                for (QGraphicsItem *sceneItem : scene->items()) {
                    if (CommentItem *comment = dynamic_cast<CommentItem*>(sceneItem)) {
                        QGraphicsItem* linked = comment->linkedItem();

                        // ПРОВЕРКА 2.0: Сравниваем не только сам элемент, но и его главную оболочку
                        if (linked && (linked == item || linked == item->topLevelItem() || linked->topLevelItem() == item->topLevelItem())) {
                            QMessageBox::warning(this, tr("Удаление заблокировано"),
                                tr("К этому объекту привязан стикер-комментарий!\n\nСначала отвяжите или удалите сам стикер."));
                            return; // Отменяем удаление
                        }
                    }
                }
            }
        }
        // ====================================================================

    // ШАГ 1: Сначала удаляем только стрелки (ConnectionItem)
    // Это защитит от краша, если блок удалится раньше привязанной к нему стрелки
    foreach (QGraphicsItem *item, selected) {
        if (dynamic_cast<ConnectionItem*>(item)) {
            scene->removeItem(item);
            delete item;
        }
    }

    // Обновляем список выделенных элементов (так как удаленные стрелки из него исчезли)
    selected = scene->selectedItems();

    // ШАГ 2: Теперь безопасно удаляем всё остальное (блоки, стикеры, мосты и рамки)
    foreach (QGraphicsItem *item, selected) {
        scene->removeItem(item);
        delete item;
    }

    // ВАЖНО: Делаем снимок для Ctrl+Z
    if (m_graphManager) m_graphManager->createSnapshot();
}
void MainWindow::on_actionSave_triggered()
{
    // Открываем диалог сохранения
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Сохранить граф"),
                                                    "",
                                                    tr("JSON Project (*.json);;All Files (*)"));

    if (fileName.isEmpty()) return; // Пользователь нажал "Отмена"

    if (m_graphManager->saveToFile(fileName, m_headerCode, m_currentLang, serializeSectors())) {
        ui->statusbar->showMessage(tr("Проект успешно сохранен!"), 3000);
    } else {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить файл!"));
    }
}

// --- ЗАГРУЗКА ---
void MainWindow::on_actionOpen_triggered()
{
    // Спрашиваем подтверждение, если есть несохраненные изменения (опционально)

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Открыть граф"), // Изменил заголовок для ясности
                                                    "",
                                                    tr("JSON Project (*.json);;All Files (*)"));

    if (fileName.isEmpty()) return;

    // --- НОВОЕ 1: Узнаем, куда сейчас смотрит камера ---
    QPointF viewCenter = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect().center());

    int loadedLang = 0;
    QJsonArray loadedSectors;

    // --- НОВОЕ 2: Передаем viewCenter и флаг useDropPos = true ---
    if (m_graphManager->loadFromFile(fileName, m_headerCode, loadedLang, loadedSectors, viewCenter, true)) {

        // ВАЖНО: Мы удалили вызов deserializeSectors(loadedSectors);
        // Граф загружается на текущую сцену, не ломая существующие вкладки (сектора) внизу экрана!

        applyProjectLanguage(loadedLang);
        ui->statusbar->showMessage(tr("Граф открыт."), 3000);

        // --- ОБНОВЛЯЕМ ПОДСВЕТКУ КНОПКИ HEADER ---
        if (!m_headerCode.trimmed().isEmpty()) {
            ui->headerButton->setStyleSheet("font-weight: bold; border: 2px solid green;");
        } else {
            ui->headerButton->setStyleSheet("");
        }

        // 1. Подключаем сетку к новым блокам
        if (grid) {
            foreach (QGraphicsItem *item, scene->items()) {
                if (DiagramItem *d = dynamic_cast<DiagramItem*>(item)) {
                    d->setGrid(grid);
                }
            }
            grid->updateSceneRect(); // Теперь это безопасно
        }


        // Если таблица создана, заставляем её перечитать сцену
        if (m_stateTable) {
            m_stateTable->updateFromScene(scene);

            // Если таблица открыта, она обновится визуально.
            // Если закрыта — обновится при следующем открытии.
        }

        // Также полезно обновить сетку под новые размеры
        if (grid) grid->updateSceneRect();

    } else {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось загрузить файл (возможно, он поврежден)."));
    }
}

// --- UNDO / REDO ---
void MainWindow::on_actionUndo_triggered()
{
    m_graphManager->undo();
    ui->statusbar->showMessage(tr("Отмена действия"), 1000);
}

void MainWindow::on_actionRedo_triggered()
{
    m_graphManager->redo();
    ui->statusbar->showMessage(tr("Повтор действия"), 1000);
}
void MainWindow::on_debugButton_clicked()
{
    // 1. Проверяем, есть ли сцена
    if (!scene) return;
    // ========================================================================
        // --- ПРЕДПУСКОВОЙ ФИЛЬТР ОТЛАДКИ (SCOPED DEBUGGING) ---
        // ========================================================================
        QDialog filterDlg(this);
        filterDlg.setWindowTitle(tr("Настройка отладки (Фильтр)"));
        filterDlg.setMinimumWidth(350);
        QVBoxLayout *layout = new QVBoxLayout(&filterDlg);
        layout->addWidget(new QLabel(tr("Выберите зоны для отладки:\n(Невыбранные зоны будут работать, но не будут отправлять логи)"), &filterDlg));

        QListWidget *listWidget = new QListWidget(&filterDlg);

        // Добавляем Сектора
        for (int i = 0; i < m_sectorTabBar->count(); ++i) {
            QListWidgetItem *item = new QListWidgetItem(tr("📍 Сектор: %1").arg(m_sectorTabBar->tabText(i)), listWidget);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(Qt::Checked); // По умолчанию включено
            item->setData(Qt::UserRole, "SECTOR_" + m_sectorTabBar->tabText(i));
        }

        // Добавляем Группы
        foreach (QGraphicsItem *item, scene->items()) {
            if (GroupItem *g = dynamic_cast<GroupItem*>(item)) {
                QListWidgetItem *listItem = new QListWidgetItem(tr("📦 Группа: %1").arg(g->name()), listWidget);
                listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
                listItem->setCheckState(Qt::Checked); // По умолчанию включено
                listItem->setData(Qt::UserRole, "GROUP_" + g->name());
            }
        }
        layout->addWidget(listWidget);

        QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &filterDlg);
        layout->addWidget(btnBox);
        connect(btnBox, &QDialogButtonBox::accepted, &filterDlg, &QDialog::accept);
        connect(btnBox, &QDialogButtonBox::rejected, &filterDlg, &QDialog::reject);

        // Показываем окно. Если юзер нажал Cancel - прерываем запуск!
        if (filterDlg.exec() != QDialog::Accepted) return;

        // Собираем то, что выбрал юзер
        QStringList activeSectors;
        QStringList activeGroups;
        for (int i = 0; i < listWidget->count(); ++i) {
            QListWidgetItem *item = listWidget->item(i);
            if (item->checkState() == Qt::Checked) {
                QString data = item->data(Qt::UserRole).toString();
                if (data.startsWith("SECTOR_")) activeSectors.append(data.mid(7));
                if (data.startsWith("GROUP_"))  activeGroups.append(data.mid(6));
            }
        }

        // Применяем математику: раздаем метки блокам на холсте!
        applyDebugFilters(activeSectors, activeGroups);
        // ========================================================================
    // ========================================================================
        // ВЕТКА 1: ДИНАМИЧЕСКИЙ РЕЖИМ (РАБОТА С ЖЕЛЕЗОМ)
        // ========================================================================
        if (m_currentDebugMode == Mode_DYNAMIC)
        {
            // --- ЗАЩИТА ОТ ДУРАКА ---
            if (m_currentLang == Lang_PYTHON) {
                QMessageBox::warning(this, tr("Несовместимость"), tr("Python не работает на МК...")); return;
            }
            if (m_currentLang == Lang_JAVA) {
                QMessageBox::warning(this, tr("Несовместимость"), tr("Java не работает на МК...")); return;
            }
            if (m_currentLang == Lang_CPP) {
                QMessageBox::information(this, tr("Режим ПК"), tr("Выберите режим 'Arduino / ESP32'...")); return;
            }
            if (m_currentLang != Lang_MICRO) {
                QMessageBox::warning(this, tr("Ошибка"), tr("Выберите режим 'Arduino / ESP32'!")); return;
            }

            // =========================================================
            // ГЛАВНАЯ РАЗВИЛКА: ВЫБОР МЕТОДА СВЯЗИ
            // =========================================================
            QStringList methods;
            methods << tr("USB (Serial Cable)") << tr("Wi-Fi (TCP/IP)");

            bool ok;
            QString method = QInputDialog::getItem(this, tr("Метод подключения"),
                                                   tr("Как плата подключена к ПК?"),
                                                   methods, 0, false, &ok);
            if (!ok || method.isEmpty()) return;

            // ####################################################################
            // ВЕТКА 1: USB
            // ####################################################################
            if (method.contains("USB") || method.contains("Serial"))
            {
                QStringList ports = HardwareListener::getAvailablePorts();
                if (ports.isEmpty()) {
                    QMessageBox::warning(this, tr("Нет устройств"), tr("Нет COM-портов.")); return;
                }
                QString portName = QInputDialog::getItem(this, tr("Выбор порта"), tr("Выберите плату:"), ports, 0, false, &ok);
                if (!ok || portName.isEmpty()) return;

                QStringList formats;
                formats << tr("Простой скетч (Один файл .ino)") << tr("Профессиональный класс (Модули .h + .cpp)");
                QString selectedFormat = QInputDialog::getItem(this, tr("Формат кода"), tr("Как сохранить проект?"), formats, 0, false, &ok);
                if (!ok) return;

                QMap<int, QGraphicsItem*> resultMap;
                bool skipGeneration = false;

                // Спрашиваем куда сохранить
                QString saveTitle = selectedFormat.contains(".ino") ? tr("Сохранить прошивку") : tr("Сохранить проект");
                QString defaultName = selectedFormat.contains(".ino") ? "DebugFirmware.ino" : "TrafficLight.ino";
                QString inoPath = QFileDialog::getSaveFileName(this, saveTitle, defaultName, "*.ino");

                // --- НОВАЯ ЛОГИКА ГОРЯЧЕГО ПОДКЛЮЧЕНИЯ ---
                if (inoPath.isEmpty()) {
                    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Горячее подключение"),
                        tr("Отменить генерацию кода и просто подключиться к текущей прошивке на контроллере?"),
                        QMessageBox::Yes | QMessageBox::No);

                    if (reply == QMessageBox::Yes) {
                        skipGeneration = true;
                    } else {
                        return;
                    }
                }

                if (!skipGeneration) {
                    if (selectedFormat.contains(".ino") && !selectedFormat.contains(".cpp"))
                    {
                        DynamicResult res = DynamicMicroTranslator::generate(scene, m_headerCode);
                        updateCodePreviewStatus({{"DebugFirmware.ino", res.code}}, tr("USB Дебаггер"), res.error.isEmpty());
                        if (!res.error.isEmpty()) { QMessageBox::warning(this, tr("Ошибка генерации"), res.error); return; }
                        resultMap = res.idMap;

                        QFile file(inoPath);
                        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) { QTextStream out(&file); out << res.code; file.close(); }
                    }
                    else
                    {
                        QFileInfo fileInfo(inoPath);
                        ClassResult res = DynamicClassTranslator::generate(scene, fileInfo.baseName(), m_headerCode);
                        updateCodePreviewStatus({
                                                    {fileInfo.baseName() + ".h", res.headerCode},
                                                    {fileInfo.baseName() + ".cpp", res.sourceCode},
                                                    {fileInfo.baseName() + ".ino", res.mainCode}
                                                }, tr("USB Дебаггер (C++)"), res.error.isEmpty());
                        if (!res.error.isEmpty()) { QMessageBox::warning(this, tr("Ошибка генерации"), res.error); return; }
                        resultMap = res.idMap;

                        QFile fIno(inoPath); if(fIno.open(QIODevice::WriteOnly)) { QTextStream(&fIno) << res.mainCode; fIno.close(); }
                        QFile fH(fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".h"); if(fH.open(QIODevice::WriteOnly)) { QTextStream(&fH) << res.headerCode; fH.close(); }
                        QFile fCpp(fileInfo.absolutePath() + "/" + fileInfo.baseName() + ".cpp"); if(fCpp.open(QIODevice::WriteOnly)) { QTextStream(&fCpp) << res.sourceCode; fCpp.close(); }
                    }
                    QMessageBox::information(this, tr("Этап 1: Прошивка"), tr("Загрузите скетч через USB. НЕ открывайте Serial Monitor."));
                } else {
                    // СОБИРАЕМ КАРТУ ВРУЧНУЮ ДЛЯ ГОРЯЧЕГО ПОДКЛЮЧЕНИЯ
                    for (QGraphicsItem *item : scene->items()) {
                        if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(item)) {
                            resultMap.insert(dItem->id(), dItem);
                        } else if (ConnectionItem *cItem = dynamic_cast<ConnectionItem*>(item)) {
                            resultMap.insert(cItem->id().toInt(), cItem);
                        }
                    }
                }

                // --- ЗАПУСК ДЕБАГГЕРА ---
                DebuggerWindow *debugWin = new DebuggerWindow(this);
                // --- ПЕРЕДАЕМ СЕКТОРА В ДЕБАГГЕР ---
                    QStringList sectorNames;
                    for(int i = 0; i < m_sectorTabBar->count(); ++i) {
                        sectorNames << m_sectorTabBar->tabText(i);
                    }
                    debugWin->setSectors(sectorNames, m_sectorCenters);
                debugWin->setAttribute(Qt::WA_DeleteOnClose);
                debugWin->setDebugScene(scene);
                debugWin->setItemMap(resultMap);
                debugWin->setHardwareListener(m_hwListener, portName);

                connect(m_hwListener, &HardwareListener::onBlockEntered, debugWin, &DebuggerWindow::externalSetCurrentBlock);
                connect(m_hwListener, &HardwareListener::onTransitionTaken, debugWin, [debugWin](int id){ debugWin->externalHighlightArrow(id, true); });
                connect(m_hwListener, &HardwareListener::onTransitionFalse, debugWin, [debugWin](int id){ debugWin->externalHighlightArrow(id, false); });
                connect(m_hwListener, &HardwareListener::onLogMessage, debugWin, [debugWin](QString msg){ debugWin->appendLog(tr("[USB] %1").arg(msg)); });

                connect(debugWin, &QDialog::finished, this, &MainWindow::clearDebugVisuals);
                connect(debugWin, &QDialog::finished, m_hwListener, &HardwareListener::disconnectPort);

                if (m_hwListener->connectPort(portName, 115200)) {
                    debugWin->show();
                    debugWin->appendLog(tr("--- Подключено к %1 ---").arg(portName));
                } else {
                    delete debugWin;
                }
            }
            // ####################################################################
            // ВЕТКА 2: WI-FI (TCP/IP)
            // ####################################################################
            else
            {
                QString ssid = QInputDialog::getText(this, tr("Настройки Wi-Fi"), tr("Введите имя сети (SSID):\n(К которой подключен ваш компьютер)"));
                if (ssid.isEmpty()) return;

                QString pass = QInputDialog::getText(this, tr("Настройки Wi-Fi"), tr("Введите пароль:\n(Оставьте пустым, если сеть открытая/без пароля)"), QLineEdit::Password);

                QStringList formats;
                formats << tr("Простой скетч (Один файл .ino)") << tr("Профессиональный класс (Модули .h + .cpp)");
                QString selectedFormat = QInputDialog::getItem(this, tr("Формат кода"), tr("Как сохранить проект?"), formats, 0, false, &ok);
                if (!ok) return;

                QMap<int, QGraphicsItem*> resultMap;
                bool skipGeneration = false;

                QString defaultName = selectedFormat.contains(".ino") && !selectedFormat.contains(".cpp") ? "DebugWiFi.ino" : "TrafficLightWifi.ino";
                QString path = QFileDialog::getSaveFileName(this, tr("Сохранить"), defaultName, "*.ino");

                // --- НОВАЯ ЛОГИКА ГОРЯЧЕГО ПОДКЛЮЧЕНИЯ ---
                if (path.isEmpty()) {
                    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Горячее подключение"),
                        tr("Отменить генерацию кода и просто подключиться к ESP32 по Wi-Fi?"),
                        QMessageBox::Yes | QMessageBox::No);

                    if (reply == QMessageBox::Yes) {
                        skipGeneration = true;
                    } else {
                        return;
                    }
                }

                if (!skipGeneration) {
                    if (selectedFormat.contains(".ino") && !selectedFormat.contains(".cpp")) {
                        DynamicResult res = DynamicWifiTranslator::generate(scene, m_headerCode, ssid, pass);
                        updateCodePreviewStatus({{"DebugWiFi.ino", res.code}}, tr("Wi-Fi Дебаггер"), res.error.isEmpty());
                        if (!res.error.isEmpty()) { QMessageBox::warning(this, tr("Ошибка"), res.error); return; }
                        resultMap = res.idMap;

                        QFile f(path); if(f.open(QIODevice::WriteOnly)) { QTextStream(&f) << res.code; f.close(); }
                    } else {
                        QFileInfo fi(path);
                        ClassResult res = DynamicWifiClassTranslator::generate(scene, fi.baseName(), m_headerCode, ssid, pass);
                        updateCodePreviewStatus({
                                                    {fi.baseName() + ".h", res.headerCode},
                                                    {fi.baseName() + ".cpp", res.sourceCode},
                                                    {fi.baseName() + ".ino", res.mainCode}
                                                }, tr("Wi-Fi Дебаггер (C++)"), res.error.isEmpty());
                        if (!res.error.isEmpty()) { QMessageBox::warning(this, tr("Ошибка"), res.error); return; }
                        resultMap = res.idMap;

                        QFile f1(path); if(f1.open(QIODevice::WriteOnly)) { QTextStream(&f1) << res.mainCode; f1.close(); }
                        QFile f2(fi.absolutePath() + "/" + fi.baseName() + ".h"); if(f2.open(QIODevice::WriteOnly)) { QTextStream(&f2) << res.headerCode; f2.close(); }
                        QFile f3(fi.absolutePath() + "/" + fi.baseName() + ".cpp"); if(f3.open(QIODevice::WriteOnly)) { QTextStream(&f3) << res.sourceCode; f3.close(); }
                    }

                    QMessageBox::information(this, tr("Этап 1: Прошивка"),
                        tr("1. Загрузите код в контроллер (ESP32 / Arduino Wi-Fi).\n"
                           "2. Откройте Serial Monitor, чтобы узнать полученный IP адрес.\n"
                           "3. Убедитесь, что компьютер и контроллер в одной Wi-Fi сети.\n\n"
                           "Нажмите OK, когда будете готовы подключиться."));
                } else {
                    // СОБИРАЕМ КАРТУ ВРУЧНУЮ ДЛЯ ГОРЯЧЕГО ПОДКЛЮЧЕНИЯ
                    for (QGraphicsItem *item : scene->items()) {
                        if (DiagramItem *dItem = dynamic_cast<DiagramItem*>(item)) {
                            resultMap.insert(dItem->id(), dItem);
                        } else if (ConnectionItem *cItem = dynamic_cast<ConnectionItem*>(item)) {
                            resultMap.insert(cItem->id().toInt(), cItem);
                        }
                    }
                }

                // 5. Спрашиваем IP
                QString targetIp = QInputDialog::getText(this, tr("Подключение"), tr("Введите IP адрес контроллера:"));
                if (targetIp.isEmpty()) return;

                // 6. Запуск Дебаггера
                DebuggerWindow *debugWin = new DebuggerWindow(this);
                // --- ПЕРЕДАЕМ СЕКТОРА В ДЕБАГГЕР ---
                    QStringList sectorNames;
                    for(int i = 0; i < m_sectorTabBar->count(); ++i) {
                        sectorNames << m_sectorTabBar->tabText(i);
                    }
                    debugWin->setSectors(sectorNames, m_sectorCenters);
                debugWin->setAttribute(Qt::WA_DeleteOnClose);
                debugWin->setDebugScene(scene);
                debugWin->setItemMap(resultMap);
                debugWin->setHardwareWifiListener(m_hwWifiListener);

                connect(m_hwWifiListener, &HardwareWifiListener::onBlockEntered, debugWin, &DebuggerWindow::externalSetCurrentBlock);
                connect(m_hwWifiListener, &HardwareWifiListener::onTransitionTaken, debugWin, [debugWin](int id){ debugWin->externalHighlightArrow(id, true); });
                connect(m_hwWifiListener, &HardwareWifiListener::onTransitionFalse, debugWin, [debugWin](int id){ debugWin->externalHighlightArrow(id, false); });
                connect(m_hwWifiListener, &HardwareWifiListener::onLogMessage, debugWin, [debugWin](QString msg){ debugWin->appendLog(tr("[WiFi] %1").arg(msg)); });

                connect(debugWin, &QDialog::finished, this, &MainWindow::clearDebugVisuals);
                connect(debugWin, &QDialog::finished, m_hwWifiListener, &HardwareWifiListener::disconnectDevice);

                if (m_hwWifiListener->connectToDevice(targetIp)) {
                    debugWin->show();
                    debugWin->appendLog(tr("Connecting to %1...").arg(targetIp));
                } else {
                    delete debugWin;
                }
            }
        }
    // ========================================================================
    // ВЕТКА 2: ЭМУЛЯТОР (СИМУЛЯЦИЯ НА ПК)
    // ========================================================================
    else
    {
        // Создаем окно (общее для всех)
        DebuggerWindow *debugWin = new DebuggerWindow(this);
        // --- ПЕРЕДАЕМ СЕКТОРА В ДЕБАГГЕР ---
            QStringList sectorNames;
            for(int i = 0; i < m_sectorTabBar->count(); ++i) {
                sectorNames << m_sectorTabBar->tabText(i);
            }
            debugWin->setSectors(sectorNames, m_sectorCenters);
        debugWin->setAttribute(Qt::WA_DeleteOnClose);
        connect(debugWin, &QDialog::finished, this, &MainWindow::clearDebugVisuals);
        debugWin->resize(900, 600);
        debugWin->setDebugScene(scene);

        // --- ВАРИАНТ: PYTHON ---
        if (m_currentLang == Lang_PYTHON) {
            debugWin->setLanguage(DebuggerWindow::Lang_PYTHON);

            auto result = PythonTranslator::generate(scene, m_headerCode, true);
            updateCodePreviewStatus({{"debug_script.py", result.code}}, tr("Python Эмулятор"), result.error.isEmpty());
            if (!result.error.isEmpty()) {
                QMessageBox::warning(this, tr("Ошибка генерации"), result.error);
                delete debugWin; return;
            }

            // Проверка синтаксиса
            QString tempPath = QDir::tempPath() + "/temp_debug_check.py";
            QFile file(tempPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                            QTextStream out(&file); out << result.code; file.close();
                            if (!Utils::checkPythonSyntax(tempPath)) {
                                // КОМПИЛЯТОР УПАЛ: Красим в красный и передаем строку
                                updateCodePreviewStatus({{"debug_script.py", result.code}}, tr("Python Эмулятор"), false, Utils::getLastErrorLines());
                                delete debugWin;
                                return;
                            }
                        }
                        // ВСЕ ОК: Подтверждаем зеленый статус
                        updateCodePreviewStatus({{"debug_script.py", result.code}}, tr("Python Эмулятор"), true);

            debugWin->setItemMap(result.idMap);
            debugWin->show();
            debugWin->startDebugSession(result.code);
        }
        // --- ВАРИАНТ: C++ ---
        else if (m_currentLang == Lang_CPP) {
            debugWin->setLanguage(DebuggerWindow::Lang_CPP);

            auto result = CppTranslator2::generateDebugSource(scene, m_headerCode);
            updateCodePreviewStatus({{"DebugRobot.cpp", result.sourceCode}}, tr("C++ Эмулятор"), result.error.isEmpty());
            if (!result.error.isEmpty()) {
                QMessageBox::warning(this, tr("Ошибка генерации"), result.error);
                delete debugWin; return;
            }

            QString exePath = QDir::tempPath() + "/debug_robot.exe";
            ui->statusbar->showMessage(tr("Компиляция C++..."), 2000);
            QApplication::processEvents();

            if (!Utils::compileCpp(result.sourceCode, exePath)) {
                            // КОМПИЛЯТОР УПАЛ: Красим в красный и передаем строку
                            updateCodePreviewStatus({{"DebugRobot.cpp", result.sourceCode}}, tr("C++ Эмулятор"), false, Utils::getLastErrorLines());
                            delete debugWin;
                            return;
                        }
                        // ВСЕ ОК: Подтверждаем зеленый статус
                        updateCodePreviewStatus({{"DebugRobot.cpp", result.sourceCode}}, tr("C++ Эмулятор"), true);
                        ui->statusbar->showMessage(tr("Компиляция успешна! Запуск..."), 2000);

            debugWin->setItemMap(result.idMap);
            debugWin->show();
            debugWin->startDebugSession(exePath);
        }
        // --- ВАРИАНТ: ARDUINO (ЭМУЛЯТОР) ---
        else if (m_currentLang == Lang_MICRO) {
            debugWin->setLanguage(DebuggerWindow::Lang_CPP);

            auto result = MicroTranslator2::generateDebugSource(scene, m_headerCode);
            updateCodePreviewStatus({{"EmulatorMicro.cpp", result.sourceCode}}, tr("Эмулятор МК на ПК"), result.error.isEmpty());
            if (!result.error.isEmpty()) {
                QMessageBox::warning(this, tr("Ошибка генерации"), result.error);
                delete debugWin; return;
            }

            QString exePath = QDir::tempPath() + "/debug_micro.exe";
            ui->statusbar->showMessage(tr("Компиляция симулятора..."), 2000);
            QApplication::processEvents();

            if (!Utils::compileCpp(result.sourceCode, exePath)) {
                            // КОМПИЛЯТОР УПАЛ: Красим в красный и передаем строку
                            updateCodePreviewStatus({{"EmulatorMicro.cpp", result.sourceCode}}, tr("Эмулятор МК на ПК"), false, Utils::getLastErrorLines());
                            delete debugWin;
                            return;
                        }
                        // ВСЕ ОК
                        updateCodePreviewStatus({{"EmulatorMicro.cpp", result.sourceCode}}, tr("Эмулятор МК на ПК"), true);
                        ui->statusbar->showMessage(tr("Запуск..."), 2000);

            debugWin->setItemMap(result.idMap);
            debugWin->show();
            debugWin->startDebugSession(exePath);
        }
        // --- ВАРИАНТ: JAVA ---
        else if (m_currentLang == Lang_JAVA) {
            debugWin->setLanguage(DebuggerWindow::Lang_JAVA);

            auto result = JavaTranslator2::generateDebug(scene, "DebugRobot", m_headerCode);
            updateCodePreviewStatus({{"DebugRobot.java", result.javaCode}}, tr("Java Эмулятор"), result.error.isEmpty());
            if (!result.error.isEmpty()) {
                QMessageBox::warning(this, tr("Ошибка генерации"), result.error);
                delete debugWin; return;
            }

            QString tempDir = QDir::tempPath();
            ui->statusbar->showMessage(tr("Компиляция Java..."), 2000);
            QApplication::processEvents();

            if (!Utils::compileJava(result.javaCode, tempDir)) {
                            // КОМПИЛЯТОР УПАЛ: Красим в красный и передаем строку
                            updateCodePreviewStatus({{"DebugRobot.java", result.javaCode}}, tr("Java Эмулятор"), false, Utils::getLastErrorLines());
                            delete debugWin;
                            return;
                        }
                        // ВСЕ ОК
                        updateCodePreviewStatus({{"DebugRobot.java", result.javaCode}}, tr("Java Эмулятор"), true);
                        QString javaExePath = Utils::findJavaTool("java");

            if (javaExePath.isEmpty()) javaExePath = "java";

            QString batPath = tempDir + "/debug_java.bat";
            QFile batFile(batPath);
            if (batFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&batFile);
                out << "@echo off\nchcp 65001 > nul\ncd /d \"" << tempDir << "\"\n";
                QString libPath = QCoreApplication::applicationDirPath() + "/JavaLibraries";
                out << "\"" << javaExePath << "\" -cp \".;" + libPath + "/*\" DebugRobot\n";
                batFile.close();
            }

            ui->statusbar->showMessage(tr("Запуск Java..."), 2000);
            debugWin->setItemMap(result.idMap);
            debugWin->show();
            debugWin->startDebugSession(batPath);
        }
    }
}
void MainWindow::clearDebugVisuals()
{
    if (!scene) return;

    // 1. Сброс СТРЕЛОК
    for (QGraphicsItem *item : scene->items()) {
        if (ConnectionItem *conn = dynamic_cast<ConnectionItem*>(item)) {
            conn->resetVisualState(); // Возвращаем черный/серый цвет
        }
    }

    // 2. Сброс БЛОКОВ (DiagramItem)
    // Я предполагаю, что у DiagramItem есть метод setBrush или вы красили его стандартно.
    // Если у вас нет спец. метода, возвращаем белый цвет напрямую:
    for (QGraphicsItem *item : scene->items()) {
        if (DiagramItem *block = dynamic_cast<DiagramItem*>(item)) {
            block->setBrush(Qt::white); // Или ваш стандартный цвет блока
            // (Если у вас есть такой метод в DiagramItem, а он должен быть,
                        // так как вы используете его в debuggerwindow.cpp)
                        block->clearError();
            block->update();

        }
    }
}
// mainwindow.cpp








void MainWindow::on_actionImport_triggered()
{
    // 1. Открываем диалог выбора файла
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Импортировать схему"),
                                                    "",
                                                    tr("JSON Project (*.json);;All Files (*)"));

    if (fileName.isEmpty()) return;
    QPointF viewCenter = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect().center());
    // 2. Вызываем метод ИМПОРТА (он не чистит сцену)
    if (m_graphManager->importFromFile(fileName, m_headerCode, viewCenter)) {

        ui->statusbar->showMessage(tr("Схема добавлена."), 3000);
        // --- ОБНОВЛЯЕМ ПОДСВЕТКУ КНОПКИ HEADER ---
                if (!m_headerCode.trimmed().isEmpty()) {
                    ui->headerButton->setStyleSheet("font-weight: bold; border: 2px solid green;");
                } else {
                    ui->headerButton->setStyleSheet("");
                }
        // 3. Обновляем подключения к сетке для НОВЫХ блоков
        if (grid) {
            foreach (QGraphicsItem *item, scene->items()) {
                if (DiagramItem *d = dynamic_cast<DiagramItem*>(item)) {
                    // Если у блока еще нет сетки (он только что загружен)
                    d->setGrid(grid);
                }
            }
            grid->updateSceneRect();
        }

        // 4. Обновляем таблицу состояний
        if (m_stateTable) {
            m_stateTable->updateFromScene(scene);
        }

    } else {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось импортировать файл."));
    }
}
// 1. АЛГОРИТМ ПОИСКА СВЯЗЕЙ (BFS - Поиск в ширину)
QList<DiagramItem*> MainWindow::findConnectedComponent(DiagramItem* startNode)
{
    QList<DiagramItem*> group;
    if (!startNode) return group;

    QList<DiagramItem*> queue;
    queue.append(startNode);
    group.append(startNode);

    // Пока очередь не пуста
    while (!queue.isEmpty()) {
        DiagramItem* current = queue.takeFirst();

        // Ищем соседей через стрелки
        // (Предполагаем, что у вас есть доступ к сцене или стрелкам)
        foreach (QGraphicsItem* item, scene->items()) {
             if (ConnectionItem* arrow = dynamic_cast<ConnectionItem*>(item)) {
                 DiagramItem* neighbor = nullptr;

                 // Если стрелка выходит ИЗ текущего или входит В текущий
                 if (arrow->startBlock() == current) neighbor = arrow->endBlock();
                 else if (arrow->endBlock() == current) neighbor = arrow->startBlock();

                 // Если нашли соседа и его еще нет в группе
                 if (neighbor && !group.contains(neighbor)) {
                     group.append(neighbor);
                     queue.append(neighbor);
                 }
             }
        }
    }
    return group;
}

// 2. СОЗДАНИЕ ГРУППЫ
void MainWindow::createGroupFromSelection()
{
    // Берем то, что выделил юзер
    if (!scene) return;
    QList<QGraphicsItem*> selected = scene->selectedItems();

    // --- [НОВОЕ] ПРОВЕРКА 1: Пустое выделение ---
    if (selected.isEmpty()) {
        QMessageBox::warning(this, tr("Создание группы"),
                             tr("Ничего не выделено!\n\n"
                                "Пожалуйста, выделите блоки мышкой (обведите рамкой или кликните с Ctrl), "
                                "а затем нажмите кнопку."));
        return;
    }

    // Фильтруем только БЛОКИ
    QList<DiagramItem*> seedBlocks;
    foreach(QGraphicsItem* item, selected) {
        if (DiagramItem* b = dynamic_cast<DiagramItem*>(item)) {
            seedBlocks.append(b);
        }
    }

    // --- [НОВОЕ] ПРОВЕРКА 2: Нет блоков (только стрелки/текст) ---
    if (seedBlocks.isEmpty()) {
        QMessageBox::warning(this, tr("Внимание"),
                             tr("В выделении нет ни одного Блока!\n\n"
                                "Группировать можно только Блоки (автоматы). "
                                "Стрелки добавятся автоматически."));
        return;
    }

    // ЗАПУСКАЕМ УМНЫЙ ЗАХВАТ
    // Если выделен хотя бы один блок, находим ВСЮ его "семью" (все, что соединено стрелками)
    QSet<DiagramItem*> finalGroupSet;
    foreach(DiagramItem* seed, seedBlocks) {
        QList<DiagramItem*> cluster = findConnectedComponent(seed);
        for(auto b : cluster) finalGroupSet.insert(b);
    }

    if (finalGroupSet.isEmpty()) return;

    // 3. ВЫЧИСЛЯЕМ ОБЩУЮ ГРАНИЦУ (Bounding Rect)
    QRectF unionRect;
    bool first = true;
    for (DiagramItem* b : finalGroupSet) {
        if (first) {
            unionRect = b->sceneBoundingRect();
            first = false;
        } else {
            unionRect = unionRect.united(b->sceneBoundingRect());
        }
    }

    // Добавляем отступы (padding), чтобы рамка не прилипала к блокам
    unionRect.adjust(-20, -30, 20, 20);

    // 4. СОЗДАЕМ РАМКУ
    static int groupCounter = 1;
    QString name = tr("Group %1").arg(groupCounter++);

    GroupItem* groupItem = new GroupItem(unionRect, name);
    // Сразу сообщаем группе, кто в ней сидит
    for (DiagramItem* b : finalGroupSet) {
        groupItem->addBlock(b);
    }
    scene->addItem(groupItem);

    // Снимаем выделение с блоков и выделяем группу
    scene->clearSelection();
    groupItem->setSelected(true);
}
// Слот, который вызывается при нажатии кнопки в интерфейсе
void MainWindow::on_createGroupButton_clicked()
{
    // Вызываем логику создания группы
    createGroupFromSelection();
}
// Вспомогательная функция: Найти, в какой группе сидит блок
GroupItem* MainWindow::findGroupOf(DiagramItem* block)
{
    foreach (QGraphicsItem *item, scene->items()) {
        if (GroupItem *g = dynamic_cast<GroupItem*>(item)) {
            if (g->hasBlock(block)) return g;

            // Если блок просто визуально внутри, но еще не добавлен в список (для старых групп)
            if (g->sceneBoundingRect().contains(block->sceneBoundingRect().center())) {
                g->addBlock(block); // "Легализация" блока
                return g;
            }
        }
    }
    return nullptr;
}

// ГЛАВНАЯ ЛОГИКА
void MainWindow::checkForGroupExpansion()
{
    // 1. Перебираем все стрелки на сцене
    foreach (QGraphicsItem *item, scene->items()) {
        ConnectionItem *arrow = dynamic_cast<ConnectionItem*>(item);
        if (!arrow) continue;

        DiagramItem* start = arrow->startBlock();
        DiagramItem* end = arrow->endBlock();

        if (!start || !end) continue;

        // 2. Ищем группу, откуда идет стрелка
        GroupItem* startGroup = findGroupOf(start);

        // Если начало стрелки не в группе — нам все равно
        if (!startGroup) continue;

        // 3. Проверяем, чей конец стрелки
        GroupItem* endGroup = findGroupOf(end);

        // ЛОГИКА:
        if (endGroup == nullptr) {
            // СИТУАЦИЯ 1: Конец "ничей" -> ЗАХВАТЫВАЕМ!
            startGroup->addBlock(end);
            // Рамка сама расширится внутри addBlock -> updateGeometry
        }
        else if (endGroup != startGroup) {
            // СИТУАЦИЯ 2: Конец в ЧУЖОЙ группе -> НЕ ТРОГАЕМ
            // (Стрелка просто соединяет две рамки, это нормально)
        }
        else {
            // СИТУАЦИЯ 3: Конец в НАШЕЙ же группе -> ПРОСТО ОБНОВЛЯЕМ
            // (На случай, если блок подвинули внутри рамки)
            startGroup->updateGeometry();
        }
    }
}
// Слот для кнопки добавления комментария
void MainWindow::on_addCommentButton_clicked()
{
    // 1. Проверяем, есть ли сцена (на всякий случай)
    if (!scene) return;

    // 2. Проверяем, есть ли выделенный объект для привязки
    QGraphicsItem *target = nullptr;
    if (!scene->selectedItems().isEmpty()) {
        target = scene->selectedItems().first();
    }

    // 3. Создаем стикер
    CommentItem *comment = new CommentItem(tr("Заметка..."));

    // 4. Позиционируем
    if (target) {
        comment->setLinkedItem(target);
        // Ставим чуть правее и выше цели
        comment->setPos(target->sceneBoundingRect().topRight() + QPointF(20, -20));
    } else {
        // Просто в центре экрана (или чуть со смещением от центра вида)
        QPointF centerPos = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect().center());
        comment->setPos(centerPos);
    }

    scene->addItem(comment);

    // 5. Делаем снимок для Ctrl+Z
    if (m_graphManager) m_graphManager->createSnapshot();
}
void MainWindow::on_addBridgeButton_clicked()
{
    // Создаем блок-мост
    BridgeItem *bridge = new BridgeItem();

    // Ставим его в центр видимой области
    // (Используем ui->graphicsView, если у вас так назван виджет, или просто graphicsView)
    if (ui->graphicsView) {
        bridge->setPos(ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect().center()));
    } else {
        // Запасной вариант, если ui->graphicsView недоступен напрямую, но есть scene
        bridge->setPos(0, 0);
    }

    scene->addItem(bridge);

    // Сохраняем состояние для отмены (Ctrl+Z)
    if (m_graphManager) m_graphManager->createSnapshot();
}
void MainWindow::on_addBackButton_clicked()
{
    // Создаем блок-возврат (Круг)
    BackItem *backItem = new BackItem();

    // Ставим его в центр видимой области
    if (ui->graphicsView) {
        backItem->setPos(ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect().center()));
    } else {
        backItem->setPos(0, 0);
    }

    scene->addItem(backItem);

    // Сохраняем состояние для отмены (Ctrl+Z)
    if (m_graphManager) m_graphManager->createSnapshot();
}
void MainWindow::on_addHistoryButton_clicked()
{
    // 1. Ищем, выделена ли на сцене хотя бы одна рамка (GroupItem)
    GroupItem *targetGroup = nullptr;
    for (QGraphicsItem *item : scene->selectedItems()) {
        if (GroupItem *g = dynamic_cast<GroupItem*>(item)) {
            targetGroup = g;
            break;
        }
    }

    // 2. Если рамка не выделена - ругаемся и отменяем создание!
    if (!targetGroup) {
        QMessageBox::warning(this, tr("Запрет"), tr("Блок 'История' нельзя добавить в пустоту!\n\nСначала кликните по Рамке (Группе), чтобы выделить её, а затем нажмите эту кнопку."));
        return;
    }

    // 3. Если рамка выделена - создаем историю
    HistoryItem *history = new HistoryItem();
    scene->addItem(history);

    // 4. Ставим блок ровно в центр этой выделенной рамки
    QPointF groupCenter = targetGroup->sceneBoundingRect().center();
    // Смещаем на половину размера блока (30х30), чтобы центр блока совпал с центром рамки
    history->setPos(groupCenter.x() - 30, groupCenter.y() - 30);

    // 5. Сохраняем состояние для отмены (Ctrl+Z)
    if (m_graphManager) {
        m_graphManager->createSnapshot();
    }
}


void MainWindow::on_actionNew_Project_triggered()
{
    // 1. Умный путь: берем из настроек или используем стандартный в Документах
    QString defaultWorkspace = QSettings("VisualBuild", "IDE").value("defaultWorkspace",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/VisualBuildProjects").toString();

    // 2. Спрашиваем сразу ПАПКУ ПРОЕКТА
    // Пользователь может выбрать существующую пустую папку или создать новую прямо в этом окне
    QString projectPath = QFileDialog::getExistingDirectory(this, tr("Выберите папку для НОВОГО проекта (она станет папкой проекта)"), defaultWorkspace);
    if (projectPath.isEmpty()) return;

    // 3. Имя проекта автоматически берется из названия выбранной папки
    QString projectName = QFileInfo(projectPath).fileName();

    // 4. Защита от перезаписи: проверяем, нет ли там уже проекта
    if (QFile::exists(projectPath + "/graph.json")) {
        QMessageBox::warning(this, tr("Ошибка"), tr("В этой папке уже существует проект VisualBuild!"));
        return;
    }

    // 5. Создаем структуру подпапок библиотек прямо внутри выбранной папки
    QDir dir(projectPath);
    dir.mkpath("CppLibraries");
    dir.mkpath("PythonLibraries");
    dir.mkpath("JavaLibraries");

    grid = nullptr;

    // 6. БЕЗОПАСНАЯ ОЧИСТКА ХОЛСТА (согласно вашей текущей логике)
    QList<QGraphicsItem*> itemsToClear = scene->items();
    // Сначала аккуратно удаляем все стрелки
    for (QGraphicsItem *item : itemsToClear) {
        if (dynamic_cast<ConnectionItem*>(item)) {
            scene->removeItem(item);
            delete item;
        }
    }
    // Теперь безопасно удаляем блоки, рамки и всё остальное
    itemsToClear = scene->items();
    for (QGraphicsItem *item : itemsToClear) {
        scene->removeItem(item);
        delete item;
    }
    scene->clear();

    m_headerCode.clear();
    ui->headerButton->setStyleSheet("");

    // 7. Настройка новой сетки и параметров сцены
    grid = new Grid(16);
    scene->addItem(grid);
    grid->setData(0, 1);
    grid->setScaleFactor(m_currentZoom);
    grid->updateSceneRect();

    // 8. ЖЕЛЕЗОБЕТОННАЯ ОЧИСТКА ИСТОРИИ
    m_graphManager->clearHistory();

    // 9. Сохраняем пустой проектный файл graph.json
    QString graphPath = projectPath + "/graph.json";
    m_graphManager->saveToFile(graphPath, m_headerCode, m_currentLang, serializeSectors());

    // 10. МГНОВЕННОЕ ОТОБРАЖЕНИЕ В ДЕРЕВЕ (Шторка)
    // Показываем родительскую папку в шторке, чтобы видеть созданный проект в списке
    QString parentDir = QFileInfo(projectPath).absolutePath();
    m_projectSidebar->setWorkspace("");
    m_projectSidebar->setWorkspace(parentDir);

    m_currentProjectPath = projectPath;

    // 11. Сохраняем путь в настройки как последний открытый проект
    QSettings settings("VisualBuild", "IDE");
    settings.setValue("lastProject", projectPath);

    // 12. Обновление интерфейса
    ui->statusbar->showMessage(tr("Проект %1 успешно создан!").arg(projectName), 3000);
    if (m_welcomeScreen) m_welcomeScreen->hide();
    setProjectUIEnabled(true);
}



void MainWindow::on_actionOpen_Project_triggered()
{
    // 1. УМНЫЙ ПУТЬ: При открытии диалога читаем папку из настроек (Project Settings)
    QString defaultWorkspace = QSettings("VisualBuild", "IDE").value("defaultWorkspace", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/VisualBuildProjects").toString();
    QString projectPath = QFileDialog::getExistingDirectory(this, tr("Выберите папку проекта"), defaultWorkspace);

    if (projectPath.isEmpty()) return;

    // 2. Проверяем, есть ли там наш главный файл
    QString graphPath = projectPath + "/graph.json";
    if (!QFile::exists(graphPath)) {
        QMessageBox::warning(this, tr("Ошибка"), tr("В этой папке нет файла graph.json. Это не проект VisualBuild!"));
        return;
    }

    // ====================================================================
    // --- ПРОВЕРКА ЦЕЛОСТНОСТИ ПРОЕКТА (Интерактивное восстановление) ---
    // ====================================================================
    QDir dir(projectPath);
    if (!dir.exists("CppLibraries") || !dir.exists("PythonLibraries") || !dir.exists("JavaLibraries")) {

        QMessageBox::StandardButton reply = QMessageBox::question(this,
            tr("Устаревший или поврежденный проект"),
            tr("В этой папке отсутствуют системные папки библиотек (CppLibraries, PythonLibraries и т.д.).\n\n"
               "Восстановить структуру проекта (создать пустые папки)?\n"
               "Ваши существующие файлы затронуты не будут."),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            // Юзер разрешил -> безопасно достраиваем недостающее
            dir.mkpath("CppLibraries");
            dir.mkpath("PythonLibraries");
            dir.mkpath("JavaLibraries");
        } else {
            // Юзер отказался -> прерываем открытие, чтобы ничего не сломать
            ui->statusbar->showMessage(tr("Открытие проекта отменено."), 3000);
            return;
        }
    }
    // ====================================================================

    // --- МГНОВЕННОЕ ОТОБРАЖЕНИЕ ПАПКИ В ДЕРЕВЕ ---
    QString parentDir = QFileInfo(projectPath).absolutePath();
    m_projectSidebar->setWorkspace("");        // 1. Сбрасываем кэш
    m_projectSidebar->setWorkspace(parentDir); // 2. Загружаем родительскую папку

    // 3. Загружаем проект
    loadProject(projectPath);
}
void MainWindow::on_actionSave_Project_triggered()
{
    // 1. Проверяем, открыт ли сейчас какой-то проект
    if (m_currentProjectPath.isEmpty()) {
        QMessageBox::warning(this, tr("Внимание"), tr("Сначала создайте или откройте проект!"));
        return;
    }

    // 2. Тихо сохраняем граф прямо в папку АКТИВНОГО проекта
    QString graphPath = m_currentProjectPath + "/graph.json";
    if (m_graphManager->saveToFile(graphPath, m_headerCode, m_currentLang, serializeSectors())) {
        ui->statusbar->showMessage(tr("Проект успешно сохранен!"), 3000);
    } else {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить файл проекта!"));
    }
}

void MainWindow::loadProject(const QString &projectPath)
{
    // --- ЗАЩИТА 1: Не загружать заново то, что уже открыто ---
    if (QDir::cleanPath(m_currentProjectPath) == QDir::cleanPath(projectPath)) {
        ui->statusbar->showMessage(tr("Этот проект уже открыт!"), 3000);
        return;
    }

    // АВТОСОХРАНЕНИЕ: Если у нас уже был открыт проект, тихо сохраняем его перед переключением
    if (!m_currentProjectPath.isEmpty() && m_currentProjectPath != projectPath) {
        m_graphManager->saveToFile(m_currentProjectPath + "/graph.json", m_headerCode, m_currentLang, serializeSectors());
        // Удаляем теневой файл старого проекта (он нам больше не нужен, так как мы сохранились штатно)
        QFile::remove(m_currentProjectPath + "/autosave.json");
    }

    // --- НОВАЯ ЛОГИКА АВАРИЙНОГО ВОССТАНОВЛЕНИЯ ---
    QString graphPath = projectPath + "/graph.json";
    QString autoSavePath = projectPath + "/autosave.json";
    QString fileToLoad = graphPath; // По умолчанию грузим основной файл

    // Если есть файл автосохранения — значит программа закрылась аварийно!
    if (QFile::exists(autoSavePath)) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Аварийное восстановление"),
            tr("Обнаружено автосохранение от предыдущей сессии (возможно, программа закрылась аварийно).\n\n"
               "Хотите восстановить последние несохраненные изменения?"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            fileToLoad = autoSavePath; // Подменяем путь, грузим из теневого файла
        } else {
            QFile::remove(autoSavePath); // Пользователь отказался, удаляем мусор
        }
    }

    // ЗАГРУЗКА НОВОГО
    showLoading(tr("Открытие проекта...\n%1").arg(QFileInfo(projectPath).fileName()));
    int loadedLang = 0;
    QJsonArray loadedSectors;
    if (m_graphManager->loadFromFile(fileToLoad, m_headerCode, loadedLang, loadedSectors)) {

        // ====================================================================
        // --- НОВОЕ 1: Очищаем историю от предыдущего проекта (Защита от крашей) ---
        m_graphManager->clearHistory();

        // --- НОВОЕ 2: Железобетонная гарантия наличия папок библиотек ---
        // (На случай, если проект грузится при старте программы, а папки случайно удалили)
        QDir dir(projectPath);
        dir.mkpath("CppLibraries");
        dir.mkpath("PythonLibraries");
        dir.mkpath("JavaLibraries");
        // ====================================================================

        deserializeSectors(loadedSectors);
        applyProjectLanguage(loadedLang);
        m_currentProjectPath = projectPath;

        // ЗАПОМИНАЕМ ПРОЕКТ НА БУДУЩЕЕ
        QSettings settings("VisualBuild", "IDE");
        settings.setValue("lastProject", projectPath);

        ui->statusbar->showMessage(tr("Проект переключен на: %1").arg(QFileInfo(projectPath).fileName()), 3000);

        if (!m_headerCode.trimmed().isEmpty()) {
            ui->headerButton->setStyleSheet("font-weight: bold; border: 2px solid green;");
        } else {
            ui->headerButton->setStyleSheet("");
        }

        if (grid) {
            foreach (QGraphicsItem *item, scene->items()) {
                if (DiagramItem *d = dynamic_cast<DiagramItem*>(item)) d->setGrid(grid);
            }
            grid->updateSceneRect();
        }

        if (m_stateTable) m_stateTable->updateFromScene(scene);

        // --- ФИНАЛ ВОССТАНОВЛЕНИЯ ---
        if (fileToLoad == autoSavePath) {
            m_graphManager->saveToFile(graphPath, m_headerCode, m_currentLang, serializeSectors());
            QFile::remove(autoSavePath);
        }
    }
    hideLoading(); // <-- Скрываем загрузку
    if (m_welcomeScreen) m_welcomeScreen->hide();
    setProjectUIEnabled(true); // <-- Разблокируем!
}
void MainWindow::applyProjectLanguage(int langId)
{
    // Ищем нужную кнопку в памяти по имени
    QAction* targetAct = nullptr;
    if (langId == Lang_JAVA) targetAct = this->findChild<QAction*>("actTransJava");
    else if (langId == Lang_PYTHON) targetAct = this->findChild<QAction*>("actTransPy");
    else if (langId == Lang_MICRO) targetAct = this->findChild<QAction*>("actTransMicro");
    else targetAct = this->findChild<QAction*>("actTransCpp"); // C++ по умолчанию

    // Если нашли - программно "кликаем" по ней!
    // Это автоматически обновит m_currentLang, галочки в меню и подсветку синтаксиса.
    if (targetAct) {
        targetAct->trigger();
    }
}
void MainWindow::performAutoSave()
{
    // Сохраняем только если открыт какой-то проект и есть менеджер
    if (!m_currentProjectPath.isEmpty() && m_graphManager) {
        QString autoSavePath = m_currentProjectPath + "/autosave.json";
        // Тихо перезаписываем теневой файл
        m_graphManager->saveToFile(autoSavePath, m_headerCode, m_currentLang, serializeSectors());

        // Проверяем настройку уведомлений
        QSettings s("VisualBuild", "IDE");
        if (s.value("showAutosaveMsg", false).toBool()) {
            ui->statusbar->showMessage(tr("💾 Автосохранение..."), 2000);
        }
    }
}


// =============================================================
// --- ЛОГИКА СЕКТОРОВ (МОНТАЖНЫХ ОБЛАСТЕЙ) ---
// =============================================================


const qreal SECTOR_WIDTH = 5000.0;
const qreal SECTOR_HEIGHT = 3000.0;

void MainWindow::onAddSectorClicked()
{
    bool ok;
    QString defaultName = "A" + QString::number(m_sectorTabBar->count() + 1);
    QString name = QInputDialog::getText(this, tr("Новый сектор"),
                                         tr("Введите имя сектора:"),
                                         QLineEdit::Normal, defaultName, &ok);

    if (!ok || name.trimmed().isEmpty()) return;

    int sectorIndex = m_sectorTabBar->count();
    qreal startX = sectorIndex * SECTOR_WIDTH;
    qreal startY = 0.0;

    // ВАЖНО: Теперь сохраняем левый верхний угол нового сектора!
    QPointF topLeft(startX, startY);
    m_sectorCenters.append(topLeft);
    m_sectorTabBar->addTab(name);

    // Рисуем рамку сектора
    if (scene) {
        QGraphicsRectItem* bgRect = new QGraphicsRectItem(startX, startY, SECTOR_WIDTH, SECTOR_HEIGHT);
        bgRect->setPen(QPen(Qt::gray, 2, Qt::DashLine));
        bgRect->setZValue(-1000);
        scene->addItem(bgRect);

        // Название прижимаем в левый верхний угол рамки
        QGraphicsTextItem* label = new QGraphicsTextItem(name);
        label->setPos(startX + 10, startY + 10);
        label->setDefaultTextColor(QColor(150, 150, 150, 150));
        QFont f = label->font();
        f.setPointSize(48);
        f.setBold(true);
        label->setFont(f);
        label->setZValue(-1000);
        scene->addItem(label);
    }

    m_sectorTabBar->setCurrentIndex(sectorIndex);
    if (m_graphManager) m_graphManager->createSnapshot();
    ui->statusbar->showMessage(tr("Сектор '%1' успешно создан!").arg(name), 3000);
}
void MainWindow::onSectorChanged(int index)
{
    if (index >= 0 && index < m_sectorCenters.size()) {
        QPointF topLeft = m_sectorCenters[index];

        QRectF itemsRect;
        bool hasItems = false;

        // 1. Ищем все блоки, которые лежат в этом конкретном секторе
        foreach (QGraphicsItem *item, scene->items()) {
            if (DiagramItem *block = dynamic_cast<DiagramItem*>(item)) {
                QPointF center = block->sceneBoundingRect().center();
                // Если блок внутри сектора
                if (center.x() >= topLeft.x() && center.x() < topLeft.x() + SECTOR_WIDTH &&
                    center.y() >= topLeft.y() && center.y() < topLeft.y() + SECTOR_HEIGHT) {

                    if (!hasItems) {
                        itemsRect = block->sceneBoundingRect();
                        hasItems = true;
                    } else {
                        itemsRect = itemsRect.united(block->sceneBoundingRect());
                    }
                }
            }
        }

        // 2. УМНАЯ КАМЕРА
        if (hasItems) {
            // Если в секторе есть схема — камера центрируется ИДЕАЛЬНО на ней
            ui->graphicsView->centerOn(itemsRect.center());
        } else {
            // Если сектор пустой — камера смотрит в самый центр пустого поля
            QPointF center(topLeft.x() + (SECTOR_WIDTH / 2.0), topLeft.y() + (SECTOR_HEIGHT / 2.0));
            ui->graphicsView->centerOn(center);
        }
    }
}
QJsonArray MainWindow::serializeSectors()
{
    QJsonArray arr;
    for(int i = 0; i < m_sectorTabBar->count(); ++i) {
        QJsonObject sec;
        sec["name"] = m_sectorTabBar->tabText(i);
        sec["x"] = m_sectorCenters[i].x();
        sec["y"] = m_sectorCenters[i].y();
        arr.append(sec);
    }
    return arr;
}

void MainWindow::deserializeSectors(const QJsonArray &sectors)
{
    // 1. Очищаем текущие вкладки
    while (m_sectorTabBar->count() > 0) m_sectorTabBar->removeTab(0);
    m_sectorCenters.clear();

    // 2. УДАЛЯЕМ старые фоновые рамки и надписи (они имеют Z-value -1000)
    foreach (QGraphicsItem *item, scene->items()) {
        if (item->zValue() == -1000 && (dynamic_cast<QGraphicsRectItem*>(item) || dynamic_cast<QGraphicsTextItem*>(item))) {
            scene->removeItem(item);
            delete item;
        }
    }

    // Лямбда для создания "резиновой" рамки
    auto createSectorVisuals = [this](QPointF topLeft, QString name) {
        qreal width = SECTOR_WIDTH;
        qreal height = SECTOR_HEIGHT;

        // Расширяем рамку, если блоки вылезли за края!
        foreach (QGraphicsItem *item, scene->items()) {
            if (DiagramItem *block = dynamic_cast<DiagramItem*>(item)) {
                QRectF bRect = block->sceneBoundingRect();
                if (bRect.center().x() >= topLeft.x() && bRect.center().x() < topLeft.x() + SECTOR_WIDTH) {
                    if (bRect.right() > topLeft.x() + width) width = bRect.right() - topLeft.x() + 100;
                    if (bRect.bottom() > topLeft.y() + height) height = bRect.bottom() - topLeft.y() + 100;
                }
            }
        }

        QGraphicsRectItem* bgRect = new QGraphicsRectItem(topLeft.x(), topLeft.y(), width, height);
        bgRect->setPen(QPen(Qt::gray, 2, Qt::DashLine));
        bgRect->setZValue(-1000);
        scene->addItem(bgRect);

        QGraphicsTextItem* label = new QGraphicsTextItem(name);
        label->setPos(topLeft.x() + 10, topLeft.y() + 10);
        label->setDefaultTextColor(QColor(150, 150, 150, 150));
        QFont f = label->font(); f.setPointSize(48); f.setBold(true);
        label->setFont(f);
        label->setZValue(-1000);
        scene->addItem(label);
    };

    // 3. Восстанавливаем вкладки и рисуем рамки заново
    if (sectors.isEmpty()) {
        m_sectorTabBar->addTab("A1");
        m_sectorCenters.append(QPointF(0.0, 0.0));
        createSectorVisuals(QPointF(0,0), "A1");
    } else {
        for (int i = 0; i < sectors.size(); ++i) {
            QJsonObject sec = sectors[i].toObject();
            QPointF pt(sec["x"].toDouble(), sec["y"].toDouble());
            QString name = sec["name"].toString();
            m_sectorTabBar->addTab(name);
            m_sectorCenters.append(pt);
            createSectorVisuals(pt, name);
        }
    }

    if (m_sectorTabBar->count() > 0) {
        m_sectorTabBar->setCurrentIndex(0);
        QTimer::singleShot(100, this, [this]() { onSectorChanged(0); });
    }
}
// =============================================================
// --- ЛОГИКА ВЫБОРОЧНОЙ ОТЛАДКИ (SCOPED DEBUGGING) ---
// =============================================================
void MainWindow::applyDebugFilters(const QStringList &activeSectors, const QStringList &activeGroups)
{
    // 1. Сначала жестко выключаем дебаг для ВСЕХ элементов на сцене
    foreach (QGraphicsItem *item, scene->items()) {
        if (dynamic_cast<DiagramItem*>(item) || dynamic_cast<ConnectionItem*>(item)) {
            item->setData(ROLE_DEBUG_ENABLED, false);
        }
    }

    // 2. Включаем дебаг только для блоков из АКТИВНЫХ зон
    foreach (QGraphicsItem *item, scene->items()) {
        if (DiagramItem *block = dynamic_cast<DiagramItem*>(item)) {
            bool enable = false;
            QPointF center = block->sceneBoundingRect().center();

            // А) Проверяем Сектора
            for (int i = 0; i < m_sectorTabBar->count(); ++i) {
                QString secName = m_sectorTabBar->tabText(i);
                if (activeSectors.contains(secName)) {
                    QPointF topLeft = m_sectorCenters[i];
                    if (center.x() >= topLeft.x() && center.x() < topLeft.x() + SECTOR_WIDTH &&
                        center.y() >= topLeft.y() && center.y() < topLeft.y() + SECTOR_HEIGHT) {
                        enable = true;
                        break;
                    }
                }
            }

            // Б) Проверяем Группы (если блок еще не включен)
            if (!enable) {
                GroupItem *g = findGroupOf(block);
                if (g && activeGroups.contains(g->name())) {
                    enable = true;
                }
            }

            // В) Если блок вообще лежит в "пустоте" (ни в секторе, ни в группе),
            // по умолчанию тоже его слушаем (чтобы ничего не потерять).
            if (!enable && !findGroupOf(block)) {
                bool inAnySector = false;
                for (int i = 0; i < m_sectorCenters.size(); ++i) {
                    QPointF topLeft = m_sectorCenters[i];
                    if (center.x() >= topLeft.x() && center.x() < topLeft.x() + SECTOR_WIDTH &&
                        center.y() >= topLeft.y() && center.y() < topLeft.y() + SECTOR_HEIGHT) {
                        inAnySector = true; break;
                    }
                }
                if (!inAnySector) enable = true;
            }

            // Ставим финальный флаг блоку
            block->setData(ROLE_DEBUG_ENABLED, enable);
        }
    }

    // 3. Умное включение стрелок (только если ОБА конца стрелки активны)
    foreach (QGraphicsItem *item, scene->items()) {
        if (ConnectionItem *arrow = dynamic_cast<ConnectionItem*>(item)) {
            if (arrow->startBlock() && arrow->endBlock()) {
                bool startOk = arrow->startBlock()->data(ROLE_DEBUG_ENABLED).toBool();
                bool endOk = arrow->endBlock()->data(ROLE_DEBUG_ENABLED).toBool();
                arrow->setData(ROLE_DEBUG_ENABLED, startOk && endOk);
            }
        }
    }
}
// =========================================================

// =========================================================
// =========================================================
// --- ОКНО ПРЕДПРОСМОТРА КОДА (CODE PREVIEW) ---
// =========================================================
void MainWindow::showCodePreview()
{
    if (m_lastGeneratedFiles.isEmpty()) { // <-- Используем новую переменную
        QMessageBox::information(this, tr("Пусто"), tr("Сначала сгенерируйте код."));
        return;
    }
    // Передаем новый список ошибок (m_lastErrorLines) в диалог
    CodePreviewDialog dialog(m_lastGeneratedFiles, m_lastTranslatorName, m_lastGenerationStatus, m_lastErrorLines, this);
    dialog.exec();
}

void MainWindow::updateCodePreviewStatus(const QVector<QPair<QString, QString>> &files, const QString &translator, bool success, QList<int> errorLines)
{
    m_lastGeneratedFiles = files; // <-- Сохраняем вектор файлов
    m_lastTranslatorName = translator;
    m_lastGenerationStatus = success;
    m_lastErrorLines = errorLines; // <-- Сохраняем список строк с ошибками

    // Универсальный и надежный CSS (сработает для любой кнопки)
    if (success) {
        ui->previewButton->setStyleSheet(
            "border: 2px solid #4CAF50; "
            "border-radius: 4px; "
            "background-color: rgba(76, 175, 80, 0.15); "
            "padding: 2px;"
        );
    } else {
        ui->previewButton->setStyleSheet(
            "border: 2px solid #F44336; "
            "border-radius: 4px; "
            "background-color: rgba(244, 67, 54, 0.15); "
            "padding: 2px;"
        );
    }
}
// =========================================================
// =========================================================
// --- ФУНКЦИЯ БЛОКИРОВКИ/РАЗБЛОКИРОВКИ ИНТЕРФЕЙСА ---
// =========================================================
void MainWindow::setProjectUIEnabled(bool enabled)
{
    // 1. Верхний тулбар (Рисование и настройки)
    if (ui->addItemButton) ui->addItemButton->setEnabled(enabled);
    if (ui->addBridgeButton) ui->addBridgeButton->setEnabled(enabled);
    if (ui->addBackButton) ui->addBackButton->setEnabled(enabled);
    if (ui->addCommentButton) ui->addCommentButton->setEnabled(enabled);
    if (ui->createGroupButton) ui->createGroupButton->setEnabled(enabled);
    if (ui->addHistoryButton) ui->addHistoryButton->setEnabled(enabled);
    if (ui->headerButton) ui->headerButton->setEnabled(enabled);

    // 2. Нижняя панель (Проверка, Генерация, Дебаг)
    if (ui->checkButton) ui->checkButton->setEnabled(enabled);
    if (ui->generateButton) ui->generateButton->setEnabled(enabled);
    if (ui->startButton) ui->startButton->setEnabled(enabled);
    if (ui->previewButton) ui->previewButton->setEnabled(enabled);
    if (ui->debugButton) ui->debugButton->setEnabled(enabled);
    if (ui->openTableButton) ui->openTableButton->setEnabled(enabled);
    if (ui->speedComboBox) ui->speedComboBox->setEnabled(enabled);

    // 3. Меню графа (Открытие, Сохранение, Импорт)
    if (ui->actionOpen) ui->actionOpen->setEnabled(enabled);
    if (ui->actionSave) ui->actionSave->setEnabled(enabled);
    if (ui->actionImport_JSON) ui->actionImport_JSON->setEnabled(enabled);
    if (ui->actionSave_Project) ui->actionSave_Project->setEnabled(enabled);
    // 4. Панель вкладок секторов

    if (m_sectorTabBar) m_sectorTabBar->setEnabled(enabled);
    // --- ДОБАВЛЕНО: Блокировка кнопки "+" (Создать сектор) ---
        if (QToolButton *btn = this->findChild<QToolButton*>("addSectorBtn")) btn->setEnabled(enabled);
        // --- ДОБАВЛЕНО: Блокировка верхних меню ---
            // Блокируем меню Синтаксис (по нашему новому ID)
            if (QAction *act = this->findChild<QAction*>("menuSyntax")) act->setEnabled(enabled);
            if (QAction *act = this->findChild<QAction*>("menuCompiler")) act->setEnabled(enabled);
            // Блокируем меню Edit целиком (оно уже есть в ui)
            if (ui->menuedit) ui->menuedit->setEnabled(enabled);

    // 5. ВЫПАДАЮЩИЕ НАСТРОЙКИ (Project Settings)
    if (QAction *act = this->findChild<QAction*>("settingAutoLoad")) act->setEnabled(enabled);
    if (QAction *act = this->findChild<QAction*>("settingAutoSave")) act->setEnabled(enabled);
    if (QAction *act = this->findChild<QAction*>("settingGroupSpeed")) act->setEnabled(enabled);

    if (QAction *act = this->findChild<QAction*>("settingStructured")) act->setEnabled(enabled);
        if (QAction *act = this->findChild<QAction*>("actionHelpTips")) act->setEnabled(enabled);
        // =========================================================
            // 6. УМНАЯ БЛОКИРОВКА КНОПКИ "LIBRARY MANAGER" В ШТОРКЕ
            // =========================================================
            if (m_projectSidebar) {
                // Ищем обычные кнопки (QPushButton)
                QList<QPushButton*> pushBtns = m_projectSidebar->findChildren<QPushButton*>();
                for (QPushButton* btn : pushBtns) {
                    if (btn->text().contains("Library Manager", Qt::CaseInsensitive)) {
                        btn->setEnabled(enabled);
                    }
                }
                // На всякий случай ищем Tool-кнопки (QToolButton)
                // На всякий случай ищем Tool-кнопки (QToolButton)
                                QList<QToolButton*> toolBtns = m_projectSidebar->findChildren<QToolButton*>();
                                for (QToolButton* btn : toolBtns) {
                                    // Ищем либо оригинальное название, либо переведенное
                                    if (btn->text().contains("Library Manager", Qt::CaseInsensitive) ||
                                        btn->text().contains(tr("Library Manager"), Qt::CaseInsensitive)) {
                                        btn->setEnabled(enabled);
                                    }
                                }

            }
    // Раскомментируйте строку ниже, если добавили ID и для структурированных папок:
    // if (QAction *act = this->findChild<QAction*>("settingStructured")) act->setEnabled(enabled);
}
// =========================================================
// =========================================================
// --- ФУНКЦИИ УПРАВЛЕНИЯ ПОЛЗУНКОМ ЗАГРУЗКИ ---
// =========================================================
void MainWindow::showLoading(const QString &text)
{
    if (ui->progressBar) {
        ui->progressBar->show();
        ui->progressBar->raise(); // На всякий случай вытаскиваем на передний план

        // Показываем текст в статус-баре
        ui->statusbar->showMessage(text);

        // Принудительно отрисовываем интерфейс до зависания
        QApplication::processEvents();
    }
}

void MainWindow::hideLoading()
{
    if (ui->progressBar) {
        ui->progressBar->hide();
    }
    ui->statusbar->clearMessage(); // Убираем текст загрузки
}
// =========================================================
// --- ЛОГИКА КОПИРОВАНИЯ И ВСТАВКИ (CTRL+C / CTRL+V) ---
// =========================================================
void MainWindow::copySelectedItems()
{
    if (!m_graphManager || !scene) return;
    if (scene->selectedItems().isEmpty()) return;

    QJsonObject json = m_graphManager->selectionToJson();
    if (json.isEmpty() || (json["blocks"].toArray().isEmpty() && json["groups"].toArray().isEmpty() && json["comments"].toArray().isEmpty())) {
        return; // Нечего копировать
    }

    // Сохраняем граф как текст в системный буфер обмена Windows/Linux/Mac!
    QJsonDocument doc(json);
    QApplication::clipboard()->setText(doc.toJson());
    ui->statusbar->showMessage(tr("Элементы скопированы (Ctrl+C)"), 2000);
}

void MainWindow::pasteCopiedItems()
{
    if (!m_graphManager || !scene) return;

    // Читаем текст из системного буфера
    QString text = QApplication::clipboard()->text();
    if (text.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject json = doc.object();
    // Защита от дурака: проверяем, что в буфере именно кусок нашего графа, а не просто текст
    if (!json.contains("blocks") && !json.contains("groups") && !json.contains("comments")) {
        return;
    }

    // Снимаем выделение со старых объектов, чтобы пользователь видел, что вставилось новое
    scene->clearSelection();

    // Точка вставки - идеально в центр экрана, куда сейчас смотрит пользователь
    QPointF dropPos = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect().center());

    // Вставляем на сцену (false = не очищать старую сцену, true = использовать точку вставки)
    m_graphManager->jsonToScene(json, false, dropPos, true);

    // Обязательно перепривязываем сетку к новым блокам
    if (grid) {
        foreach (QGraphicsItem *item, scene->items()) {
            if (DiagramItem *d = dynamic_cast<DiagramItem*>(item)) {
                d->setGrid(grid);
            }
        }
        grid->updateSceneRect();
    }

    m_graphManager->createSnapshot(); // Сохраняем в историю для возможности отмены (Ctrl+Z)
    ui->statusbar->showMessage(tr("Элементы вставлены (Ctrl+V)"), 2000);
}
