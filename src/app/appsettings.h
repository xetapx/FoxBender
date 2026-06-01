#ifndef FOXBENDER_APPSETTINGS_H
#define FOXBENDER_APPSETTINGS_H

#include "src/core/toolstation.h"

#include <QColor>
#include <QJsonObject>
#include <QList>
#include <QStringList>

class AngleBendTableRow
{
public:
    double angleDeg = 0.0;
    double left = 0.0;
    double right = 0.0;
    double startCorrection = 0.0;
    double endCorrection = 0.0;

    QJsonObject toJson() const;
    static AngleBendTableRow fromJson(const QJsonObject &json);
};

class ArcBendTableRow
{
public:
    double radiusMm = 0.0;
    double segmentLengthMm = 0.0;
    int segments = 0;
    int right90 = 0;
    int right45 = 0;
    int left90 = 0;
    int left45 = 0;
    double startCorrection = 0.0;
    double endCorrection = 0.0;
    double bridgeReduction = 0.0;

    QJsonObject toJson() const;
    static ArcBendTableRow fromJson(const QJsonObject &json);
};

class AppSettings
{
public:
    QString bendParametersName = QStringLiteral("Default");
    QString bendParametersDescription;
    QString bendParametersNotes;
    QList<ToolStation> toolStations = ToolStation::createDefaultStations();
    QList<AngleBendTableRow> angleBendTable = createDefaultAngleBendTable();
    QList<ArcBendTableRow> arcBendTable = createDefaultArcBendTable();
    double defaultToleranceMm = 0.1;
    QString mainWindowGeometry;
    QString mainWindowState;
    QString layerTreeHeaderState;
    QString entityPropertiesHeaderState;
    QString ruleProfileTreeHeaderState;
    QStringList recentDxfFiles;
    QStringList recentProjects;
    QString lastDxfDirectory;
    QString lastBendParametersFilePath;
    QStringList hiddenLayers;
    QColor viewBackgroundColor = QColor(245, 245, 245);
    QColor selectedEntityColor = QColor(220, 40, 40);
    QColor segmentSelectionColor = QColor(255, 140, 0);
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
    QColor bridgeColor = QColor(230, 40, 140);
    bool fillFlags = true;
    double flagScale = 1.0;
    double handleScale = 1.0;
    double bridgeMinLengthMm = 2.0;
    double bridgeMaxLengthMm = 6.0;

    static QList<AngleBendTableRow> createDefaultAngleBendTable();
    static QList<ArcBendTableRow> createDefaultArcBendTable();
    QJsonObject toJson() const;
    static AppSettings fromJson(const QJsonObject &json);
};

#endif // FOXBENDER_APPSETTINGS_H
