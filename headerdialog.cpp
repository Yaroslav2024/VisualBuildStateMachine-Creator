/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "headerdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFont>
#include "codehighlighter.h"
HeaderDialog::HeaderDialog(const QString &currentText, QWidget *parent)
    : QDialog(parent)
{

    setWindowTitle(tr("Глобальные настройки и переменные (Header)"));
    resize(600, 500);

    QVBoxLayout *layout = new QVBoxLayout(this);
    // Обновленная и переведенная подсказка
    // Обновленная и переведенная подсказка
        QLabel *info = new QLabel(tr("Подключите библиотеки (include/import) и объявите ваши переменные здесь."), this);
        info->setFont(QFont("Arial", 10, QFont::Bold));
        layout->addWidget(info);

    m_editor = new QTextEdit(this);
    m_editor->setPlainText(currentText);
    m_editor->setFont(QFont("Consolas", 11)); // Моноширинный шрифт

        new CodeHighlighter(m_editor->document());
    connect(m_editor, &QTextEdit::cursorPositionChanged, this, &HeaderDialog::matchBrackets);
    layout->addWidget(m_editor);

    QDialogButtonBox *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(btns);
}

QString HeaderDialog::getCode() const
{
    return m_editor->toPlainText();
}
// --- ДОБАВИТЬ ЭТУ ФУНКЦИЮ ---
void HeaderDialog::setPlaceholderText(const QString &text)
{
    m_editor->setPlaceholderText(text);
}
void HeaderDialog::matchBrackets()
{
    // Очищаем предыдущую подсветку
    QList<QTextEdit::ExtraSelection> selections;
    m_editor->setExtraSelections(selections);

    QTextCursor cursor = m_editor->textCursor();
    int pos = cursor.position();
    QString text = m_editor->toPlainText();

    if (text.isEmpty()) return;

    // Вспомогательная лямбда-функция для поиска парной скобки
    auto findMatch = [&](int startPos, QChar openChar, QChar closeChar, int direction) -> int {
        int count = 1;
        int curr = startPos + direction;
        while (curr >= 0 && curr < text.length()) {
            if (text[curr] == openChar) count++;
            else if (text[curr] == closeChar) count--;

            if (count == 0) return curr; // Нашли пару!
            curr += direction;
        }
        return -1; // Пара не найдена
    };

    // Смотрим символ СПРАВА от курсора и СЛЕВА от курсора
    QChar charRight = (pos < text.length()) ? text[pos] : QChar();
    QChar charLeft = (pos > 0) ? text[pos - 1] : QChar();

    int matchPos = -1;
    int bracketPos = -1;

    // Проверяем все варианты: { } [ ] ( )
    if (charRight == '{')      { matchPos = findMatch(pos, '{', '}', 1); bracketPos = pos; }
    else if (charRight == '(') { matchPos = findMatch(pos, '(', ')', 1); bracketPos = pos; }
    else if (charRight == '[') { matchPos = findMatch(pos, '[', ']', 1); bracketPos = pos; }
    else if (charLeft == '}')  { matchPos = findMatch(pos - 1, '}', '{', -1); bracketPos = pos - 1; }
    else if (charLeft == ')')  { matchPos = findMatch(pos - 1, ')', '(', -1); bracketPos = pos - 1; }
    else if (charLeft == ']')  { matchPos = findMatch(pos - 1, ']', '[', -1); bracketPos = pos - 1; }

    // Если нашли парную скобку — подсвечиваем обе!
    if (matchPos != -1) {
        QTextCharFormat format;
        // Светло-голубой фон для выделения скобок (можно изменить на QColor(200, 255, 200) для зеленого)
        format.setBackground(QColor(180, 220, 255));
        format.setFontWeight(QFont::Bold);

        QTextEdit::ExtraSelection sel1, sel2;

        // Выделяем первую скобку
        QTextCursor cur1 = m_editor->textCursor();
        cur1.setPosition(bracketPos);
        cur1.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        sel1.cursor = cur1;
        sel1.format = format;

        // Выделяем парную скобку
        QTextCursor cur2 = m_editor->textCursor();
        cur2.setPosition(matchPos);
        cur2.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        sel2.cursor = cur2;
        sel2.format = format;

        selections.append(sel1);
        selections.append(sel2);

        // Применяем выделение
        m_editor->setExtraSelections(selections);
    }
}
