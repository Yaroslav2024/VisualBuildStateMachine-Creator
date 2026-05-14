/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "librarymanagerdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>

LibraryManagerDialog::LibraryManagerDialog(const QString &targetDir, QWidget *parent) {
    setWindowTitle(tr("🌐 Онлайн Каталог Библиотек"));
    resize(750, 450);
    setupUi();

    // Подключаем сигналы от нашего NetworkManager
    connect(&NetworkManager::instance(), &NetworkManager::catalogFetched, this, &LibraryManagerDialog::onCatalogFetched);
    connect(&NetworkManager::instance(), &NetworkManager::libraryDownloaded, this, &LibraryManagerDialog::onLibraryDownloaded);

}

void LibraryManagerDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Верхушка: Статус и кнопка обновления
    QHBoxLayout *topLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(tr("Готово"), this);
    m_statusLabel->setStyleSheet("font-weight: bold; color: #333;");

    // --- НОВОЕ: СОЗДАЕМ ПОЛЕ ВВОДА URL ---
    m_urlInput = new QLineEdit(this);
    m_urlInput->setPlaceholderText(tr("Вставьте URL каталога (JSON)..."));

    // -------------------------------------

    m_refreshBtn = new QPushButton(tr("🔄 Обновить список"), this);
    connect(m_refreshBtn, &QPushButton::clicked, this, &LibraryManagerDialog::refreshCatalog);

    // === ИСПРАВЛЕНО ЗДЕСЬ ===
    topLayout->addWidget(m_statusLabel);
    topLayout->addWidget(m_urlInput, 1); // Цифра 1 заставит поле ввода растягиваться на всё свободное место!
    topLayout->addWidget(m_refreshBtn);
    // ========================

    mainLayout->addLayout(topLayout);

    // Главная часть: Таблица
    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({tr("Название"), tr("Язык"), tr("Описание"), tr("Установка")});
    // Растягиваем колонку "Описание", чтобы она занимала всё свободное место
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers); // Запрещаем редактировать текст

    mainLayout->addWidget(m_table);
}

void LibraryManagerDialog::refreshCatalog() {
    // 1. Берем адрес из нашей новой строки ввода (и убираем случайные пробелы по краям)
    QString url = m_urlInput->text().trimmed();

    // Если пользователь ничего не ввел - ругаемся и отменяем загрузку
    if (url.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Пожалуйста, введите URL-адрес каталога библиотек (JSON)!"));
        return;
    }

    // 2. Обновляем интерфейс (как у тебя и было)
    m_statusLabel->setText(tr("⏳ Подключение к серверу..."));
    m_refreshBtn->setEnabled(false);
    m_table->setRowCount(0); // Очищаем старые данные из таблицы

    // 3. Запускаем скачивание по ВВЕДЕННОМУ адресу (передаем переменную url)
    NetworkManager::instance().fetchCatalog(url);
}

void LibraryManagerDialog::onCatalogFetched(bool success, const QByteArray &data, const QString &errorMsg) {
    m_refreshBtn->setEnabled(true);

    if (!success) {
        m_statusLabel->setText(tr("❌ Ошибка сети: %1").arg(errorMsg));
        // Пока ссылки нет, это нормально, что выдаст ошибку 404
        return;
    }

    QList<OnlineLibrary> libs = parseCatalog(data); // Расшифровываем JSON

    if (libs.isEmpty()) {
        m_statusLabel->setText(tr("⚠️ Каталог найден, но он пуст (или неверный формат)."));
        return;
    }

    m_statusLabel->setText(tr("✅ Каталог успешно загружен"));
    m_table->setRowCount(libs.size());

    // Заполняем таблицу
    for (int i = 0; i < libs.size(); ++i) {
        addLibraryToTable(libs[i], i);
    }
}

void LibraryManagerDialog::addLibraryToTable(const OnlineLibrary &lib, int row) {
    m_table->setItem(row, 0, new QTableWidgetItem(lib.name));
    m_table->setItem(row, 1, new QTableWidgetItem(lib.language));
    m_table->setItem(row, 2, new QTableWidgetItem(lib.description));

    // Кнопка в последней колонке
    QPushButton *downloadBtn = new QPushButton(tr("⬇️ Скачать"));
    downloadBtn->setStyleSheet("background-color: #008CBA; color: white; border-radius: 4px; padding: 5px; font-weight: bold;");

    // ==============================================================
    // --- ИСПРАВЛЕНО: Скачиваем ПРЯМО В ТУ ПАПКУ, ОТКУДА ВЫЗВАЛИ ---
    // ==============================================================
    QString savePath = m_targetDir + "/" + lib.filename;
    // ==============================================================

    // Умная фича: если файл уже есть на диске, меняем кнопку!
    if (QFile::exists(savePath)) {
        downloadBtn->setText(tr("✅ Установлено"));
        downloadBtn->setStyleSheet("background-color: #4CAF50; color: white; border-radius: 4px; padding: 5px; font-weight: bold;");
        downloadBtn->setEnabled(false);
    }

    // Что делать при нажатии на "Скачать"
    connect(downloadBtn, &QPushButton::clicked, this, [this, lib, savePath, downloadBtn]() {
        downloadBtn->setText(tr("⏳ Загрузка..."));
        downloadBtn->setEnabled(false);
        m_statusLabel->setText(tr("Скачивание %1...").arg(lib.filename));

        // Скачиваем файл!
        NetworkManager::instance().downloadLibrary(lib.url, savePath);
    });

    m_table->setCellWidget(row, 3, downloadBtn);
}

void LibraryManagerDialog::onLibraryDownloaded(bool success, const QString &savePath, const QString &errorMsg) {
    if (success) {
        m_statusLabel->setText(tr("✅ Библиотека успешно установлена!"));
        QFileInfo fi(savePath);
        QMessageBox::information(this, tr("Успех"), tr("Файл %1 успешно добавлен в библиотеку!").arg(fi.fileName()));
        refreshCatalog(); // Перерисовываем таблицу, чтобы кнопка стала зеленой "Установлено"
    } else {
        m_statusLabel->setText(tr("❌ Ошибка загрузки: %1").arg(errorMsg));
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось скачать файл:\n%1").arg(errorMsg));
        refreshCatalog(); // Сбрасываем зависшие кнопки
    }
}
