#include "dxfdocument.h"

#include <QJsonArray>
#include <QtMath>

namespace
{
QString colorToString(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

QColor colorFromJson(const QJsonValue &value, const QColor &fallback = Qt::black)
{
    const QString text = value.toString();
    const QColor color(text);
    return color.isValid() ? color : fallback;
}

QJsonArray pointArray(const QVector<QPointF> &points)
{
    QJsonArray array;
    for (const QPointF &point : points) {
        QJsonObject object;
        object["x"] = point.x();
        object["y"] = point.y();
        array.append(object);
    }
    return array;
}

QVector<QPointF> pointsFromJson(const QJsonArray &array)
{
    QVector<QPointF> points;
    points.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        points.append(QPointF(object["x"].toDouble(), object["y"].toDouble()));
    }
    return points;
}

QString entityTypeToString(DxfEntityType type)
{
    switch (type) {
    case DxfEntityType::Line:
        return QStringLiteral("LINE");
    case DxfEntityType::Arc:
        return QStringLiteral("ARC");
    case DxfEntityType::Polyline:
        return QStringLiteral("POLYLINE");
    }

    return QStringLiteral("LINE");
}

DxfEntityType entityTypeFromJson(const QString &type)
{
    if (type == QStringLiteral("ARC")) {
        return DxfEntityType::Arc;
    }
    if (type == QStringLiteral("POLYLINE")) {
        return DxfEntityType::Polyline;
    }
    return DxfEntityType::Line;
}
}

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

QJsonObject DxfDocument::toJson() const
{
    QJsonObject json;
    json["sourceName"] = sourceName;

    QJsonArray layerArray;
    for (const LayerDefinition &layer : layers) {
        QJsonObject layerJson;
        layerJson["name"] = layer.name;
        layerJson["color"] = colorToString(layer.color);
        layerJson["visible"] = layer.visible;
        layerJson["locked"] = layer.locked;
        layerArray.append(layerJson);
    }
    json["layers"] = layerArray;

    QJsonArray entityArray;
    for (const DxfEntity &entity : entities) {
        QJsonObject entityJson;
        entityJson["id"] = entity.id;
        entityJson["sourceHandle"] = entity.sourceHandle;
        entityJson["type"] = entityTypeToString(entity.type);
        entityJson["layerName"] = entity.layerName;
        entityJson["color"] = colorToString(entity.color);
        entityJson["points"] = pointArray(entity.points);
        entityJson["centerX"] = entity.center.x();
        entityJson["centerY"] = entity.center.y();
        entityJson["radius"] = entity.radius;
        entityJson["startAngleDeg"] = entity.startAngleDeg;
        entityJson["endAngleDeg"] = entity.endAngleDeg;
        entityArray.append(entityJson);
    }
    json["entities"] = entityArray;

    return json;
}

DxfDocument DxfDocument::fromJson(const QJsonObject &json)
{
    DxfDocument document;
    document.sourceName = json["sourceName"].toString();

    const QJsonArray layerArray = json["layers"].toArray();
    document.layers.reserve(layerArray.size());
    for (const QJsonValue &value : layerArray) {
        const QJsonObject layerJson = value.toObject();
        LayerDefinition layer;
        layer.name = layerJson["name"].toString();
        layer.color = colorFromJson(layerJson["color"], layer.color);
        layer.visible = layerJson.contains("visible") ? layerJson["visible"].toBool(true) : true;
        layer.locked = layerJson["locked"].toBool(false);
        document.layers.append(layer);
    }

    const QJsonArray entityArray = json["entities"].toArray();
    document.entities.reserve(entityArray.size());
    for (const QJsonValue &value : entityArray) {
        const QJsonObject entityJson = value.toObject();
        DxfEntity entity;
        entity.id = entityJson["id"].toString();
        entity.sourceHandle = entityJson["sourceHandle"].toString();
        entity.type = entityTypeFromJson(entityJson["type"].toString());
        entity.layerName = entityJson["layerName"].toString();
        entity.color = colorFromJson(entityJson["color"], entity.color);
        entity.points = pointsFromJson(entityJson["points"].toArray());
        entity.center = QPointF(entityJson["centerX"].toDouble(), entityJson["centerY"].toDouble());
        entity.radius = entityJson["radius"].toDouble();
        entity.startAngleDeg = entityJson["startAngleDeg"].toDouble();
        entity.endAngleDeg = entityJson["endAngleDeg"].toDouble();
        document.entities.append(entity);
    }

    return document;
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
