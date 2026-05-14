/*
 * VisualBuildStateMachine Creator
 * Copyright (C) 2026 Yaroslav Donchenko, Yevhenii Donchenko
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 */
#ifndef SCXMLCOMPILER_H
#define SCXMLCOMPILER_H

#include <QObject>
#include <QGraphicsScene>
#include <QList>
#include "diagramitem.h"
#include "connect.h"
#include "codeexecutor.h" // Добавить
#include "bridgeitem.h"
// Структура, описывающая одну найденную ошибку
struct ValidationError {
    QString message;         // Текст ошибки ("Непарная скобка")
    DiagramItem* block;      // Проблемный блок (чтобы выделить его)
    ConnectionItem* arrow;   // Или проблемная стрелка
    bool isCritical;         // true = нельзя продолжать, false = предупреждение
};

class ScxmlCompiler
{
public:
    // Метод, который пробегает по сцене и сохраняет код в исполнитель
        static void collectCode(QGraphicsScene *scene, CodeExecutor *executor);
    // Главный метод: проверяет сцену и возвращает список ошибок
    static QList<ValidationError> validate(QGraphicsScene *scene);

    // Метод генерации (напишем его на следующем этапе, пока заглушка)
    static bool compileToScxml(QGraphicsScene *scene, const QString &filePath);

private:
    // Вспомогательные проверки
    static void checkBlockSyntax(DiagramItem* block, QList<ValidationError> &errors);
    static void checkArrowSyntax(ConnectionItem* arrow, QList<ValidationError> &errors);
};

#endif // SCXMLCOMPILER_H
