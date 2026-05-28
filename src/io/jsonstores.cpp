#include "jsonstores.h"

#include <QFile>
#include <QJsonDocument>

namespace
{
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
