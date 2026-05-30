#ifndef FOXBENDER_PROJECTDOCUMENT_H
#define FOXBENDER_PROJECTDOCUMENT_H

#include "toolstation.h"

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

class ProjectDocument
{
public:
    int projectVersion = 1;
    QString projectFilePath;
    QString dxfFilePath;
    QList<QStringList> ruleProfiles;
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
