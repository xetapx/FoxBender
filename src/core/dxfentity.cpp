#include "dxfentity.h"

QString DxfEntity::typeName() const
{
    switch (type) {
    case DxfEntityType::Line:
        return QStringLiteral("LINE");
    case DxfEntityType::Arc:
        return QStringLiteral("ARC");
    case DxfEntityType::Polyline:
        return QStringLiteral("POLYLINE");
    }

    return QStringLiteral("UNKNOWN");
}

QString DxfEntity::summary() const
{
    return QStringLiteral("%1 on %2").arg(typeName(), layerName);
}
