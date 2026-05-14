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
    // Разбиваем на две строки, чтобы компилятор не путался
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
    // То же самое здесь
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

// --- 3. СКАЧИВАНИЕ ФАЙЛА БИБЛИОТЕКИ ---
void NetworkManager::downloadLibrary(const QString &fileUrl, const QString &savePath) {
    // И здесь
    QUrl url(fileUrl);
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, savePath]() {
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                emit libraryDownloaded(true, savePath, "");
            } else {
                emit libraryDownloaded(false, savePath, "Failed to save file to disk.");
            }
        } else {
            emit libraryDownloaded(false, savePath, reply->errorString());
        }
        reply->deleteLater();
    });
}
