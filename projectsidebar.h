/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef PROJECTSIDEBAR_H
#define PROJECTSIDEBAR_H

#include <QDockWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QTreeView>          // <--- Для визуального дерева
#include <QFileSystemModel>   // <--- Для работы с файловой системой
#include <QPushButton>


class ProjectSidebar : public QDockWidget
{
    Q_OBJECT

public:
    QString getWorkspacePath() const { return m_workspacePath; }
    explicit ProjectSidebar(const QString &title, QWidget *parent = nullptr);
    ~ProjectSidebar() = default;
    // Функция: дать шторке команду отобразить конкретную папку
        void openProjectFolder(const QString &folderPath);
        QString currentPath() const { return m_currentProjectPath; } // <--- Поможет узнать, куда копировать файлы
        // Задаем главную папку Workspace
            void setWorkspace(const QString &workspaceDir);
            // Умная функция: возвращает путь к папке, которая сейчас выделена в дереве
                QString selectedFolder() const;

        signals:
            void requestLibraryManager(); // Сигнал: Открой онлайн-менеджер
            void requestAddFile();        // Сигнал: Открой окно добавления локального файла
            void projectDoubleClicked(const QString &folderPath); // Сигнал для открытия проекта
            void requestCloseProject();

private slots:
    void showContextMenu(const QPoint &pos);
private:
    // QDockWidget требует, чтобы внутрь него положили один главный виджет
    QWidget *m_contentWidget;
    QVBoxLayout *m_layout;
    QTreeView *m_treeView;            // Виджет дерева
    QFileSystemModel *m_fileModel;    // Модель файлов
    QString m_currentProjectPath;     // Запоминаем текущий путь
    QPushButton *m_btnLibManager;
    QPushButton *m_btnAddFile;
    QString m_workspacePath;

};

#endif // PROJECTSIDEBAR_H
