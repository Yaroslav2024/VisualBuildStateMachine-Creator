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

#include <QFileDialog>
#include <QStandardPaths>
#include <QSettings>

LibraryManagerDialog::LibraryManagerDialog(const QString &targetDir, QWidget *parent) {
    setWindowTitle(tr("🌐 Онлайн Каталог Библиотек"));
    resize(750, 450);

    // Сохраняем начальную директорию (если нужна)
    m_targetDir = targetDir;

    setupUi();

    // Подключаем сигналы от нашего NetworkManager
    connect(&NetworkManager::instance(), &NetworkManager::catalogFetched, this, &LibraryManagerDialog::onCatalogFetched);
    connect(&NetworkManager::instance(), &NetworkManager::libraryDownloaded, this, &LibraryManagerDialog::onLibraryDownloaded);
}

void LibraryManagerDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // --- Верхушка: Статус и кнопка обновления ---
    QHBoxLayout *topLayout = new QHBoxLayout();
    m_statusLabel = new QLabel(tr("Готово"), this);
    m_statusLabel->setStyleSheet("font-weight: bold; color: #333;");

    m_urlInput = new QLineEdit(this);
    m_urlInput->setPlaceholderText(tr("Вставьте URL каталога (JSON)..."));

    m_refreshBtn = new QPushButton(tr("🔄 Обновить список"), this);
    connect(m_refreshBtn, &QPushButton::clicked, this, &LibraryManagerDialog::refreshCatalog);

    topLayout->addWidget(m_statusLabel);
    topLayout->addWidget(m_urlInput, 1);
    topLayout->addWidget(m_refreshBtn);

    mainLayout->addLayout(topLayout);

    // --- Главная часть: Таблица ---
    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({tr("Название"), tr("Язык"), tr("Описание"), tr("Установка")});
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mainLayout->addWidget(m_table);

    // ==========================================================
    // --- НОВОЕ: НИЖНЯЯ ПАНЕЛЬ (ВЫБОР ПАПКИ) ---
    // ==========================================================
    QHBoxLayout *bottomLayout = new QHBoxLayout();

    m_pathInput = new QLineEdit(this);
    m_pathInput->setPlaceholderText(tr("Папка для сохранения (По умолчанию: Документы/VisualBuild/Libraries)"));

    // === НОВОЕ: Загружаем последний сохраненный путь ===
        QSettings settings("VisualBuild", "IDE");
        QString lastPath = settings.value("LibraryManager/LastSavedPath", "").toString();
        if (!lastPath.isEmpty()) {
            m_pathInput->setText(lastPath);
        }

    m_browseBtn = new QPushButton(tr("📁 Обзор..."), this);
    m_browseBtn->setStyleSheet("background-color: #e7e7e7; color: black; font-weight: bold; padding: 5px 10px; border-radius: 3px;");

    bottomLayout->addWidget(m_pathInput, 1);
    bottomLayout->addWidget(m_browseBtn);

    mainLayout->addLayout(bottomLayout);

    // Логика кнопки "Обзор"
    connect(m_browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this,
                                tr("Выберите папку для библиотек"),
                                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (!dir.isEmpty()) {
            m_pathInput->setText(dir); // Записываем выбранный путь в поле
            // === НОВОЕ: Сохраняем ручной выбор ===
                        QSettings settings("VisualBuild", "IDE");
                        settings.setValue("LibraryManager/LastSavedPath", dir);
        }
    });
    // ==========================================================
}

void LibraryManagerDialog::refreshCatalog() {
    QString url = m_urlInput->text().trimmed();

    if (url.isEmpty()) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Пожалуйста, введите URL-адрес каталога библиотек (JSON)!"));
        return;
    }

    m_statusLabel->setText(tr("⏳ Подключение к серверу..."));
    m_refreshBtn->setEnabled(false);
    m_table->setRowCount(0);

    NetworkManager::instance().fetchCatalog(url);
}

void LibraryManagerDialog::onCatalogFetched(bool success, const QByteArray &data, const QString &errorMsg) {
    m_refreshBtn->setEnabled(true);

    if (!success) {
        m_statusLabel->setText(tr("❌ Ошибка сети: %1").arg(errorMsg));
        return;
    }

    QList<OnlineLibrary> libs = parseCatalog(data);

    if (libs.isEmpty()) {
        m_statusLabel->setText(tr("⚠️ Каталог найден, но он пуст (или неверный формат)."));
        return;
    }

    m_statusLabel->setText(tr("✅ Каталог успешно загружен"));
    m_table->setRowCount(libs.size());

    for (int i = 0; i < libs.size(); ++i) {
        addLibraryToTable(libs[i], i);
    }
}

void LibraryManagerDialog::addLibraryToTable(const OnlineLibrary &lib, int row) {
    m_table->setItem(row, 0, new QTableWidgetItem(lib.name));
    m_table->setItem(row, 1, new QTableWidgetItem(lib.language));
    m_table->setItem(row, 2, new QTableWidgetItem(lib.description));

    QPushButton *downloadBtn = new QPushButton(tr("⬇️ Скачать"));
    downloadBtn->setStyleSheet("background-color: #008CBA; color: white; border-radius: 4px; padding: 5px; font-weight: bold;");

    // Формируем примерный путь, чтобы проверить, скачан ли файл
    QString targetFolder = m_pathInput->text().trimmed();
    if (targetFolder.isEmpty()) {
        targetFolder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/VisualBuild/Libraries";
    }
    QString savePath = targetFolder + "/" + lib.filename;

    // ==========================================================
    // --- ИСПРАВЛЕНО: РАЗРЕШАЕМ ПЕРЕУСТАНОВКУ ---
    // ==========================================================
    if (QFile::exists(savePath)) {
        downloadBtn->setText(tr("🔄 Переустановить"));
        downloadBtn->setStyleSheet("background-color: #f0ad4e; color: white; border-radius: 4px; padding: 5px; font-weight: bold;");
        downloadBtn->setEnabled(true); // РАЗБЛОКИРОВКА!
    }

    // Что делать при нажатии на "Скачать" / "Переустановить"
    connect(downloadBtn, &QPushButton::clicked, this, [this, lib, downloadBtn]() {
        downloadBtn->setText(tr("⏳ Загрузка..."));
        downloadBtn->setEnabled(false);
        m_statusLabel->setText(tr("Скачивание %1...").arg(lib.filename));

        // Берем АКТУАЛЬНУЮ папку из текстового поля и передаем в NetworkManager
        QString currentFolder = m_pathInput->text().trimmed();
        NetworkManager::instance().downloadLibrary(lib.url, currentFolder);
    });

    m_table->setCellWidget(row, 3, downloadBtn);
}

void LibraryManagerDialog::onLibraryDownloaded(bool success, const QString &savePath, const QString &errorMsg) {
    if (success) {
        m_statusLabel->setText(tr("✅ Библиотека успешно установлена!"));

        // --- НОВОЕ: Показываем точную папку пользователю! ---
        QFileInfo fi(savePath);

        m_pathInput->setText(fi.absolutePath());

        QSettings settings("VisualBuild", "IDE");

        settings.setValue("LibraryManager/LastSavedPath", fi.absolutePath());
        QMessageBox::information(this, tr("Успех"), tr("Файл %1 успешно добавлен в папку:\n%2").arg(fi.fileName(), fi.absolutePath()));

        refreshCatalog(); // Перерисовываем таблицу, чтобы кнопка стала оранжевой
    } else {
        m_statusLabel->setText(tr("❌ Ошибка загрузки: %1").arg(errorMsg));
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось скачать файл:\n%1").arg(errorMsg));
        refreshCatalog(); // Сбрасываем зависшие кнопки
    }
}
