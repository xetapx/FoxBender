#include "appsettings.h"

#include <QJsonArray>

namespace
{
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

QJsonObject AppSettings::toJson() const
{
    QJsonObject json;
    json["defaultToleranceMm"] = defaultToleranceMm;
    json["toolStations"] = toolStationArray(toolStations);
    json["lastDxfDirectory"] = lastDxfDirectory;
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
    json["portColor"] = colorToString(portColor);
    json["fillFlags"] = fillFlags;
    json["flagScale"] = flagScale;
    json["handleScale"] = handleScale;
    json["portMinLengthMm"] = portMinLengthMm;
    json["portMaxLengthMm"] = portMaxLengthMm;
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
    settings.lastDxfDirectory = json["lastDxfDirectory"].toString();
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
    settings.portColor = colorFromJson(json["portColor"], settings.portColor);
    settings.fillFlags = json["fillFlags"].toBool(settings.fillFlags);
    settings.flagScale = json["flagScale"].toDouble(settings.flagScale);
    if (settings.flagScale < 0.5 || settings.flagScale > 3.0) {
        settings.flagScale = 1.0;
    }
    settings.handleScale = json["handleScale"].toDouble(settings.handleScale);
    if (settings.handleScale < 0.5 || settings.handleScale > 3.0) {
        settings.handleScale = 1.0;
    }
    settings.portMinLengthMm = json["portMinLengthMm"].toDouble(settings.portMinLengthMm);
    settings.portMaxLengthMm = json["portMaxLengthMm"].toDouble(settings.portMaxLengthMm);
    if (settings.portMinLengthMm < 0.0) {
        settings.portMinLengthMm = 0.0;
    }
    if (settings.portMaxLengthMm < settings.portMinLengthMm) {
        settings.portMaxLengthMm = settings.portMinLengthMm;
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

    return settings;
}
