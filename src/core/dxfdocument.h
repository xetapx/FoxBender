#ifndef FOXBENDER_DXFDOCUMENT_H
#define FOXBENDER_DXFDOCUMENT_H

#include "dxfentity.h"
#include "layerdefinition.h"

#include <QJsonObject>
#include <QList>
#include <QRectF>
#include <QString>

class DxfDocument
{
public:
    QString sourceName;
    QList<LayerDefinition> layers;
    QList<DxfEntity> entities;

    bool isEmpty() const;
    QRectF boundingRect() const;
    QJsonObject toJson() const;
    static DxfDocument fromJson(const QJsonObject &json);

    static DxfDocument createDemoDocument();
};

#endif // FOXBENDER_DXFDOCUMENT_H
