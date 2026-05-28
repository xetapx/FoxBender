#include "projectdocument.h"

#include <QFileInfo>
#include <QJsonArray>

namespace
{
QJsonArray toJsonArray(const QStringList &items)
{
    QJsonArray array;
    for (const QString &item : items) {
        array.append(item);
    }
    return array;
}

QStringList fromJsonArray(const QJsonArray &array)
{
    QStringList items;
    items.reserve(array.size());
    for (const QJsonValue &value : array) {
        items.append(value.toString());
    }
    return items;
}

QJsonArray toJsonArrayOfArrays(const QList<QStringList> &items)
{
    QJsonArray array;
    for (const QStringList &list : items) {
        array.append(toJsonArray(list));
    }
    return array;
}

QList<QStringList> fromJsonArrayOfArrays(const QJsonArray &array)
{
    QList<QStringList> items;
    items.reserve(array.size());
    for (const QJsonValue &value : array) {
        items.append(fromJsonArray(value.toArray()));
    }
    return items;
}
}

bool ProjectDocument::hasDxfFile() const
{
    return !dxfFilePath.trimmed().isEmpty();
}

QString ProjectDocument::displayName() const
{
    if (!projectFilePath.isEmpty()) {
        return QFileInfo(projectFilePath).fileName();
    }
    return QStringLiteral("Untitled");
}

QJsonObject ProjectDocument::toJson() const
{
    QJsonObject json;
    json["projectVersion"] = projectVersion;
    json["dxfFilePath"] = dxfFilePath;
    json["bladeLines"] = toJsonArrayOfArrays(bladeLines);
    json["activeBladeLineIndex"] = activeBladeLineIndex;
    json["layerAssignments"] = toJsonArray(layerAssignments);
    json["features"] = toJsonArray(features);
    json["modifiers"] = toJsonArray(modifiers);
    json["toothingSubpaths"] = toJsonArray(toothingSubpaths);
    json["bridgeSettings"] = bridgeSettings;
    json["simulationSettings"] = simulationSettings;
    return json;
}

ProjectDocument ProjectDocument::fromJson(const QJsonObject &json)
{
    ProjectDocument document;
    document.projectVersion = json["projectVersion"].toInt(1);
    document.dxfFilePath = json["dxfFilePath"].toString();
    document.bladeLines = fromJsonArrayOfArrays(json["bladeLines"].toArray());
    if (document.bladeLines.isEmpty()) {
        const QStringList legacyBladeLine = fromJsonArray(json["bladeLinePaths"].toArray());
        if (!legacyBladeLine.isEmpty()) {
            document.bladeLines.append(legacyBladeLine);
        }
    }
    document.activeBladeLineIndex = json["activeBladeLineIndex"].toInt(0);
    if (document.activeBladeLineIndex < 0 || document.activeBladeLineIndex >= document.bladeLines.size()) {
        document.activeBladeLineIndex = document.bladeLines.isEmpty() ? 0 : document.bladeLines.size() - 1;
    }
    document.layerAssignments = fromJsonArray(json["layerAssignments"].toArray());
    document.features = fromJsonArray(json["features"].toArray());
    document.modifiers = fromJsonArray(json["modifiers"].toArray());
    document.toothingSubpaths = fromJsonArray(json["toothingSubpaths"].toArray());
    document.bridgeSettings = json["bridgeSettings"].toObject();
    document.simulationSettings = json["simulationSettings"].toObject();
    return document;
}
