#include "toolstation.h"

#include <QJsonArray>

QJsonObject ToolStation::toJson() const
{
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["centerOffsetMm"] = centerOffsetMm;
    json["widthMm"] = widthMm;
    json["actionTimeoutMs"] = actionTimeoutMs;
    json["enabled"] = enabled;
    return json;
}

ToolStation ToolStation::fromJson(const QJsonObject &json)
{
    ToolStation station;
    station.id = json["id"].toString();
    station.name = json["name"].toString();
    station.centerOffsetMm = json["centerOffsetMm"].toDouble();
    station.widthMm = json["widthMm"].toDouble();
    station.actionTimeoutMs = json["actionTimeoutMs"].toInt(0);
    station.enabled = json.contains("enabled") ? json["enabled"].toBool(true) : true;
    return station;
}

QList<ToolStation> ToolStation::createDefaultStations()
{
    return {
        {"roughing_left", "Roughing Left", -150.0, 0.0, 1000, true},
        {"roughing_right", "Roughing Right", -150.0, 0.0, 1000, true},
        {"nicking", "Nicking", -110.0, 0.0, 1000, true},
        {"bridge_2_5", "Bridge 2.5", -70.0, 2.5, 1000, true},
        {"bridge_3_0", "Bridge 3.0", -60.0, 3.0, 1000, true},
        {"scraping", "Scraping", -40.0, 6.0, 1000, true},
        {"notching", "Notching", -30.0, 6.0, 1000, true},
        {"optical_sensor", "Optical Sensor", 0.0, 0.0, 250, true},
        {"toothing_1_0", "Toothing 1.0", 30.0, 1.0, 1000, true},
        {"toothing_2_5", "Toothing 2.5", 40.0, 2.5, 1000, true},
        {"bending", "Bending", 100.0, 0.0, 1000, true},
        {"cutting", "Cutting", 160.0, 6.0, 1000, true}
    };
}
