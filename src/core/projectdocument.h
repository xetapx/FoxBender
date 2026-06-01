#ifndef FOXBENDER_PROJECTDOCUMENT_H
#define FOXBENDER_PROJECTDOCUMENT_H

#include "toolstation.h"
#include "dxfdocument.h"

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

class RuleProfileOptions
{
public:
    int count = 1;
    bool mirror = false;
    double startCorrectionMm = 0.0;
    double endCorrectionMm = 0.0;
    bool startLip = false;
    bool endLip = false;

    QJsonObject toJson() const;
    static RuleProfileOptions fromJson(const QJsonObject &json);
};

class RuleProfileSegmentOptions
{
public:
    double angleCorrectionDeg = 0.0;
    double lengthCorrectionMm = 0.0;
    QString openMode = QStringLiteral("Normal");

    QJsonObject toJson() const;
    static RuleProfileSegmentOptions fromJson(const QJsonObject &json);
};

class ProjectDocument
{
public:
    int projectVersion = 1;
    QString projectFilePath;
    QString dxfFilePath;
    DxfDocument geometrySnapshot;
    bool hasGeometrySnapshot = false;
    QList<QStringList> ruleProfiles;
    QList<RuleProfileOptions> ruleProfileOptions;
    QList<QList<RuleProfileSegmentOptions>> ruleProfileSegmentOptions;
    int activeRuleProfileIndex = 0;
    QStringList layerAssignments;
    QStringList features;
    QStringList modifiers;
    QStringList toothingSubpaths;
    QJsonObject bridgeSettings;
    QJsonObject simulationSettings;

    bool hasDxfFile() const;
    QString displayName() const;

    QJsonObject toJson() const;
    static ProjectDocument fromJson(const QJsonObject &json);
};

#endif // FOXBENDER_PROJECTDOCUMENT_H
