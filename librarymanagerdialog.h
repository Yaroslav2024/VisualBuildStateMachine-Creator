/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef LIBRARYMANAGERDIALOG_H
#define LIBRARYMANAGERDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include "networkmanager.h"

class LibraryManagerDialog : public QDialog {
    Q_OBJECT
public:
    explicit LibraryManagerDialog(const QString &targetDir, QWidget *parent = nullptr);

private slots:
    void onCatalogFetched(bool success, const QByteArray &data, const QString &errorMsg);
    void onLibraryDownloaded(bool success, const QString &savePath, const QString &errorMsg);
    void refreshCatalog();

private:
    QLineEdit *m_pathInput;  // Поле для пути к папке
        QPushButton *m_browseBtn; // Кнопка "Обзор"
    void setupUi();
    void addLibraryToTable(const OnlineLibrary &lib, int row);

    QTableWidget *m_table;
    QLabel *m_statusLabel;
    QPushButton *m_refreshBtn;
    QString m_targetDir;
    QLineEdit *m_urlInput;
};

#endif // LIBRARYMANAGERDIALOG_H
