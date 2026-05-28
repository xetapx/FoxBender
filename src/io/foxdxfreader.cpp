#include "foxdxfreader.h"

#include "third_party/libdxfrw/src/drw_interface.h"
#include "third_party/libdxfrw/src/libdxfrw.h"

#include <QFileInfo>
#include <QtMath>

namespace
{
QString toQString(const std::string &value)
{
    return QString::fromUtf8(value.c_str());
}

QColor colorFromDxfIndex(int colorIndex)
{
    switch (qAbs(colorIndex)) {
    case 1: return QColor(255, 0, 0);
    case 2: return QColor(255, 255, 0);
    case 3: return QColor(0, 255, 0);
    case 4: return QColor(0, 255, 255);
    case 5: return QColor(0, 0, 255);
    case 6: return QColor(255, 0, 255);
    case 7: return QColor(255, 255, 255);
    case 8: return QColor(128, 128, 128);
    case 9: return QColor(192, 192, 192);
    default: return QColor(30, 30, 30);
    }
}

QColor resolveColor(int color24, int colorIndex)
{
    if (color24 >= 0) {
        return QColor::fromRgb(static_cast<QRgb>(color24));
    }
    return colorFromDxfIndex(colorIndex);
}

QPointF toPointF(const DRW_Coord &point)
{
    return {point.x, point.y};
}

QString handleString(duint32 handle)
{
    return handle == 0
               ? QStringLiteral("0")
               : QString::number(static_cast<qulonglong>(handle), 16).toUpper();
}

QString entityTypeToken(DxfEntityType type)
{
    switch (type) {
    case DxfEntityType::Line:
        return QStringLiteral("line");
    case DxfEntityType::Arc:
        return QStringLiteral("arc");
    case DxfEntityType::Polyline:
        return QStringLiteral("pline");
    }

    return QStringLiteral("ent");
}

class LibdxfrwAdapter final : public DRW_Interface
{
public:
    DxfDocument document;
    int nextEntityOrdinal = 1;

    void assignIdentity(DxfEntity &entity, duint32 handle)
    {
        entity.sourceHandle = handleString(handle);
        entity.id = QStringLiteral("%1_%2_h%3")
                        .arg(entityTypeToken(entity.type))
                        .arg(nextEntityOrdinal++, 6, 10, QChar('0'))
                        .arg(entity.sourceHandle);
    }

    void addHeader(const DRW_Header * /*data*/) override {}
    void addLType(const DRW_LType & /*data*/) override {}
    void addDimStyle(const DRW_Dimstyle & /*data*/) override {}
    void addVport(const DRW_Vport & /*data*/) override {}
    void addTextStyle(const DRW_Textstyle & /*data*/) override {}
    void addAppId(const DRW_AppId & /*data*/) override {}
    void addBlock(const DRW_Block & /*data*/) override {}
    void setBlock(const int /*handle*/) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point & /*data*/) override {}
    void addRay(const DRW_Ray & /*data*/) override {}
    void addXline(const DRW_Xline & /*data*/) override {}
    void addEllipse(const DRW_Ellipse & /*data*/) override {}
    void addSpline(const DRW_Spline * /*data*/) override {}
    void addKnot(const DRW_Entity & /*data*/) override {}
    void addInsert(const DRW_Insert & /*data*/) override {}
    void addTrace(const DRW_Trace & /*data*/) override {}
    void add3dFace(const DRW_3Dface & /*data*/) override {}
    void addSolid(const DRW_Solid & /*data*/) override {}
    void addMText(const DRW_MText & /*data*/) override {}
    void addText(const DRW_Text & /*data*/) override {}
    void addDimAlign(const DRW_DimAligned * /*data*/) override {}
    void addDimLinear(const DRW_DimLinear * /*data*/) override {}
    void addDimRadial(const DRW_DimRadial * /*data*/) override {}
    void addDimDiametric(const DRW_DimDiametric * /*data*/) override {}
    void addDimAngular(const DRW_DimAngular * /*data*/) override {}
    void addDimAngular3P(const DRW_DimAngular3p * /*data*/) override {}
    void addDimOrdinate(const DRW_DimOrdinate * /*data*/) override {}
    void addLeader(const DRW_Leader * /*data*/) override {}
    void addHatch(const DRW_Hatch * /*data*/) override {}
    void addViewport(const DRW_Viewport & /*data*/) override {}
    void addImage(const DRW_Image * /*data*/) override {}
    void linkImage(const DRW_ImageDef * /*data*/) override {}
    void addComment(const char * /*comment*/) override {}
    void addPlotSettings(const DRW_PlotSettings * /*data*/) override {}

    void writeHeader(DRW_Header & /*data*/) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}

    void addLayer(const DRW_Layer &data) override
    {
        LayerDefinition layer;
        layer.name = toQString(data.name);
        layer.color = resolveColor(data.color24, data.color);
        layer.visible = data.color >= 0;
        layer.locked = false;
        document.layers.append(layer);
    }

    void addLine(const DRW_Line &data) override
    {
        DxfEntity entity;
        entity.type = DxfEntityType::Line;
        assignIdentity(entity, data.handle);
        entity.layerName = toQString(data.layer);
        entity.color = resolveColor(data.color24, data.color);
        entity.points = {toPointF(data.basePoint), toPointF(data.secPoint)};
        document.entities.append(entity);
    }

    void addArc(const DRW_Arc &data) override
    {
        DxfEntity entity;
        entity.type = DxfEntityType::Arc;
        assignIdentity(entity, data.handle);
        entity.layerName = toQString(data.layer);
        entity.color = resolveColor(data.color24, data.color);
        entity.center = toPointF(data.basePoint);
        entity.radius = data.radious;
        entity.startAngleDeg = qRadiansToDegrees(data.staangle);
        entity.endAngleDeg = qRadiansToDegrees(data.endangle);
        document.entities.append(entity);
    }

    void addCircle(const DRW_Circle &data) override
    {
        DxfEntity entity;
        entity.type = DxfEntityType::Arc;
        assignIdentity(entity, data.handle);
        entity.layerName = toQString(data.layer);
        entity.color = resolveColor(data.color24, data.color);
        entity.center = toPointF(data.basePoint);
        entity.radius = data.radious;
        entity.startAngleDeg = 0.0;
        entity.endAngleDeg = 360.0;
        document.entities.append(entity);
    }

    void addLWPolyline(const DRW_LWPolyline &data) override
    {
        DxfEntity entity;
        entity.type = DxfEntityType::Polyline;
        assignIdentity(entity, data.handle);
        entity.layerName = toQString(data.layer);
        entity.color = resolveColor(data.color24, data.color);
        for (const auto &vertex : data.vertlist) {
            entity.points.append(QPointF(vertex->x, vertex->y));
        }
        document.entities.append(entity);
    }

    void addPolyline(const DRW_Polyline &data) override
    {
        DxfEntity entity;
        entity.type = DxfEntityType::Polyline;
        assignIdentity(entity, data.handle);
        entity.layerName = toQString(data.layer);
        entity.color = resolveColor(data.color24, data.color);
        for (const auto &vertex : data.vertlist) {
            entity.points.append(toPointF(vertex->basePoint));
        }
        document.entities.append(entity);
    }
};
}

FoxDxfReader::Result FoxDxfReader::loadFile(const QString &filePath) const
{
    Result result;

    if (filePath.trimmed().isEmpty()) {
        result.errorMessage = QStringLiteral("DXF file path is empty.");
        return result;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        result.errorMessage = QStringLiteral("DXF file does not exist: %1").arg(filePath);
        return result;
    }

    const QByteArray filePathBytes = QFile::encodeName(filePath);
    dxfRW reader(filePathBytes.constData());
    LibdxfrwAdapter adapter;
    adapter.document.sourceName = fileInfo.fileName();

    result.success = reader.read(&adapter, false);
    result.document = adapter.document;
    result.document.sourceName = fileInfo.fileName();

    if (!result.success) {
        result.errorMessage = QStringLiteral("libdxfrw could not parse the DXF file.");
    } else if (result.document.layers.isEmpty() && result.document.entities.isEmpty()) {
        result.success = false;
        result.errorMessage = QStringLiteral("DXF parsed but no supported entities were imported.");
    }

    return result;
}

DxfDocument FoxDxfReader::createDemoDocument() const
{
    return DxfDocument::createDemoDocument();
}
