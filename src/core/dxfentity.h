#ifndef FOXBENDER_DXFENTITY_H
#define FOXBENDER_DXFENTITY_H

#include <QColor>
#include <QPointF>
#include <QString>
#include <QVector>

enum class DxfEntityType
{
    Line,
    Arc,
    Polyline
};

class DxfEntity
{
public:
    QString id;
    QString sourceHandle;
    DxfEntityType type = DxfEntityType::Line;
    QString layerName;
    QColor color = Qt::black;
    QVector<QPointF> points;
    QPointF center;
    double radius = 0.0;
    double startAngleDeg = 0.0;
    double endAngleDeg = 0.0;

    QString typeName() const;
    QString summary() const;
};

#endif // FOXBENDER_DXFENTITY_H
