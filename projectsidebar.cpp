/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "projectsidebar.h"
#include <QHeaderView>
#include <QMenu>
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QAction>
#include <QFileDialog>
#include <QLabel>

ProjectSidebar::ProjectSidebar(const QString &title, QWidget *parent)
    : QDockWidget(title, parent)
{
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    m_contentWidget = new QWidget(this);
    m_layout = new QVBoxLayout(m_contentWidget);
    m_layout->setContentsMargins(4, 4, 4, 0); // Небольшие отступы для красоты

    // === 1. СОЗДАЕМ КНОПКИ УПРАВЛЕНИЯ ===
        QHBoxLayout *btnLayout = new QHBoxLayout();

        m_btnLibManager = new QPushButton(tr("🌐 Library Manager (Online Catalog)"), this);
        m_btnLibManager->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; border-radius: 3px; padding: 7px;");

        // Добавляем только одну кнопку, она автоматически растянется на всю ширину
        btnLayout->addWidget(m_btnLibManager);

        m_layout->addLayout(btnLayout); // Добавляем кнопку в самый верх

        // Связываем нажатие с сигналом открытия менеджера
        connect(m_btnLibManager, &QPushButton::clicked, this, &ProjectSidebar::requestLibraryManager);

    // === 2. СОЗДАЕМ ДЕРЕВО ФАЙЛОВ ===
        m_fileModel = new QFileSystemModel(this);
        m_fileModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
        m_fileModel->setReadOnly(false); // Разрешаем изменение файлов для Drag&Drop

        // СНАЧАЛА СОЗДАЕМ ОБЪЕКТ!
        m_treeView = new QTreeView(this);
        m_treeView->setModel(m_fileModel);
        m_treeView->setHeaderHidden(true);

        // А ТЕПЕРЬ НАСТРАИВАЕМ ЕГО (DRAG & DROP)
        m_treeView->setIndentation(25);
        m_treeView->setDragEnabled(true);
        m_treeView->setAcceptDrops(true);
        m_treeView->setDropIndicatorShown(true);

        for (int i = 1; i < m_fileModel->columnCount(); ++i) {
            m_treeView->hideColumn(i);
        }

    m_layout->addWidget(m_treeView);
    // =====================================================================
            // --- USER TIPS AT THE BOTTOM OF THE SIDEBAR ---
            QLabel *hintLabel = new QLabel(tr(
                "💡 <b>Pro Tips:</b><br><br>"
                "📁 <b>Projects:</b> Try to keep all your projects in a single folder <i>(Workspace)</i>. "
                "This will speed up loading and prevent confusion.<br><br>"
                "🌐 <b>Libraries:</b> Paste a direct link (Raw URL) into the <i>Library Manager</i> "
                "to a <b>.json</b> catalog file to download new modules."
                ), this
            );

            hintLabel->setWordWrap(true); // Allow text wrapping to new lines
            hintLabel->setStyleSheet(
                "color: #7A7A7A; "          // Pleasant gray color
                "font-size: 11px; "         // Slightly smaller font size
                "padding: 8px; "            // Padding around the edges
                "background: transparent;"  // Transparent background
            );

            // Add the hint to the very bottom of the sidebar (under the tree)
            m_layout->addWidget(hintLabel);
            // =====================================================================
    m_contentWidget->setLayout(m_layout);
    setWidget(m_contentWidget);


        // 1. ВКЛЮЧАЕМ МЕНЮ ПО ПРАВОМУ КЛИКУ
            m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(m_treeView, &QTreeView::customContextMenuRequested, this, &ProjectSidebar::showContextMenu);

            // 2. УМНЫЙ ДВОЙНОЙ КЛИК (Только для папок проектов!)
                connect(m_treeView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
                    QString path = m_fileModel->filePath(index);
                    QFileInfo fi(path);

                    // ИСПОЛЬЗУЕМ QDir ДЛЯ 100% ТОЧНОГО ПОИСКА В WINDOWS
                    if (fi.isDir() && QDir(path).exists("graph.json")) {
                        emit projectDoubleClicked(path);
                    }
                });
}
void ProjectSidebar::setWorkspace(const QString &workspaceDir)
{
    m_workspacePath = workspaceDir;
    m_fileModel->setRootPath(workspaceDir);
    m_treeView->setRootIndex(m_fileModel->index(workspaceDir));
}

QString ProjectSidebar::selectedFolder() const
{
    QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid()) return m_workspacePath; // Если ничего не выделено, кидаем в корень Workspace

    QString path = m_fileModel->filePath(index);
    QFileInfo fi(path);

    // Если пользователь выделил файл (например graph.json), возвращаем папку, где он лежит
    if (fi.isFile()) {
        return fi.absolutePath();
    }
    // Если выделил папку - возвращаем её
    return fi.absoluteFilePath();
}
void ProjectSidebar::showContextMenu(const QPoint &pos)
{
    // Узнаем, на какой файл/папку кликнули
    QModelIndex index = m_treeView->indexAt(pos);
    if (!index.isValid()) return;

    QString path = m_fileModel->filePath(index);
    QFileInfo fi(path);

    QMenu menu(this);



    // Проверяем: это Главная Папка Проекта? (Ищем файл graph.json внутри)
        bool isProjectFolder = fi.isDir() && QDir(path).exists("graph.json");
        // --- ЖЕСТКАЯ ЗАЩИТА: Это системная папка или главный файл? ---
            bool isProtectedFolder = fi.isDir() &&
                                     (fi.fileName() == "CppLibraries" ||
                                      fi.fileName() == "PythonLibraries" ||
                                      fi.fileName() == "JavaLibraries");

            bool isCoreFile = fi.fileName() == "graph.json";
    // Если это Проект — добавляем кнопку "Открыть"
    if (isProjectFolder) {
        QAction *openAct = menu.addAction(tr("📂 Open project"));
        // способ сделать текст жирным для QAction:
                QFont boldFont = openAct->font();
                boldFont.setBold(true);
                openAct->setFont(boldFont);
        connect(openAct, &QAction::triggered, this, [this, path]() {
            emit projectDoubleClicked(path);
        });
        // ---  ЗАКРЫТЬ ПРОЕКТ ---
                QAction *closeAct = menu.addAction(tr("❌ Close the project"));
                connect(closeAct, &QAction::triggered, this, [this]() {
                    emit requestCloseProject(); // Отправляем сигнал в Главное окно
                });
        menu.addSeparator(); // Линия-разделитель
    }
    // ---ДОБАВЛЯЕМ ФАЙЛ ЧЕРЕЗ ПРАВЫЙ КЛИК ПО ПАПКЕ ---
    // Разрешаем добавлять файлы ТОЛЬКО в папки библиотек
            if (isProtectedFolder) {
                QAction *addFileAct = menu.addAction(tr("➕ Add file here"));

                // Делаем текст жирным, чтобы выделялся
                QFont boldFont = addFileAct->font();
                boldFont.setBold(true);
                addFileAct->setFont(boldFont);

                connect(addFileAct, &QAction::triggered, this, [this, path]() {
                    QStringList fileNames = QFileDialog::getOpenFileNames(this,
                        tr("Выберите файлы для папки: %1").arg(QFileInfo(path).fileName()), "", tr("All Files (*.*)"));

                    if (fileNames.isEmpty()) return;

                    for (const QString &filePath : fileNames) {
                        QFileInfo sourceFi(filePath);
                        QString destPath = path + "/" + sourceFi.fileName();

                        if (QFile::exists(destPath)) QFile::remove(destPath);
                        QFile::copy(filePath, destPath);
                    }
                });
                menu.addSeparator();
            }
    // Стандартные кнопки для ВСЕХ файлов и папок
            QAction *renameAct = menu.addAction(tr("✏️ Rename"));
            QAction *deleteAct = menu.addAction(tr("🗑 Delete"));
    // --- ЗАЩИТА ОТ ДУРАКА: ОТКЛЮЧАЕМ КНОПКИ ДЛЯ ВАЖНЫХ ФАЙЛОВ ---
        if (isProtectedFolder || isCoreFile) {
            renameAct->setEnabled(false);
            deleteAct->setEnabled(false);

            // Добавим подсказку, почему кнопки не работают
            renameAct->setToolTip(tr("Системные папки и graph.json нельзя переименовывать!"));
            deleteAct->setToolTip(tr("Системные папки и graph.json нельзя удалять!"));
        }
        // =========================================================
    // ЛОГИКА: Переименование
    connect(renameAct, &QAction::triggered, this, [this, path, fi]() {
        bool ok;
        QString newName = QInputDialog::getText(this, tr("Переименовать"), tr("Новое имя:"), QLineEdit::Normal, fi.fileName(), &ok);
        if (ok && !newName.isEmpty() && newName != fi.fileName()) {
            QString newPath = fi.absolutePath() + "/" + newName;
            QFile::rename(path, newPath);
        }
    });

    // ЛОГИКА: Удаление
    connect(deleteAct, &QAction::triggered, this, [this, path, fi, isProjectFolder]() {
        QString warningText = isProjectFolder ?
            tr("Вы уверены, что хотите удалить <b>ВЕСЬ ПРОЕКТ '%1'</b> со всеми файлами?").arg(fi.fileName()) :
            tr("Вы уверены, что хотите удалить <b>%1</b>?").arg(fi.fileName());

        QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Удаление"),
            tr("%1\n\nЭто действие нельзя отменить!").arg(warningText), QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            if (fi.isDir()) {
                QDir dir(path);
                dir.removeRecursively(); // Удаляет папку и ВСЁ, что внутри
            } else {
                QFile::remove(path); // Удаляет одиночный файл
            }
        }
    });

    // Показываем меню в том месте, где находится мышка
    menu.exec(m_treeView->viewport()->mapToGlobal(pos));
}
