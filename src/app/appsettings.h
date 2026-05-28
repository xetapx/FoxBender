#ifndef FOXBENDER_APPSETTINGS_H
#define FOXBENDER_APPSETTINGS_H

#include "src/core/toolstation.h"

#include <QColor>
#include <QJsonObject>
#include <QList>
#include <QStringList>

class AppSettings
{
public:
    QList<ToolStation> toolStations = ToolStation::createDefaultStations();
    double defaultToleranceMm = 0.1;
    QStringList recentProjects;
    QColor viewBackgroundColor = QColor(245, 245, 245);
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
    double portMinLengthMm = 2.0;
    double portMaxLengthMm = 6.0;

    QJsonObject toJson() const;
    static AppSettings fromJson(const QJsonObject &json);
};

#endif // FOXBENDER_APPSETTINGS_H
