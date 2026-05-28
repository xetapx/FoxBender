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
}

QJsonObject AppSettings::toJson() const
{
    QJsonObject json;
    json["defaultToleranceMm"] = defaultToleranceMm;
    json["toolStations"] = toolStationArray(toolStations);
    json["lastDxfDirectory"] = lastDxfDirectory;
    json["viewBackgroundColor"] = colorToString(viewBackgroundColor);
    json["selectedEntityColor"] = colorToString(selectedEntityColor);
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
    json["portMinLengthMm"] = portMinLengthMm;
    json["portMaxLengthMm"] = portMaxLengthMm;

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
    settings.viewBackgroundColor = colorFromJson(json["viewBackgroundColor"], settings.viewBackgroundColor);
    settings.selectedEntityColor = colorFromJson(json["selectedEntityColor"], settings.selectedEntityColor);
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

    const QJsonArray projects = json["recentProjects"].toArray();
    settings.recentProjects.clear();
    settings.recentProjects.reserve(projects.size());
    for (const QJsonValue &value : projects) {
        settings.recentProjects.append(value.toString());
    }

    return settings;
}
