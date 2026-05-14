#include "grid.h"
#include <cmath>
#include <QGraphicsScene>

// Конструктор
Grid::Grid(qreal step, QGraphicsItem* parent)
    : QGraphicsItem(parent), m_step(step), m_scale(1.0)
{
    setZValue(-100); // Рисуем сетку под всеми элементами
    updateSceneRect();
}

// Устанавливаем коэффициент масштаба (например, при zoom)
void Grid::setScaleFactor(qreal scale)
{
    m_scale = scale;
    update(); // Перерисовываем сетку
}

// Устанавливаем шаг сетки в логических единицах
void Grid::setStep(qreal step)
{
    m_step = step;
    update();
}

// Обновление прямоугольника сцены (например, при изменении размеров)
void Grid::updateSceneRect()
{
    if (scene()) {
        m_sceneRect = scene()->sceneRect();
    } else {
        m_sceneRect = QRectF(-10000, -10000, 20000, 20000); // Большой прямоугольник по умолчанию
    }
    update();
}

// Определяем область, которую занимает сетка
QRectF Grid::boundingRect() const
{
    return m_sceneRect;
}

// Основной метод рисования сетки
void Grid::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(option);

    painter->save();

    // Настраиваем стиль линии: тонкая серая линия
    QPen pen(Qt::lightGray);
    pen.setWidth(0); // тонкая линия 0 означает "в масштабе с экраном"
    painter->setPen(pen);

    // Шаг сетки с учетом текущего масштаба
    qreal stepPx = m_step * m_scale;

    if (stepPx < 4) {
        // Если слишком мелко, не рисуем
        painter->restore();
        return;
    }

    QRectF rect = m_sceneRect;

    // Вертикальные линии
    qreal xStart = std::floor(rect.left() / stepPx) * stepPx;
    qreal xEnd = rect.right();
    for (qreal x = xStart; x <= xEnd; x += stepPx) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }

    // Горизонтальные линии
    qreal yStart = std::floor(rect.top() / stepPx) * stepPx;
    qreal yEnd = rect.bottom();
    for (qreal y = yStart; y <= yEnd; y += stepPx) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }

    painter->restore();
}
