#ifndef FOXBENDER_LAYERDEFINITION_H
#define FOXBENDER_LAYERDEFINITION_H

#include <QColor>
#include <QString>

class LayerDefinition
{
public:
    QString name;
    QColor color = Qt::black;
    bool visible = true;
    bool locked = false;
};

#endif // FOXBENDER_LAYERDEFINITION_H
