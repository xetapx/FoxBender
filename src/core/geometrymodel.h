#ifndef FOXBENDER_GEOMETRYMODEL_H
#define FOXBENDER_GEOMETRYMODEL_H

#include "dxfdocument.h"

#include <QList>

enum class DetectedPortType
{
    Line,
    Arc
};

class DetectedPort
{
public:
    DetectedPortType type = DetectedPortType::Line;
    QPointF startPoint;
    QPointF endPoint;
    QPointF center;
    double radius = 0.0;
    double startAngleDeg = 0.0;
    double endAngleDeg = 0.0;
    double lengthMm = 0.0;
    QList<int> relatedEntityIndexes;
};

class GeometryModel
{
public:
    DxfDocument dxfDocument;
    QList<DetectedPort> detectedPorts;
};

#endif // FOXBENDER_GEOMETRYMODEL_H
