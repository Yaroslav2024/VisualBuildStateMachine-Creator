/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "codepreviewdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QLabel>
#include <QPushButton>
#include <QClipboard>
#include <QApplication>
#include <QFontDatabase>
#include <QTabWidget> // Обязательно добавляем для вкладок
#include <QTabWidget> // Обязательно добавляем для вкладок
#include "codehighlighter.h"

CodePreviewDialog::CodePreviewDialog(const QVector<QPair<QString, QString>> &files, const QString &translatorName, bool status, QList<int> errorLines, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Предпросмотр кода"));
    resize(700, 500); // Сделал окно чуть шире, чтобы вкладки хорошо помещались

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 1. Создаем ярлык статуса (Имя транслятора)
    statusLabel = new QLabel(this);
    QFont labelFont = statusLabel->font();
    labelFont.setBold(true);
    labelFont.setPointSize(10);
    statusLabel->setFont(labelFont);

    if (status) {
            statusLabel->setText(tr("🟢 Транслятор: %1 — Ошибок не найдено").arg(translatorName));
            statusLabel->setStyleSheet("color: #4CAF50;"); // Приятный зеленый
        } else {
            statusLabel->setText(tr("🔴 Транслятор: %1 — Обнаружена ошибка!").arg(translatorName));
            statusLabel->setStyleSheet("color: #F44336;"); // Сигнальный красный
        }
    mainLayout->addWidget(statusLabel);

    // 2. Создаем панель вкладок (ВМЕСТО одного QTextBrowser)
    tabWidget = new QTabWidget(this);

    // Подготавливаем шрифт и стиль для всех вкладок
    QFont codeFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    codeFont.setPointSize(10);
    QString borderStyle = status ? "2px solid #4CAF50" : "2px solid #F44336";

    // 3. Динамически создаем вкладку для КАЖДОГО переданного файла
    for (const auto &file : files) {
        QTextBrowser *textBrowser = new QTextBrowser(this);
        textBrowser->setPlainText(file.second); // Записываем сам код
        textBrowser->setFont(codeFont);
        textBrowser->setStyleSheet("QTextBrowser { border: " + borderStyle + "; border-radius: 4px; background-color: #FAFAFA; color: #333333; }");
        // --- Подсветка и скобки ---
                new CodeHighlighter(textBrowser->document()); // Включаем цветной синтаксис
                connect(textBrowser, &QTextBrowser::cursorPositionChanged, this, &CodePreviewDialog::matchBrackets); // Подключаем поиск скобок
        tabWidget->addTab(textBrowser, file.first); // Добавляем вкладку (Название = имя файла)
        // --- ПОДСВЕТКА СТРОКИ С ОШИБКОЙ ---
        // =========================================================
                // --- НОВОЕ: ПОДСВЕТКА ВСЕХ СТРОК С ОШИБКАМИ ---
                // =========================================================
                if (!status && !errorLines.isEmpty()) {
                    // Закрашиваем красным фоном каждую строку, где есть баг
                    for (int lineNum : errorLines) {
                        if (lineNum > 0) {
                            QTextBlock block = textBrowser->document()->findBlockByNumber(lineNum - 1); // Строки начинаются с 0
                            if (block.isValid()) {
                                QTextCursor cursor(block);
                                QTextBlockFormat blockFormat = cursor.blockFormat();
                                blockFormat.setBackground(QColor(255, 180, 180)); // Нежно-красный фон
                                cursor.setBlockFormat(blockFormat);
                                textBrowser->setTextCursor(cursor);
                            }
                        }
                    }

                    // Автоматически прокручиваем окно вниз прямо к САМОЙ ПЕРВОЙ ошибке!
                    int firstError = errorLines.first();
                    if (firstError > 0) {
                        QTextBlock firstBlock = textBrowser->document()->findBlockByNumber(firstError - 1);
                        if (firstBlock.isValid()) {
                            QTextCursor cursor(firstBlock);
                            textBrowser->setTextCursor(cursor);
                            textBrowser->ensureCursorVisible(); // Скроллим

                            cursor.clearSelection();
                            textBrowser->setTextCursor(cursor); // Убираем мигающий курсор
                        }
                    }
                }
                // =========================================================
    }
    mainLayout->addWidget(tabWidget);

    // 4. Создаем нижнюю панель с кнопками
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(); // Прижимаем кнопки к правому краю

    // Немного изменил текст, чтобы было понятно, что копируется
    QPushButton *copyBtn = new QPushButton(tr("Скопировать текущую вкладку"), this);
        QPushButton *closeBtn = new QPushButton(tr("Закрыть"), this);

    buttonLayout->addWidget(copyBtn);
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);

    // 5. Подключаем действия к кнопкам
    connect(copyBtn, &QPushButton::clicked, this, &CodePreviewDialog::copyToClipboard);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept); // Закрыть окно
}

void CodePreviewDialog::copyToClipboard()
{
    // Получаем именно ту вкладку, которая сейчас открыта у пользователя
    QTextBrowser *currentBrowser = qobject_cast<QTextBrowser*>(tabWidget->currentWidget());

    if (currentBrowser) {
        // Копируем текст в системный буфер обмена
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(currentBrowser->toPlainText());

        // Визуальная обратная связь для пользователя
        QPushButton *btn = qobject_cast<QPushButton*>(sender());
        if (btn) {
                    btn->setText(tr("Скопировано!"));
                    btn->setStyleSheet("color: #4CAF50; font-weight: bold;");
                }
    }
}
// --- НОВАЯ ФУНКЦИЯ: Поиск парных скобок ---
void CodePreviewDialog::matchBrackets()
{
    // Получаем ИМЕННО ТУ вкладку, в которой сейчас кликнул пользователь
    QTextBrowser *editor = qobject_cast<QTextBrowser*>(tabWidget->currentWidget());
    if (!editor) return;

    // Очищаем предыдущую подсветку
    QList<QTextEdit::ExtraSelection> selections;
    editor->setExtraSelections(selections);

    QTextCursor cursor = editor->textCursor();
    int pos = cursor.position();
    QString text = editor->toPlainText();

    if (text.isEmpty()) return;

    // Вспомогательная лямбда-функция
    auto findMatch = [&](int startPos, QChar openChar, QChar closeChar, int direction) -> int {
        int count = 1;
        int curr = startPos + direction;
        while (curr >= 0 && curr < text.length()) {
            if (text[curr] == openChar) count++;
            else if (text[curr] == closeChar) count--;

            if (count == 0) return curr;
            curr += direction;
        }
        return -1;
    };

    QChar charRight = (pos < text.length()) ? text[pos] : QChar();
    QChar charLeft = (pos > 0) ? text[pos - 1] : QChar();

    int matchPos = -1;
    int bracketPos = -1;

    if (charRight == '{')      { matchPos = findMatch(pos, '{', '}', 1); bracketPos = pos; }
    else if (charRight == '(') { matchPos = findMatch(pos, '(', ')', 1); bracketPos = pos; }
    else if (charRight == '[') { matchPos = findMatch(pos, '[', ']', 1); bracketPos = pos; }
    else if (charLeft == '}')  { matchPos = findMatch(pos - 1, '}', '{', -1); bracketPos = pos - 1; }
    else if (charLeft == ')')  { matchPos = findMatch(pos - 1, ')', '(', -1); bracketPos = pos - 1; }
    else if (charLeft == ']')  { matchPos = findMatch(pos - 1, ']', '[', -1); bracketPos = pos - 1; }

    if (matchPos != -1) {
        QTextCharFormat format;
        format.setBackground(QColor(180, 220, 255)); // Светло-голубой фон
        format.setFontWeight(QFont::Bold);

        QTextEdit::ExtraSelection sel1, sel2;

        QTextCursor cur1 = editor->textCursor();
        cur1.setPosition(bracketPos);
        cur1.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        sel1.cursor = cur1;
        sel1.format = format;

        QTextCursor cur2 = editor->textCursor();
        cur2.setPosition(matchPos);
        cur2.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        sel2.cursor = cur2;
        sel2.format = format;

        selections.append(sel1);
        selections.append(sel2);

        editor->setExtraSelections(selections);
    }
}
