#ifndef FOXBENDER_JSONSTORES_H
#define FOXBENDER_JSONSTORES_H

#include "src/app/appsettings.h"
#include "src/core/projectdocument.h"

#include <QString>

class JsonProjectStore
{
public:
    static bool save(const QString &filePath, const ProjectDocument &document, QString *errorMessage = nullptr);
    static bool load(const QString &filePath, ProjectDocument *document, QString *errorMessage = nullptr);
};

class JsonSettingsStore
{
public:
    static bool save(const QString &filePath, const AppSettings &settings, QString *errorMessage = nullptr);
    static bool load(const QString &filePath, AppSettings *settings, QString *errorMessage = nullptr);
};

class JsonBendParametersStore
{
public:
    static bool save(const QString &filePath, const AppSettings &settings, QString *errorMessage = nullptr);
    static bool load(const QString &filePath, AppSettings *settings, QString *errorMessage = nullptr);
};

#endif // FOXBENDER_JSONSTORES_H
