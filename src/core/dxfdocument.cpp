#include "dxfdocument.h"

#include <QtMath>

bool DxfDocument::isEmpty() const
{
    return entities.isEmpty();
}

QRectF DxfDocument::boundingRect() const
{
    QRectF bounds;
    bool hasBounds = false;

    for (const DxfEntity &entity : entities) {
        QRectF entityRect;

        if (entity.type == DxfEntityType::Arc) {
            entityRect = QRectF(entity.center.x() - entity.radius,
                                entity.center.y() - entity.radius,
                                entity.radius * 2.0,
                                entity.radius * 2.0);
        } else {
            for (const QPointF &point : entity.points) {
                if (!hasBounds) {
                    bounds = QRectF(point, point);
                    hasBounds = true;
                } else {
                    bounds = bounds.united(QRectF(point, point));
                }
            }
            continue;
        }

        if (!hasBounds) {
            bounds = entityRect;
            hasBounds = true;
        } else {
            bounds = bounds.united(entityRect);
        }
    }

    if (!hasBounds) {
        return QRectF(-100.0, -100.0, 200.0, 200.0);
    }

    return bounds.adjusted(-20.0, -20.0, 20.0, 20.0);
}

DxfDocument DxfDocument::createDemoDocument()
{
    DxfDocument document;
    document.sourceName = QStringLiteral("demo_bracket.dxf");

    document.layers = {
        {QStringLiteral("OUTLINE"), QColor(30, 30, 30), true, false},
        {QStringLiteral("TOOTHING"), QColor(0, 110, 220), true, false},
        {QStringLiteral("BRIDGE"), QColor(220, 110, 0), true, false}
    };

    DxfEntity baseLine;
    baseLine.id = QStringLiteral("E1");
    baseLine.type = DxfEntityType::Line;
    baseLine.layerName = QStringLiteral("OUTLINE");
    baseLine.color = QColor(30, 30, 30);
    baseLine.points = {QPointF(0.0, 0.0), QPointF(260.0, 0.0)};

    DxfEntity leftSide;
    leftSide.id = QStringLiteral("E2");
    leftSide.type = DxfEntityType::Line;
    leftSide.layerName = QStringLiteral("OUTLINE");
    leftSide.color = QColor(30, 30, 30);
    leftSide.points = {QPointF(0.0, 0.0), QPointF(0.0, 90.0)};

    DxfEntity topArc;
    topArc.id = QStringLiteral("E3");
    topArc.type = DxfEntityType::Arc;
    topArc.layerName = QStringLiteral("OUTLINE");
    topArc.color = QColor(30, 30, 30);
    topArc.center = QPointF(70.0, 90.0);
    topArc.radius = 70.0;
    topArc.startAngleDeg = 180.0;
    topArc.endAngleDeg = 0.0;

    DxfEntity rightSide;
    rightSide.id = QStringLiteral("E4");
    rightSide.type = DxfEntityType::Line;
    rightSide.layerName = QStringLiteral("OUTLINE");
    rightSide.color = QColor(30, 30, 30);
    rightSide.points = {QPointF(140.0, 90.0), QPointF(140.0, 10.0)};

    DxfEntity bridgeLine;
    bridgeLine.id = QStringLiteral("E5");
    bridgeLine.type = DxfEntityType::Line;
    bridgeLine.layerName = QStringLiteral("BRIDGE");
    bridgeLine.color = QColor(220, 110, 0);
    bridgeLine.points = {QPointF(150.0, 10.0), QPointF(205.0, 10.0)};

    DxfEntity toothPath;
    toothPath.id = QStringLiteral("E6");
    toothPath.type = DxfEntityType::Polyline;
    toothPath.layerName = QStringLiteral("TOOTHING");
    toothPath.color = QColor(0, 110, 220);
    toothPath.points = {
        QPointF(205.0, 10.0),
        QPointF(215.0, 18.0),
        QPointF(225.0, 10.0),
        QPointF(235.0, 18.0),
        QPointF(245.0, 10.0),
        QPointF(260.0, 10.0)
    };

    document.entities = {baseLine, leftSide, topArc, rightSide, bridgeLine, toothPath};
    return document;
}
