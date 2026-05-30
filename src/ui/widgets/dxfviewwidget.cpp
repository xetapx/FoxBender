#include "dxfviewwidget.h"

#include <QGraphicsPathItem>
#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPen>
#include <QPolygonF>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>
#include <QTransform>
#include <limits>
#include <QtMath>
#include <QWheelEvent>

namespace
{
constexpr int EntityIndexRole = 0;
constexpr int EntityColorRole = 1;
constexpr int HandleTypeRole = 2;
constexpr int HandleIndexRole = 3;

constexpr int HandleTypePoint = 0;
constexpr int HandleTypeMid = 1;

QPointF modelToScenePoint(const QPointF &point)
{
    return QPointF(point.x(), -point.y());
}

QPointF sceneToModelPoint(const QPointF &point)
{
    return QPointF(point.x(), -point.y());
}

QRectF modelToSceneRect(const QRectF &rect)
{
    return QRectF(rect.left(), -rect.bottom(), rect.width(), rect.height());
}

QPointF arcPointAtAngle(const QPointF &center, double radius, double angleDeg)
{
    const double angleRad = qDegreesToRadians(angleDeg);
    return QPointF(center.x() + radius * qCos(angleRad),
                   center.y() + radius * qSin(angleRad));
}

double normalizeAngle360(double angleDeg)
{
    while (angleDeg < 0.0) {
        angleDeg += 360.0;
    }
    while (angleDeg >= 360.0) {
        angleDeg -= 360.0;
    }
    return angleDeg;
}

double distancePointToSegment(const QPointF &point, const QPointF &start, const QPointF &end)
{
    const QLineF segment(start, end);
    const double lengthSquared = segment.length() * segment.length();
    if (qFuzzyIsNull(lengthSquared)) {
        return QLineF(point, start).length();
    }

    const QPointF segmentVector = end - start;
    const double t = qBound(0.0,
                            ((point.x() - start.x()) * segmentVector.x()
                             + (point.y() - start.y()) * segmentVector.y()) / lengthSquared,
                            1.0);
    const QPointF projection(start.x() + segmentVector.x() * t,
                             start.y() + segmentVector.y() * t);
    return QLineF(point, projection).length();
}

bool angleWithinArcSweep(double angleDeg, double startDeg, double endDeg)
{
    const double normalizedAngle = normalizeAngle360(angleDeg);
    const double normalizedStart = normalizeAngle360(startDeg);
    double normalizedEnd = normalizeAngle360(endDeg);
    while (normalizedEnd < normalizedStart) {
        normalizedEnd += 360.0;
    }

    double candidateAngle = normalizedAngle;
    while (candidateAngle < normalizedStart) {
        candidateAngle += 360.0;
    }
    return candidateAngle >= normalizedStart && candidateAngle <= normalizedEnd;
}

double distanceToEntity(const DxfEntity &entity, const QPointF &modelPoint)
{
    if (entity.type == DxfEntityType::Line && entity.points.size() >= 2) {
        return distancePointToSegment(modelPoint, entity.points.at(0), entity.points.at(1));
    }

    if (entity.type == DxfEntityType::Polyline && entity.points.size() >= 2) {
        double bestDistance = std::numeric_limits<double>::max();
        for (int index = 1; index < entity.points.size(); ++index) {
            bestDistance = qMin(bestDistance,
                                distancePointToSegment(modelPoint,
                                                       entity.points.at(index - 1),
                                                       entity.points.at(index)));
        }
        return bestDistance;
    }

    if (entity.type == DxfEntityType::Arc && entity.radius > 0.0) {
        const double pointAngleDeg = qRadiansToDegrees(qAtan2(modelPoint.y() - entity.center.y(),
                                                              modelPoint.x() - entity.center.x()));
        const double radialDistance = qAbs(QLineF(modelPoint, entity.center).length() - entity.radius);
        if (angleWithinArcSweep(pointAngleDeg, entity.startAngleDeg, entity.endAngleDeg)) {
            return radialDistance;
        }

        const QPointF startPoint = arcPointAtAngle(entity.center, entity.radius, entity.startAngleDeg);
        const QPointF endPoint = arcPointAtAngle(entity.center, entity.radius, entity.endAngleDeg);
        return qMin(QLineF(modelPoint, startPoint).length(),
                    QLineF(modelPoint, endPoint).length());
    }

    return std::numeric_limits<double>::max();
}

QPointF entityMidPoint(const DxfEntity &entity)
{
    if (entity.type == DxfEntityType::Line && entity.points.size() == 2) {
        return (entity.points.at(0) + entity.points.at(1)) * 0.5;
    }

    if (entity.type == DxfEntityType::Arc) {
        double startDeg = entity.startAngleDeg;
        double endDeg = entity.endAngleDeg;
        while (endDeg < startDeg) {
            endDeg += 360.0;
        }
        return arcPointAtAngle(entity.center, entity.radius, startDeg + (endDeg - startDeg) * 0.5);
    }

    if (!entity.points.isEmpty()) {
        if (entity.points.size() == 1) {
            return entity.points.first();
        }

        QRectF bounds(entity.points.first(), entity.points.first());
        for (const QPointF &point : entity.points) {
            bounds = bounds.united(QRectF(point, point));
        }
        return bounds.center();
    }

    return entity.center;
}

double pointDotProduct(const QPointF &left, const QPointF &right)
{
    return left.x() * right.x() + left.y() * right.y();
}

double normalizedArcSweepDegrees(double startAngleDeg, double endAngleDeg)
{
    double sweepDeg = endAngleDeg - startAngleDeg;
    while (sweepDeg < 0.0) {
        sweepDeg += 360.0;
    }
    if (qFuzzyIsNull(sweepDeg)) {
        sweepDeg = 360.0;
    }
    return sweepDeg;
}

bool setArcEndpointWithFixedOpposite(DxfEntity &entity, int handleIndex, const QPointF &movedPoint)
{
    if (entity.type != DxfEntityType::Arc || (handleIndex != 0 && handleIndex != 1)) {
        return false;
    }

    const QPointF startPoint = arcPointAtAngle(entity.center, entity.radius, entity.startAngleDeg);
    const QPointF endPoint = arcPointAtAngle(entity.center, entity.radius, entity.endAngleDeg);
    const QPointF fixedPoint = handleIndex == 0 ? endPoint : startPoint;
    const QPointF newStartPoint = handleIndex == 0 ? movedPoint : fixedPoint;
    const QPointF newEndPoint = handleIndex == 0 ? fixedPoint : movedPoint;
    const double sweepDeg = normalizedArcSweepDegrees(entity.startAngleDeg, entity.endAngleDeg);
    const double sweepRad = qDegreesToRadians(sweepDeg);
    const double chordLength = QLineF(newStartPoint, newEndPoint).length();

    if (qFuzzyIsNull(chordLength)
        || qAbs(qSin(sweepRad * 0.5)) < 1e-6
        || qAbs(qTan(sweepRad * 0.5)) < 1e-6) {
        return false;
    }

    const double radius = chordLength / (2.0 * qSin(sweepRad * 0.5));
    const double centerOffset = chordLength / (2.0 * qTan(sweepRad * 0.5));
    const QPointF chordVector = newEndPoint - newStartPoint;
    const QPointF midpoint = (newStartPoint + newEndPoint) * 0.5;
    const QPointF leftNormal(-chordVector.y() / chordLength, chordVector.x() / chordLength);
    const QPointF center = midpoint + leftNormal * centerOffset;

    entity.center = center;
    entity.radius = radius;
    entity.startAngleDeg = qRadiansToDegrees(qAtan2(newStartPoint.y() - center.y(),
                                                    newStartPoint.x() - center.x()));
    entity.endAngleDeg = qRadiansToDegrees(qAtan2(newEndPoint.y() - center.y(),
                                                  newEndPoint.x() - center.x()));
    return true;
}

bool isLayerVisible(const DxfDocument &document, const QString &layerName)
{
    for (const LayerDefinition &layer : document.layers) {
        if (layer.name == layerName) {
            return layer.visible;
        }
    }
    return true;
}

class SelectableLineItem final : public QGraphicsLineItem
{
public:
    using QGraphicsLineItem::QGraphicsLineItem;

    QPainterPath shape() const override
    {
        QPainterPath path;
        path.moveTo(line().p1());
        path.lineTo(line().p2());

        QPainterPathStroker stroker;
        stroker.setWidth(14.0);
        return stroker.createStroke(path);
    }

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override
    {
        QStyleOptionGraphicsItem cleanOption(*option);
        cleanOption.state &= ~QStyle::State_Selected;
        QGraphicsLineItem::paint(painter, &cleanOption, widget);
    }
};

class SelectablePathItem final : public QGraphicsPathItem
{
public:
    using QGraphicsPathItem::QGraphicsPathItem;

    QPainterPath shape() const override
    {
        QPainterPathStroker stroker;
        stroker.setWidth(14.0);
        return stroker.createStroke(path());
    }

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override
    {
        QStyleOptionGraphicsItem cleanOption(*option);
        cleanOption.state &= ~QStyle::State_Selected;
        QGraphicsPathItem::paint(painter, &cleanOption, widget);
    }
};

QPainterPath buildArcPath(const DxfEntity &entity)
{
    QPainterPath path;
    if (entity.radius <= 0.0) {
        return path;
    }

    double startDeg = entity.startAngleDeg;
    double endDeg = entity.endAngleDeg;
    while (endDeg < startDeg) {
        endDeg += 360.0;
    }

    double sweepDeg = endDeg - startDeg;
    if (qFuzzyIsNull(sweepDeg)) {
        sweepDeg = 360.0;
    }

    const int segmentCount = qMax(24, static_cast<int>(qCeil(sweepDeg / 7.5)));
    for (int i = 0; i <= segmentCount; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(segmentCount);
        const double angleDeg = startDeg + sweepDeg * t;
        const double angleRad = qDegreesToRadians(angleDeg);
        const QPointF point = modelToScenePoint(QPointF(entity.center.x() + entity.radius * qCos(angleRad),
                                                        entity.center.y() + entity.radius * qSin(angleRad)));

        if (i == 0) {
            path.moveTo(point);
        } else {
            path.lineTo(point);
        }
    }

    return path;
}
}

DxfViewWidget::DxfViewWidget(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setBackgroundBrush(m_viewColors.backgroundColor);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setMouseTracking(true);

    connect(m_scene, &QGraphicsScene::selectionChanged,
            this, &DxfViewWidget::handleSceneSelectionChanged);
}

void DxfViewWidget::setDocument(const DxfDocument &document)
{
    setDocument(document, true);
}

void DxfViewWidget::setDocument(const DxfDocument &document, bool fitView)
{
    m_document = document;
    m_selectedEntityIndex = -1;
    m_hoveredEntityIndex = -1;
    m_hoveredHandleEntityIndex = -1;
    m_hoveredHandleType = -1;
    m_hoveredHandleIndex = -1;
    clearHandles();
    clearSnapPreview();
    clearBreakPreview();
    clearPendingTrimPreview();
    clearRuleProfileGuide();
    renderDocument(fitView);
}

void DxfViewWidget::setArmedEntityIndex(int entityIndex)
{
    m_armedEntityIndex = entityIndex;

    const QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (item->data(EntityIndexRole).isValid()) {
            updateItemSelectionStyle(item);
        }
    }
}

void DxfViewWidget::setRuleProfileEntityIndexes(const QList<int> &entityIndexes)
{
    m_ruleProfileEntityIndexes = entityIndexes;

    const QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (item->data(EntityIndexRole).isValid()) {
            updateItemSelectionStyle(item);
        }
    }
}

void DxfViewWidget::setActiveRuleProfileEntityIndexes(const QList<int> &entityIndexes)
{
    m_activeRuleProfileEntityIndexes = entityIndexes;

    const QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (item->data(EntityIndexRole).isValid()) {
            updateItemSelectionStyle(item);
        }
    }
}

void DxfViewWidget::setCandidateEntityIndexes(const QList<int> &entityIndexes)
{
    m_candidateEntityIndexes = entityIndexes;

    const QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (item->data(EntityIndexRole).isValid()) {
            updateItemSelectionStyle(item);
        }
    }
}

void DxfViewWidget::setTreeSelectionEntityIndexes(const QList<int> &entityIndexes)
{
    m_treeSelectionEntityIndexes = entityIndexes;

    const QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (item->data(EntityIndexRole).isValid()) {
            updateItemSelectionStyle(item);
        }
    }
}

void DxfViewWidget::setHighlightedPortIndexes(const QList<int> &portIndexes)
{
    m_highlightedPortIndexes = portIndexes;
    clearPortItems();
    renderDetectedPorts();
}

void DxfViewWidget::setTreeSelectionPortIndexes(const QList<int> &portIndexes)
{
    m_treeSelectionPortIndexes = portIndexes;
    clearPortItems();
    renderDetectedPorts();
}

void DxfViewWidget::setHoveredCandidateEntityIndexes(const QList<int> &entityIndexes)
{
    m_hoveredCandidateEntityIndexes = entityIndexes;

    const QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (item->data(EntityIndexRole).isValid()) {
            updateItemSelectionStyle(item);
        }
    }
}

void DxfViewWidget::setHoveredPortIndexes(const QList<int> &portIndexes)
{
    m_hoveredPortIndexes = portIndexes;
    clearPortItems();
    renderDetectedPorts();
}

void DxfViewWidget::setRuleProfileGuide(const QPointF &startPoint,
                                        const QPointF &nextPoint,
                                        bool showArrow,
                                        const QPointF &openPoint,
                                        bool showOpenPoint)
{
    m_ruleProfileGuideStartPoint = startPoint;
    m_ruleProfileGuideNextPoint = nextPoint;
    m_showRuleProfileGuideArrow = showArrow;
    m_ruleProfileOpenPoint = openPoint;
    m_showRuleProfileOpenPoint = showOpenPoint;

    clearRuleProfileGuide();
    renderRuleProfileGuide();
}

void DxfViewWidget::setDetectedPorts(const QList<DetectedPort> &ports)
{
    m_detectedPorts = ports;
    clearPortItems();
    renderDetectedPorts();
}

void DxfViewWidget::setViewColors(const ViewColors &colors)
{
    m_viewColors = colors;
    setBackgroundBrush(m_viewColors.backgroundColor);

    const QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (item->data(EntityIndexRole).isValid()) {
            updateItemSelectionStyle(item);
        }
    }

    clearRuleProfileGuide();
    renderRuleProfileGuide();
    clearPortItems();
    renderDetectedPorts();
}

void DxfViewWidget::setBreakPreviewEnabled(bool enabled)
{
    m_breakPreviewEnabled = enabled;
    if (!enabled) {
        clearBreakPreview();
    }
}

void DxfViewWidget::setPendingTrimPreview(const QPointF &modelPoint, bool visible)
{
    clearPendingTrimPreview();
    if (!visible || m_scene == nullptr) {
        return;
    }

    const QPointF scenePoint = modelToScenePoint(modelPoint);
    m_pendingTrimPreviewItem = m_scene->addEllipse(-4.0,
                                                   -4.0,
                                                   8.0,
                                                   8.0,
                                                   QPen(m_viewColors.hoverEntityColor, 1.4),
                                                   QBrush(Qt::white));
    m_pendingTrimPreviewItem->setPos(scenePoint);
    m_pendingTrimPreviewItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    m_pendingTrimPreviewItem->setZValue(12.5);
}

void DxfViewWidget::selectEntityIndex(int entityIndex)
{
    const QList<QGraphicsItem *> sceneItems = m_scene->items();
    for (QGraphicsItem *item : sceneItems) {
        if (item->data(EntityIndexRole).isValid()) {
            const bool isTarget = item->data(EntityIndexRole).toInt() == entityIndex;
            item->setSelected(isTarget);
            if (isTarget) {
                centerOn(item->boundingRect().center() + item->scenePos());
            }
        }
    }
}

const DxfDocument &DxfViewWidget::document() const
{
    return m_document;
}

int DxfViewWidget::selectedEntityIndex() const
{
    return m_selectedEntityIndex;
}

QList<int> DxfViewWidget::selectedEntityIndexes() const
{
    QList<int> indexes;
    const QList<QGraphicsItem *> selectedItems = m_scene->selectedItems();
    for (QGraphicsItem *item : selectedItems) {
        if (item->data(EntityIndexRole).isValid()) {
            const int index = item->data(EntityIndexRole).toInt();
            if (!indexes.contains(index)) {
                indexes.append(index);
            }
        }
    }
    std::sort(indexes.begin(), indexes.end());
    return indexes;
}

void DxfViewWidget::fitAll()
{
    if (m_scene == nullptr || m_scene->items().isEmpty()) {
        return;
    }

    resetTransform();
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void DxfViewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        emit cancelRequested();
        event->accept();
        return;
    }

    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanScenePos = mapToScene(event->pos());
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && !m_breakPreviewEnabled) {
        if (QGraphicsItem *item = findHandleItemAt(event->pos())) {
            if (item->data(HandleTypeRole).isValid()) {
                m_isDraggingHandle = true;
                m_dragHandleEntityIndex = item->data(EntityIndexRole).toInt();
                m_dragHandleType = item->data(HandleTypeRole).toInt();
                m_dragHandleIndex = item->data(HandleIndexRole).toInt();
                m_dragStartScenePos = sceneToModelPoint(mapToScene(event->pos()));
                m_lastHandleScenePos = sceneToModelPoint(mapToScene(event->pos()));
                m_dragSceneRect = m_scene->sceneRect();
                m_documentBeforeEdit = m_document;
                setCursor(Qt::CrossCursor);
                event->accept();
                return;
            }
        }
    }

    if (event->button() == Qt::LeftButton) {
        if (QGraphicsItem *item = findEntityItemAt(event->pos(), 7)) {
            if (item->data(EntityIndexRole).isValid() && !item->data(HandleTypeRole).isValid()) {
                emit entityClicked(item->data(EntityIndexRole).toInt(),
                                   sceneToModelPoint(mapToScene(event->pos())));
            }
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void DxfViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    emit pointerMoved(sceneToModelPoint(mapToScene(event->pos())));

    if (QGraphicsItem *handleItem = findHandleItemAt(event->pos())) {
        m_hoveredHandleEntityIndex = handleItem->data(EntityIndexRole).toInt();
        m_hoveredHandleType = handleItem->data(HandleTypeRole).toInt();
        m_hoveredHandleIndex = handleItem->data(HandleIndexRole).toInt();
    } else {
        m_hoveredHandleEntityIndex = -1;
        m_hoveredHandleType = -1;
        m_hoveredHandleIndex = -1;
    }
    updateHandleHoverStyles();

    if (m_isDraggingHandle) {
        const QPointF modelPos = sceneToModelPoint(mapToScene(event->pos()));
        const QPointF deltaScene = modelPos - m_dragStartScenePos;
        m_lastHandleScenePos = modelPos;
        m_document = m_documentBeforeEdit;

        updateEntityFromHandleDrag(m_dragHandleEntityIndex,
                                   m_dragHandleType,
                                   m_dragHandleIndex,
                                   deltaScene);

        QPointF snapPoint;
        const bool snapped = snapDraggedHandle(m_dragHandleEntityIndex,
                                               m_dragHandleType,
                                               m_dragHandleIndex,
                                               &snapPoint);

        renderDocument(false);
        m_scene->setSceneRect(m_dragSceneRect);

        const QList<QGraphicsItem *> items = m_scene->items();
        for (QGraphicsItem *item : items) {
            if (item->data(EntityIndexRole).toInt() == m_dragHandleEntityIndex) {
                item->setSelected(true);
                break;
            }
        }

        if (snapped) {
            const QPointF sceneSnapPoint = modelToScenePoint(snapPoint);
            m_snapPreviewItem = m_scene->addEllipse(sceneSnapPoint.x() - 3.5,
                                                    sceneSnapPoint.y() - 3.5,
                                                    7.0,
                                                    7.0,
                                                    QPen(m_viewColors.snapPreviewColor, 0.0),
                                                    QBrush(Qt::NoBrush));
            m_snapPreviewItem->setZValue(12.0);
        }

        emit documentEdited();
        event->accept();
        return;
    }

    if (m_isPanning) {
        const QPointF currentScenePos = mapToScene(event->pos());
        const QPointF sceneDelta = currentScenePos - m_lastPanScenePos;
        m_lastPanScenePos = currentScenePos;
        centerOn(mapToScene(viewport()->rect().center()) - sceneDelta);
        event->accept();
        return;
    }

    if (m_breakPreviewEnabled && m_armedEntityIndex >= 0) {
        clearBreakPreview();
        if (QGraphicsItem *item = findEntityItemAt(event->pos(), 7)) {
            if (item->data(EntityIndexRole).isValid()
                && !item->data(HandleTypeRole).isValid()
                && item->data(EntityIndexRole).toInt() == m_armedEntityIndex) {
                const QPointF snapPoint = modelToScenePoint(
                    closestPointOnBreakEntity(m_armedEntityIndex,
                                              sceneToModelPoint(mapToScene(event->pos()))));
                const DxfEntity &entity = m_document.entities.at(m_armedEntityIndex);
                if ((entity.type == DxfEntityType::Line && entity.points.size() == 2)
                    || entity.type == DxfEntityType::Arc) {
                    m_breakPreviewItem = m_scene->addEllipse(snapPoint.x() - 3.0,
                                                             snapPoint.y() - 3.0,
                                                             6.0,
                                                             6.0,
                                                             QPen(m_viewColors.breakPreviewColor, 0.0),
                                                             QBrush(Qt::white));
                    m_breakPreviewItem->setZValue(12.0);
                }
            }
        }
    }

    int hoveredEntityIndex = -1;
    if (QGraphicsItem *item = findEntityItemAt(event->pos(), 7)) {
        if (item->data(EntityIndexRole).isValid() && !item->data(HandleTypeRole).isValid()) {
            hoveredEntityIndex = item->data(EntityIndexRole).toInt();
        }
    }

    if (hoveredEntityIndex != m_hoveredEntityIndex) {
        m_hoveredEntityIndex = hoveredEntityIndex;
        const QList<QGraphicsItem *> sceneItems = m_scene->items();
        for (QGraphicsItem *item : sceneItems) {
            if (item->data(EntityIndexRole).isValid()) {
                updateItemSelectionStyle(item);
            }
        }
        emit entityHovered(m_hoveredEntityIndex);
    }

    QGraphicsView::mouseMoveEvent(event);
}

void DxfViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_isDraggingHandle) {
        const DxfDocument documentAfterEdit = m_document;
        m_isDraggingHandle = false;
        m_dragHandleEntityIndex = -1;
        m_dragHandleType = -1;
        m_dragHandleIndex = -1;
        m_dragStartScenePos = {};
        m_dragSceneRect = {};
        clearSnapPreview();
        unsetCursor();
        emit documentEdited();
        emit documentEditCommitted(m_documentBeforeEdit,
                                   documentAfterEdit,
                                   QStringLiteral("Move DXF geometry handle"));
        event->accept();
        return;
    }

    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        m_lastPanScenePos = {};
        unsetCursor();
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void DxfViewWidget::leaveEvent(QEvent *event)
{
    if (m_hoveredEntityIndex != -1) {
        m_hoveredEntityIndex = -1;
        const QList<QGraphicsItem *> sceneItems = m_scene->items();
        for (QGraphicsItem *item : sceneItems) {
            if (item->data(EntityIndexRole).isValid()) {
                updateItemSelectionStyle(item);
            }
        }
        emit entityHovered(-1);
    }

    m_hoveredHandleEntityIndex = -1;
    m_hoveredHandleType = -1;
    m_hoveredHandleIndex = -1;
    updateHandleHoverStyles();

    QGraphicsView::leaveEvent(event);
}

void DxfViewWidget::wheelEvent(QWheelEvent *event)
{
    constexpr double zoomInFactor = 1.15;
    constexpr double zoomOutFactor = 1.0 / zoomInFactor;

    const double factor = event->angleDelta().y() > 0 ? zoomInFactor : zoomOutFactor;
    scale(factor, factor);
    event->accept();
}

void DxfViewWidget::handleSceneSelectionChanged()
{
    m_selectedEntityIndex = -1;

    const QList<QGraphicsItem *> selectedItems = m_scene->selectedItems();
    if (!selectedItems.isEmpty()) {
        m_selectedEntityIndex = selectedItems.first()->data(0).toInt();
    }

    const QList<QGraphicsItem *> items = m_scene->items();
    for (QGraphicsItem *item : items) {
        if (item->data(EntityIndexRole).isValid()) {
            updateItemSelectionStyle(item);
        }
    }

    clearHandles();
    clearSnapPreview();
    clearBreakPreview();
    clearPendingTrimPreview();
    if (m_selectedEntityIndex >= 0) {
        renderHandlesForEntity(m_selectedEntityIndex);
    }
    renderRuleProfileGuide();

    emit entitySelected(m_selectedEntityIndex);
}

void DxfViewWidget::renderDocument(bool fitView)
{
    m_scene->clear();
    m_handleItems.clear();
    m_ruleProfileGuideItems.clear();
    m_portItems.clear();
    m_snapPreviewItem = nullptr;
    m_breakPreviewItem = nullptr;

    const QRectF bounds = modelToSceneRect(m_document.boundingRect());
    if (m_isDraggingHandle && !m_dragSceneRect.isNull()) {
        m_scene->setSceneRect(m_dragSceneRect);
    } else if (fitView || m_viewSceneRect.isNull()) {
        m_viewSceneRect = bounds;
        m_scene->setSceneRect(m_viewSceneRect);
    } else {
        m_viewSceneRect = m_viewSceneRect.united(bounds);
        m_scene->setSceneRect(m_viewSceneRect);
    }

    if (m_document.isEmpty()) {
        auto *placeholderText = m_scene->addSimpleText(QStringLiteral("No DXF content"));
        placeholderText->setBrush(QBrush(QColor(80, 80, 80)));
        placeholderText->setPos(m_scene->sceneRect().center());
        if (fitView) {
            fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
        }
        return;
    }

    for (int entityIndex = 0; entityIndex < m_document.entities.size(); ++entityIndex) {
        const DxfEntity &entity = m_document.entities.at(entityIndex);
        if (!isLayerVisible(m_document, entity.layerName)) {
            continue;
        }
        const QPen pen(entity.color, 0.0);
        QGraphicsItem *graphicsItem = nullptr;

        switch (entity.type) {
        case DxfEntityType::Line:
            if (entity.points.size() >= 2) {
                auto *item = new SelectableLineItem(QLineF(modelToScenePoint(entity.points.at(0)),
                                                          modelToScenePoint(entity.points.at(1))));
                item->setPen(pen);
                m_scene->addItem(item);
                graphicsItem = item;
            }
            break;
        case DxfEntityType::Arc: {
            auto *item = new SelectablePathItem(buildArcPath(entity));
            item->setPen(pen);
            m_scene->addItem(item);
            graphicsItem = item;
            break;
        }
        case DxfEntityType::Polyline: {
            if (entity.points.isEmpty()) {
                break;
            }
            QPainterPath path(modelToScenePoint(entity.points.first()));
            for (int index = 1; index < entity.points.size(); ++index) {
                path.lineTo(modelToScenePoint(entity.points.at(index)));
            }
            auto *item = new SelectablePathItem(path);
            item->setPen(pen);
            m_scene->addItem(item);
            graphicsItem = item;
            break;
        }
        }

        if (graphicsItem != nullptr) {
            graphicsItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
            graphicsItem->setData(EntityIndexRole, entityIndex);
            graphicsItem->setData(EntityColorRole, entity.color);
            updateItemSelectionStyle(graphicsItem);
        }
    }

    renderDetectedPorts();

    auto *label = m_scene->addSimpleText(QStringLiteral("%1 | %2 entities | %3 layers")
                                             .arg(m_document.sourceName)
                                             .arg(m_document.entities.size())
                                             .arg(m_document.layers.size()));
    label->setBrush(QBrush(QColor(70, 70, 70)));
    label->setScale(0.9);
    label->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    label->setPos(m_scene->sceneRect().left(), m_scene->sceneRect().top());

    renderRuleProfileGuide();

    if (fitView) {
        fitAll();
    }
}

void DxfViewWidget::updateItemSelectionStyle(QGraphicsItem *item)
{
    const QColor baseColor = item->data(EntityColorRole).value<QColor>();
    const int entityIndex = item->data(EntityIndexRole).toInt();
    const bool isRuleProfileEntity = m_ruleProfileEntityIndexes.contains(entityIndex);
    const bool isActiveRuleProfileEntity = m_activeRuleProfileEntityIndexes.contains(entityIndex);
    const bool isCandidateEntity = m_candidateEntityIndexes.contains(entityIndex);
    const bool isTreeSelectedEntity = m_treeSelectionEntityIndexes.contains(entityIndex);
    const bool isHoveredCandidateEntity = m_hoveredCandidateEntityIndexes.contains(entityIndex);
    const bool isArmed = entityIndex == m_armedEntityIndex;
    const bool isHovered = entityIndex == m_hoveredEntityIndex;
    const QColor activeColor = item->isSelected()
                                   ? m_viewColors.selectedEntityColor
                                   : isTreeSelectedEntity ? m_viewColors.segmentSelectionColor
                                   : isArmed ? m_viewColors.armedEntityColor
                                   : isHoveredCandidateEntity ? m_viewColors.hoverEntityColor
                                   : isCandidateEntity ? m_viewColors.armedEntityColor
                                   : isActiveRuleProfileEntity ? m_viewColors.activeRuleProfileColor
                                   : isRuleProfileEntity ? m_viewColors.ruleProfileColor
                                   : isHovered ? m_viewColors.hoverEntityColor
                                               : baseColor;
    const qreal width = item->isSelected() ? 1.8
                        : isTreeSelectedEntity ? 2.6
                        : isArmed ? 1.4
                        : isHoveredCandidateEntity ? 1.8
                        : isCandidateEntity ? 1.4
                        : isActiveRuleProfileEntity ? 1.8
                        : isRuleProfileEntity ? 1.4
                        : isHovered ? 1.2
                                    : 0.0;
    QPen pen(activeColor, width);
    pen.setCosmetic(true);

    if (auto *lineItem = qgraphicsitem_cast<QGraphicsLineItem *>(item)) {
        lineItem->setPen(pen);
        return;
    }

    if (auto *pathItem = qgraphicsitem_cast<QGraphicsPathItem *>(item)) {
        pathItem->setPen(pen);
    }
}

void DxfViewWidget::renderHandlesForEntity(int entityIndex)
{
    if (entityIndex < 0 || entityIndex >= m_document.entities.size()) {
        return;
    }

    const DxfEntity &entity = m_document.entities.at(entityIndex);
    QVector<QPointF> handlePoints = entity.points;
    const qreal handleScale = std::clamp(m_viewColors.handleScale, 0.5, 3.0);
    const qreal pointHalfSize = 2.5 * handleScale;
    const qreal midRadius = 3.0 * handleScale;

    if (entity.type == DxfEntityType::Arc) {
        handlePoints.clear();

        const double startRad = qDegreesToRadians(entity.startAngleDeg);
        const double endRad = qDegreesToRadians(entity.endAngleDeg);
        handlePoints.append(QPointF(entity.center.x() + entity.radius * qCos(startRad),
                                    entity.center.y() + entity.radius * qSin(startRad)));
        handlePoints.append(QPointF(entity.center.x() + entity.radius * qCos(endRad),
                                    entity.center.y() + entity.radius * qSin(endRad)));
        handlePoints.append(entity.center);
    }

    for (const QPointF &point : handlePoints) {
        const QPointF scenePoint = modelToScenePoint(point);
        auto *handle = m_scene->addRect(-pointHalfSize,
                                        -pointHalfSize,
                                        pointHalfSize * 2.0,
                                        pointHalfSize * 2.0,
                                        QPen(m_viewColors.handleColor, 0.0),
                                        QBrush(Qt::NoBrush));
        handle->setFlag(QGraphicsItem::ItemIsSelectable, false);
        handle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        handle->setPos(scenePoint);
        handle->setData(EntityIndexRole, entityIndex);
        handle->setData(HandleTypeRole, HandleTypePoint);
        handle->setData(HandleIndexRole, handlePoints.indexOf(point));
        handle->setZValue(10.0);
        m_handleItems.append(handle);
    }

    const QPointF sceneMidPoint = modelToScenePoint(entityMidPoint(entity));
    auto *midHandle = m_scene->addEllipse(-midRadius,
                                          -midRadius,
                                          midRadius * 2.0,
                                          midRadius * 2.0,
                                          QPen(m_viewColors.handleColor, 0.0),
                                          QBrush(Qt::NoBrush));
    midHandle->setFlag(QGraphicsItem::ItemIsSelectable, false);
    midHandle->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    midHandle->setPos(sceneMidPoint);
    midHandle->setData(EntityIndexRole, entityIndex);
    midHandle->setData(HandleTypeRole, HandleTypeMid);
    midHandle->setData(HandleIndexRole, -1);
    midHandle->setZValue(11.0);
    m_handleItems.append(midHandle);

    updateHandleHoverStyles();
}

void DxfViewWidget::updateHandleHoverStyles()
{
    for (QGraphicsItem *handle : std::as_const(m_handleItems)) {
        if (handle == nullptr) {
            continue;
        }

        const bool isHovered = handle->data(EntityIndexRole).toInt() == m_hoveredHandleEntityIndex
                               && handle->data(HandleTypeRole).toInt() == m_hoveredHandleType
                               && handle->data(HandleIndexRole).toInt() == m_hoveredHandleIndex;

        if (auto *ellipse = qgraphicsitem_cast<QGraphicsEllipseItem *>(handle)) {
            ellipse->setPen(QPen(isHovered ? m_viewColors.handleHoverColor : m_viewColors.handleColor, 0.0));
            ellipse->setBrush(isHovered ? QBrush(m_viewColors.handleHoverFillColor) : QBrush(Qt::NoBrush));
            continue;
        }

        if (auto *rect = qgraphicsitem_cast<QGraphicsRectItem *>(handle)) {
            rect->setPen(QPen(isHovered ? m_viewColors.handleHoverColor : m_viewColors.handleColor, 0.0));
            rect->setBrush(isHovered ? QBrush(m_viewColors.handleHoverFillColor) : QBrush(Qt::NoBrush));
        }
    }
}

QGraphicsItem *DxfViewWidget::findEntityItemAt(const QPoint &viewPos, int radiusPixels) const
{
    const int radius = qMax(0, radiusPixels);
    const QRect searchRect(viewPos.x() - radius,
                           viewPos.y() - radius,
                           radius * 2 + 1,
                           radius * 2 + 1);
    const QList<QGraphicsItem *> nearbyItems = items(searchRect);
    if (nearbyItems.isEmpty()) {
        return nullptr;
    }

    const QPointF modelPoint = sceneToModelPoint(mapToScene(viewPos));
    QGraphicsItem *bestItem = nullptr;
    double bestDistance = std::numeric_limits<double>::max();
    QList<int> seenEntityIndexes;

    for (QGraphicsItem *item : nearbyItems) {
        if (!item->data(EntityIndexRole).isValid() || item->data(HandleTypeRole).isValid()) {
            continue;
        }

        const int entityIndex = item->data(EntityIndexRole).toInt();
        if (seenEntityIndexes.contains(entityIndex)
            || entityIndex < 0
            || entityIndex >= m_document.entities.size()) {
            continue;
        }
        seenEntityIndexes.append(entityIndex);

        const double distance = distanceToEntity(m_document.entities.at(entityIndex), modelPoint);
        if (distance < bestDistance) {
            bestDistance = distance;
            bestItem = item;
        }
    }

    return bestItem;
}

void DxfViewWidget::clearHandles()
{
    for (QGraphicsItem *handle : std::as_const(m_handleItems)) {
        if (handle != nullptr) {
            m_scene->removeItem(handle);
            delete handle;
        }
    }
    m_handleItems.clear();
}

QGraphicsItem *DxfViewWidget::findHandleItemAt(const QPoint &viewPos) const
{
    const QList<QGraphicsItem *> hitItems = items(viewPos);
    for (QGraphicsItem *item : hitItems) {
        if (item->data(HandleTypeRole).isValid()
            && item->data(HandleTypeRole).toInt() == HandleTypePoint) {
            return item;
        }
    }

    for (QGraphicsItem *item : hitItems) {
        if (item->data(HandleTypeRole).isValid()
            && item->data(HandleTypeRole).toInt() == HandleTypeMid) {
            return item;
        }
    }

    return nullptr;
}

void DxfViewWidget::clearSnapPreview()
{
    if (m_snapPreviewItem != nullptr) {
        m_scene->removeItem(m_snapPreviewItem);
        delete m_snapPreviewItem;
        m_snapPreviewItem = nullptr;
    }
}

void DxfViewWidget::clearBreakPreview()
{
    if (m_breakPreviewItem != nullptr) {
        m_scene->removeItem(m_breakPreviewItem);
        delete m_breakPreviewItem;
        m_breakPreviewItem = nullptr;
    }
}

void DxfViewWidget::clearPendingTrimPreview()
{
    if (m_pendingTrimPreviewItem != nullptr) {
        m_scene->removeItem(m_pendingTrimPreviewItem);
        delete m_pendingTrimPreviewItem;
        m_pendingTrimPreviewItem = nullptr;
    }
}

void DxfViewWidget::clearRuleProfileGuide()
{
    for (QGraphicsItem *item : std::as_const(m_ruleProfileGuideItems)) {
        if (item != nullptr) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_ruleProfileGuideItems.clear();
}

void DxfViewWidget::clearPortItems()
{
    for (QGraphicsItem *item : std::as_const(m_portItems)) {
        if (item != nullptr) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_portItems.clear();
}

void DxfViewWidget::renderRuleProfileGuide()
{
    if (m_scene == nullptr) {
        return;
    }

    const qreal scale = std::clamp(m_viewColors.flagScale, 0.5, 3.0);

    if (m_showRuleProfileGuideArrow) {
        const QPointF sceneStart = modelToScenePoint(m_ruleProfileGuideStartPoint);
        const QPointF sceneNext = modelToScenePoint(m_ruleProfileGuideNextPoint);
        const QLineF directionLine(sceneStart, sceneNext);
        if (directionLine.length() > 0.001) {
            const QPointF tangent = directionLine.p2() - directionLine.p1();
            const qreal tangentLength = std::hypot(tangent.x(), tangent.y());
            const QPointF tangentUnit = tangentLength > 0.0
                                            ? QPointF(tangent.x() / tangentLength, tangent.y() / tangentLength)
                                            : QPointF(1.0, 0.0);
            const QPointF normalUnit(-tangentUnit.y(), tangentUnit.x());

            const qreal mastLength = 14.0 * scale;
            const qreal flagLength = 10.0 * scale;
            const qreal flagHalfHeight = 4.0 * scale;
            const qreal markerRadius = 2.5 * scale;

            auto *mastItem = m_scene->addLine(QLineF(QPointF(0.0, 0.0),
                                                     normalUnit * mastLength),
                                              QPen(m_viewColors.startFlagColor, 1.4));
            mastItem->setPos(sceneStart);
            mastItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
            mastItem->setZValue(13.0);
            m_ruleProfileGuideItems.append(mastItem);

            QPolygonF startFlag;
            const QPointF mastTip = normalUnit * mastLength;
            startFlag << mastTip + tangentUnit * flagLength
                      << mastTip + normalUnit * flagHalfHeight
                      << mastTip - normalUnit * flagHalfHeight;
            auto *startFlagItem = m_scene->addPolygon(startFlag,
                                                      QPen(m_viewColors.startFlagColor, 1.2),
                                                      m_viewColors.fillFlags
                                                          ? QBrush(m_viewColors.startFlagColor)
                                                          : QBrush(Qt::NoBrush));
            startFlagItem->setPos(sceneStart);
            startFlagItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
            startFlagItem->setZValue(13.0);
            m_ruleProfileGuideItems.append(startFlagItem);

            auto *startMarker = m_scene->addEllipse(-markerRadius,
                                                    -markerRadius,
                                                    markerRadius * 2.0,
                                                    markerRadius * 2.0,
                                                    QPen(m_viewColors.startFlagColor, 1.2),
                                                    QBrush(Qt::white));
            startMarker->setPos(sceneStart);
            startMarker->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
            startMarker->setZValue(13.0);
            m_ruleProfileGuideItems.append(startMarker);
        }
    }

    if (m_showRuleProfileOpenPoint) {
        const QPointF sceneOpen = modelToScenePoint(m_ruleProfileOpenPoint);
        const QPointF scenePrev = modelToScenePoint(m_ruleProfileGuideNextPoint);
        const QPointF tangent = sceneOpen - scenePrev;
        const qreal tangentLength = std::hypot(tangent.x(), tangent.y());
        const QPointF tangentUnit = tangentLength > 0.0
                                        ? QPointF(tangent.x() / tangentLength, tangent.y() / tangentLength)
                                        : QPointF(1.0, 0.0);
        const QPointF normalUnit(-tangentUnit.y(), tangentUnit.x());

        const qreal mastLength = 14.0 * scale;
        const qreal flagLength = 10.0 * scale;
        const qreal flagHalfHeight = 4.0 * scale;
        const qreal markerRadius = 2.5 * scale;

        auto *mastItem = m_scene->addLine(QLineF(QPointF(0.0, 0.0),
                                                 normalUnit * mastLength),
                                          QPen(m_viewColors.endFlagColor, 1.4));
        mastItem->setPos(sceneOpen);
        mastItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        mastItem->setZValue(13.0);
        m_ruleProfileGuideItems.append(mastItem);

        QPolygonF endFlag;
        const QPointF mastTip = normalUnit * mastLength;
        endFlag << mastTip
                << mastTip - tangentUnit * flagLength + normalUnit * flagHalfHeight
                << mastTip - tangentUnit * flagLength - normalUnit * flagHalfHeight;
        auto *endFlagItem = m_scene->addPolygon(endFlag,
                                                QPen(m_viewColors.endFlagColor, 1.2),
                                                m_viewColors.fillFlags
                                                    ? QBrush(m_viewColors.endFlagColor)
                                                    : QBrush(Qt::NoBrush));
        endFlagItem->setPos(sceneOpen);
        endFlagItem->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        endFlagItem->setZValue(13.0);
        m_ruleProfileGuideItems.append(endFlagItem);

        auto *openMarker = m_scene->addEllipse(-markerRadius,
                                               -markerRadius,
                                               markerRadius * 2.0,
                                               markerRadius * 2.0,
                                               QPen(m_viewColors.endFlagColor, 1.2),
                                               QBrush(Qt::white));
        openMarker->setPos(sceneOpen);
        openMarker->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        openMarker->setZValue(13.0);
        m_ruleProfileGuideItems.append(openMarker);
    }
}

void DxfViewWidget::renderDetectedPorts()
{
    if (m_scene == nullptr) {
        return;
    }

    for (int portIndex = 0; portIndex < m_detectedPorts.size(); ++portIndex) {
        const DetectedPort &port = m_detectedPorts.at(portIndex);
        bool portVisible = true;
        for (int entityIndex : port.relatedEntityIndexes) {
            if (entityIndex >= 0 && entityIndex < m_document.entities.size()) {
                const DxfEntity &relatedEntity = m_document.entities.at(entityIndex);
                if (!isLayerVisible(m_document, relatedEntity.layerName)) {
                    portVisible = false;
                    break;
                }
            }
        }
        if (!portVisible) {
            continue;
        }

        const bool isTreeSelected = m_treeSelectionPortIndexes.contains(portIndex);
        const bool isHighlighted = m_highlightedPortIndexes.contains(portIndex);
        const bool isHovered = m_hoveredPortIndexes.contains(portIndex);
        const QColor penColor = isTreeSelected
                                    ? (m_treeSelectionEntityIndexes.isEmpty()
                                           ? m_viewColors.selectedEntityColor
                                           : m_viewColors.segmentSelectionColor)
                                    : m_viewColors.portColor;
        QPen pen(penColor,
                 isTreeSelected ? 5.2 : isHovered ? 4.6 : isHighlighted ? 3.6 : 2.2,
                 (isTreeSelected || isHovered || isHighlighted) ? Qt::SolidLine : Qt::DashLine);
        pen.setCosmetic(true);
        QGraphicsItem *item = nullptr;
        if (port.type == DetectedPortType::Line) {
            item = m_scene->addLine(QLineF(modelToScenePoint(port.startPoint),
                                           modelToScenePoint(port.endPoint)),
                                    pen);
        } else {
            DxfEntity arcEntity;
            arcEntity.type = DxfEntityType::Arc;
            arcEntity.center = port.center;
            arcEntity.radius = port.radius;
            arcEntity.startAngleDeg = port.startAngleDeg;
            arcEntity.endAngleDeg = port.endAngleDeg;
            auto *pathItem = new QGraphicsPathItem(buildArcPath(arcEntity));
            pathItem->setPen(pen);
            m_scene->addItem(pathItem);
            item = pathItem;
        }

        if (item != nullptr) {
            item->setZValue(6.0);
            m_portItems.append(item);
        }
    }
}

QPointF DxfViewWidget::closestPointOnBreakEntity(int entityIndex, const QPointF &scenePos) const
{
    if (entityIndex < 0 || entityIndex >= m_document.entities.size()) {
        return scenePos;
    }

    const DxfEntity &entity = m_document.entities.at(entityIndex);
    if (entity.type == DxfEntityType::Arc) {
        const QPointF radial = scenePos - entity.center;
        const double radialLength = std::hypot(radial.x(), radial.y());
        if (qFuzzyIsNull(radialLength) || qFuzzyIsNull(entity.radius)) {
            return arcPointAtAngle(entity.center, entity.radius, entity.startAngleDeg);
        }

        double angleDeg = qRadiansToDegrees(std::atan2(radial.y(), radial.x()));
        angleDeg = normalizeAngle360(angleDeg);
        if (!angleWithinArcSweep(angleDeg, entity.startAngleDeg, entity.endAngleDeg)) {
            const QPointF startPoint = arcPointAtAngle(entity.center, entity.radius, entity.startAngleDeg);
            const QPointF endPoint = arcPointAtAngle(entity.center, entity.radius, entity.endAngleDeg);
            return QLineF(scenePos, startPoint).length() <= QLineF(scenePos, endPoint).length()
                       ? startPoint
                       : endPoint;
        }

        return arcPointAtAngle(entity.center, entity.radius, angleDeg);
    }

    if (entity.type != DxfEntityType::Line || entity.points.size() != 2) {
        return scenePos;
    }

    const QPointF start = entity.points.at(0);
    const QPointF end = entity.points.at(1);
    const QPointF lineVector = end - start;
    const double lengthSquared = lineVector.x() * lineVector.x() + lineVector.y() * lineVector.y();
    if (qFuzzyIsNull(lengthSquared)) {
        return start;
    }

    const double t = std::clamp(pointDotProduct(scenePos - start, lineVector) / lengthSquared, 0.0, 1.0);
    return start + lineVector * t;
}

void DxfViewWidget::updateEntityFromHandleDrag(int entityIndex,
                                               int handleType,
                                               int handleIndex,
                                               const QPointF &deltaScene)
{
    if (entityIndex < 0 || entityIndex >= m_document.entities.size()) {
        return;
    }

    DxfEntity &entity = m_document.entities[entityIndex];

    if (handleType == HandleTypeMid) {
        if (entity.type == DxfEntityType::Arc) {
            entity.center += deltaScene;
            return;
        }

        for (QPointF &point : entity.points) {
            point += deltaScene;
        }
        return;
    }

    if (entity.type == DxfEntityType::Arc) {
        if (handleIndex == 0) {
            const QPointF startPoint = arcPointAtAngle(entity.center, entity.radius, entity.startAngleDeg) + deltaScene;
            setArcEndpointWithFixedOpposite(entity, handleIndex, startPoint);
        } else if (handleIndex == 1) {
            const QPointF endPoint = arcPointAtAngle(entity.center, entity.radius, entity.endAngleDeg) + deltaScene;
            setArcEndpointWithFixedOpposite(entity, handleIndex, endPoint);
        } else if (handleIndex == 2) {
            entity.center += deltaScene;
        }
        return;
    }

    if (handleIndex >= 0 && handleIndex < entity.points.size()) {
        entity.points[handleIndex] += deltaScene;
    }
}

bool DxfViewWidget::snapDraggedHandle(int entityIndex,
                                      int handleType,
                                      int handleIndex,
                                      QPointF *snapPoint)
{
    if (handleType != HandleTypePoint || entityIndex < 0 || entityIndex >= m_document.entities.size()) {
        return false;
    }

    const DxfEntity &entity = m_document.entities.at(entityIndex);
    QPointF movingPoint;

    if (entity.type == DxfEntityType::Arc) {
        if (handleIndex == 0) {
            movingPoint = arcPointAtAngle(entity.center, entity.radius, entity.startAngleDeg);
        } else if (handleIndex == 1) {
            movingPoint = arcPointAtAngle(entity.center, entity.radius, entity.endAngleDeg);
        } else {
            return false;
        }
    } else {
        if (handleIndex < 0 || handleIndex >= entity.points.size()) {
            return false;
        }
        movingPoint = entity.points.at(handleIndex);
    }

    const qreal snapThreshold = QLineF(mapToScene(QPoint(0, 0)), mapToScene(QPoint(14, 14))).length();
    qreal bestDistance = snapThreshold;
    bool found = false;
    QPointF bestPoint;

    for (int otherIndex = 0; otherIndex < m_document.entities.size(); ++otherIndex) {
        if (otherIndex == entityIndex) {
            continue;
        }

        const DxfEntity &other = m_document.entities.at(otherIndex);
        QVector<QPointF> candidatePoints;

        if (other.type == DxfEntityType::Arc) {
            candidatePoints.append(arcPointAtAngle(other.center, other.radius, other.startAngleDeg));
            candidatePoints.append(arcPointAtAngle(other.center, other.radius, other.endAngleDeg));
        } else if (!other.points.isEmpty()) {
            candidatePoints.append(other.points.first());
            if (other.points.size() > 1) {
                candidatePoints.append(other.points.last());
            }
        }

        for (const QPointF &candidate : candidatePoints) {
            const qreal distance = QLineF(movingPoint, candidate).length();
            if (distance <= bestDistance) {
                bestDistance = distance;
                bestPoint = candidate;
                found = true;
            }
        }
    }

    if (!found) {
        return false;
    }

    DxfEntity &editableEntity = m_document.entities[entityIndex];
    if (editableEntity.type == DxfEntityType::Arc) {
        if (handleIndex == 0) {
            setArcEndpointWithFixedOpposite(editableEntity, handleIndex, bestPoint);
        } else if (handleIndex == 1) {
            setArcEndpointWithFixedOpposite(editableEntity, handleIndex, bestPoint);
        }
    } else if (handleIndex >= 0 && handleIndex < editableEntity.points.size()) {
        editableEntity.points[handleIndex] = bestPoint;
    }

    if (snapPoint != nullptr) {
        *snapPoint = bestPoint;
    }
    return true;
}
