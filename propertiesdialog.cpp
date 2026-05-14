/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#include "propertiesdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMap>
#include "syntaxregistry.h"
#include "codehighlighter.h"
#include <QTimer>
PropertiesDialog::PropertiesDialog(const Data &initialData, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Создать новое состояние / Настройки блока"));
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    // --- 1. Тип ---
    m_typeCombo = new QComboBox(this);
    m_typeCombo->addItem(tr("Вершина"));
    m_typeCombo->addItem(tr("Стартовый блок"));
    m_typeCombo->addItem(tr("Мост"));
    m_typeCombo->setCurrentText(initialData.type);
    formLayout->addRow(tr("Тип:"), m_typeCombo);

    // --- 2. Описание ---
    m_descriptionEdit = new QLineEdit(this);
    m_descriptionEdit->setText(initialData.description);
    m_descriptionEdit->setPlaceholderText(tr("Настройте блок"));
    formLayout->addRow(tr("Discript:"), m_descriptionEdit);


    // --- 3. Label (Автоматический) ---
    m_labelEdit = new QLineEdit(this);
    m_labelEdit->setReadOnly(true); // Только для чтения, генерируется само
    m_labelEdit->setText(initialData.label);
    // Цвет текста серым, чтобы показать что оно readonly
    QPalette pal = m_labelEdit->palette();
    pal.setColor(QPalette::Text, Qt::gray);
    m_labelEdit->setPalette(pal);

    formLayout->addRow(tr("Label:"), m_labelEdit);
    mainLayout->addLayout(formLayout);
    // =========================================================================
        // УМНАЯ ФУНКЦИЯ ДЛЯ ПОДСВЕТКИ СКОБОК (Лямбда)
        // =========================================================================
        auto setupBracketMatching = [](QTextEdit *editor) {
            QObject::connect(editor, &QTextEdit::cursorPositionChanged, editor, [editor]() {
                QList<QTextEdit::ExtraSelection> selections;
                editor->setExtraSelections(selections);

                QTextCursor cursor = editor->textCursor();
                int pos = cursor.position();
                QString text = editor->toPlainText();
                if (text.isEmpty()) return;

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

                int matchPos = -1, bracketPos = -1;

                if (charRight == '{')      { matchPos = findMatch(pos, '{', '}', 1); bracketPos = pos; }
                else if (charRight == '(') { matchPos = findMatch(pos, '(', ')', 1); bracketPos = pos; }
                else if (charRight == '[') { matchPos = findMatch(pos, '[', ']', 1); bracketPos = pos; }
                else if (charLeft == '}')  { matchPos = findMatch(pos - 1, '}', '{', -1); bracketPos = pos - 1; }
                else if (charLeft == ')')  { matchPos = findMatch(pos - 1, ')', '(', -1); bracketPos = pos - 1; }
                else if (charLeft == ']')  { matchPos = findMatch(pos - 1, ']', '[', -1); bracketPos = pos - 1; }

                if (matchPos != -1) {
                    QTextCharFormat format;
                    format.setBackground(QColor(180, 220, 255));
                    format.setFontWeight(QFont::Bold);

                    QTextEdit::ExtraSelection sel1, sel2;
                    QTextCursor cur1 = editor->textCursor();
                    cur1.setPosition(bracketPos); cur1.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    sel1.cursor = cur1; sel1.format = format;

                    QTextCursor cur2 = editor->textCursor();
                    cur2.setPosition(matchPos); cur2.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                    sel2.cursor = cur2; sel2.format = format;

                    selections.append(sel1); selections.append(sel2);
                    editor->setExtraSelections(selections);
                }
            });
        };
        // =========================================================================
    // --- 4. onEnter ---
    mainLayout->addWidget(new QLabel("onEnter():"));
    m_onEnterEdit = new QTextEdit(this);
    m_onEnterEdit->setPlainText(initialData.onEnter);
    m_onEnterEdit->setMaximumHeight(60); // Компактный вид
    mainLayout->addWidget(m_onEnterEdit);
    new CodeHighlighter(m_onEnterEdit->document());
            setupBracketMatching(m_onEnterEdit);
            // *************************


        // *************************
    // --- 5. onExit ---
    mainLayout->addWidget(new QLabel("onExit():"));
    m_onExitEdit = new QTextEdit(this);
    m_onExitEdit->setPlainText(initialData.onExit);
    m_onExitEdit->setMaximumHeight(60);
    mainLayout->addWidget(m_onExitEdit);
    new CodeHighlighter(m_onExitEdit->document());
            setupBracketMatching(m_onExitEdit);

        new CodeHighlighter(m_onExitEdit->document());
        // *************************
    // --- Кнопки OK/Cancel ---
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PropertiesDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    // Подключаем транслитерацию
    connect(m_descriptionEdit, &QLineEdit::textChanged, this, &PropertiesDialog::onDescriptionChanged);

    // =========================================================================
    // =========================================================================
        // СУПЕР-УМНЫЙ ФОКУС (Учитывает точное место клика мышкой по блоку)
        // Должен быть здесь, когда все поля (m_onEnterEdit и т.д.) уже существуют!
        // =========================================================================
        if (initialData.clickedSection == 1) {
            m_onEnterEdit->setFocus();
            // Многострочное поле: двигаем курсор в самый конец
            m_onEnterEdit->moveCursor(QTextCursor::End);
        } else if (initialData.clickedSection == 2) {
            m_onExitEdit->setFocus();
            // Многострочное поле: двигаем курсор в самый конец
            m_onExitEdit->moveCursor(QTextCursor::End);
        } else {
            // Однострочное поле (Заголовок): ставим фокус
            m_descriptionEdit->setFocus();

            // МАГИЯ ТОЛЬКО ДЛЯ ЗАГОЛОВКА:
            // Отложенный вызов (0 мс), чтобы перебить системное выделение Qt после открытия окна
            QTimer::singleShot(0, m_descriptionEdit, [this]() {
                m_descriptionEdit->deselect();
                m_descriptionEdit->setCursorPosition(m_descriptionEdit->text().length());
            });
        }
}

PropertiesDialog::Data PropertiesDialog::getData() const
{
    Data d;
    d.type = m_typeCombo->currentText();
    d.description = m_descriptionEdit->text();
    d.label = m_labelEdit->text();
    d.onEnter = m_onEnterEdit->toPlainText();
    d.onExit = m_onExitEdit->toPlainText();
    return d;
}

void PropertiesDialog::onDescriptionChanged(const QString &text)
{
    // При изменении описания автоматически меняем Label
    QString trans = transliterate(text);
    // Добавляем подчеркивание в начале, как на скрине
    if (!trans.isEmpty()) {
        m_labelEdit->setText("_" + trans);
    } else {
        m_labelEdit->clear();
    }
}

QString PropertiesDialog::transliterate(const QString &text)
{
    // Простая карта транслитерации
    static QMap<QChar, QString> map;
    if (map.isEmpty()) {
        // ИСПОЛЬЗУЕМ u'' ДЛЯ UNICODE СИМВОЛОВ
        map[u'а'] = "a"; map[u'б'] = "b"; map[u'в'] = "v"; map[u'г'] = "g"; map[u'д'] = "d";
        map[u'е'] = "e"; map[u'ё'] = "yo"; map[u'ж'] = "zh"; map[u'з'] = "z"; map[u'и'] = "i";
        map[u'й'] = "y"; map[u'к'] = "k"; map[u'л'] = "l"; map[u'м'] = "m"; map[u'н'] = "n";
        map[u'о'] = "o"; map[u'п'] = "p"; map[u'р'] = "r"; map[u'с'] = "s"; map[u'т'] = "t";
        map[u'у'] = "u"; map[u'ф'] = "f"; map[u'х'] = "kh"; map[u'ц'] = "ts"; map[u'ч'] = "ch";
        map[u'ш'] = "sh"; map[u'щ'] = "shch"; map[u'ъ'] = ""; map[u'ы'] = "y"; map[u'ь'] = "";
        map[u'э'] = "e"; map[u'ю'] = "yu"; map[u'я'] = "ya";
    }

    QString result;
    for (QChar c : text) {
        if (map.contains(c.toLower())) {
            QString s = map[c.toLower()];
            // Если исходная буква была заглавной, делаем первую букву перевода заглавной
            if (c.isUpper() && !s.isEmpty()) {
                s[0] = s[0].toUpper();
            }
            result.append(s);
        } else if (c.isLetterOrNumber()) {
            result.append(c);
        } else if (c.isSpace()) {
            result.append("_");
        }
    }
    return result;
}
// Вставьте это в конец файла propertiesdialog.cpp

void PropertiesDialog::onOkClicked()
{
    // Мы убрали блокирующие сообщения (QMessageBox).
    // Теперь красная волнистая линия (ошибки) будет видна в текстовом поле
    // благодаря ScxmlHighlighter, но программа позволит нажать ОК и сохранить текст.

    accept();
}
