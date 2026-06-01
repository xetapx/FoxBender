#include "appsettings.h"

#include <QJsonArray>

namespace
{
double arcSegmentTargetLengthMm(double radiusMm)
{
    const double absoluteRadius = qAbs(radiusMm);
    if (absoluteRadius <= 5.0) {
        return 0.5;
    }
    if (absoluteRadius <= 10.0) {
        return 0.7;
    }
    if (absoluteRadius <= 20.0) {
        return 1.0;
    }
    return 1.5;
}

int segmentCountForArc(double radiusMm, double totalAngleDeg, double segmentLengthMm)
{
    if (segmentLengthMm <= 0.0 || totalAngleDeg <= 0.0 || radiusMm <= 0.0) {
        return 1;
    }

    const double arcLengthMm = radiusMm * qDegreesToRadians(totalAngleDeg);
    return qMax(1, qRound(arcLengthMm / segmentLengthMm));
}

int bendValueForArc(double radiusMm, double totalAngleDeg, double segmentLengthMm)
{
    const int segments = segmentCountForArc(radiusMm, totalAngleDeg, segmentLengthMm);
    return qRound((totalAngleDeg / static_cast<double>(segments)) * 100.0);
}

ArcBendTableRow defaultArcBendRow(double radiusMm)
{
    ArcBendTableRow row;
    row.radiusMm = radiusMm;
    const double segmentLengthMm = arcSegmentTargetLengthMm(radiusMm);
    row.segmentLengthMm = segmentLengthMm;
    row.segments = segmentCountForArc(radiusMm, 90.0, segmentLengthMm);
    row.right90 = bendValueForArc(radiusMm, 90.0, segmentLengthMm);
    row.right45 = bendValueForArc(radiusMm, 45.0, segmentLengthMm);
    row.left90 = bendValueForArc(radiusMm, 90.0, segmentLengthMm);
    row.left45 = bendValueForArc(radiusMm, 45.0, segmentLengthMm);
    return row;
}

QList<AngleBendTableRow> angleBendTableFromJsonArray(const QJsonArray &array)
{
    QList<AngleBendTableRow> rows;
    rows.reserve(array.size());
    for (const QJsonValue &value : array) {
        rows.append(AngleBendTableRow::fromJson(value.toObject()));
    }
    return rows;
}

QList<ArcBendTableRow> arcBendTableFromJsonArray(const QJsonArray &array)
{
    QList<ArcBendTableRow> rows;
    rows.reserve(array.size());
    for (const QJsonValue &value : array) {
        rows.append(ArcBendTableRow::fromJson(value.toObject()));
    }
    return rows;
}

QString colorToString(const QColor &color)
{
    return color.name(QColor::HexRgb);
}

QColor colorFromJson(const QJsonValue &value, const QColor &fallback)
{
    const QColor parsed(value.toString());
    return parsed.isValid() ? parsed : fallback;
}

QJsonArray toolStationArray(const QList<ToolStation> &stations)
{
    QJsonArray array;
    for (const ToolStation &station : stations) {
        array.append(station.toJson());
    }
    return array;
}

QJsonArray stringListToJsonArray(const QStringList &values)
{
    QJsonArray array;
    for (const QString &value : values) {
        array.append(value);
    }
    return array;
}

QStringList jsonArrayToStringList(const QJsonArray &array)
{
    QStringList values;
    values.reserve(array.size());
    for (const QJsonValue &value : array) {
        values.append(value.toString());
    }
    return values;
}
}

QJsonObject AngleBendTableRow::toJson() const
{
    QJsonObject json;
    json["angleDeg"] = angleDeg;
    json["left"] = left;
    json["right"] = right;
    json["startCorrection"] = startCorrection;
    json["endCorrection"] = endCorrection;
    return json;
}

AngleBendTableRow AngleBendTableRow::fromJson(const QJsonObject &json)
{
    AngleBendTableRow row;
    row.angleDeg = json["angleDeg"].toDouble(row.angleDeg);
    row.left = json["left"].toDouble(row.left);
    row.right = json["right"].toDouble(row.right);
    row.startCorrection = json["startCorrection"].toDouble(row.startCorrection);
    row.endCorrection = json["endCorrection"].toDouble(row.endCorrection);
    return row;
}

QJsonObject ArcBendTableRow::toJson() const
{
    QJsonObject json;
    json["radiusMm"] = radiusMm;
    json["segmentLengthMm"] = segmentLengthMm;
    json["segments"] = segments;
    json["right90"] = right90;
    json["right45"] = right45;
    json["left90"] = left90;
    json["left45"] = left45;
    json["startCorrection"] = startCorrection;
    json["endCorrection"] = endCorrection;
    json["bridgeReduction"] = bridgeReduction;
    return json;
}

ArcBendTableRow ArcBendTableRow::fromJson(const QJsonObject &json)
{
    ArcBendTableRow row;
    row.radiusMm = json["radiusMm"].toDouble(row.radiusMm);
    row.segmentLengthMm = json["segmentLengthMm"].toDouble(row.segmentLengthMm);
    row.segments = json["segments"].toInt();
    if (row.segments <= 0) {
        row.segments = json["hits"].toInt(row.segments);
    }
    row.right90 = json["right90"].toDouble(row.right90);
    row.right45 = json["right45"].toDouble(row.right45);
    row.left90 = json["left90"].toDouble(row.left90);
    row.left45 = json["left45"].toDouble(row.left45);
    row.startCorrection = json["startCorrection"].toDouble(row.startCorrection);
    row.endCorrection = json["endCorrection"].toDouble(row.endCorrection);
    row.bridgeReduction = json["bridgeReduction"].toDouble(row.bridgeReduction);
    if (row.segmentLengthMm <= 0.0) {
        row.segmentLengthMm = arcSegmentTargetLengthMm(row.radiusMm);
    }
    return row;
}

QList<AngleBendTableRow> AppSettings::createDefaultAngleBendTable()
{
    QList<AngleBendTableRow> rows;
    for (int angle = 1; angle <= 10; ++angle) {
        AngleBendTableRow row;
        row.angleDeg = angle;
        row.left = angle;
        row.right = angle;
        rows.append(row);
    }
    for (int angle = 15; angle <= 90; angle += 5) {
        AngleBendTableRow row;
        row.angleDeg = angle;
        row.left = angle;
        row.right = angle;
        rows.append(row);
    }
    return rows;
}

QList<ArcBendTableRow> AppSettings::createDefaultArcBendTable()
{
    QList<ArcBendTableRow> rows;

    auto appendRadius = [&](double radiusMm) {
        rows.append(defaultArcBendRow(radiusMm));
    };

    for (int radius = 1; radius <= 10; ++radius) {
        appendRadius(radius);
    }
    for (int radius = 12; radius <= 30; radius += 2) {
        appendRadius(radius);
    }
    for (int radius = 35; radius <= 100; radius += 5) {
        appendRadius(radius);
    }
    return rows;
}

QJsonObject AppSettings::toJson() const
{
    QJsonObject json;
    json["defaultToleranceMm"] = defaultToleranceMm;
    json["toolStations"] = toolStationArray(toolStations);
    json["mainWindowGeometry"] = mainWindowGeometry;
    json["mainWindowState"] = mainWindowState;
    json["layerTreeHeaderState"] = layerTreeHeaderState;
    json["entityPropertiesHeaderState"] = entityPropertiesHeaderState;
    json["ruleProfileTreeHeaderState"] = ruleProfileTreeHeaderState;
    json["lastDxfDirectory"] = lastDxfDirectory;
    json["lastBendParametersFilePath"] = lastBendParametersFilePath;
    json["hiddenLayers"] = stringListToJsonArray(hiddenLayers);
    json["viewBackgroundColor"] = colorToString(viewBackgroundColor);
    json["selectedEntityColor"] = colorToString(selectedEntityColor);
    json["segmentSelectionColor"] = colorToString(segmentSelectionColor);
    json["hoverEntityColor"] = colorToString(hoverEntityColor);
    json["armedEntityColor"] = colorToString(armedEntityColor);
    json["bladeLineColor"] = colorToString(bladeLineColor);
    json["activeBladeLineColor"] = colorToString(activeBladeLineColor);
    json["handleColor"] = colorToString(handleColor);
    json["handleHoverColor"] = colorToString(handleHoverColor);
    json["handleHoverFillColor"] = colorToString(handleHoverFillColor);
    json["snapPreviewColor"] = colorToString(snapPreviewColor);
    json["breakPreviewColor"] = colorToString(breakPreviewColor);
    json["startFlagColor"] = colorToString(startFlagColor);
    json["endFlagColor"] = colorToString(endFlagColor);
    json["bridgeColor"] = colorToString(bridgeColor);
    json["fillFlags"] = fillFlags;
    json["flagScale"] = flagScale;
    json["handleScale"] = handleScale;
    json["bridgeMinLengthMm"] = bridgeMinLengthMm;
    json["bridgeMaxLengthMm"] = bridgeMaxLengthMm;
    json["recentDxfFiles"] = stringListToJsonArray(recentDxfFiles);

    QJsonArray recentProjectsArray;
    for (const QString &project : recentProjects) {
        recentProjectsArray.append(project);
    }
    json["recentProjects"] = recentProjectsArray;
    return json;
}

AppSettings AppSettings::fromJson(const QJsonObject &json)
{
    AppSettings settings;
    settings.defaultToleranceMm = json["defaultToleranceMm"].toDouble(settings.defaultToleranceMm);
    settings.angleBendTable = angleBendTableFromJsonArray(json["angleBendTable"].toArray());
    settings.arcBendTable = arcBendTableFromJsonArray(json["arcBendTable"].toArray());
    settings.mainWindowGeometry = json["mainWindowGeometry"].toString();
    settings.mainWindowState = json["mainWindowState"].toString();
    settings.layerTreeHeaderState = json["layerTreeHeaderState"].toString();
    settings.entityPropertiesHeaderState = json["entityPropertiesHeaderState"].toString();
    settings.ruleProfileTreeHeaderState = json["ruleProfileTreeHeaderState"].toString();
    settings.lastDxfDirectory = json["lastDxfDirectory"].toString();
    settings.lastBendParametersFilePath = json["lastBendParametersFilePath"].toString();
    settings.hiddenLayers = jsonArrayToStringList(json["hiddenLayers"].toArray());
    if (settings.hiddenLayers.isEmpty()) {
        const QJsonObject legacyHiddenLayersByDxf = json["hiddenLayersByDxf"].toObject();
        if (!legacyHiddenLayersByDxf.isEmpty()) {
            if (!settings.lastDxfDirectory.isEmpty()
                && legacyHiddenLayersByDxf.contains(settings.lastDxfDirectory)) {
                settings.hiddenLayers =
                    jsonArrayToStringList(legacyHiddenLayersByDxf.value(settings.lastDxfDirectory).toArray());
            } else {
                const QString firstKey = legacyHiddenLayersByDxf.keys().value(0);
                settings.hiddenLayers =
                    jsonArrayToStringList(legacyHiddenLayersByDxf.value(firstKey).toArray());
            }
        }
    }
    settings.viewBackgroundColor = colorFromJson(json["viewBackgroundColor"], settings.viewBackgroundColor);
    settings.selectedEntityColor = colorFromJson(json["selectedEntityColor"], settings.selectedEntityColor);
    settings.segmentSelectionColor = colorFromJson(json["segmentSelectionColor"], settings.segmentSelectionColor);
    settings.hoverEntityColor = colorFromJson(json["hoverEntityColor"], settings.hoverEntityColor);
    settings.armedEntityColor = colorFromJson(json["armedEntityColor"], settings.armedEntityColor);
    settings.bladeLineColor = colorFromJson(json["bladeLineColor"], settings.bladeLineColor);
    settings.activeBladeLineColor = colorFromJson(json["activeBladeLineColor"], settings.activeBladeLineColor);
    settings.handleColor = colorFromJson(json["handleColor"], settings.handleColor);
    settings.handleHoverColor = colorFromJson(json["handleHoverColor"], settings.handleHoverColor);
    settings.handleHoverFillColor = colorFromJson(json["handleHoverFillColor"], settings.handleHoverFillColor);
    settings.snapPreviewColor = colorFromJson(json["snapPreviewColor"], settings.snapPreviewColor);
    settings.breakPreviewColor = colorFromJson(json["breakPreviewColor"], settings.breakPreviewColor);
    settings.startFlagColor = colorFromJson(json["startFlagColor"], settings.startFlagColor);
    settings.endFlagColor = colorFromJson(json["endFlagColor"], settings.endFlagColor);
    settings.bridgeColor = colorFromJson(json["bridgeColor"], settings.bridgeColor);
    settings.fillFlags = json["fillFlags"].toBool(settings.fillFlags);
    settings.flagScale = json["flagScale"].toDouble(settings.flagScale);
    if (settings.flagScale < 0.5 || settings.flagScale > 3.0) {
        settings.flagScale = 1.0;
    }
    settings.handleScale = json["handleScale"].toDouble(settings.handleScale);
    if (settings.handleScale < 0.5 || settings.handleScale > 3.0) {
        settings.handleScale = 1.0;
    }
    settings.bridgeMinLengthMm = json["bridgeMinLengthMm"].toDouble(settings.bridgeMinLengthMm);
    settings.bridgeMaxLengthMm = json["bridgeMaxLengthMm"].toDouble(settings.bridgeMaxLengthMm);
    if (settings.bridgeMinLengthMm < 0.0) {
        settings.bridgeMinLengthMm = 0.0;
    }
    if (settings.bridgeMaxLengthMm < settings.bridgeMinLengthMm) {
        settings.bridgeMaxLengthMm = settings.bridgeMinLengthMm;
    }

    const QJsonArray tools = json["toolStations"].toArray();
    if (!tools.isEmpty()) {
        settings.toolStations.clear();
        settings.toolStations.reserve(tools.size());
        for (const QJsonValue &value : tools) {
            settings.toolStations.append(ToolStation::fromJson(value.toObject()));
        }
    }

    settings.recentDxfFiles = jsonArrayToStringList(json["recentDxfFiles"].toArray());
    if (settings.recentDxfFiles.isEmpty()) {
        settings.recentDxfFiles = jsonArrayToStringList(json["recentFiles"].toArray());
    }
    settings.recentProjects = jsonArrayToStringList(json["recentProjects"].toArray());
    if (settings.recentDxfFiles.isEmpty() && !settings.recentProjects.isEmpty()) {
        settings.recentDxfFiles = settings.recentProjects;
    }

    if (settings.angleBendTable.isEmpty()) {
        settings.angleBendTable = createDefaultAngleBendTable();
    }
    if (settings.arcBendTable.isEmpty()) {
        settings.arcBendTable = createDefaultArcBendTable();
    }

    return settings;
}
