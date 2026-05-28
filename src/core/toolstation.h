#ifndef FOXBENDER_TOOLSTATION_H
#define FOXBENDER_TOOLSTATION_H

#include <QJsonObject>
#include <QList>
#include <QString>

class ToolStation
{
public:
    QString id;
    QString name;
    double centerOffsetMm = 0.0;
    double widthMm = 0.0;
    int actionTimeoutMs = 0;
    bool enabled = true;

    QJsonObject toJson() const;
    static ToolStation fromJson(const QJsonObject &json);
    static QList<ToolStation> createDefaultStations();
};

#endif // FOXBENDER_TOOLSTATION_H
