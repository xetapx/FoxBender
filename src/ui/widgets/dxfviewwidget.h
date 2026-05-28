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
        QColor hoverEntityColor = QColor(60, 120, 220);
        QColor armedEntityColor = QColor(255, 140, 0);
        QColor bladeLineColor = QColor(20, 150, 90);
        QColor activeBladeLineColor = QColor(0, 170, 110);
        QColor handleColor = QColor(220, 40, 40);
        QColor handleHoverColor = QColor(40, 120, 220);
        QColor handleHoverFillColor = QColor(120, 170, 240);
        QColor snapPreviewColor = QColor(20, 150, 20);
        QColor breakPreviewColor = QColor(0, 120, 255);
        QColor startFlagColor = QColor(0, 170, 110);
        QColor endFlagColor = QColor(255, 140, 0);
        QColor portColor = QColor(230, 40, 140);
        bool fillFlags = true;
        double flagScale = 1.0;
    };

    explicit DxfViewWidget(QWidget *parent = nullptr);
    void setDocument(const DxfDocument &document);
    void setDocument(const DxfDocument &document, bool fitView);
    void setArmedEntityIndex(int entityIndex);
    void setBladeLineEntityIndexes(const QList<int> &entityIndexes);
    void setActiveBladeLineEntityIndexes(const QList<int> &entityIndexes);
    void setCandidateEntityIndexes(const QList<int> &entityIndexes);
    void setHighlightedPortIndexes(const QList<int> &portIndexes);
    void setBladeLineGuide(const QPointF &startPoint,
                           const QPointF &nextPoint,
                           bool showArrow,
                           const QPointF &openPoint,
                           bool showOpenPoint);
    void setDetectedPorts(const QList<DetectedPort> &ports);
    void setViewColors(const ViewColors &colors);
    void setBreakPreviewEnabled(bool enabled);
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
    void clearBladeLineGuide();
    void clearPortItems();
    void renderBladeLineGuide();
    void renderDetectedPorts();
    QGraphicsItem *findHandleItemAt(const QPoint &viewPos) const;
    QPointF closestPointOnLineEntity(int entityIndex, const QPointF &scenePos) const;
    void updateItemSelectionStyle(QGraphicsItem *item);
    void updateEntityFromHandleDrag(int entityIndex, int handleType, int handleIndex, const QPointF &deltaScene);
    bool snapDraggedHandle(int entityIndex, int handleType, int handleIndex, QPointF *snapPoint);

    QGraphicsScene *m_scene;
    DxfDocument m_document;
    QList<int> m_bladeLineEntityIndexes;
    QList<int> m_activeBladeLineEntityIndexes;
    QList<int> m_candidateEntityIndexes;
    QList<int> m_highlightedPortIndexes;
    ViewColors m_viewColors;
    QRectF m_viewSceneRect;
    int m_selectedEntityIndex = -1;
    int m_hoveredEntityIndex = -1;
    int m_armedEntityIndex = -1;
    bool m_isPanning = false;
    QPointF m_lastPanScenePos;
    QList<QGraphicsItem *> m_handleItems;
    QList<QGraphicsItem *> m_bladeLineGuideItems;
    QList<QGraphicsItem *> m_portItems;
    QList<DetectedPort> m_detectedPorts;
    int m_hoveredHandleEntityIndex = -1;
    int m_hoveredHandleType = -1;
    int m_hoveredHandleIndex = -1;
    QGraphicsEllipseItem *m_snapPreviewItem = nullptr;
    QGraphicsEllipseItem *m_breakPreviewItem = nullptr;
    bool m_isDraggingHandle = false;
    int m_dragHandleEntityIndex = -1;
    int m_dragHandleType = -1;
    int m_dragHandleIndex = -1;
    QPointF m_dragStartScenePos;
    QPointF m_lastHandleScenePos;
    QRectF m_dragSceneRect;
    DxfDocument m_documentBeforeEdit;
    bool m_breakPreviewEnabled = false;
    bool m_showBladeLineGuideArrow = false;
    bool m_showBladeLineOpenPoint = false;
    QPointF m_bladeLineGuideStartPoint;
    QPointF m_bladeLineGuideNextPoint;
    QPointF m_bladeLineOpenPoint;
};

#endif // FOXBENDER_DXFVIEWWIDGET_H
