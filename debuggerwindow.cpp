/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "debuggerwindow.h"
#include "ui_debuggerwindow.h"
#include <QGridLayout>      // <--- ВАЖНО: Добавлено для работы с сеткой
#include <QGraphicsItem>    // <--- ВАЖНО: Добавлено для работы с блоками
#include <QWheelEvent>  // <--- ВАЖНО: Добавили для работы с колесиком
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "diagramitem.h"
#include "connect.h"
#include <QTimer>
#include <QScrollArea>
#include <QCloseEvent>
DebuggerWindow::DebuggerWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DebuggerWindow)
{
    m_process = new QProcess(this);
    // 1. Слушаем нормальный вывод
    connect(m_process, &QProcess::readyReadStandardOutput, this, &DebuggerWindow::onProcessOutputReady);

    // --- ДОБАВИТЬ ЭТУ СТРОКУ (Слушаем ошибки) ---
    connect(m_process, &QProcess::readyReadStandardError, this, &DebuggerWindow::onProcessOutputReady);
    // --------------------------------------------

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &DebuggerWindow::onProcessFinished);
    ui->setupUi(this);
    // --- НОВОЕ: ОТКРЫТИЕ НА ВЕСЬ ЭКРАН И КНОПКИ СВЕРНУТЬ/РАЗВЕРНУТЬ ---
        // =================================================================
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
        setWindowState(Qt::WindowMaximized);
    // --- ВКЛЮЧАЕМ АВТОФОКУС ПО УМОЛЧАНИЮ ---
        if (ui->cbSmartFocus) {
            ui->cbSmartFocus->setChecked(true);
        }
    // --- НАСТРОЙКА ЗУМА ---
        // 1. Делаем так, чтобы зум шел туда, где курсор мышки (как в картах Google)
        ui->graphicsView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

        // 2. Устанавливаем "жучок" (фильтр) на viewport (область просмотра),
        // чтобы ловить события колесика
        ui->graphicsView->viewport()->installEventFilter(this);
    // Настройки красоты и удобства
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);       // Сглаживание
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);  // Можно таскать мышкой
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate); // Плавное обновление

    // Настройка пропорций (Верх 10 частей, Низ 1 часть)
    // Проверяем, есть ли layout, и если это сетка — настраиваем
    if (layout()) {
        QGridLayout *grid = qobject_cast<QGridLayout*>(layout());
        if (grid) {
            grid->setRowStretch(0, 10); // Верх (Схема)
            grid->setRowStretch(1, 1);  // Низ (Лог)
        }
    }
    // ==========================================================
    // ==========================================================
            // --- 1. СКРОЛЛ-ЛЕНТА ДЛЯ КНОПОК ГРУПП ---
            // ==========================================================
            m_navBar = new QWidget();
            // Стили удалены, чтобы фон наследовался от глобальной темы

            QHBoxLayout *navLayout = new QHBoxLayout(m_navBar);
            navLayout->setContentsMargins(5, 5, 5, 5);
            navLayout->setSpacing(5);
            navLayout->setAlignment(Qt::AlignLeft);

            QScrollArea *groupScrollArea = new QScrollArea();
            groupScrollArea->setWidget(m_navBar);
            groupScrollArea->setWidgetResizable(true);
            groupScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            groupScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); // Меняем на AsNeeded
            groupScrollArea->setMinimumHeight(60); // Меняем fixed на Minimum

            // Убираем системную рамку, чтобы лента сливалась с окном
            groupScrollArea->setFrameShape(QFrame::NoFrame);

            if (ui->groupContainer) {
                QVBoxLayout *groupLayout = new QVBoxLayout(ui->groupContainer);
                groupLayout->setContentsMargins(0, 0, 0, 0);
                groupLayout->addWidget(groupScrollArea);
            }

            // ==========================================================
            // --- 2. СКРОЛЛ-ЛЕНТА ДЛЯ СЕКТОРОВ (A1, A2...) ---
            // ==========================================================
            m_sectorTabBar = new QTabBar();
            m_sectorTabBar->setDrawBase(false);
            m_sectorTabBar->setExpanding(false);
            m_sectorTabBar->setUsesScrollButtons(false);
            // Стили удалены, чтобы фон наследовался от глобальной темы

            QScrollArea *sectorScrollArea = new QScrollArea();
            sectorScrollArea->setObjectName("sectorScrollArea");
            sectorScrollArea->setWidget(m_sectorTabBar);
            sectorScrollArea->setWidgetResizable(true);

            sectorScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            sectorScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded); // Меняем на AsNeeded
            sectorScrollArea->setMinimumHeight(60); // Меняем fixed на Minimum

            // Убираем системную рамку
            sectorScrollArea->setFrameShape(QFrame::NoFrame);
            sectorScrollArea->hide();

            if (ui->sectorContainer) {
                QVBoxLayout *sectorLayout = new QVBoxLayout(ui->sectorContainer);
                sectorLayout->setContentsMargins(0, 0, 0, 0);
                sectorLayout->addWidget(sectorScrollArea);
            }

            connect(m_sectorTabBar, &QTabBar::currentChanged, this, &DebuggerWindow::onSectorChanged);
            // ==========================================================
            // ==========================================================
}

DebuggerWindow::~DebuggerWindow()
{
    delete ui;
}

void DebuggerWindow::setDebugScene(QGraphicsScene *scene)
{
    if (!scene) return;

    ui->graphicsView->setScene(scene);

    // --- АЛГОРИТМ "УМНОЙ" КАМЕРЫ ---
    QRectF blocksRect;       // Здесь будем хранить область, где есть блоки
    bool hasItems = false;   // Флаг, нашли ли мы хоть что-то кроме сетки

    // Перебираем все предметы на сцене
    foreach (QGraphicsItem *item, scene->items()) {

        // Проверяем метку: 1 - это сетка (мы ставили это в MainWindow)
        // Если это НЕ сетка (значит блок или стрелка), учитываем её координаты
        if (item->data(0).toInt() != 1) {
            if (!hasItems) {
                blocksRect = item->sceneBoundingRect();
                hasItems = true;
            } else {
                // Расширяем область, чтобы включить этот предмет
                blocksRect = blocksRect.united(item->sceneBoundingRect());
            }
        }
    }

    // Если нашли блоки — центрируемся на них
    if (hasItems) {
        ui->graphicsView->centerOn(blocksRect.center());

        // (Опционально) Если хотите, чтобы схема сразу вся влезала в экран:
        // ui->graphicsView->fitInView(blocksRect, Qt::KeepAspectRatio);
    } else {
        // Если пусто — просто в центр
        ui->graphicsView->centerOn(0, 0);
    }

    // Принудительно обновляем вид
    ui->graphicsView->update();
    // Прячем железные кнопки по умолчанию (для режима Эмулятора)
        if (ui->btnTogglePort) ui->btnTogglePort->hide();
        if (ui->btnToggleWifi) ui->btnToggleWifi->hide();
    // ГЕНЕРИРУЕМ КНОПКИ ГРУПП
        setupGroupNavigation();
}
// --- ФУНКЦИЯ ЗУМА (МАСШТАБИРОВАНИЯ) ---
bool DebuggerWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Проверяем: если событие произошло в нашем graphicsView и это "Колесико мыши"
    if (watched == ui->graphicsView->viewport() && event->type() == QEvent::Wheel) {

        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

        // Определяем направление: Вверх (+) или Вниз (-)
        int angle = wheelEvent->angleDelta().y();

        // Коэффициент масштаба
        double scaleFactor = 1.15; // Скорость зума (чем больше число, тем резче)

        if (angle > 0) {
            // Крутим ВПЕРЕД -> Увеличиваем
            ui->graphicsView->scale(scaleFactor, scaleFactor);
        } else {
            // Крутим НАЗАД -> Уменьшаем
            ui->graphicsView->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        }

        // Возвращаем true -> значит "Мы обработали событие, не надо скроллить"
        return true;
    }

    // Во всех остальных случаях (клик, движение мыши) - работаем как обычно
    return QDialog::eventFilter(watched, event);
}

// Реализация установки языка
void DebuggerWindow::setLanguage(DebugLang lang)
{
    m_currentLang = lang;

    // Меняем заголовок окна для красоты
    if (m_currentLang == Lang_PYTHON) {
        setWindowTitle( tr("Визуальный Дебагер %1").arg("[Python Mode]"));
    }
    // --- ДОБАВЛЕНО ---
    else if (m_currentLang == Lang_JAVA) {        
        setWindowTitle( tr("Визуальный Дебагер %1").arg("[Java Mode]"));
    }
    // -----------------
    else {        
        setWindowTitle( tr("Визуальный Дебагер %1").arg("[C++ Mode]"));
    }
}
// Заготовка для запуска сессии
void DebuggerWindow::startDebugSession(const QString &code)
{
    // 1. ОПРЕДЕЛЯЕМ, КАКОЙ КОД ЗАПУСКАТЬ
    QString codeToRun = code;

    // Если нам передали пустоту (значит нажали кнопку Restart), берем из памяти
    if (codeToRun.isEmpty()) {
        codeToRun = m_lastCode;
    }

    // Если всё равно пусто — ошибка
    if (codeToRun.isEmpty()) {
        if (ui->logConsole) ui->logConsole->append("Error: No code generated!");
        return;
    }

    // ЗАПОМИНАЕМ КОД (чтобы кнопка Restart сработала в следующий раз)
    m_lastCode = codeToRun;


    // --- РАЗВИЛКА ЯЗЫКОВ ---
    if (m_currentLang == Lang_PYTHON) {

        if (ui->logConsole) ui->logConsole->append("--- WRITING SCRIPT ---");

        // СОХРАНЯЕМ КОД В ФАЙЛ
        QString scriptPath = QDir::tempPath() + "/debug_script.py";

        QFile file(scriptPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            // ВАЖНО: Пишем именно codeToRun, а не code!
            out << codeToRun;
            file.close();
            // Для проверки выведем начало кода в лог, чтобы убедиться, что там есть [DBG_BLOCK]
            if (ui->logConsole) ui->logConsole->append("Script saved. Preview:\n" + codeToRun.left(100) + "...\n");
        } else {
            if (ui->logConsole) ui->logConsole->append("Error: Could not save script file!");
            return;
        }

        // ЗАПУСКАЕМ PYTHON
        if (ui->logConsole) ui->logConsole->append("--- EXECUTING PYTHON ---");

        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished();
        }
        // 1. Получаем путь к папке библиотек
                QString libPath = QCoreApplication::applicationDirPath() + "/PythonLibraries";

                // 2. Настраиваем окружение (PYTHONPATH)
                QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                env.remove("PYTHONHOME");
                // Добавляем наш путь к существующим (через ; в Windows, в Linux через :)
                #ifdef Q_OS_WIN
                    env.insert("PYTHONPATH", libPath + ";" + env.value("PYTHONPATH"));
                #else
                    env.insert("PYTHONPATH", libPath + ":" + env.value("PYTHONPATH"));
                #endif

                m_process->setProcessEnvironment(env);
        // Запуск с флагом -u (обязательно!)
        m_process->start("python", QStringList() << "-u" << scriptPath);

        if (!m_process->waitForStarted(2000)) {
             m_process->start("python3", QStringList() << "-u" << scriptPath);
        }

    }
    else if (m_currentLang == Lang_CPP) {

        // === НОВАЯ ЛОГИКА ДЛЯ C++ ===
        QString exePath = codeToRun; // В C++ режиме здесь лежит ПУТЬ к exe файлу

        if (ui->logConsole) ui->logConsole->append("--- EXECUTING C++ BINARY ---");
        if (ui->logConsole) ui->logConsole->append("Path: " + exePath);

        // 1. Проверяем, существует ли файл
        if (!QFile::exists(exePath)) {
             if (ui->logConsole) ui->logConsole->append("Error: .exe file not found!");
             return;
        }

        // 2. Убиваем старый процесс, если есть
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished();
        }

        // 3. Запускаем EXE напрямую!
        m_process->start(exePath);

        if (!m_process->waitForStarted(2000)) {
             if (ui->logConsole) ui->logConsole->append("Error: Failed to start executable!");
        }
    }
    else if (m_currentLang == Lang_JAVA) {
            if (ui->logConsole) ui->logConsole->append("--- EXECUTING JAVA ---");

            // В случае Java переменная codeToRun содержит путь к .bat файлу
            // Мы просто запускаем его
            m_process->start(codeToRun);

            if (!m_process->waitForStarted(2000)) {
                 if (ui->logConsole) ui->logConsole->append("Error: Failed to start Java batch file!");
            }
        }
    // Сброс цветов перед запуском
    foreach (QGraphicsItem* item, m_itemMap.values()) {
        if (DiagramItem* b = dynamic_cast<DiagramItem*>(item)) {
            b->setBrush(Qt::white);
            b->clearError(); // Убираем старые треугольники
            b->update();
        }
        // 2. --- НОВОЕ: Если это СТРЕЛКА ---
                else if (ConnectionItem* c = dynamic_cast<ConnectionItem*>(item)) {
                    c->setHighlight(false); // Выключаем старую подсветку
                    c->setError(false);
                    c->setFalsePath(false); // <--- Сброс
                }
    }
    m_activeBlocks.clear(); // <--- Очищаем наш новый список вместо старой переменной
}
// --- ВСТАВИТЬ В КОНЕЦ debuggerwindow.cpp ---

void DebuggerWindow::onProcessOutputReady()
{
    // 1. Ошибки процесса выводим сразу (отдельно от буфера, чтобы не ломать строки)
    QByteArray errorData = m_process->readAllStandardError();
    if (!errorData.isEmpty()) {
        QString prefix = (m_currentLang == Lang_PYTHON) ? "PYTHON ERROR:\n" : "PROCESS ERROR:\n";
        if (ui->logConsole) {
            ui->logConsole->append("\n" + prefix + QString::fromLocal8Bit(errorData));
        }
    }

    // 2. Читаем стандартный вывод В БУФЕР (защита от разрывов пакетов)
    QByteArray data = m_process->readAllStandardOutput();
    m_processBuffer.append(data);

    // 3. Обрабатываем строго построчно
    while (m_processBuffer.contains('\n')) {
        int lineEndIndex = m_processBuffer.indexOf('\n');

        // Извлекаем одну чистую строку
        QByteArray lineBytes = m_processBuffer.left(lineEndIndex).trimmed();
        QString cleanLine = QString::fromLocal8Bit(lineBytes);

        // Удаляем обработанную часть из буфера
        m_processBuffer.remove(0, lineEndIndex + 1);

        if (cleanLine.isEmpty()) continue;

        // ========================================================

        // ========================================================

        // 1. БЛОКИ
        if (cleanLine.startsWith("[DBG_BLOCK]")) {
            QString idStr = cleanLine.mid(12).trimmed();
            highlightItem(idStr.toInt());
        }
        // 2. СТРЕЛКИ (Зеленые)
        else if (cleanLine.startsWith("[DBG_TRANSITION]")) {
                    QString idStr = cleanLine.mid(16).trimmed();
                    int id = idStr.toInt();
                    if (m_itemMap.contains(id)) {
                        if (ConnectionItem* arrow = dynamic_cast<ConnectionItem*>(m_itemMap[id])) {
                            // 1. Включаем ЗЕЛЕНЫЙ
                            arrow->setHighlight(true);

                            // 2. ВАЖНО: Выключаем КРАСНЫЙ (сбрасываем ошибку прошлого цикла)
                            arrow->setFalsePath(false);
                            arrow->setError(false);

                            // 3. --- НОВОЕ: ТУШИМ ПРЕДЫДУЩИЙ БЛОК ---
                                                        if (DiagramItem* startNode = arrow->startBlock()) {
                                                            startNode->setBrush(QColor(200, 255, 200)); // Оставляем бледно-зеленый след!
                                                            startNode->update();
                                                            m_activeBlocks.remove(startNode);
                                                        }
                        }
                    }
                }
        // 3. --- ОШИБКА НА СТРЕЛКЕ (Важно: проверять перед DBG_ERROR) ---
        else if (cleanLine.startsWith("[DBG_ERROR_ARROW]")) {
            QString rest = cleanLine.mid(18).trimmed();
            int spaceIndex = rest.indexOf(' ');
            if (spaceIndex != -1) {
                int arrowId = rest.left(spaceIndex).toInt();
                QString msg = rest.mid(spaceIndex + 1).trimmed();

                if (ui->logConsole) {
                    ui->logConsole->append("!!! ARROW ERROR !!! ID: " + QString::number(arrowId));
                    ui->logConsole->append("Msg: " + msg);
                }

                // Красим стрелку в КРАСНЫЙ + Треугольник
                if (m_itemMap.contains(arrowId)) {
                    if (ConnectionItem* arrow = dynamic_cast<ConnectionItem*>(m_itemMap[arrowId])) {
                         arrow->setError(true);
                         if (ui->cbSmartFocus->isChecked()) {
                                     ui->graphicsView->ensureVisible(arrow);
                                 }
                    }
                }
            }
        }
        // 4. ОШИБКА В БЛОКЕ
        else if (cleanLine.startsWith("[DBG_ERROR]")) {
            QString rest = cleanLine.mid(12).trimmed();
            int spaceIndex = rest.indexOf(' ');
            if (spaceIndex != -1) {
                int id = rest.left(spaceIndex).toInt();
                QString msg = rest.mid(spaceIndex + 1).trimmed();
                showError(id, msg); // Красит блок
            }
        }
        // 5. ЛОЖНЫЙ ПУТЬ (Красная стрелка без иконки)
        else if (cleanLine.startsWith("[DBG_FALSE]")) {
                    QString idStr = cleanLine.mid(11).trimmed();
                    int id = idStr.toInt();
                    if (m_itemMap.contains(id)) {
                        if (ConnectionItem* arrow = dynamic_cast<ConnectionItem*>(m_itemMap[id])) {
                            // 1. Включаем КРАСНЫЙ
                            arrow->setFalsePath(true);

                            // 2. ВАЖНО: Выключаем ЗЕЛЕНЫЙ (на всякий случай)
                            arrow->setHighlight(false);
                        }
                    }
                }
        // 6. --- НОВОЕ: ПРИНУДИТЕЛЬНАЯ ОЧИСТКА БЛОКА (Для Мостов) ---
                else if (cleanLine.startsWith("[DBG_CLEAR]")) {
                    QString idStr = cleanLine.mid(11).trimmed();
                    int id = idStr.toInt();
                    if (m_itemMap.contains(id)) {
                        if (DiagramItem* block = dynamic_cast<DiagramItem*>(m_itemMap[id])) {
                            block->setBrush(QColor(200, 255, 200)); // Оставляем бледно-зеленый след
                            block->update();
                            m_activeBlocks.remove(block); // Удаляем из списка активных
                        }
                    }
                }

        // Лог
        if (ui->logConsole) {
            ui->logConsole->append(cleanLine);
        }
    }
}
void DebuggerWindow::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (ui->logConsole) {
        ui->logConsole->append("--- SESSION FINISHED ---");
    }


    // Снимаем последнюю подсветку со всех
    for (DiagramItem* b : m_activeBlocks) {
            if (b) {
                b->setBrush(QColor(200, 255, 200)); // Фирменный бледно-зеленый след
                b->update();
            }
        }
        m_activeBlocks.clear();
}

void DebuggerWindow::highlightItem(int id)
{
    // --- ДИАГНОСТИКА ---
    if (ui->logConsole) {
        ui->logConsole->append(QString("DEBUG: Trying to highlight ID: %1").arg(id));
    }

    // 1. ПРЕДЫДУЩИЙ БЛОК БОЛЬШЕ НЕ ТУШИМ СРАЗУ!
    // (В параллельном режиме блок остается зеленым, пока из него не выйдет стрелка)

    // 2. Красим НОВЫЙ блок
    if (m_itemMap.contains(id)) {
        QGraphicsItem *item = m_itemMap[id];

        if (DiagramItem* b = dynamic_cast<DiagramItem*>(item)) {
            b->setBrush(Qt::green);
            b->update();

            // --- НОВОЕ: Добавляем блок в список активных (для параллельности) ---
            m_activeBlocks.insert(b);
            // --------------------------------------------------------------------

            if (ui->logConsole) ui->logConsole->append("-> SUCCESS: Block found and colored.");
        }

        // Двигаем камеру только если включен автофокус
        if (ui->cbSmartFocus->isChecked()) {
            ui->graphicsView->ensureVisible(item);
        }

    } else {
        // ЕСЛИ МЫ ПОПАЛИ СЮДА - ЗНАЧИТ КАРТА ПУСТАЯ ИЛИ ID НЕ СОВПАДАЮТ
        if (ui->logConsole) {
            ui->logConsole->append(QString("-> FAIL: Block ID %1 not found in map! (Map size: %2)")
                                   .arg(id).arg(m_itemMap.size()));
        }
    }
}
void DebuggerWindow::on_restartButton_clicked()
{
    if (ui->logConsole) {
        ui->logConsole->clear();
        ui->logConsole->append("--- RESTARTING ---");
    }
    startDebugSession("");
}
void DebuggerWindow::showError(int id, const QString &message)
{
    // 1. Находим виновный блок
    if (m_itemMap.contains(id)) {
        QGraphicsItem *item = m_itemMap[id];

        if (DiagramItem* b = dynamic_cast<DiagramItem*>(item)) {
                    // Используем новый метод, который и покрасит, и иконку включит
                    b->setError(message);

                    if (ui->cbSmartFocus->isChecked()) {
                                ui->graphicsView->ensureVisible(item);
                            }
                }
    }

    // 2. Пишем в лог ярко и заметно
    if (ui->logConsole) {
        ui->logConsole->append("!!! CRITICAL ERROR !!!");
        ui->logConsole->append("Block ID: " + QString::number(id));
        ui->logConsole->append("Message: " + message);
    }

    // (Опционально) Можно остановить процесс, если он еще жив
    if (m_process->state() != QProcess::NotRunning) {
        m_process->kill();
    }
}
// debuggerwindow.cpp

// debuggerwindow.cpp

void DebuggerWindow::externalSetCurrentBlock(int id)
{
    // МЫ БОЛЬШЕ НЕ СТИРАЕМ СТАРЫЙ БЛОК ЗДЕСЬ!
    // Благодаря этому параллельные потоки не будут гасить друг друга.

    if (m_itemMap.contains(id)) {
        QGraphicsItem *item = m_itemMap[id];
        if (DiagramItem *block = dynamic_cast<DiagramItem*>(item)) {
            block->setBrush(Qt::green); // Зажигаем активный блок конкретного потока
            block->update();

            // Фиксируем блок как активный, чтобы кнопка Stop могла корректно снять подсветку
            m_activeBlocks.insert(block);

            // Центрируем камеру, только если включен автофокус
            if (ui->cbSmartFocus->isChecked()) {
                ui->graphicsView->centerOn(block);
            }
        }
    }
}

void DebuggerWindow::externalHighlightArrow(int id, bool success)
{
    if (m_itemMap.contains(id)) {
        QGraphicsItem *item = m_itemMap[id];
        if (ConnectionItem *arrow = dynamic_cast<ConnectionItem*>(item)) {
            if (success) {
                arrow->setHighlight(true); // Зеленая

                // БЕЗОПАСНЫЙ ТАЙМЕР: Контекст 'this', захватываем 'id'
                QTimer::singleShot(500, this, [this, id]() {
                    // Проверяем, существует ли еще стрелка перед обращением
                    if (m_itemMap.contains(id)) {
                        if (ConnectionItem *a = dynamic_cast<ConnectionItem*>(m_itemMap[id])) {
                            a->setHighlight(false);
                        }
                    }
                });

                // Оставляем зеленый след на прошлом блоке
                if (DiagramItem* startNode = arrow->startBlock()) {
                    startNode->setBrush(QColor(200, 255, 200));
                    startNode->update();
                    m_activeBlocks.remove(startNode);
                }
            } else {
                arrow->setFalsePath(true); // Красная

                // БЕЗОПАСНЫЙ ТАЙМЕР: Контекст 'this', захватываем 'id'
                QTimer::singleShot(500, this, [this, id]() {
                    // Проверяем, существует ли еще стрелка перед обращением
                    if (m_itemMap.contains(id)) {
                        if (ConnectionItem *a = dynamic_cast<ConnectionItem*>(m_itemMap[id])) {
                            a->setFalsePath(false);
                        }
                    }
                });
            }
        }
    }
}


void DebuggerWindow::appendLog(const QString &message)
{
    // Выводим сообщения от железа прямо в визуальную консоль дебаггера!
    if (ui->logConsole) {
        ui->logConsole->append(message);
    }
}
void DebuggerWindow::setupGroupNavigation()
{
    // 1. Очищаем старые кнопки
    QLayoutItem *child;
    while ((child = m_navBar->layout()->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    // 2. Ищем все GroupItem на сцене
    QList<GroupItem*> groups;
    foreach (QGraphicsItem *item, ui->graphicsView->scene()->items()) {
        if (GroupItem *g = dynamic_cast<GroupItem*>(item)) {
            groups.append(g);
        }
    }

    // Сортируем по имени (Group 1, Group 2...)
    std::sort(groups.begin(), groups.end(), [](GroupItem* a, GroupItem* b){
        return a->name() < b->name();
    });

    if (groups.isEmpty()) return;

    // 3. Создаем кнопки
    foreach (GroupItem *g, groups) {
        QToolButton *btn = new QToolButton(this);
        btn->setText(g->name()); // "Group 1"
        btn->setCheckable(true);
        btn->setAutoExclusive(true); // Чтобы нажималась только одна
        btn->setMinimumWidth(80); // Задаем минимальную ширину кнопки, чтобы текст хорошо читался

        // ЛОГИКА НАЖАТИЯ: ЗУМ НА ГРУППУ
        connect(btn, &QToolButton::clicked, this, [this, g](){
            // Плавный зум на область группы
            ui->graphicsView->fitInView(g, Qt::KeepAspectRatio);

            // Немного отъехать назад, чтобы было комфортно (scale 0.9)
            ui->graphicsView->scale(0.9, 0.9);
        });

        m_navBar->layout()->addWidget(btn);
    }

}
void DebuggerWindow::setHardwareListener(HardwareListener* hwListener, const QString& portName)
{
    m_hwListener = hwListener;
    m_lastPortName = portName;

    // Сразу настраиваем правильный текст на кнопке при открытии окна
    if (m_hwListener && m_hwListener->isConnected()) {
        ui->btnTogglePort->setText(tr("Отключить порт"));
    } else {
        ui->btnTogglePort->setText(tr("Подключить порт"));
    }
    if (ui->btnTogglePort) ui->btnTogglePort->show();
}

void DebuggerWindow::on_btnTogglePort_clicked()
{
    if (!m_hwListener) return;

    if (m_hwListener->isConnected()) {
        // Отключаем порт, отдаем его Arduino IDE
        m_hwListener->disconnectPort();
        ui->btnTogglePort->setText(tr("Подключить порт"));

        if (ui->logConsole) {
            ui->logConsole->append(tr("\n[🔌] ПОРТ ОТКЛЮЧЕН."));
            ui->logConsole->append(tr("-> Вы можете свободно прошивать плату в Arduino IDE!\n"));
        }
    } else {
        // Подключаем порт обратно
        if (m_lastPortName.isEmpty()) return;

        if (ui->logConsole) ui->logConsole->append(tr("\n[🔌] Подключение к ") + m_lastPortName + "...");

        // Пытаемся подключиться (115200 - скорость из вашего Arduino кода)
        if (m_hwListener->connectPort(m_lastPortName, 115200)) {
            ui->btnTogglePort->setText("Отключить порт");
            if (ui->logConsole) ui->logConsole->append(tr("[✅] Порт успешно подключен! "));
        } else {
            if (ui->logConsole) ui->logConsole->append(tr("[❌] ОШИБКА: Не удалось открыть порт. Убедитесь, что COM-Port другой программы закрыта!"));
        }
    }
}
void DebuggerWindow::setHardwareWifiListener(HardwareWifiListener* hwWifiListener, const QString& ip, quint16 port)
{
    m_hwWifiListener = hwWifiListener;
    m_lastWifiIp = ip;
    m_lastWifiPort = port;

    // Сразу настраиваем правильный текст на кнопке при открытии окна
    // Проверяем, существует ли кнопка в UI (на случай, если вы ее удалите)
    if (ui->btnToggleWifi) {
        if (m_hwWifiListener && m_hwWifiListener->isConnected()) {
            ui->btnToggleWifi->setText(tr("Disconnect").arg("Wi-Fi"));
        } else {
            ui->btnToggleWifi->setText(tr("Connect").arg("Wi-Fi"));
        }
    }
    if (ui->btnToggleWifi) ui->btnToggleWifi->show();
}

void DebuggerWindow::on_stopButton_clicked()
{
    if (ui->logConsole) {
        ui->logConsole->append("\n🛑 --- Stop ---");
    }

    // 1. Убиваем локальный процесс на ПК (C++, Python, Java)
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(500);
        if (ui->logConsole) ui->logConsole->append("[PC] Локальный процесс завершен.");
    }

    // 2. Отключаем железо по USB
    if (m_hwListener && m_hwListener->isConnected()) {
        m_hwListener->disconnectPort();
        if (ui->btnTogglePort) ui->btnTogglePort->setText("Connect port");
        if (ui->logConsole) ui->logConsole->append("[USB] Connection Lost.");
    }

    // 3. Отключаем железо по Wi-Fi
    if (m_hwWifiListener && m_hwWifiListener->isConnected()) {
        m_hwWifiListener->disconnectDevice();
        if (ui->btnToggleWifi) ui->btnToggleWifi->setText("Connect Wi-Fi");
        if (ui->logConsole) ui->logConsole->append("[Wi-Fi] The connection was broken.");
    }

    // 4. Сбрасываем визуальную подсветку последнего блока
    // 4. Сбрасываем визуальную подсветку ВСЕХ активных блоков
    for (DiagramItem* b : m_activeBlocks) {
            if (b) {
                b->setBrush(QColor(200, 255, 200)); // Фирменный бледно-зеленый след
                b->update();
            }
        }
        m_activeBlocks.clear();
}

void DebuggerWindow::setSectors(const QStringList &names, const QList<QPointF> &centers)
{
    m_sectorCenters = centers;

    // Очищаем старые, если были
    while(m_sectorTabBar->count() > 0) m_sectorTabBar->removeTab(0);

    // Добавляем новые вкладки
    for (int i = 0; i < names.size(); ++i) {
        m_sectorTabBar->addTab(names[i]);
    }

    // Если сектора есть — показываем панель
    // Если сектора есть — показываем панель
        if (m_sectorTabBar->count() > 0) {
            m_sectorTabBar->show();

            // --- НОВОЕ: Находим и показываем сам контейнер с ползунком ---
            QScrollArea *sa = this->findChild<QScrollArea*>("sectorScrollArea");
            if (sa) sa->show();
            // --------------------------------------------------------------

            m_sectorTabBar->setCurrentIndex(0); // Ставим на первый
        }
}

void DebuggerWindow::onSectorChanged(int index)
{
    if (index >= 0 && index < m_sectorCenters.size()) {
        QPointF topLeft = m_sectorCenters[index];

        // Используем ту же магию выравнивания, что и в MainWindow
        // ВАЖНО: Убедись, что твой QGraphicsView в дебаггере называется ui->graphicsView.
        // Если он называется иначе (например, m_view), замени ui->graphicsView на свое название!

        qreal viewWidth = ui->graphicsView->viewport()->width();
        qreal viewHeight = ui->graphicsView->viewport()->height();
        qreal scale = ui->graphicsView->transform().m11();

        QPointF targetCenter(topLeft.x() + (viewWidth / 2.0) / scale,
                             topLeft.y() + (viewHeight / 2.0) / scale);

        ui->graphicsView->centerOn(targetCenter);
    }
}
void DebuggerWindow::setItemMap(const QMap<int, QGraphicsItem*> &map)
{
    m_itemMap = map;

    // --- ВИЗУАЛИЗАЦИЯ "скрытости" ---
    foreach (QGraphicsItem* item, m_itemMap.values()) {
        // Проверяем нашу метку
        bool isDebugEnabled = item->data(ROLE_DEBUG_ENABLED).toBool();

        if (!isDebugEnabled) {
            // Если дебаг отключен — делаем блок/стрелку полупрозрачной (40% видимости)
            item->setOpacity(0.4);
        } else {
            // Если включен — 100% видимость
            item->setOpacity(1.0);
        }
    }
}
// ==========================================================
// --- ОЧИСТКА ПРИ ЗАКРЫТИИ ОКНА ДЕБАГГЕРА ---
// ==========================================================
void DebuggerWindow::closeEvent(QCloseEvent *event)
{
    // 1. Останавливаем все процессы и освобождаем COM/Wi-Fi порты
    // (Имитируем нажатие на кнопку Stop, чтобы ничего не зависло в фоне)
    on_stopButton_clicked();

    // 2. Снимаем "Туман войны" и сбрасываем флаги на холсте
    foreach (QGraphicsItem* item, m_itemMap.values()) {
        if (item) {
            item->setOpacity(1.0); // Возвращаем 100% видимость
            item->setData(ROLE_DEBUG_ENABLED, true); // Сбрасываем метку
        }
    }

    // 3. Разрешаем окну закрыться
    QDialog::closeEvent(event);
}
void DebuggerWindow::on_btnToggleWifi_clicked()
{
    if (!m_hwWifiListener) return;

    if (m_hwWifiListener->isConnected()) {
        // Отключаем Wi-Fi
        m_hwWifiListener->disconnectDevice();
        if (ui->btnToggleWifi) ui->btnToggleWifi->setText("Connect Wi-Fi");

        if (ui->logConsole) {
            ui->logConsole->append("\n[📶] Wi-Fi DISCONNECTED.");
        }
    } else {
        // Подключаем Wi-Fi обратно
        if (m_lastWifiIp.isEmpty()) return;

        if (ui->logConsole) ui->logConsole->append("\n[📶] Connecting to " + m_lastWifiIp + ":" + QString::number(m_lastWifiPort) + "...");

        if (m_hwWifiListener->connectToDevice(m_lastWifiIp, m_lastWifiPort)) {
            if (ui->btnToggleWifi) ui->btnToggleWifi->setText("Disconnect Wi-Fi");
            if (ui->logConsole) ui->logConsole->append("[✅] Wi-Fi connected successfully!");
        } else {
            if (ui->logConsole) ui->logConsole->append("[❌] ERROR: Could not connect to Wi-Fi device.");
        }
    }
}
