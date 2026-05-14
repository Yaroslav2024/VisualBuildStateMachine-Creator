#include "elementsregistry.h"
#include <QDebug>
#include <algorithm>

// ---------------------------
// Вспомогательные inline-функции
// ---------------------------
static bool idEquals(const ElementsRegistry::Id &a, const ElementsRegistry::Id &b) { return a == b; }

// ---------------------------
// Конструктор / деструктор
// ---------------------------
ElementsRegistry::ElementsRegistry(QObject *parent)
    : QObject(parent)
{
    // Ничего особого пока не делаем, контейнеры по-умолчанию пустые.
}

ElementsRegistry::~ElementsRegistry()
{
    // Очистка: графические QPointer будут автоматически разыменованы,
    // но мы очищаем списки для порядка.
    m_anchors.clear();
    m_lines.clear();
    m_arrowHeads.clear();
    m_phantomLines.clear();
    m_phantomPoints.clear();
    m_phantomArrows.clear();
    m_registrations.clear();
}

// ---------------------------
// Anchors
// ---------------------------
ElementsRegistry::Id ElementsRegistry::registerAnchor(const AnchorInfo &info)
{
    AnchorInfo ii = info;
    if (ii.id.isNull())
        ii.id = Id::createUuid();
    m_anchors.append(ii);
    emit anchorRegistered(ii);
    return ii.id;
}

bool ElementsRegistry::unregisterAnchor(const Id &anchorId)
{
    const int before = m_anchors.size();
    auto it = std::remove_if(m_anchors.begin(), m_anchors.end(),
                             [&](const AnchorInfo &a){ return a.id == anchorId; });
    if (it != m_anchors.end()) {
        m_anchors.erase(it, m_anchors.end());
    }
    if (m_anchors.size() != before) {
        emit anchorUnregistered(anchorId);
        return true;
    }
    return false;
}

ElementsRegistry::AnchorInfo ElementsRegistry::findAnchor(const Id &anchorId) const
{
    for (const AnchorInfo &a : m_anchors) {
        if (a.id == anchorId) return a;
    }
    return AnchorInfo(); // пустая структура
}

QList<ElementsRegistry::AnchorInfo> ElementsRegistry::anchors() const
{
    return m_anchors;
}

// ---------------------------
// Lines
// ---------------------------
ElementsRegistry::Id ElementsRegistry::registerLine(const LineInfo &info)
{
    LineInfo ii = info;
    if (ii.id.isNull())
        ii.id = Id::createUuid();
    m_lines.append(ii);
    emit lineRegistered(ii);
    return ii.id;
}

bool ElementsRegistry::unregisterLine(const Id &lineId)
{
    const int before = m_lines.size();
    auto it = std::remove_if(m_lines.begin(), m_lines.end(),
                             [&](const LineInfo &l){ return l.id == lineId; });
    if (it != m_lines.end()) {
        m_lines.erase(it, m_lines.end());
    }
    if (m_lines.size() != before) {
        emit lineUnregistered(lineId);
        return true;
    }
    return false;
}

ElementsRegistry::LineInfo ElementsRegistry::findLine(const Id &lineId) const
{
    for (const LineInfo &l : m_lines) {
        if (l.id == lineId) return l;
    }
    return LineInfo();
}

QList<ElementsRegistry::LineInfo> ElementsRegistry::lines() const
{
    return m_lines;
}

// ---------------------------
// Arrow heads
// ---------------------------
ElementsRegistry::Id ElementsRegistry::registerArrowHead(const ArrowHeadInfo &info)
{
    ArrowHeadInfo ii = info;
    if (ii.id.isNull())
        ii.id = Id::createUuid();
    m_arrowHeads.append(ii);
    emit arrowRegistered(ii);
    return ii.id;
}

bool ElementsRegistry::unregisterArrowHead(const Id &arrowId)
{
    const int before = m_arrowHeads.size();
    auto it = std::remove_if(m_arrowHeads.begin(), m_arrowHeads.end(),
                             [&](const ArrowHeadInfo &a){ return a.id == arrowId; });
    if (it != m_arrowHeads.end()) {
        m_arrowHeads.erase(it, m_arrowHeads.end());
    }
    if (m_arrowHeads.size() != before) {
        emit arrowUnregistered(arrowId);
        return true;
    }
    return false;
}

ElementsRegistry::ArrowHeadInfo ElementsRegistry::findArrowHead(const Id &arrowId) const
{
    for (const ArrowHeadInfo &a : m_arrowHeads) {
        if (a.id == arrowId) return a;
    }
    return ArrowHeadInfo();
}

QList<ElementsRegistry::ArrowHeadInfo> ElementsRegistry::arrowHeads() const
{
    return m_arrowHeads;
}

// ---------------------------
// Phantom (temporary) objects
// ---------------------------
ElementsRegistry::Id ElementsRegistry::createPhantomLine(const PhantomLineInfo &info)
{
    PhantomLineInfo ii = info;
    if (ii.id.isNull())
        ii.id = Id::createUuid();
    m_phantomLines.append(ii);
    emit phantomCreated(ii.id);
    return ii.id;
}

bool ElementsRegistry::removePhantomLine(const Id &phantomId)
{
    const int before = m_phantomLines.size();
    auto it = std::remove_if(m_phantomLines.begin(), m_phantomLines.end(),
                             [&](const PhantomLineInfo &p){ return p.id == phantomId; });
    if (it != m_phantomLines.end()) m_phantomLines.erase(it, m_phantomLines.end());
    if (m_phantomLines.size() != before) {
        emit phantomRemoved(phantomId);
        return true;
    }
    return false;
}

QList<ElementsRegistry::PhantomLineInfo> ElementsRegistry::phantomLines() const
{
    return m_phantomLines;
}

ElementsRegistry::Id ElementsRegistry::createPhantomPoint(const PhantomPointInfo &info)
{
    PhantomPointInfo ii = info;
    if (ii.id.isNull())
        ii.id = Id::createUuid();
    m_phantomPoints.append(ii);
    emit phantomCreated(ii.id);
    return ii.id;
}

bool ElementsRegistry::removePhantomPoint(const Id &phantomId)
{
    const int before = m_phantomPoints.size();
    auto it = std::remove_if(m_phantomPoints.begin(), m_phantomPoints.end(),
                             [&](const PhantomPointInfo &p){ return p.id == phantomId; });
    if (it != m_phantomPoints.end()) m_phantomPoints.erase(it, m_phantomPoints.end());
    if (m_phantomPoints.size() != before) {
        emit phantomRemoved(phantomId);
        return true;
    }
    return false;
}

QList<ElementsRegistry::PhantomPointInfo> ElementsRegistry::phantomPoints() const
{
    return m_phantomPoints;
}

ElementsRegistry::Id ElementsRegistry::createPhantomArrow(const PhantomArrowInfo &info)
{
    PhantomArrowInfo ii = info;
    if (ii.id.isNull())
        ii.id = Id::createUuid();
    m_phantomArrows.append(ii);
    emit phantomCreated(ii.id);
    return ii.id;
}

bool ElementsRegistry::removePhantomArrow(const Id &phantomId)
{
    const int before = m_phantomArrows.size();
    auto it = std::remove_if(m_phantomArrows.begin(), m_phantomArrows.end(),
                             [&](const PhantomArrowInfo &p){ return p.id == phantomId; });
    if (it != m_phantomArrows.end()) m_phantomArrows.erase(it, m_phantomArrows.end());
    if (m_phantomArrows.size() != before) {
        emit phantomRemoved(phantomId);
        return true;
    }
    return false;
}

QList<ElementsRegistry::PhantomArrowInfo> ElementsRegistry::phantomArrows() const
{
    return m_phantomArrows;
}

// ---------------------------
// Registration management
// ---------------------------
void ElementsRegistry::addRegistration(const RegistrationEntry &entry)
{
    RegistrationEntry ee = entry;
    if (ee.id.isNull()) ee.id = Id::createUuid();
    m_registrations.append(ee);
    emit registrationAdded(ee);
}

bool ElementsRegistry::removeRegistration(const Id &regId)
{
    const int before = m_registrations.size();
    auto it = std::remove_if(m_registrations.begin(), m_registrations.end(),
                             [&](const RegistrationEntry &r){ return r.id == regId; });
    if (it != m_registrations.end()) m_registrations.erase(it, m_registrations.end());
    if (m_registrations.size() != before) {
        emit registrationRemoved(regId);
        return true;
    }
    return false;
}

QList<ElementsRegistry::RegistrationEntry> ElementsRegistry::registrations() const
{
    return m_registrations;
}

// ---------------------------
// Attach / detach anchors to blocks
// ---------------------------
bool ElementsRegistry::attachAnchorToBlock(const Id &anchorId, DiagramItem *block)
{
    for (AnchorInfo &a : m_anchors) {
        if (a.id == anchorId) {
            a.attachedBlock = block;
            return true;
        }
    }
    return false;
}

bool ElementsRegistry::detachAnchorFromBlock(const Id &anchorId)
{
    for (AnchorInfo &a : m_anchors) {
        if (a.id == anchorId) {
            a.attachedBlock.clear();
            return true;
        }
    }
    return false;
}

QList<ElementsRegistry::Id> ElementsRegistry::anchorsForBlock(DiagramItem *block) const
{
    QList<Id> out;
    for (const AnchorInfo &a : m_anchors) {
        if (!a.attachedBlock.isNull() && a.attachedBlock.data() == block) {
            out.append(a.id);
        }
    }
    return out;
}

// ---------------------------
// State save/restore (простейшая заглушка — копирование контейнеров)
// ---------------------------
ElementsRegistry::RegistryState ElementsRegistry::saveState() const
{
    RegistryState s;
    s.anchors = m_anchors;
    s.lines = m_lines;
    s.arrows = m_arrowHeads;
    s.phantomLines = m_phantomLines;
    s.phantomPoints = m_phantomPoints;
    s.phantomArrows = m_phantomArrows;
    s.registrations = m_registrations;
    // globalMeta можно оставлять пустым или ставить время/версию
    return s;
}

void ElementsRegistry::restoreState(const RegistryState &state)
{
    // Простое восстановление: заменяем внутренние контейнеры
    m_anchors = state.anchors;
    m_lines = state.lines;
    m_arrowHeads = state.arrows;
    m_phantomLines = state.phantomLines;
    m_phantomPoints = state.phantomPoints;
    m_phantomArrows = state.phantomArrows;
    m_registrations = state.registrations;

    // Можно уведомить слушателей — но по умолчанию не посылаем массу сигналов.
}

// ---------------------------
// Слоты-обёртки
// ---------------------------
void ElementsRegistry::clearAllPhantoms()
{
    // Если есть визуальные объекты — можно также удалить их со сцены здесь.
    m_phantomLines.clear();
    m_phantomPoints.clear();
    m_phantomArrows.clear();
    // уведомляем (можно послать сигналы для каждого удалённого id, но пока общий)
    // нет сигнала "all phantoms cleared", используем phantomRemoved для каждого нет
}

void ElementsRegistry::clearAll()
{
    // Очистим всё
    m_anchors.clear();
    m_lines.clear();
    m_arrowHeads.clear();
    m_phantomLines.clear();
    m_phantomPoints.clear();
    m_phantomArrows.clear();
    m_registrations.clear();
}
