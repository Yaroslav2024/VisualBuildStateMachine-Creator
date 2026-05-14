/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "mainwindow.h"  // Подключаем заголовок главного окна приложения
#include <QApplication>
#include <QSettings>
#include <QTranslator>
#include <QLocale>
#include <QThread>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>

int main(int argc, char *argv[])
{
    // Создаем объект QApplication — основной объект приложения Qt
    QApplication a(argc, argv);

    // ==========================================================
    // --- НОВОЕ: ЛОГИКА ЛОКАЛИЗАЦИИ ---
    // ==========================================================
    // 1. Читаем сохраненный язык из настроек (по умолчанию берем системный)
    QSettings settings("VisualBuild", "IDE");
    QString currentLang = settings.value("language", "en_US").toString();

    // 2. Создаем переводчик (объект должен существовать всё время работы программы)
    static QTranslator translator;

    // 3. Пытаемся загрузить файл перевода, если выбран английский
    if (currentLang.startsWith("en")) {
        // Ищем файл .qm в папке с исполняемым файлом
        if (translator.load("visualbuild_en.qm", QCoreApplication::applicationDirPath())) {
            a.installTranslator(&translator);
        }
    }

    // ==========================================================
    // --- 1. ЭКРАН ЗАГРУЗКИ (SPLASH SCREEN) ---
    // ==========================================================
    QWidget splash;
    // Убираем системные рамки и заставляем окно быть поверх всех остальных
    splash.setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    // Красивый стиль: белый фон, серая рамочка, закругленные углы
    splash.setStyleSheet("QWidget { background-color: #ffffff; border: 2px solid #bdc3c7; border-radius: 8px; }");

    // Горизонтальный слой: Картинка слева, Текст справа
    QHBoxLayout *splashLayout = new QHBoxLayout(&splash);
    splashLayout->setContentsMargins(30, 30, 40, 30);

    // --- ИКОНКА (Слева) ---
    QLabel *iconLabel = new QLabel();
    QPixmap pm(":/StateMachineicon.png");
    iconLabel->setPixmap(pm.scaled(480, 480, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setStyleSheet("border: none;");

    // --- ТЕКСТОВЫЙ БЛОК (Справа) ---
    QVBoxLayout *textLayout = new QVBoxLayout();

    // Оборачиваем заголовок в tr() для локализации
    QLabel *titleLabel = new QLabel(QObject::tr("VisualBuildStateMachine Creator"));
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2c3e50; border: none;");

    // Статус инициализации
    QLabel *statusLabel = new QLabel(QObject::tr("Initializing..."));
    statusLabel->setStyleSheet("font-size: 14px; color: #7f8c8d; border: none;");

    textLayout->addWidget(titleLabel);
    textLayout->addWidget(statusLabel);
    textLayout->setAlignment(Qt::AlignVCenter);

    splashLayout->addWidget(iconLabel);
    splashLayout->addLayout(textLayout);

    splash.show();
    // Заставляем Qt отрисовать окно немедленно
    a.processEvents();

    // ==========================================================
    // --- 2. ИМИТАЦИЯ ЗАГРУЗКИ ДАННЫХ ---
    // ==========================================================
    // Все текстовые статусы теперь поддерживают перевод через QObject::tr
    statusLabel->setText(QObject::tr("Loading graphics core..."));
    a.processEvents();
    QThread::msleep(350);

    statusLabel->setText(QObject::tr("Loading SCXML translator..."));
    a.processEvents();
    QThread::msleep(450);

    statusLabel->setText(QObject::tr("Initializing C++, Python, Java modules..."));
    a.processEvents();
    QThread::msleep(450);

    statusLabel->setText(QObject::tr("Checking ports and debugger..."));
    a.processEvents();
    QThread::msleep(350);

    statusLabel->setText(QObject::tr("Starting interface..."));
    a.processEvents();
    QThread::msleep(350);

    // ==========================================================
    // --- 3. ЗАПУСК ГЛАВНОГО ОКНА ---
    // ==========================================================
    MainWindow w;
    // Оборачиваем заголовок окна
    w.setWindowTitle(QObject::tr("VisualBuildStateMachine Creator v 1.0.0"));

    w.showMaximized();

    // Закрываем сплэш-скрин
    splash.close();

    return a.exec();
}
