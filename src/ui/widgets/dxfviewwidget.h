#ifndef FOXBENDER_DXFVIEWWIDGET_H
#define FOXBENDER_DXFVIEWWIDGET_H

#include "src/core/geometrymodel.h"
#include "src/core/dxfdocument.h"

#include <QColor>
#include <QGraphicsView>
#include <QList>

class QGraphicsScene;
class QGraphicsItem;
class QGraphicsEllipseItem;
class QGraphicsPathItem;
class QMouseEvent;
class QEvent;
class QWheelEvent;

class DxfViewWidget : public QGraphicsView
{
    Q_OBJECT

public:
    struct ViewColors
    {
        QColor backgroundColor = QColor(245, 245, 245);
        QColor selectedEntityColor = QColor(220, 40, 40);
        QColor segmentSelectionColor = QColor(255, 140, 0);
        QColor hoverEntityColor = QColor(60, 120, 220);
        QColor armedEntityColor = QColor(255, 140, 0);
        QColor ruleProfileColor = QColor(20, 150, 90);
        QColor activeRuleProfileColor = QColor(0, 170, 110);
        QColor handleColor = QColor(220, 40, 40);
        QColor handleHoverColor = QColor(40, 120, 220);
        QColor handleHoverFillColor = QColor(120, 170, 240);
        QColor snapPreviewColor = QColor(20, 150, 20);
        QColor breakPreviewColor = QColor(0, 120, 255);
        QColor startFlagColor = QColor(0, 170, 110);
        QColor endFlagColor = QColor(255, 140, 0);
        QColor bridgeColor = QColor(230, 40, 140);
        bool fillFlags = true;
        double flagScale = 1.0;
        double handleScale = 1.0;
    };

    explicit DxfViewWidget(QWidget *parent = nullptr);
    void setDocument(const DxfDocument &document);
    void setDocument(const DxfDocument &document, bool fitView);
    void setArmedEntityIndex(int entityIndex);
    void setRuleProfileEntityIndexes(const QList<int> &entityIndexes);
    void setActiveRuleProfileEntityIndexes(const QList<int> &entityIndexes);
    void setCandidateEntityIndexes(const QList<int> &entityIndexes);
    void setModifiedSegmentEntityIndexes(const QList<int> &entityIndexes);
    void setTreeSelectionEntityIndexes(const QList<int> &entityIndexes);
    void setHighlightedBridgeIndexes(const QList<int> &bridgeIndexes);
    void setTreeSelectionBridgeIndexes(const QList<int> &bridgeIndexes);
    void setHoveredCandidateEntityIndexes(const QList<int> &entityIndexes);
    void setHoveredBridgeIndexes(const QList<int> &bridgeIndexes);
    void setRuleProfileGuide(const QPointF &startPoint,
                             const QPointF &nextPoint,
                             bool showArrow,
                             const QPointF &openPoint,
                             bool showOpenPoint);
    void setDetectedBridges(const QList<DetectedBridge> &bridges);
    void setViewColors(const ViewColors &colors);
    void setBreakPreviewEnabled(bool enabled);
    void setPendingTrimPreview(const QPointF &modelPoint, bool visible);
    void selectEntityIndex(int entityIndex);
    const DxfDocument &document() const;
    int selectedEntityIndex() const;
    QList<int> selectedEntityIndexes() const;
    void fitAll();

signals:
    void entitySelected(int entityIndex);
    void entityClicked(int entityIndex, const QPointF &scenePos);
    void entityHovered(int entityIndex);
    void pointerMoved(const QPointF &scenePos);
    void cancelRequested();
    void documentEdited();
    void documentEditCommitted(const DxfDocument &before,
                               const DxfDocument &after,
                               const QString &description);

private:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void handleSceneSelectionChanged();
    void renderDocument(bool fitView);
    void renderHandlesForEntity(int entityIndex);
    void updateHandleHoverStyles();
    void clearHandles();
    void clearSnapPreview();
    void clearBreakPreview();
    void clearPendingTrimPreview();
    void clearRuleProfileGuide();
    void clearBridgeItems();
    void renderRuleProfileGuide();
    void renderDetectedBridges();
    QGraphicsItem *findEntityItemAt(const QPoint &viewPos, int radiusPixels = 6) const;
    QGraphicsItem *findHandleItemAt(const QPoint &viewPos) const;
    QPointF closestPointOnBreakEntity(int entityIndex, const QPointF &scenePos) const;
    void updateItemSelectionStyle(QGraphicsItem *item);
    void updateEntityFromHandleDrag(int entityIndex, int handleType, int handleIndex, const QPointF &deltaScene);
    bool snapDraggedHandle(int entityIndex, int handleType, int handleIndex, QPointF *snapPoint);

    QGraphicsScene *m_scene;
    DxfDocument m_document;
    QList<int> m_ruleProfileEntityIndexes;
    QList<int> m_activeRuleProfileEntityIndexes;
    QList<int> m_candidateEntityIndexes;
    QList<int> m_modifiedSegmentEntityIndexes;
    QList<int> m_treeSelectionEntityIndexes;
    QList<int> m_highlightedBridgeIndexes;
    QList<int> m_treeSelectionBridgeIndexes;
    QList<int> m_hoveredCandidateEntityIndexes;
    QList<int> m_hoveredBridgeIndexes;
    ViewColors m_viewColors;
    QRectF m_viewSceneRect;
    int m_selectedEntityIndex = -1;
    int m_hoveredEntityIndex = -1;
    int m_armedEntityIndex = -1;
    bool m_isPanning = false;
    QPointF m_lastPanScenePos;
    QList<QGraphicsItem *> m_handleItems;
    QList<QGraphicsItem *> m_ruleProfileGuideItems;
    QList<QGraphicsItem *> m_bridgeItems;
    QList<DetectedBridge> m_detectedBridges;
    int m_hoveredHandleEntityIndex = -1;
    int m_hoveredHandleType = -1;
    int m_hoveredHandleIndex = -1;
    QGraphicsEllipseItem *m_snapPreviewItem = nullptr;
    QGraphicsEllipseItem *m_breakPreviewItem = nullptr;
    QGraphicsEllipseItem *m_pendingTrimPreviewItem = nullptr;
    bool m_isDraggingHandle = false;
    int m_dragHandleEntityIndex = -1;
    int m_dragHandleType = -1;
    int m_dragHandleIndex = -1;
    QPointF m_dragStartScenePos;
    QPointF m_lastHandleScenePos;
    QRectF m_dragSceneRect;
    DxfDocument m_documentBeforeEdit;
    bool m_breakPreviewEnabled = false;
    bool m_showRuleProfileGuideArrow = false;
    bool m_showRuleProfileOpenPoint = false;
    QPointF m_ruleProfileGuideStartPoint;
    QPointF m_ruleProfileGuideNextPoint;
    QPointF m_ruleProfileOpenPoint;
};

#endif // FOXBENDER_DXFVIEWWIDGET_H
