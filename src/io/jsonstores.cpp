#include "jsonstores.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace
{
QJsonArray angleBendTableArray(const QList<AngleBendTableRow> &rows)
{
    QJsonArray array;
    for (const AngleBendTableRow &row : rows) {
        array.append(row.toJson());
    }
    return array;
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

QJsonArray arcBendTableArray(const QList<ArcBendTableRow> &rows)
{
    QJsonArray array;
    for (const ArcBendTableRow &row : rows) {
        array.append(row.toJson());
    }
    return array;
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

bool writeJsonFile(const QString &filePath, const QJsonObject &json, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
    return true;
}

bool readJsonFile(const QString &filePath, QJsonObject *json, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage != nullptr) {
            *errorMessage = file.errorString();
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage != nullptr) {
            *errorMessage = parseError.errorString();
        }
        return false;
    }

    *json = document.object();
    return true;
}
}

bool JsonProjectStore::save(const QString &filePath, const ProjectDocument &document, QString *errorMessage)
{
    return writeJsonFile(filePath, document.toJson(), errorMessage);
}

bool JsonProjectStore::load(const QString &filePath, ProjectDocument *document, QString *errorMessage)
{
    QJsonObject json;
    if (!readJsonFile(filePath, &json, errorMessage)) {
        return false;
    }

    if (document != nullptr) {
        *document = ProjectDocument::fromJson(json);
        document->projectFilePath = filePath;
    }
    return true;
}

bool JsonSettingsStore::save(const QString &filePath, const AppSettings &settings, QString *errorMessage)
{
    return writeJsonFile(filePath, settings.toJson(), errorMessage);
}

bool JsonSettingsStore::load(const QString &filePath, AppSettings *settings, QString *errorMessage)
{
    QJsonObject json;
    if (!readJsonFile(filePath, &json, errorMessage)) {
        return false;
    }

    if (settings != nullptr) {
        *settings = AppSettings::fromJson(json);
    }
    return true;
}

bool JsonBendParametersStore::save(const QString &filePath, const AppSettings &settings, QString *errorMessage)
{
    QJsonObject json;
    json["angleBendTable"] = angleBendTableArray(settings.angleBendTable);
    json["arcBendTable"] = arcBendTableArray(settings.arcBendTable);
    return writeJsonFile(filePath, json, errorMessage);
}

bool JsonBendParametersStore::load(const QString &filePath, AppSettings *settings, QString *errorMessage)
{
    QJsonObject json;
    if (!readJsonFile(filePath, &json, errorMessage)) {
        return false;
    }

    if (settings != nullptr) {
        const QList<AngleBendTableRow> angleRows = angleBendTableFromJsonArray(json["angleBendTable"].toArray());
        const QList<ArcBendTableRow> arcRows = arcBendTableFromJsonArray(json["arcBendTable"].toArray());
        settings->angleBendTable = angleRows.isEmpty() ? AppSettings::createDefaultAngleBendTable() : angleRows;
        settings->arcBendTable = arcRows.isEmpty() ? AppSettings::createDefaultArcBendTable() : arcRows;
    }
    return true;
}
