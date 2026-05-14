/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "utils.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QStandardPaths>
#include "statetable.h"
#include <QGraphicsScene>
#include "grid.h"
#include <QApplication>
#include <QSettings>
#include <QAbstractButton>
#include <QPushButton>
#include <QRegularExpression>
static bool g_autoRoutingEnabled = false; // По умолчанию - ручное
static bool g_highlightAllErrors = false; // По умолчанию подсвечиваем только одну
static bool g_showYellowLabels = true; // По умолчанию включено
static QString g_lastCompileError = "";

QIcon Utils::createBlockIcon() {
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Светло-синий/серый блок с закругленными краями
    p.setBrush(QColor(230, 240, 255));
    p.setPen(QPen(Qt::black, 2));
    p.drawRoundedRect(4, 6, 24, 20, 4, 4);

    return QIcon(pix);
}

QIcon Utils::createBridgeIcon() {
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(Qt::white);

    // Левый и правый квадратики
    p.drawRect(2, 10, 6, 12);
    p.drawRect(24, 10, 6, 12);

    // Стрелка вправо ->
    p.drawLine(8, 16, 24, 16);
    p.drawLine(20, 13, 24, 16); // Усик 1
    p.drawLine(20, 19, 24, 16); // Усик 2

    return QIcon(pix);
}

QIcon Utils::createBackIcon() {
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(Qt::white);

    // Левый и правый квадратики
    p.drawRect(2, 8, 6, 16);
    p.drawRect(24, 8, 6, 16);

    // Верхняя стрелка вправо ->
    p.drawLine(8, 12, 24, 12);
    p.drawLine(20, 9, 24, 12);
    p.drawLine(20, 15, 24, 12);

    // Нижняя стрелка влево <-
    p.drawLine(8, 20, 24, 20);
    p.drawLine(12, 17, 8, 20);
    p.drawLine(12, 23, 8, 20);

    return QIcon(pix);
}

QIcon Utils::createCommentIcon() {
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Желтый листик
    p.setBrush(QColor(255, 255, 200));
    p.setPen(QPen(Qt::black, 1));

    // Полигон для листа с загнутым уголком
    QPolygon poly;
    poly << QPoint(6, 4) << QPoint(18, 4) << QPoint(26, 12) << QPoint(26, 28) << QPoint(6, 28);
    p.drawPolygon(poly);

    // Линия сгиба
    p.drawLine(18, 4, 18, 12);
    p.drawLine(18, 12, 26, 12);

    // Строки текста на стикере
    p.drawLine(10, 16, 22, 16);
    p.drawLine(10, 20, 22, 20);
    p.drawLine(10, 24, 18, 24);

    return QIcon(pix);
}

QIcon Utils::createGroupIcon() {
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Пунктирная рамка
    QPen dashedPen(Qt::black, 2, Qt::DashLine);
    p.setPen(dashedPen);
    p.setBrush(Qt::NoBrush);
    p.drawRect(4, 4, 24, 24);

    return QIcon(pix);
}

QIcon Utils::createHistoryIcon() {
    QPixmap histPix(32, 32);
    histPix.fill(Qt::transparent);
    QPainter histPainter(&histPix);
    histPainter.setRenderHint(QPainter::Antialiasing);

    // Светло-зеленый фон
    histPainter.setBrush(QColor(235, 255, 235));
    histPainter.setPen(QPen(Qt::black, 2));
    histPainter.drawEllipse(2, 2, 28, 28);

    // Буква H
    histPainter.setPen(Qt::black);
    histPainter.setFont(QFont("Arial", 14, QFont::Bold));
    histPainter.drawText(histPix.rect(), Qt::AlignCenter, "H");

    return QIcon(histPix);
}
QIcon Utils::createPreviewIcon() {
    QPixmap pix(32, 32);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    // Основа: белый лист
    p.setBrush(Qt::white);
    p.setPen(QPen(Qt::black, 2));

    // Рисуем контур листа с "отрезанным" правым верхним углом
    QPolygon poly;
    poly << QPoint(6, 2)
         << QPoint(20, 2)
         << QPoint(26, 8)
         << QPoint(26, 30)
         << QPoint(6, 30);
    p.drawPolygon(poly);

    // Рисуем линию загнутого уголка
    p.drawLine(20, 2, 20, 8);
    p.drawLine(20, 8, 26, 8);

    // Рисуем линии текста (имитация строк кода)
    p.setPen(QPen(Qt::gray, 2));
    p.drawLine(10, 12, 22, 12);
    p.drawLine(10, 16, 18, 16); // Короткая строка (отступ)
    p.drawLine(10, 20, 22, 20);
    p.drawLine(10, 24, 16, 24); // Короткая строка

    return QIcon(pix);
}
//
//Library
//
void Utils::openFolder(const QString &folderName)
{
    QString path = QCoreApplication::applicationDirPath() + "/" + folderName;
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

QStringList Utils::getLibraryFiles(const QString &folderName, const QStringList &nameFilters)
{
    QString path = QCoreApplication::applicationDirPath() + "/" + folderName;
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.entryList(nameFilters, QDir::Files);
}

QString Utils::findJavaTool(const QString &toolName, QWidget *parent, bool silent)
{
    QString exeName = toolName + ".exe";
    QString foundPath = "";

    // 1. ПРОВЕРКА "PORTABLE"
    QString localPath = QCoreApplication::applicationDirPath() + "/jdk/bin/" + exeName;
    if (QFile::exists(localPath)) {
        foundPath = localPath;
    }

    // 2. ПРОВЕРКА СТАНДАРТНЫХ ПУТЕЙ
    if (foundPath.isEmpty()) {
        QStringList searchRoots;
        searchRoots << QDir::homePath() + "/AppData/Local/Programs/Eclipse Adoptium"
                    << QDir::homePath() + "/AppData/Local/Programs/Java"
                    << "C:/Program Files/Eclipse Adoptium"
                    << "C:/Program Files/Amazon Corretto"
                    << "C:/Program Files/Java"
                    << "C:/Program Files (x86)/Java";

        for (const QString &root : searchRoots) {
            QDir dir(root);
            if (dir.exists()) {
                QStringList versions = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                if (!versions.isEmpty()) {
                    versions.sort();
                    QString latest = versions.last();
                    QString candidate = root + "/" + latest + "/bin/" + exeName;
                    if (QFile::exists(candidate)) {
                        foundPath = candidate;
                        break;
                    }
                }
            }
        }
    }

    // 3. ПРОВЕРКА SYSTEM PATH
    if (foundPath.isEmpty()) {
        QString systemPath = QStandardPaths::findExecutable(toolName);
        if (!systemPath.isEmpty()) {
            foundPath = systemPath;
        }
    }

    // ======================================================
    // --- ГЛАВНОЕ ИСПРАВЛЕНИЕ: БОЛЬШЕ НИКАКИХ ВОПРОСОВ! ---
    // Если путь найден автоматически — просто возвращаем его молча.
    // ======================================================
    if (!foundPath.isEmpty()) {
        return foundPath;
    }

    // Если ничего не нашли, но включен тихий режим (например, при "Автопоиске" в настройках) — выходим
    if (silent) {
        return "";
    }

    // --- ЭТАП 4: РУЧНОЙ ПОИСК ---
    // Сюда программа дойдет ТОЛЬКО если Java вообще нигде нет
    QMessageBox::StandardButton manualReply;
    QString msg = QObject::tr("Java не найдена автоматически. Найти файл %1 вручную?").arg(exeName);
        manualReply = QMessageBox::question(parent, QObject::tr("Поиск вручную"), msg, QMessageBox::Yes | QMessageBox::No);

        if (manualReply == QMessageBox::Yes) {
            QString manualPath = QFileDialog::getOpenFileName(parent,
                                    QObject::tr("Выберите файл %1").arg(exeName),
                                    "C:/Program Files",
                                    QObject::tr("Executables (*.exe);;All Files (*)"));

        if (!manualPath.isEmpty()) return manualPath;
    }

    return "";
}

bool Utils::compileJava(const QString &sourceCode, const QString &tempDir, QWidget *parent)
{
    g_lastCompileError = ""; // Сброс старой ошибки
    // 1. Читаем путь из настроек Toolchains
    QSettings settings("VisualBuild", "IDE");
    QString javacPath = settings.value("compilerPathJava", "javac").toString();

    // 2. ПРОВЕРКА НАЛИЧИЯ КОМПИЛЯТОРА (Защита от удаленных или неверных путей)
    bool isCompilerValid = false;
    if (QFileInfo(javacPath).isAbsolute()) {
        isCompilerValid = QFile::exists(javacPath); // Проверяем точный путь (например, C:/Java/javac.exe)
    } else {
        // Проверяем системный PATH (если в настройках стоит просто "javac")
        isCompilerValid = !QStandardPaths::findExecutable(javacPath).isEmpty();
    }

    // 3. Если компилятор по указанному пути не найден (системный javac теперь работает сразу!)
    if (!isCompilerValid) {
        // Запускаем ваш умный поиск (который проверит папки и сам спросит про ручной выбор)
        javacPath = findJavaTool("javac", parent);

        // Если умный поиск нашел компилятор, СРАЗУ СОХРАНЯЕМ его в настройки,
        // чтобы больше никогда не спрашивать пользователя!
        if (!javacPath.isEmpty()) {
            settings.setValue("compilerPathJava", javacPath);
        }
    }

    // 4. Если даже умный поиск ничего не дал (пользователь отказался искать вручную)
    if (javacPath.isEmpty()) {
        QMessageBox::StandardButton reply;

        reply = QMessageBox::question(parent, QObject::tr("Java не найдена"),
                    QObject::tr("Для симуляции Java-кода требуется JDK (Java Development Kit).\n"
                    "Указанный компилятор не найден или удален.\n\n"
                    "Хотите открыть страницу загрузки JDK?"),
                    QMessageBox::Yes | QMessageBox::No);


        if (reply == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl("https://adoptium.net/temurin/releases/"));
        }
        return false; // Отменяем генерацию
    }

    // --- Далее идет ваш оригинальный код компиляции ---
    QString javaFile = tempDir + "/DebugRobot.java";
    QFile file(javaFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&file);
    out << sourceCode;
    file.close();

    QFile::remove(tempDir + "/DebugRobot.class");
    QFile::remove(tempDir + "/DebugRobot$SerialMock.class");

    QProcess compiler;
    compiler.setWorkingDirectory(tempDir);

    QStringList args;
    args << "-encoding" << "UTF-8";
    QString libPath = QCoreApplication::applicationDirPath() + "/JavaLibraries";
    QString classPath = ".;" + libPath + "/*";
    args << "-cp" << classPath;
    args << "DebugRobot.java";

    // Запускаем 100% проверенный рабочий путь
    compiler.start(javacPath, args);

    if (!compiler.waitForFinished(10000)) {
        QMessageBox::critical(parent, QObject::tr("Ошибка"), QObject::tr("Компилятор завис."));

        return false;
    }

    if (compiler.exitCode() != 0) {
        QString err = QString::fromLocal8Bit(compiler.readAllStandardError());
        g_lastCompileError = err; // Сохранение новой ошибки
        QMessageBox::warning(parent, QObject::tr("Ошибка компиляции"), err);
        return false;
    }
    return true;
}

bool Utils::compileCpp(const QString &sourceCode, const QString &exePath, QWidget *parent)
{
    g_lastCompileError = ""; // Сброс старой ошибки
    QString sourcePath = QDir::tempPath() + "/temp_debug_src.cpp";

    QFile sourceFile(sourcePath);
    if (!sourceFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(parent, QObject::tr("Ошибка системы"), QObject::tr("Не удалось создать временный файл:\n%1").arg(sourcePath));
        return false;
    }
    QTextStream out(&sourceFile);
    out << sourceCode;
    sourceFile.close();

    if (QFile::exists(exePath)) {
        if (!QFile::remove(exePath)) {
            QMessageBox::warning(parent, QObject::tr("Файл занят"),
                        QObject::tr("Невозможно запустить новую симуляцию, так как предыдущая еще работает.\n\n"
                        "Пожалуйста, закройте открытое окно 'Визуальный Дебагер' и попробуйте снова."));
            return false;
        }
    }

    QProcess compiler;
    QString libPath = QCoreApplication::applicationDirPath() + "/CppLibraries";

    QStringList args;
    args << "-std=c++17";
    args << sourcePath;
    args << "-o" << exePath;
    args << "-I" << libPath;

    // Читаем путь из настроек Toolchains
    QSettings settings("VisualBuild", "IDE");
    QString compilerCmd = settings.value("compilerPathCpp", "g++").toString();

    // ========================================================
    // --- ПРОВЕРКА СУЩЕСТВОВАНИЯ КОМПИЛЯТОРА ---
    // ========================================================
    bool isCompilerValid = false;
    if (QFileInfo(compilerCmd).isAbsolute()) {
        isCompilerValid = QFile::exists(compilerCmd); // Точный путь (например, C:/MinGW/bin/g++.exe)
    } else {
        isCompilerValid = !QStandardPaths::findExecutable(compilerCmd).isEmpty(); // Системная команда ("g++")
    }

    if (!isCompilerValid) {
        QMessageBox msgBox(parent);
        msgBox.setWindowTitle(QObject::tr("C++ (MinGW) не найден"));
        msgBox.setText(QObject::tr("Для симуляции C++ кода требуется компилятор (g++)."));
        msgBox.setInformativeText(QObject::tr("Если он уже установлен на вашем ПК, нажмите «Указать путь». Если нет — скачайте MinGW."));
        msgBox.setIcon(QMessageBox::Warning);

        // Создаем свои кнопки
        // Создаем свои кнопки (ИСПРАВЛЕНО: QPushButton заменено на QAbstractButton)
        // Создаем свои кнопки
                QPushButton *browseBtn = msgBox.addButton(QObject::tr("Указать путь..."), QMessageBox::ActionRole);
                QPushButton *downloadBtn = msgBox.addButton(QObject::tr("Скачать MinGW"), QMessageBox::ActionRole);
                msgBox.addButton(QObject::tr("Отмена"), QMessageBox::RejectRole);



        msgBox.exec();

        // Обрабатываем выбор пользователя
        if (msgBox.clickedButton() == browseBtn) {
            QString newPath = QFileDialog::getOpenFileName(parent, QObject::tr("Выберите исполняемый файл g++.exe"), "C:/", QObject::tr("Executables (*.exe);;All Files (*)"));
            if (!newPath.isEmpty()) {
                // Мгновенно сохраняем новый путь в настройки
                settings.setValue("compilerPathCpp", newPath);
                QMessageBox::information(parent, QObject::tr("Сохранено"), QObject::tr("Путь к компилятору успешно сохранен!\n\nПожалуйста, запустите симуляцию еще раз."));
            }
        } else if (msgBox.clickedButton() == downloadBtn) {
            QDesktopServices::openUrl(QUrl("https://code.visualstudio.com/docs/cpp/config-mingw"));
        }

        return false; // Прерываем компиляцию без зависаний
    }
    // ========================================================

    // Запускаем проверенный компилятор
    compiler.start(compilerCmd, args);

    if (!compiler.waitForFinished(10000)) {
        // Слегка изменил текст, так как ошибка теперь может быть не только в PATH
        QMessageBox::critical(parent, QObject::tr("Ошибка компилятора"),
                              QObject::tr("Компилятор не отвечает или завис.\nУбедитесь, что MinGW установлен корректно."));
        return false;
    }

    if (compiler.exitCode() != 0) {
        QByteArray errData = compiler.readAllStandardError();
        g_lastCompileError = QString::fromLocal8Bit(errData); // <--- ДОБАВИТЬ ЭТУ СТРОКУ
        QString errStr = QString::fromLocal8Bit(errData);
        QMessageBox::warning(parent, QObject::tr("Ошибка компиляции C++"),
                             QObject::tr("Компилятор нашел ошибки:\n\n%1").arg(errStr));
        return false;
    }
    return true;
}

bool Utils::checkPythonSyntax(const QString &filePath, QWidget *parent)
{
    g_lastCompileError = ""; // Сброс старой ошибки
    // 1. Читаем путь из настроек
    QSettings settings("VisualBuild", "IDE");
    QString pyCmd = settings.value("compilerPathPython", "python").toString();

    // ========================================================
    // --- ПРОВЕРКА СУЩЕСТВОВАНИЯ PYTHON ---
    // ========================================================
    bool isPythonValid = false;
    if (QFileInfo(pyCmd).isAbsolute()) {
        isPythonValid = QFile::exists(pyCmd); // Точный путь (например, C:/Python39/python.exe)
    } else {
        isPythonValid = !QStandardPaths::findExecutable(pyCmd).isEmpty(); // Системная команда ("python")
    }

    if (!isPythonValid) {
        QMessageBox msgBox(parent);
        msgBox.setWindowTitle(QObject::tr("Python не найден"));
                msgBox.setText(QObject::tr("Для симуляции и проверки синтаксиса требуется Python."));
                msgBox.setInformativeText(QObject::tr("Если он уже установлен, нажмите «Указать путь». Если нет — скачайте его с официального сайта."));
                // ...
        msgBox.setIcon(QMessageBox::Warning);

        // Создаем свои кнопки
        QPushButton *browseBtn = msgBox.addButton(QObject::tr("Указать путь..."), QMessageBox::ActionRole);
                QPushButton *downloadBtn = msgBox.addButton(QObject::tr("Скачать Python"), QMessageBox::ActionRole);
                msgBox.addButton(QObject::tr("Отмена"), QMessageBox::RejectRole);

        msgBox.exec();

        if (msgBox.clickedButton() == browseBtn) {
            QString newPath = QFileDialog::getOpenFileName(parent, QObject::tr("Выберите исполняемый файл python.exe"), "C:/", QObject::tr("Executables (*.exe);;All Files (*)"));
            if (!newPath.isEmpty()) {
                // Мгновенно сохраняем новый путь в настройки
                settings.setValue("compilerPathPython", newPath);
                QMessageBox::information(parent, QObject::tr("Сохранено"), QObject::tr("Путь к Python успешно сохранен!\n\nПожалуйста, запустите симуляцию еще раз."));
            }
        } else if (msgBox.clickedButton() == downloadBtn) {
            QDesktopServices::openUrl(QUrl("https://www.python.org/downloads/"));
        }

        return false; // Прерываем процесс, чтобы не было вылетов
    }
    // ========================================================

    // ========================================================
    // ЭТАП 1: Базовая проверка (py_compile)
    // ========================================================
    QProcess process;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("PYTHONHOME");
    process.setProcessEnvironment(env);

    // Запускаем проверенный python
    process.start(pyCmd, QStringList() << "-m" << "py_compile" << filePath);

    process.waitForFinished();

    if (process.exitCode() != 0) {
        QString err = process.readAllStandardError();
        g_lastCompileError = err; // Сохранение базовой ошибки

        QMessageBox::warning(parent, QObject::tr("Синтаксическая ошибка"),
                             QObject::tr("Python не может прочитать этот код.\n%1").arg(err));
        return false;
    }

    // ========================================================
    // ЭТАП 2: Умный AST Сканер (Через временный файл)
    // ========================================================

    // 1. Создаем временный файл для скрипта-анализатора
    QString scannerScriptPath = QDir::tempPath() + "/ast_checker_temp.py";
    QFile scriptFile(scannerScriptPath);

    if (scriptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&scriptFile);
        // Мы пишем нормальный Python код, без сжатия в одну строку
        out << "import ast, sys\n";
        out << "try:\n";
        out << "    # Читаем файл пользователя (кодировка utf-8 важна!)\n";
        out << "    with open(sys.argv[1], 'r', encoding='utf-8') as f:\n";
        out << "        code = f.read()\n";
        out << "    tree = ast.parse(code)\n";
        out << "    # Ищем деление на ноль\n";
        out << "    for node in ast.walk(tree):\n";
        out << "        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Div):\n";
        out << "            if isinstance(node.right, ast.Constant) and node.right.value == 0:\n";
        out << "                print(f'Line {node.lineno}: Division by Zero (0) detected!')\n";
        out << "                sys.exit(1)\n";
        out << "except Exception as e:\n";
        out << "    print(f'Scan Error: {e}')\n";
        out << "    sys.exit(1)\n";
        scriptFile.close();
    }

    // 2. Запускаем этот скрипт и натравливаем его на файл пользователя
    QProcess scanner;
    // Запускаем пользовательский проверенный python
    scanner.start(pyCmd, QStringList() << scannerScriptPath << filePath);

    scanner.waitForFinished();

    // 3. Если скрипт нашел ошибку (код выхода 1)
    if (scanner.exitCode() != 0) {
        QString output = scanner.readAllStandardOutput();
        // Если пусто в Output, читаем Error (на всякий случай)
        if (output.isEmpty()) output = scanner.readAllStandardError();
        g_lastCompileError = output; // Сохранение ошибки AST
        // ИЗМЕНЕНИЕ: this -> parent

        QMessageBox::warning(parent, QObject::tr("Логическая ошибка"),
                             QObject::tr("Система защиты обнаружила проблему:\n\n%1"
                                         "\n\nЗапуск отменен.").arg(output));
        return false;
    }

    return true; // Все чисто
}
bool Utils::checkCppSyntax(const QString &sourceCode, const QString &headerCode, const QString &headerName, QWidget *parent)
{
    g_lastCompileError = "";
    QSettings settings("VisualBuild", "IDE");
    QString compilerCmd = settings.value("compilerPathCpp", "g++").toString();

    bool isCompilerValid = false;
    if (QFileInfo(compilerCmd).isAbsolute()) isCompilerValid = QFile::exists(compilerCmd);
    else isCompilerValid = !QStandardPaths::findExecutable(compilerCmd).isEmpty();

    // Если компилятор не установлен, пропускаем проверку (возвращаем true), чтобы разрешить сохранение
    if (!isCompilerValid) return true;

    QString tempDir = QDir::tempPath() + "/VisualBuildTemp";
    QDir().mkpath(tempDir);

    QFile hFile(tempDir + "/" + headerName);
    if (hFile.open(QIODevice::WriteOnly | QIODevice::Text)) { QTextStream out(&hFile); out << headerCode; hFile.close(); }

    QFile cppFile(tempDir + "/temp_check.cpp");
    if (cppFile.open(QIODevice::WriteOnly | QIODevice::Text)) { QTextStream out(&cppFile); out << sourceCode; cppFile.close(); }

    QProcess compiler;
    compiler.setWorkingDirectory(tempDir);
    QString libPath = QCoreApplication::applicationDirPath() + "/CppLibraries";

    // Флаг -fsyntax-only делает проверку мгновенной, не собирая .exe файл!
    compiler.start(compilerCmd, QStringList() << "-std=c++17" << "-fsyntax-only" << "temp_check.cpp" << "-I" << libPath);
    compiler.waitForFinished();

    if (compiler.exitCode() != 0) {
        g_lastCompileError = QString::fromLocal8Bit(compiler.readAllStandardError());
        return false;
    }
    return true;
}

bool Utils::checkJavaSyntax(const QString &sourceCode, const QString &className, QWidget *parent)
{
    g_lastCompileError = "";
    QString javacPath = findJavaTool("javac", parent, true); // Тихий режим!

    if (javacPath.isEmpty()) return true; // Пропускаем проверку, если нет Java

    QString tempDir = QDir::tempPath() + "/VisualBuildTempJava";
    QDir().mkpath(tempDir);
    QString javaFile = tempDir + "/" + className + ".java";

    QFile f(javaFile);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) { QTextStream out(&f); out << sourceCode; f.close(); }

    QProcess compiler;
    compiler.setWorkingDirectory(tempDir);
    QString libPath = QCoreApplication::applicationDirPath() + "/JavaLibraries";
    QString classPath = ".;" + libPath + "/*";

    compiler.start(javacPath, QStringList() << "-encoding" << "UTF-8" << "-cp" << classPath << (className + ".java"));
    compiler.waitForFinished();

    if (compiler.exitCode() != 0) {
        g_lastCompileError = QString::fromLocal8Bit(compiler.readAllStandardError());
        return false;
    }
    return true;
}
// Возвращает задержку в мс на основе выбранного индекса комбобокса
int Utils::getSpeedInterval(int index) {
    switch(index) {
        case 0: return 0;    // Бесконечный цикл
        case 1: return 100;
        case 2: return 200;  // По умолчанию
        case 3: return 500;
        case 4: return 1000;
        default: return 200;
    }
}

// Открывает и обновляет окно таблицы переходов
void Utils::openTransitionTable(StateTable* table, QGraphicsScene* scene) {
    if (table && scene) {
        table->updateFromScene(scene);
        table->show();
        table->raise();
        table->activateWindow();
    }
}
void Utils::toggleGrid(Grid* grid, bool visible) {
    if (grid) {
        grid->setVisible(visible);
    }
}
void Utils::applyTheme(bool isDark) {
    if (isDark) {
        // --- ОБНОВЛЕННЫЙ ТЁМНЫЙ СТИЛЬ (Светло-серый дебаггер, черные группы) ---
        qApp->setStyleSheet(
            /* Основной темный фон для всего приложения */
            "QMainWindow, QDockWidget { background-color: #2b2b2b; color: #f0f0f0; }"
            "QWidget { background-color: #2b2b2b; color: #f0f0f0; }"
            "QMenuBar { background-color: #1e1e1e; color: #f0f0f0; }"
            "QMenuBar::item:selected { background-color: #3e3e3e; }"
            "QMenu { background-color: #1e1e1e; color: #f0f0f0; border: 1px solid #3e3e3e; }"
            "QMenu::item:selected { background-color: #3e3e3e; }"

            /* --- СПЕЦИАЛЬНО ДЛЯ DEBUGGER WINDOW --- */
            /* Находим окно дебаггера по имени класса и ставим светло-серый фон */
            "QDialog[objectName^=\"DebuggerWindow\"] { background-color: #d3d3d3; color: #000; }"

            /* Стили для GroupBox внутри дебаггера, чтобы они выделялись на светло-сером */
            "QDialog[objectName^=\"DebuggerWindow\"] QGroupBox { background-color: #2b2b2b; border: 1px solid #555; margin-top: 1.5ex; border-radius: 3px; color: #f0f0f0; }"
            "QDialog[objectName^=\"DebuggerWindow\"] QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 5px; color: #f0f0f0; }"

            /* Кнопки в дебаггере (переключатели групп) - СДЕЛАЛИ ЧЕРНЫМИ со светло-серым текстом */
                    /* ВСЕ Кнопки в дебаггере (QToolButton и обычные QPushButton) - ЧЕРНЫЕ */
                    /* ВСЕ Кнопки в дебаггере (QToolButton и обычные QPushButton) - СЕРЫЕ */
                                "QDialog[objectName^=\"DebuggerWindow\"] QToolButton, QDialog[objectName^=\"DebuggerWindow\"] QPushButton { background-color: #3e3e3e; color: #f0f0f0; border: 1px solid #555; padding: 4px; border-radius: 3px; font-weight: bold; }"
                                "QDialog[objectName^=\"DebuggerWindow\"] QToolButton:hover, QDialog[objectName^=\"DebuggerWindow\"] QPushButton:hover { background-color: #505050; }"

                                /* Выделение активной (выбранной) группы/сектора */
                                "QDialog[objectName^=\"DebuggerWindow\"] QToolButton:checked, "
                                "QDialog[objectName^=\"DebuggerWindow\"] QPushButton:checked { background-color: #0060c0; border: 1px solid #0060c0; color: white; }"
                                "QDialog[objectName^=\"DebuggerWindow\"] QPushButton:pressed { background-color: #1e1e1e; }"
            // ----------------------------------------

            /* Стили для вкладок и остальных кнопок вне дебаггера */
            "QPushButton { background-color: #3e3e3e; color: #f0f0f0; border: 1px solid #555; padding: 5px; border-radius: 3px; }"
            "QPushButton:hover { background-color: #505050; }"
            "QPushButton:pressed { background-color: #1e1e1e; }"

            "QTabBar::tab { background-color: #3e3e3e; color: #f0f0f0; border: 1px solid #555; padding: 6px 15px; margin-right: 2px; border-radius: 3px; }"
            "QTabBar::tab:selected { background-color: #0060c0; color: white; border: 1px solid #0060c0; }"

            "QLineEdit, QTextEdit, QComboBox { background-color: #1e1e1e; color: #f0f0f0; border: 1px solid #555; padding: 3px; }"
            "QGraphicsView { background-color: #1e1e1e; border: none; }"
            "QTableWidget { background-color: #1e1e1e; color: #f0f0f0; gridline-color: #3e3e3e; }"
            "QTreeView { background-color: #1e1e1e; color: #f0f0f0; }"
            "QTreeView::item:selected { background-color: #0060c0; color: white; }"
        );
    } else {
        // Сбрасываем стили на стандартную системную (Светлую) тему
        qApp->setStyleSheet("");
    }
}
bool Utils::showYellowLabels() {
    return g_showYellowLabels;
}

void Utils::setShowYellowLabels(bool show) {
    g_showYellowLabels = show;
}
bool Utils::isAutoRoutingEnabled() {
    return g_autoRoutingEnabled;
}

void Utils::setAutoRoutingEnabled(bool enable) {
    g_autoRoutingEnabled = enable;
}
QString Utils::getLastCompileError() {
    return g_lastCompileError;
}

bool Utils::highlightAllErrors() { return g_highlightAllErrors; }
void Utils::setHighlightAllErrors(bool enable) { g_highlightAllErrors = enable; }

QList<int> Utils::getLastErrorLines() {
    QList<int> lines;
    if (g_lastCompileError.isEmpty()) return lines;

    // 1. Формат C++ (MinGW/Arduino) и Java: "file.cpp:45:10: error:" или "file.java:45: error:"
    QRegularExpression rxCpp(":(\\d+):\\d+: error:|:(\\d+): error:");
    QRegularExpressionMatchIterator iCpp = rxCpp.globalMatch(g_lastCompileError);
    while (iCpp.hasNext()) {
        QRegularExpressionMatch match = iCpp.next();
        QString lineStr = match.captured(1).isEmpty() ? match.captured(2) : match.captured(1);
        int lineNum = lineStr.toInt();
        if (!lines.contains(lineNum)) lines.append(lineNum); // Защита от дубликатов

        if (!g_highlightAllErrors) return lines; // Если нужна только одна ошибка - сразу выходим
    }
    if (!lines.isEmpty()) return lines; // Если нашли ошибки C++, дальше не ищем

    // 2. Формат Python (базовый): "line 45"
    QRegularExpression rxPy("line (\\d+)");
    QRegularExpressionMatchIterator iPy = rxPy.globalMatch(g_lastCompileError);
    while (iPy.hasNext()) {
        QRegularExpressionMatch match = iPy.next();
        int lineNum = match.captured(1).toInt();
        if (!lines.contains(lineNum)) lines.append(lineNum);

        if (!g_highlightAllErrors) return lines;
    }
    if (!lines.isEmpty()) return lines;

    // 3. Формат Python (Наш умный сканер AST): "Line 45:"
    QRegularExpression rxAst("Line (\\d+):");
    QRegularExpressionMatchIterator iAst = rxAst.globalMatch(g_lastCompileError);
    while (iAst.hasNext()) {
        QRegularExpressionMatch match = iAst.next();
        int lineNum = match.captured(1).toInt();
        if (!lines.contains(lineNum)) lines.append(lineNum);

        if (!g_highlightAllErrors) return lines;
    }

    return lines; // Возвращаем пустой список, если ошибки не распознаны
}
