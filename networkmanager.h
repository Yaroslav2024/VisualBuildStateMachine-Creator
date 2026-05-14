/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QByteArray>
#include <QFile>

struct OnlineLibrary {
    QString name;
    QString language; // "C++", "Python" или "Java"
    QString description;
    QString filename;
    QString url;
    };
QList<OnlineLibrary> parseCatalog(const QByteArray &jsonData);
class NetworkManager : public QObject
{
    Q_OBJECT
public:
    // Делаем класс Одиночкой (Singleton), чтобы легко вызывать его из любого места
    static NetworkManager& instance();

    // 1. Проверка связи (Пинг)
    void checkConnection();

    // 2. Скачивание JSON каталога библиотек
    void fetchCatalog(const QString &catalogUrl);

    // 3. Скачивание самой библиотеки (сохраняет файл на диск)
    void downloadLibrary(const QString &fileUrl, const QString &savePath);

signals:
    // Сигналы, которые мы будем ловить в главном окне
    void connectionStatus(bool isOnline);
    void catalogFetched(bool success, const QByteArray &data, const QString &errorMsg);
    void libraryDownloaded(bool success, const QString &savePath, const QString &errorMsg);

private:
    explicit NetworkManager(QObject *parent = nullptr);
    QNetworkAccessManager *manager;
};

#endif
