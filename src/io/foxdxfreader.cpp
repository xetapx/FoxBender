#include "foxdxfreader.h"

#include "third_party/libdxfrw/src/drw_interface.h"
#include "third_party/libdxfrw/src/libdxfrw.h"

#include <QFileInfo>
#include <QLineF>
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

QPointF toPointF(const DRW_Vertex2D &vertex)
{
    return {vertex.x, vertex.y};
}

QPointF toPointF(const DRW_Vertex &vertex)
{
    return {vertex.basePoint.x, vertex.basePoint.y};
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

    void assignIdentity(DxfEntity &entity, duint32 handle, const QString &suffix = {})
    {
        entity.sourceHandle = handleString(handle);
        entity.id = QStringLiteral("%1_%2_h%3%4")
                        .arg(entityTypeToken(entity.type))
                        .arg(nextEntityOrdinal++, 6, 10, QChar('0'))
                        .arg(entity.sourceHandle)
                        .arg(suffix);
    }

    void appendPolylineSegment(const QPointF &startPoint,
                               const QPointF &endPoint,
                               double bulge,
                               duint32 handle,
                               const QString &layerName,
                               const QColor &color,
                               int segmentIndex)
    {
        if (QLineF(startPoint, endPoint).length() <= 1e-9) {
            return;
        }

        if (qAbs(bulge) <= 1e-9) {
            DxfEntity entity;
            entity.type = DxfEntityType::Line;
            assignIdentity(entity, handle, QStringLiteral("_s%1").arg(segmentIndex + 1));
            entity.layerName = layerName;
            entity.color = color;
            entity.points = {startPoint, endPoint};
            document.entities.append(entity);
            return;
        }

        const double thetaRad = 4.0 * std::atan(bulge);
        const double halfThetaRad = thetaRad * 0.5;
        const double chordLength = QLineF(startPoint, endPoint).length();
        const double sinHalfTheta = std::sin(std::abs(halfThetaRad));
        if (qFuzzyIsNull(sinHalfTheta)) {
            DxfEntity entity;
            entity.type = DxfEntityType::Line;
            assignIdentity(entity, handle, QStringLiteral("_s%1").arg(segmentIndex + 1));
            entity.layerName = layerName;
            entity.color = color;
            entity.points = {startPoint, endPoint};
            document.entities.append(entity);
            return;
        }

        const double radius = chordLength / (2.0 * sinHalfTheta);
        const QPointF chordVector = endPoint - startPoint;
        const QPointF chordUnit = chordVector / chordLength;
        const QPointF leftNormal(-chordUnit.y(), chordUnit.x());
        const QPointF midPoint = (startPoint + endPoint) * 0.5;
        const double centerOffset = chordLength * (1.0 - bulge * bulge) / (4.0 * bulge);
        const QPointF center = midPoint + leftNormal * centerOffset;

        DxfEntity entity;
        entity.type = DxfEntityType::Arc;
        assignIdentity(entity, handle, QStringLiteral("_s%1").arg(segmentIndex + 1));
        entity.layerName = layerName;
        entity.color = color;
        entity.center = center;
        entity.radius = radius;

        const double startAngleDeg =
            qRadiansToDegrees(std::atan2(startPoint.y() - center.y(), startPoint.x() - center.x()));
        const double endAngleDeg =
            qRadiansToDegrees(std::atan2(endPoint.y() - center.y(), endPoint.x() - center.x()));

        if (bulge > 0.0) {
            entity.startAngleDeg = startAngleDeg;
            entity.endAngleDeg = endAngleDeg;
        } else {
            entity.startAngleDeg = endAngleDeg;
            entity.endAngleDeg = startAngleDeg;
        }

        document.entities.append(entity);
    }

    template <typename VertexPtr>
    void appendExplodedPolyline(const std::vector<VertexPtr> &vertices,
                                bool closed,
                                duint32 handle,
                                const QString &layerName,
                                const QColor &color)
    {
        if (vertices.size() < 2) {
            return;
        }

        const int segmentCount = closed ? static_cast<int>(vertices.size())
                                        : static_cast<int>(vertices.size()) - 1;
        for (int segmentIndex = 0; segmentIndex < segmentCount; ++segmentIndex) {
            const int nextIndex = (segmentIndex + 1) % static_cast<int>(vertices.size());
            const auto &startVertex = vertices.at(segmentIndex);
            const auto &endVertex = vertices.at(nextIndex);
            appendPolylineSegment(toPointF(*startVertex),
                                  toPointF(*endVertex),
                                  startVertex->bulge,
                                  handle,
                                  layerName,
                                  color,
                                  segmentIndex);
        }
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
        appendExplodedPolyline(data.vertlist,
                               (data.flags & 0x01) != 0,
                               data.handle,
                               toQString(data.layer),
                               resolveColor(data.color24, data.color));
    }

    void addPolyline(const DRW_Polyline &data) override
    {
        appendExplodedPolyline(data.vertlist,
                               (data.flags & 0x01) != 0,
                               data.handle,
                               toQString(data.layer),
                               resolveColor(data.color24, data.color));
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
