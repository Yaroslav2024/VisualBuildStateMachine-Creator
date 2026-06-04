/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "networkmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
// Библиотеки для безопасной работы с путями ОС
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

QList<OnlineLibrary> parseCatalog(const QByteArray &jsonData) {
    QList<OnlineLibrary> result;

    // Пытаемся прочитать текст как JSON
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (doc.isNull() || !doc.isObject()) return result; // Ошибка чтения

    QJsonObject root = doc.object();
    QJsonArray libs = root["libraries"].toArray();

    // Перебираем все библиотеки в JSON и складываем в наш список
    for (int i = 0; i < libs.size(); ++i) {
        QJsonObject libObj = libs[i].toObject();
        OnlineLibrary lib;
        lib.name = libObj["name"].toString();
        lib.language = libObj["language"].toString();
        lib.description = libObj["description"].toString();
        lib.filename = libObj["filename"].toString();
        lib.url = libObj["url"].toString();

        result.append(lib);
    }

    return result;
}

// Реализация Singleton
NetworkManager& NetworkManager::instance() {
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

// --- 1. ПРОВЕРКА ИНТЕРНЕТА ---
void NetworkManager::checkConnection() {
    QUrl url("http://clients3.google.com/generate_204");
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        bool isOnline = (reply->error() == QNetworkReply::NoError);
        emit connectionStatus(isOnline);
        reply->deleteLater();
    });
}

// --- 2. СКАЧИВАНИЕ КАТАЛОГА (JSON) ---
void NetworkManager::fetchCatalog(const QString &catalogUrl) {
    QUrl url(catalogUrl);
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            emit catalogFetched(true, data, "");
        } else {
            emit catalogFetched(false, QByteArray(), reply->errorString());
        }
        reply->deleteLater();
    });
}

// --- 3. СКАЧИВАНИЕ ФАЙЛА БИБЛИОТЕКИ (БЕЗ ПРАВ АДМИНА) ---
// Теперь второй аргумент targetFolder - это ПАПКА, которую указал пользователь
void NetworkManager::downloadLibrary(const QString &fileUrl, const QString &targetFolder) {
    QUrl url(fileUrl);
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    // 1. Берем имя файла прямо из URL (например "RobotControl.h")
    QString fileName = QFileInfo(url.path()).fileName();
    if (fileName.isEmpty()) fileName = "downloaded_library.h";

    // 2. Умный выбор папки без запроса прав Администратора
    QString safeDir = targetFolder.trimmed();
    if (safeDir.isEmpty()) {
        // Если поле в UI пустое, сохраняем в "Мои Документы/VisualBuild/Libraries"
        // Windows ВСЕГДА разрешает туда запись без прав админа!
        safeDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/VisualBuild/Libraries";
    }

    // 3. Создаем папку, если ее еще нет
    QDir dir(safeDir);
    if (!dir.exists()) {
        dir.mkpath("."); // Безопасно создает всю цепочку папок
    }

    // 4. Формируем финальный полный путь к файлу
    QString finalFilePath = dir.absoluteFilePath(fileName);

    connect(reply, &QNetworkReply::finished, this, [this, reply, finalFilePath]() {
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(finalFilePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                // Возвращаем в интерфейс успешный статус и точный путь к файлу
                emit libraryDownloaded(true, finalFilePath, "");
            } else {
                            emit libraryDownloaded(false, finalFilePath, tr("Windows blocked access to the folder:\n%1").arg(finalFilePath));
                        }
        } else {
            emit libraryDownloaded(false, finalFilePath, reply->errorString());
        }
        reply->deleteLater();
    });
}
