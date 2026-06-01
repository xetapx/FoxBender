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

QJsonArray ruleProfileOptionsToJsonArray(const QList<RuleProfileOptions> &items)
{
    QJsonArray array;
    for (const RuleProfileOptions &item : items) {
        array.append(item.toJson());
    }
    return array;
}

QList<RuleProfileOptions> ruleProfileOptionsFromJsonArray(const QJsonArray &array)
{
    QList<RuleProfileOptions> items;
    items.reserve(array.size());
    for (const QJsonValue &value : array) {
        items.append(RuleProfileOptions::fromJson(value.toObject()));
    }
    return items;
}

QJsonArray ruleProfileSegmentOptionsToJsonArray(const QList<QList<RuleProfileSegmentOptions>> &items)
{
    QJsonArray array;
    for (const QList<RuleProfileSegmentOptions> &segmentList : items) {
        QJsonArray segmentArray;
        for (const RuleProfileSegmentOptions &item : segmentList) {
            segmentArray.append(item.toJson());
        }
        array.append(segmentArray);
    }
    return array;
}

QList<QList<RuleProfileSegmentOptions>> ruleProfileSegmentOptionsFromJsonArray(const QJsonArray &array)
{
    QList<QList<RuleProfileSegmentOptions>> items;
    items.reserve(array.size());
    for (const QJsonValue &value : array) {
        QList<RuleProfileSegmentOptions> segmentItems;
        const QJsonArray segmentArray = value.toArray();
        segmentItems.reserve(segmentArray.size());
        for (const QJsonValue &segmentValue : segmentArray) {
            segmentItems.append(RuleProfileSegmentOptions::fromJson(segmentValue.toObject()));
        }
        items.append(segmentItems);
    }
    return items;
}

void normalizeRuleProfileOptions(ProjectDocument *document)
{
    if (document == nullptr) {
        return;
    }

    while (document->ruleProfileOptions.size() < document->ruleProfiles.size()) {
        document->ruleProfileOptions.append(RuleProfileOptions());
    }
    while (document->ruleProfileOptions.size() > document->ruleProfiles.size()) {
        document->ruleProfileOptions.removeLast();
    }
    while (document->ruleProfileSegmentOptions.size() < document->ruleProfiles.size()) {
        document->ruleProfileSegmentOptions.append(QList<RuleProfileSegmentOptions>());
    }
    while (document->ruleProfileSegmentOptions.size() > document->ruleProfiles.size()) {
        document->ruleProfileSegmentOptions.removeLast();
    }
}
}

QJsonObject RuleProfileOptions::toJson() const
{
    QJsonObject json;
    json["count"] = count;
    json["mirror"] = mirror;
    json["startCorrectionMm"] = startCorrectionMm;
    json["endCorrectionMm"] = endCorrectionMm;
    json["startLip"] = startLip;
    json["endLip"] = endLip;
    return json;
}

RuleProfileOptions RuleProfileOptions::fromJson(const QJsonObject &json)
{
    RuleProfileOptions options;
    options.count = qMax(1, json["count"].toInt(options.count));
    options.mirror = json["mirror"].toBool(options.mirror);
    options.startCorrectionMm = json["startCorrectionMm"].toDouble(options.startCorrectionMm);
    options.endCorrectionMm = json["endCorrectionMm"].toDouble(options.endCorrectionMm);
    options.startLip = json["startLip"].toBool(options.startLip);
    options.endLip = json["endLip"].toBool(options.endLip);
    return options;
}

QJsonObject RuleProfileSegmentOptions::toJson() const
{
    QJsonObject json;
    json["angleCorrectionDeg"] = angleCorrectionDeg;
    json["lengthCorrectionMm"] = lengthCorrectionMm;
    json["openMode"] = openMode;
    return json;
}

RuleProfileSegmentOptions RuleProfileSegmentOptions::fromJson(const QJsonObject &json)
{
    RuleProfileSegmentOptions options;
    options.angleCorrectionDeg = json["angleCorrectionDeg"].toDouble(options.angleCorrectionDeg);
    options.lengthCorrectionMm = json["lengthCorrectionMm"].toDouble(options.lengthCorrectionMm);
    options.openMode = json["openMode"].toString(options.openMode);
    if (options.openMode.isEmpty()) {
        options.openMode = QStringLiteral("Normal");
    }
    return options;
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
    if (hasGeometrySnapshot) {
        json["geometrySnapshot"] = geometrySnapshot.toJson();
    }
    json["ruleProfiles"] = toJsonArrayOfArrays(ruleProfiles);
    json["ruleProfileOptions"] = ruleProfileOptionsToJsonArray(ruleProfileOptions);
    json["ruleProfileSegmentOptions"] = ruleProfileSegmentOptionsToJsonArray(ruleProfileSegmentOptions);
    json["activeRuleProfileIndex"] = activeRuleProfileIndex;
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
    if (json.contains("geometrySnapshot") && json["geometrySnapshot"].isObject()) {
        document.geometrySnapshot = DxfDocument::fromJson(json["geometrySnapshot"].toObject());
        document.hasGeometrySnapshot = true;
    }
    document.ruleProfiles = fromJsonArrayOfArrays(json["ruleProfiles"].toArray());
    if (document.ruleProfiles.isEmpty()) {
        document.ruleProfiles = fromJsonArrayOfArrays(json["bladeLines"].toArray());
    }
    if (document.ruleProfiles.isEmpty()) {
        const QStringList legacyRuleProfile = fromJsonArray(json["bladeLinePaths"].toArray());
        if (!legacyRuleProfile.isEmpty()) {
            document.ruleProfiles.append(legacyRuleProfile);
        }
    }
    document.ruleProfileOptions = ruleProfileOptionsFromJsonArray(json["ruleProfileOptions"].toArray());
    document.ruleProfileSegmentOptions =
        ruleProfileSegmentOptionsFromJsonArray(json["ruleProfileSegmentOptions"].toArray());
    normalizeRuleProfileOptions(&document);
    document.activeRuleProfileIndex = json["activeRuleProfileIndex"].toInt(
        json["activeBladeLineIndex"].toInt(0));
    if (document.activeRuleProfileIndex < 0 || document.activeRuleProfileIndex >= document.ruleProfiles.size()) {
        document.activeRuleProfileIndex = document.ruleProfiles.isEmpty() ? 0 : document.ruleProfiles.size() - 1;
    }
    document.layerAssignments = fromJsonArray(json["layerAssignments"].toArray());
    document.features = fromJsonArray(json["features"].toArray());
    document.modifiers = fromJsonArray(json["modifiers"].toArray());
    document.toothingSubpaths = fromJsonArray(json["toothingSubpaths"].toArray());
    document.bridgeSettings = json["bridgeSettings"].toObject();
    document.simulationSettings = json["simulationSettings"].toObject();
    return document;
}
