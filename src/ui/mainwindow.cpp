#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "src/io/jsonstores.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QCloseEvent>
#include <QAction>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QJsonArray>
#include <QMessageBox>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QPushButton>
#include <QShortcut>
#include <QSlider>
#include <QStatusBar>
#include <QSet>
#include <QTableWidgetItem>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUndoCommand>
#include <QUndoStack>
#include <QLineF>
#include <optional>
#include <QtMath>

namespace
{
constexpr int BladeTreeItemTypeRole = Qt::UserRole + 1;
constexpr int BladeTreePathIndexRole = Qt::UserRole + 2;
constexpr int BladeTreeEntityIndexRole = Qt::UserRole + 3;
constexpr int BladeTreeEntityIdsRole = Qt::UserRole + 4;
constexpr int BladeTreeBridgeIndexesRole = Qt::UserRole + 5;
constexpr int BladeTreeBridgeIndexRole = Qt::UserRole + 6;
constexpr int BladeTreeOptionKeyRole = Qt::UserRole + 7;
constexpr int BladeTreeFamilyIndexRole = Qt::UserRole + 8;
constexpr int BladeTreeFamilyAngleRole = Qt::UserRole + 9;
constexpr int ToolStationIdRole = Qt::UserRole + 10;

constexpr int BladeTreeItemTypePath = 1;
constexpr int BladeTreeItemTypeSegment = 2;
constexpr int BladeTreeItemTypeFamily = 3;
constexpr int BladeTreeItemTypeBridge = 4;
constexpr int LayerNameRole = Qt::UserRole + 20;
constexpr int MaxRecentFiles = 10;

constexpr const char *RuleProfileOptionCountKey = "count";
constexpr const char *RuleProfileOptionMirrorKey = "mirror";
constexpr const char *RuleProfileOptionStartCorrectionKey = "startCorrectionMm";
constexpr const char *RuleProfileOptionEndCorrectionKey = "endCorrectionMm";
constexpr const char *RuleProfileOptionStartLipKey = "startLip";
constexpr const char *RuleProfileOptionEndLipKey = "endLip";
constexpr const char *RuleProfileSegmentAngleCorrectionKey = "segmentAngleCorrectionDeg";
constexpr const char *RuleProfileSegmentLengthCorrectionKey = "segmentLengthCorrectionMm";
constexpr const char *RuleProfileSegmentOpenModeKey = "segmentOpenMode";

QStringList arcOpenModeOptions()
{
    return {
        QStringLiteral("Normal"),
        QStringLiteral("1/5"),
        QStringLiteral("2/3"),
        QStringLiteral("1/2"),
        QStringLiteral("All open")
    };
}

double interpolateLinear(double targetX,
                         double leftX,
                         double leftY,
                         double rightX,
                         double rightY)
{
    if (qFuzzyCompare(leftX + 1.0, rightX + 1.0)) {
        return leftY;
    }

    const double t = (targetX - leftX) / (rightX - leftX);
    return leftY + (rightY - leftY) * t;
}

void sortAngleBendTable(QList<AngleBendTableRow> *rows)
{
    if (rows == nullptr) {
        return;
    }

    std::sort(rows->begin(), rows->end(), [](const AngleBendTableRow &left, const AngleBendTableRow &right) {
        return left.angleDeg < right.angleDeg;
    });
}

void sortArcBendTable(QList<ArcBendTableRow> *rows)
{
    if (rows == nullptr) {
        return;
    }

    std::sort(rows->begin(), rows->end(), [](const ArcBendTableRow &left, const ArcBendTableRow &right) {
        return left.radiusMm < right.radiusMm;
    });
}

double bendArcSegmentLengthMm(double radiusMm)
{
    const double absoluteRadius = qAbs(radiusMm);
    if (absoluteRadius <= 5.0) {
        return 0.5;
    }
    if (absoluteRadius <= 10.0) {
        return 0.7;
    }
    if (absoluteRadius <= 20.0) {
        return 1.0;
    }
    return 1.5;
}

int bendArcSegmentCount(double radiusMm, double totalAngleDeg, double segmentLengthMm)
{
    if (segmentLengthMm <= 0.0 || totalAngleDeg <= 0.0 || radiusMm <= 0.0) {
        return 1;
    }

    const double arcLengthMm = radiusMm * qDegreesToRadians(totalAngleDeg);
    return qMax(1, qRound(arcLengthMm / segmentLengthMm));
}

int bendArcValue(double radiusMm, double totalAngleDeg, double segmentLengthMm)
{
    const int segments = bendArcSegmentCount(radiusMm, totalAngleDeg, segmentLengthMm);
    return qRound((totalAngleDeg / static_cast<double>(segments)) * 100.0);
}

double bendArcDegreesFor90(double segments)
{
    if (segments <= 0.0) {
        return 0.0;
    }

    return 90.0 / segments;
}

ArcBendTableRow defaultArcBendRow(double radiusMm)
{
    ArcBendTableRow row;
    row.radiusMm = radiusMm;
    const double segmentLengthMm = bendArcSegmentLengthMm(radiusMm);
    row.segmentLengthMm = segmentLengthMm;
    row.segments = bendArcSegmentCount(radiusMm, 90.0, segmentLengthMm);
    row.right90 = bendArcValue(radiusMm, 90.0, segmentLengthMm);
    row.right45 = bendArcValue(radiusMm, 45.0, segmentLengthMm);
    row.left90 = bendArcValue(radiusMm, 90.0, segmentLengthMm);
    row.left45 = bendArcValue(radiusMm, 45.0, segmentLengthMm);
    return row;
}

QString settingsFilePath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/settings.json");
}

QString bendParametersFilePath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/bend_default.json");
}

QString sanitizeBendParametersFileBase(const QString &name)
{
    QString sanitized;
    sanitized.reserve(name.size());
    bool previousUnderscore = false;

    for (const QChar ch : name.trimmed()) {
        if (ch.isLetterOrNumber()) {
            sanitized.append(ch.toLower());
            previousUnderscore = false;
            continue;
        }

        if (ch == QChar('-') || ch == QChar('_') || ch.isSpace()) {
            if (!previousUnderscore && !sanitized.isEmpty()) {
                sanitized.append(QChar('_'));
                previousUnderscore = true;
            }
        }
    }

    while (sanitized.endsWith(QChar('_'))) {
        sanitized.chop(1);
    }

    if (sanitized.isEmpty()) {
        sanitized = QStringLiteral("default");
    }

    return QStringLiteral("bend_%1.json").arg(sanitized);
}

QString treeItemKey(const QTreeWidgetItem *item)
{
    if (item == nullptr) {
        return QString();
    }

    QStringList parts;
    const QTreeWidgetItem *current = item;
    while (current != nullptr) {
        parts.prepend(current->text(0));
        current = current->parent();
    }
    return parts.join(QStringLiteral("/"));
}

void collectExpandedTreeKeys(const QTreeWidgetItem *item, QSet<QString> *expandedKeys)
{
    if (item == nullptr || expandedKeys == nullptr) {
        return;
    }

    if (item->isExpanded()) {
        expandedKeys->insert(treeItemKey(item));
    }

    for (int childIndex = 0; childIndex < item->childCount(); ++childIndex) {
        collectExpandedTreeKeys(item->child(childIndex), expandedKeys);
    }
}

QSet<QString> expandedTreeKeys(const QTreeWidget *tree)
{
    QSet<QString> expandedKeys;
    if (tree == nullptr) {
        return expandedKeys;
    }

    for (int topLevelIndex = 0; topLevelIndex < tree->topLevelItemCount(); ++topLevelIndex) {
        collectExpandedTreeKeys(tree->topLevelItem(topLevelIndex), &expandedKeys);
    }
    return expandedKeys;
}

void restoreExpandedTreeKeys(QTreeWidgetItem *item, const QSet<QString> &expandedKeys)
{
    if (item == nullptr) {
        return;
    }

    item->setExpanded(expandedKeys.contains(treeItemKey(item)));
    for (int childIndex = 0; childIndex < item->childCount(); ++childIndex) {
        restoreExpandedTreeKeys(item->child(childIndex), expandedKeys);
    }
}

void restoreExpandedTreeKeys(QTreeWidget *tree, const QSet<QString> &expandedKeys)
{
    if (tree == nullptr) {
        return;
    }

    for (int topLevelIndex = 0; topLevelIndex < tree->topLevelItemCount(); ++topLevelIndex) {
        restoreExpandedTreeKeys(tree->topLevelItem(topLevelIndex), expandedKeys);
    }
}

QString pointText(const QPointF &point)
{
    return QStringLiteral("(%1, %2)")
        .arg(QString::number(point.x(), 'f', 3),
             QString::number(point.y(), 'f', 3));
}

QStringList intListToStringList(const QList<int> &values)
{
    QStringList result;
    result.reserve(values.size());
    for (int value : values) {
        result.append(QString::number(value));
    }
    return result;
}

QList<int> stringListToIntList(const QStringList &values)
{
    QList<int> result;
    for (const QString &value : values) {
        bool ok = false;
        const int parsed = value.toInt(&ok);
        if (ok) {
            result.append(parsed);
        }
    }
    return result;
}

QStringList hiddenLayersFromSettings(const AppSettings &settings)
{
    return settings.hiddenLayers;
}

void storeHiddenLayersToSettings(AppSettings *settings, const DxfDocument &document)
{
    if (settings == nullptr) {
        return;
    }

    QStringList hiddenLayers;
    for (const LayerDefinition &layer : document.layers) {
        if (!layer.visible) {
            hiddenLayers.append(layer.name);
        }
    }

    settings->hiddenLayers = hiddenLayers;
}

double polylineLength(const QVector<QPointF> &points)
{
    double totalLength = 0.0;
    for (int index = 1; index < points.size(); ++index) {
        totalLength += QLineF(points.at(index - 1), points.at(index)).length();
    }
    return totalLength;
}

int findToolStationIndexById(const QList<ToolStation> &stations, const QString &id)
{
    for (int index = 0; index < stations.size(); ++index) {
        if (stations.at(index).id == id) {
            return index;
        }
    }
    return -1;
}

double normalizedArcSweepDegrees(double startAngleDeg, double endAngleDeg)
{
    double sweepDeg = endAngleDeg - startAngleDeg;
    while (sweepDeg < 0.0) {
        sweepDeg += 360.0;
    }
    if (qFuzzyIsNull(sweepDeg)) {
        sweepDeg = 360.0;
    }
    return sweepDeg;
}

double normalizeAngle360(double angleDeg)
{
    while (angleDeg < 0.0) {
        angleDeg += 360.0;
    }
    while (angleDeg >= 360.0) {
        angleDeg -= 360.0;
    }
    return angleDeg;
}

QPointF arcPointAtAngle(const QPointF &center, double radius, double angleDeg)
{
    const double angleRad = qDegreesToRadians(angleDeg);
    return QPointF(center.x() + radius * qCos(angleRad),
                   center.y() + radius * qSin(angleRad));
}

QPointF canonicalUnitVector(const QPointF &vector)
{
    const double length = std::hypot(vector.x(), vector.y());
    if (qFuzzyIsNull(length)) {
        return {};
    }

    QPointF unit(vector.x() / length, vector.y() / length);
    if (unit.x() < -1e-9 || (qAbs(unit.x()) <= 1e-9 && unit.y() < 0.0)) {
        unit = -unit;
    }
    return unit;
}

double cross2d(const QPointF &left, const QPointF &right)
{
    return left.x() * right.y() - left.y() * right.x();
}

double vectorDotProduct(const QPointF &left, const QPointF &right)
{
    return left.x() * right.x() + left.y() * right.y();
}

double pointDistanceToInfiniteLine(const QPointF &point,
                                   const QPointF &linePoint,
                                   const QPointF &lineUnit)
{
    return qAbs(cross2d(point - linePoint, lineUnit));
}

bool linesShareInfiniteLine(const DxfEntity &left, const DxfEntity &right, double tolerance)
{
    if (left.type != DxfEntityType::Line || right.type != DxfEntityType::Line
        || left.points.size() != 2 || right.points.size() != 2) {
        return false;
    }

    const QPointF leftUnit = canonicalUnitVector(left.points.at(1) - left.points.at(0));
    const QPointF rightUnit = canonicalUnitVector(right.points.at(1) - right.points.at(0));
    if (std::hypot(leftUnit.x(), leftUnit.y()) < 1e-9
        || std::hypot(rightUnit.x(), rightUnit.y()) < 1e-9) {
        return false;
    }

    if (qAbs(cross2d(leftUnit, rightUnit)) > tolerance) {
        return false;
    }

    return pointDistanceToInfiniteLine(right.points.at(0), left.points.at(0), leftUnit) <= tolerance
           && pointDistanceToInfiniteLine(right.points.at(1), left.points.at(0), leftUnit) <= tolerance;
}

QList<DetectedBridge> detectLineBridges(const DxfDocument &document,
                                    double minGapMm,
                                    double maxGapMm,
                                    double tolerance)
{
    struct LineCoverage
    {
        int entityIndex = -1;
        double minT = 0.0;
        double maxT = 0.0;
    };

    struct MergedCoverage
    {
        double minT = 0.0;
        double maxT = 0.0;
        int leftEntityIndex = -1;
        int rightEntityIndex = -1;
    };

    QList<int> lineEntityIndexes;
    for (int index = 0; index < document.entities.size(); ++index) {
        const DxfEntity &entity = document.entities.at(index);
        if (entity.type == DxfEntityType::Line && entity.points.size() == 2) {
            lineEntityIndexes.append(index);
        }
    }

    QList<DetectedBridge> bridges;
    QVector<bool> visited(lineEntityIndexes.size(), false);
    for (int groupSeed = 0; groupSeed < lineEntityIndexes.size(); ++groupSeed) {
        if (visited.at(groupSeed)) {
            continue;
        }

        const int baseEntityIndex = lineEntityIndexes.at(groupSeed);
        const DxfEntity &baseEntity = document.entities.at(baseEntityIndex);
        const QPointF axisUnit = canonicalUnitVector(baseEntity.points.at(1) - baseEntity.points.at(0));
        if (std::hypot(axisUnit.x(), axisUnit.y()) < 1e-9) {
            visited[groupSeed] = true;
            continue;
        }

        const QPointF origin = baseEntity.points.at(0);
        QList<LineCoverage> coverages;
        for (int candidateListIndex = groupSeed; candidateListIndex < lineEntityIndexes.size(); ++candidateListIndex) {
            if (visited.at(candidateListIndex)) {
                continue;
            }

            const int candidateEntityIndex = lineEntityIndexes.at(candidateListIndex);
            const DxfEntity &candidate = document.entities.at(candidateEntityIndex);
            if (!linesShareInfiniteLine(baseEntity, candidate, tolerance)) {
                continue;
            }

            visited[candidateListIndex] = true;
            const double firstT = vectorDotProduct(candidate.points.at(0) - origin, axisUnit);
            const double secondT = vectorDotProduct(candidate.points.at(1) - origin, axisUnit);

            LineCoverage coverage;
            coverage.entityIndex = candidateEntityIndex;
            coverage.minT = qMin(firstT, secondT);
            coverage.maxT = qMax(firstT, secondT);
            coverages.append(coverage);
        }

        if (coverages.size() < 2) {
            continue;
        }

        std::sort(coverages.begin(), coverages.end(), [](const LineCoverage &left, const LineCoverage &right) {
            if (!qFuzzyCompare(left.minT + 1.0, right.minT + 1.0)) {
                return left.minT < right.minT;
            }
            return left.maxT < right.maxT;
        });

        MergedCoverage current;
        current.minT = coverages.first().minT;
        current.maxT = coverages.first().maxT;
        current.leftEntityIndex = coverages.first().entityIndex;
        current.rightEntityIndex = coverages.first().entityIndex;

        for (int coverageIndex = 1; coverageIndex < coverages.size(); ++coverageIndex) {
            const LineCoverage &next = coverages.at(coverageIndex);
            if (next.minT <= current.maxT + tolerance) {
                if (next.maxT > current.maxT) {
                    current.maxT = next.maxT;
                    current.rightEntityIndex = next.entityIndex;
                }
                continue;
            }

            const double gapLength = next.minT - current.maxT;
            if (gapLength >= minGapMm - tolerance && gapLength <= maxGapMm + tolerance) {
                DetectedBridge bridge;
                bridge.type = DetectedBridgeType::Line;
                bridge.startPoint = origin + axisUnit * current.maxT;
                bridge.endPoint = origin + axisUnit * next.minT;
                bridge.lengthMm = gapLength;
                bridge.relatedEntityIndexes = {current.rightEntityIndex, next.entityIndex};
                bridges.append(bridge);
            }

            current.minT = next.minT;
            current.maxT = next.maxT;
            current.leftEntityIndex = next.entityIndex;
            current.rightEntityIndex = next.entityIndex;
        }
    }

    return bridges;
}

QList<DetectedBridge> detectArcBridges(const DxfDocument &document,
                                   double minGapMm,
                                   double maxGapMm,
                                   double tolerance)
{
    struct ArcPart
    {
        double startDeg = 0.0;
        double endDeg = 0.0;
        int entityIndex = -1;
    };

    struct ArcCoverage
    {
        double startDeg = 0.0;
        double endDeg = 0.0;
        int startEntityIndex = -1;
        int endEntityIndex = -1;
    };

    QList<int> arcEntityIndexes;
    for (int index = 0; index < document.entities.size(); ++index) {
        const DxfEntity &entity = document.entities.at(index);
        if (entity.type == DxfEntityType::Arc && entity.radius > tolerance) {
            arcEntityIndexes.append(index);
        }
    }

    QList<DetectedBridge> bridges;
    QVector<bool> visited(arcEntityIndexes.size(), false);
    for (int groupSeed = 0; groupSeed < arcEntityIndexes.size(); ++groupSeed) {
        if (visited.at(groupSeed)) {
            continue;
        }

        const int baseEntityIndex = arcEntityIndexes.at(groupSeed);
        const DxfEntity &base = document.entities.at(baseEntityIndex);

        QList<int> groupIndexes;
        for (int candidateListIndex = groupSeed; candidateListIndex < arcEntityIndexes.size(); ++candidateListIndex) {
            if (visited.at(candidateListIndex)) {
                continue;
            }

            const int candidateEntityIndex = arcEntityIndexes.at(candidateListIndex);
            const DxfEntity &candidate = document.entities.at(candidateEntityIndex);
            if (QLineF(base.center, candidate.center).length() > tolerance
                || qAbs(base.radius - candidate.radius) > tolerance) {
                continue;
            }

            visited[candidateListIndex] = true;
            groupIndexes.append(candidateEntityIndex);
        }

        if (groupIndexes.size() < 2) {
            continue;
        }

        QList<ArcPart> parts;
        for (int entityIndex : groupIndexes) {
            const DxfEntity &entity = document.entities.at(entityIndex);
            const double startDeg = normalizeAngle360(entity.startAngleDeg);
            const double sweepDeg = normalizedArcSweepDegrees(entity.startAngleDeg, entity.endAngleDeg);
            if (sweepDeg >= 360.0 - tolerance) {
                parts.clear();
                break;
            }

            const double endDeg = startDeg + sweepDeg;
            if (endDeg <= 360.0 + tolerance) {
                parts.append({startDeg, qMin(endDeg, 360.0), entityIndex});
            } else {
                parts.append({startDeg, 360.0, entityIndex});
                parts.append({0.0, endDeg - 360.0, entityIndex});
            }
        }

        if (parts.size() < 2) {
            continue;
        }

        std::sort(parts.begin(), parts.end(), [](const ArcPart &left, const ArcPart &right) {
            if (!qFuzzyCompare(left.startDeg + 1.0, right.startDeg + 1.0)) {
                return left.startDeg < right.startDeg;
            }
            return left.endDeg < right.endDeg;
        });

        QList<ArcCoverage> coverages;
        ArcCoverage current;
        current.startDeg = parts.first().startDeg;
        current.endDeg = parts.first().endDeg;
        current.startEntityIndex = parts.first().entityIndex;
        current.endEntityIndex = parts.first().entityIndex;

        for (int partIndex = 1; partIndex < parts.size(); ++partIndex) {
            const ArcPart &next = parts.at(partIndex);
            if (next.startDeg <= current.endDeg + tolerance) {
                if (next.endDeg > current.endDeg) {
                    current.endDeg = next.endDeg;
                    current.endEntityIndex = next.entityIndex;
                }
                continue;
            }

            coverages.append(current);
            current.startDeg = next.startDeg;
            current.endDeg = next.endDeg;
            current.startEntityIndex = next.entityIndex;
            current.endEntityIndex = next.entityIndex;
        }
        coverages.append(current);

        if (coverages.size() < 2) {
            continue;
        }

        for (int coverageIndex = 0; coverageIndex < coverages.size(); ++coverageIndex) {
            const ArcCoverage &currentCoverage = coverages.at(coverageIndex);
            const ArcCoverage &nextCoverage = coverages.at((coverageIndex + 1) % coverages.size());
            const double gapDeg = coverageIndex + 1 < coverages.size()
                                      ? nextCoverage.startDeg - currentCoverage.endDeg
                                      : nextCoverage.startDeg + 360.0 - currentCoverage.endDeg;
            const double arcLength = qDegreesToRadians(gapDeg) * base.radius;
            if (gapDeg <= tolerance || arcLength < minGapMm - tolerance || arcLength > maxGapMm + tolerance) {
                continue;
            }

            DetectedBridge bridge;
            bridge.type = DetectedBridgeType::Arc;
            bridge.center = base.center;
            bridge.radius = base.radius;
            bridge.startAngleDeg = currentCoverage.endDeg;
            bridge.endAngleDeg = normalizeAngle360(currentCoverage.endDeg + gapDeg);
            bridge.lengthMm = arcLength;
            bridge.startPoint = arcPointAtAngle(base.center, base.radius, bridge.startAngleDeg);
            bridge.endPoint = arcPointAtAngle(base.center, base.radius, bridge.endAngleDeg);
            bridge.relatedEntityIndexes = {currentCoverage.endEntityIndex, nextCoverage.startEntityIndex};
            bridges.append(bridge);
        }
    }

    return bridges;
}

QList<DetectedBridge> detectBridges(const DxfDocument &document,
                                double minGapMm,
                                double maxGapMm,
                                double tolerance)
{
    QList<DetectedBridge> bridges = detectLineBridges(document, minGapMm, maxGapMm, tolerance);
    const QList<DetectedBridge> arcBridges = detectArcBridges(document, minGapMm, maxGapMm, tolerance);
    for (const DetectedBridge &bridge : arcBridges) {
        bridges.append(bridge);
    }
    return bridges;
}

void drawPreviewFlag(QPainter &painter,
                     const QPointF &anchor,
                     const QPointF &tangentUnit,
                     const QColor &color,
                     bool fillFlag,
                     double scale,
                     bool reverseTangent)
{
    const QPointF normalUnit(-tangentUnit.y(), tangentUnit.x());
    const qreal mastLength = 14.0 * scale;
    const qreal flagLength = 10.0 * scale;
    const qreal flagHalfHeight = 4.0 * scale;
    const qreal anchorRadius = 2.5 * scale;

    QPen pen(color, 1.4);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(anchor, anchor + normalUnit * mastLength);

    const QPointF mastTip = anchor + normalUnit * mastLength;
    const QPointF directedTangent = reverseTangent ? -tangentUnit : tangentUnit;
    QPolygonF flag;
    flag << mastTip
         << mastTip + directedTangent * flagLength + normalUnit * flagHalfHeight
         << mastTip + directedTangent * flagLength - normalUnit * flagHalfHeight;
    painter.setBrush(fillFlag ? QBrush(color) : QBrush(Qt::NoBrush));
    painter.drawPolygon(flag);

    painter.setBrush(Qt::white);
    painter.drawEllipse(anchor, anchorRadius, anchorRadius);
}
struct FilletResult
{
    bool success = false;
    DxfDocument document;
    QString errorMessage;
};

struct BreakResult
{
    bool success = false;
    DxfDocument document;
    QString errorMessage;
};

struct PathEndpointInfo
{
    bool valid = false;
    QPointF startPoint;
    QPointF endPoint;
};

struct PendingRuleProfileState
{
    bool valid = false;
    bool hasDirection = false;
    QPointF startPoint;
    QPointF nextPoint;
    QPointF openPoint;
    int openEntityIndex = -1;
};

struct DirectedPendingEntityInfo
{
    int entityIndex = -1;
    QPointF pathStartPoint;
    QPointF pathEndPoint;
};

PendingRuleProfileState pendingRuleProfileState(const DxfDocument &document,
                                            const QStringList &pendingEntityIds,
                                            const QList<DetectedBridge> &bridges);

QPointF normalizedVector(const QPointF &vector)
{
    const double length = std::hypot(vector.x(), vector.y());
    if (qFuzzyIsNull(length)) {
        return {};
    }
    return QPointF(vector.x() / length, vector.y() / length);
}

double crossProduct(const QPointF &left, const QPointF &right)
{
    return left.x() * right.y() - left.y() * right.x();
}

double dotProduct(const QPointF &left, const QPointF &right)
{
    return left.x() * right.x() + left.y() * right.y();
}

bool pointsNear(const QPointF &left, const QPointF &right, double tolerance = 0.01)
{
    return QLineF(left, right).length() <= tolerance;
}

PathEndpointInfo endpointInfoForEntity(const DxfEntity &entity);

double signedTurnDegrees(const QPointF &fromVector, const QPointF &toVector)
{
    const QPointF normalizedFrom = normalizedVector(fromVector);
    const QPointF normalizedTo = normalizedVector(toVector);
    if (qFuzzyIsNull(std::hypot(normalizedFrom.x(), normalizedFrom.y()))
        || qFuzzyIsNull(std::hypot(normalizedTo.x(), normalizedTo.y()))) {
        return 0.0;
    }

    return qRadiansToDegrees(std::atan2(crossProduct(normalizedFrom, normalizedTo),
                                        dotProduct(normalizedFrom, normalizedTo)));
}

bool directedTangentsForEntity(const DxfEntity &entity,
                               const QPointF &pathStartPoint,
                               const QPointF &pathEndPoint,
                               QPointF *entryTangent,
                               QPointF *exitTangent)
{
    if (entryTangent == nullptr || exitTangent == nullptr) {
        return false;
    }

    if (entity.type == DxfEntityType::Line && entity.points.size() >= 2) {
        const QPointF tangent = normalizedVector(pathEndPoint - pathStartPoint);
        if (qFuzzyIsNull(std::hypot(tangent.x(), tangent.y()))) {
            return false;
        }
        *entryTangent = tangent;
        *exitTangent = tangent;
        return true;
    }

    if (entity.type != DxfEntityType::Arc || qFuzzyIsNull(entity.radius)) {
        return false;
    }

    const PathEndpointInfo endpointInfo = endpointInfoForEntity(entity);
    if (!endpointInfo.valid) {
        return false;
    }

    const bool forward = pointsNear(pathStartPoint, endpointInfo.startPoint)
                         && pointsNear(pathEndPoint, endpointInfo.endPoint);
    const bool reverse = pointsNear(pathStartPoint, endpointInfo.endPoint)
                         && pointsNear(pathEndPoint, endpointInfo.startPoint);
    if (!forward && !reverse) {
        return false;
    }

    const auto tangentAtPoint = [&](const QPointF &point, bool ccwDirection) {
        const double angleRad = std::atan2(point.y() - entity.center.y(), point.x() - entity.center.x());
        QPointF tangent(-std::sin(angleRad), std::cos(angleRad));
        if (!ccwDirection) {
            tangent = -tangent;
        }
        return normalizedVector(tangent);
    };

    *entryTangent = tangentAtPoint(pathStartPoint, forward);
    *exitTangent = tangentAtPoint(pathEndPoint, forward);
    return true;
}

QPointF entityMidPointForOrdering(const DxfEntity &entity)
{
    if (entity.type == DxfEntityType::Arc) {
        double endDeg = entity.endAngleDeg;
        while (endDeg < entity.startAngleDeg) {
            endDeg += 360.0;
        }
        const double midDeg = entity.startAngleDeg + (endDeg - entity.startAngleDeg) * 0.5;
        return arcPointAtAngle(entity.center, entity.radius, midDeg);
    }

    if (entity.points.size() >= 2) {
        return (entity.points.first() + entity.points.last()) * 0.5;
    }

    return entity.center;
}

PathEndpointInfo endpointInfoForEntity(const DxfEntity &entity)
{
    PathEndpointInfo info;

    if (entity.type == DxfEntityType::Arc) {
        info.valid = true;
        info.startPoint = QPointF(entity.center.x() + entity.radius * qCos(qDegreesToRadians(entity.startAngleDeg)),
                                  entity.center.y() + entity.radius * qSin(qDegreesToRadians(entity.startAngleDeg)));
        info.endPoint = QPointF(entity.center.x() + entity.radius * qCos(qDegreesToRadians(entity.endAngleDeg)),
                                entity.center.y() + entity.radius * qSin(qDegreesToRadians(entity.endAngleDeg)));
        return info;
    }

    if (entity.points.size() >= 2) {
        info.valid = true;
        info.startPoint = entity.points.first();
        info.endPoint = entity.points.last();
    }

    return info;
}

QString nextFilletId(const DxfDocument &document)
{
    return QStringLiteral("fillet_%1").arg(document.entities.size() + 1);
}

QString nextBreakId(const DxfDocument &document)
{
    return QStringLiteral("break_%1").arg(document.entities.size() + 1);
}

QList<int> resolveRuleProfileIndexes(const DxfDocument &document, const QStringList &entityIds)
{
    QList<int> indexes;
    for (const QString &entityId : entityIds) {
        for (int index = 0; index < document.entities.size(); ++index) {
            if (document.entities.at(index).id == entityId) {
                indexes.append(index);
                break;
            }
        }
    }
    return indexes;
}

const QStringList *activeRuleProfile(const ProjectDocument *document)
{
    if (document == nullptr || document->ruleProfiles.isEmpty()) {
        return nullptr;
    }

    if (document->activeRuleProfileIndex < 0 || document->activeRuleProfileIndex >= document->ruleProfiles.size()) {
        return nullptr;
    }

    return &document->ruleProfiles[document->activeRuleProfileIndex];
}

QList<int> resolveAllRuleProfileIndexes(const DxfDocument &document, const QList<QStringList> &ruleProfiles)
{
    QList<int> indexes;
    for (const QStringList &ruleProfile : ruleProfiles) {
        const QList<int> pathIndexes = resolveRuleProfileIndexes(document, ruleProfile);
        for (int index : pathIndexes) {
            if (!indexes.contains(index)) {
                indexes.append(index);
            }
        }
    }
    return indexes;
}

int totalRuleProfileEntityCount(const QList<QStringList> &ruleProfiles)
{
    int count = 0;
    for (const QStringList &ruleProfile : ruleProfiles) {
        count += ruleProfile.size();
    }
    return count;
}

QList<int> mergeUniqueIndexes(const QList<int> &left, const QList<int> &right)
{
    QList<int> merged = left;
    for (int index : right) {
        if (!merged.contains(index)) {
            merged.append(index);
        }
    }
    return merged;
}

bool bridgeEndpointsForEntities(const QList<DetectedBridge> &bridges,
                                int firstEntityIndex,
                                int secondEntityIndex,
                                QPointF *firstBridgePoint,
                                QPointF *secondBridgePoint)
{
    for (const DetectedBridge &bridge : bridges) {
        if (bridge.relatedEntityIndexes.size() != 2) {
            continue;
        }

        if (bridge.relatedEntityIndexes.at(0) == firstEntityIndex
            && bridge.relatedEntityIndexes.at(1) == secondEntityIndex) {
            if (firstBridgePoint != nullptr) {
                *firstBridgePoint = bridge.startPoint;
            }
            if (secondBridgePoint != nullptr) {
                *secondBridgePoint = bridge.endPoint;
            }
            return true;
        }

        if (bridge.relatedEntityIndexes.at(0) == secondEntityIndex
            && bridge.relatedEntityIndexes.at(1) == firstEntityIndex) {
            if (firstBridgePoint != nullptr) {
                *firstBridgePoint = bridge.endPoint;
            }
            if (secondBridgePoint != nullptr) {
                *secondBridgePoint = bridge.startPoint;
            }
            return true;
        }
    }

    return false;
}

bool entitiesConnectedAtAnyEndpoint(const QList<DetectedBridge> &bridges,
                                    int firstEntityIndex,
                                    const PathEndpointInfo &firstInfo,
                                    int secondEntityIndex,
                                    const PathEndpointInfo &secondInfo)
{
    if (pointsNear(firstInfo.startPoint, secondInfo.startPoint)
        || pointsNear(firstInfo.startPoint, secondInfo.endPoint)
        || pointsNear(firstInfo.endPoint, secondInfo.startPoint)
        || pointsNear(firstInfo.endPoint, secondInfo.endPoint)) {
        return true;
    }

    QPointF firstBridgePoint;
    QPointF secondBridgePoint;
    if (!bridgeEndpointsForEntities(bridges,
                                    firstEntityIndex,
                                    secondEntityIndex,
                                    &firstBridgePoint,
                                    &secondBridgePoint)) {
        return false;
    }

    return (pointsNear(firstInfo.startPoint, firstBridgePoint) || pointsNear(firstInfo.endPoint, firstBridgePoint))
           && (pointsNear(secondInfo.startPoint, secondBridgePoint) || pointsNear(secondInfo.endPoint, secondBridgePoint));
}

bool tryConnectToNextEntity(const QList<DetectedBridge> &bridges,
                            int currentEntityIndex,
                            const PathEndpointInfo &/*currentInfo*/,
                            const QPointF &currentOpenPoint,
                            int nextEntityIndex,
                            const PathEndpointInfo &nextInfo,
                            QPointF *nextOpenPoint)
{
    if (pointsNear(currentOpenPoint, nextInfo.startPoint)) {
        if (nextOpenPoint != nullptr) {
            *nextOpenPoint = nextInfo.endPoint;
        }
        return true;
    }
    if (pointsNear(currentOpenPoint, nextInfo.endPoint)) {
        if (nextOpenPoint != nullptr) {
            *nextOpenPoint = nextInfo.startPoint;
        }
        return true;
    }

    QPointF currentBridgePoint;
    QPointF nextBridgePoint;
    if (!bridgeEndpointsForEntities(bridges,
                                    currentEntityIndex,
                                    nextEntityIndex,
                                    &currentBridgePoint,
                                    &nextBridgePoint)
        || !pointsNear(currentOpenPoint, currentBridgePoint)) {
        return false;
    }

    if (pointsNear(nextInfo.startPoint, nextBridgePoint)) {
        if (nextOpenPoint != nullptr) {
            *nextOpenPoint = nextInfo.endPoint;
        }
        return true;
    }
    if (pointsNear(nextInfo.endPoint, nextBridgePoint)) {
        if (nextOpenPoint != nullptr) {
            *nextOpenPoint = nextInfo.startPoint;
        }
        return true;
    }

    return false;
}

QList<int> expandBridgeCandidates(const QList<DetectedBridge> &bridges,
                                  const QList<int> &pendingIndexes,
                                  const QList<int> &seedIndexes)
{
    QList<int> expanded = seedIndexes;
    for (int queueIndex = 0; queueIndex < expanded.size(); ++queueIndex) {
        const int currentIndex = expanded.at(queueIndex);
        for (const DetectedBridge &bridge : bridges) {
            if (bridge.relatedEntityIndexes.size() != 2) {
                continue;
            }

            int otherIndex = -1;
            if (bridge.relatedEntityIndexes.at(0) == currentIndex) {
                otherIndex = bridge.relatedEntityIndexes.at(1);
            } else if (bridge.relatedEntityIndexes.at(1) == currentIndex) {
                otherIndex = bridge.relatedEntityIndexes.at(0);
            } else {
                continue;
            }

            if (otherIndex < 0 || otherIndex == currentIndex || expanded.contains(otherIndex)) {
                continue;
            }

            if (pendingIndexes.contains(otherIndex)) {
                continue;
            }

            expanded.append(otherIndex);
        }
    }

    return expanded;
}

int findBridgeIndexForEntities(const QList<DetectedBridge> &bridges, int firstEntityIndex, int secondEntityIndex)
{
    for (int bridgeIndex = 0; bridgeIndex < bridges.size(); ++bridgeIndex) {
        const DetectedBridge &bridge = bridges.at(bridgeIndex);
        if (bridge.relatedEntityIndexes.size() != 2) {
            continue;
        }
        if ((bridge.relatedEntityIndexes.at(0) == firstEntityIndex && bridge.relatedEntityIndexes.at(1) == secondEntityIndex)
            || (bridge.relatedEntityIndexes.at(0) == secondEntityIndex && bridge.relatedEntityIndexes.at(1) == firstEntityIndex)) {
            return bridgeIndex;
        }
    }
    return -1;
}

bool entitiesShareSelectableFamily(const DxfDocument &document,
                                   int firstEntityIndex,
                                   int secondEntityIndex,
                                   double tolerance)
{
    if (firstEntityIndex < 0 || secondEntityIndex < 0
        || firstEntityIndex >= document.entities.size()
        || secondEntityIndex >= document.entities.size()) {
        return false;
    }

    const DxfEntity &first = document.entities.at(firstEntityIndex);
    const DxfEntity &second = document.entities.at(secondEntityIndex);
    if (first.type != second.type) {
        return false;
    }

    if (first.type == DxfEntityType::Line) {
        return linesShareInfiniteLine(first, second, tolerance);
    }

    if (first.type == DxfEntityType::Arc) {
        return QLineF(first.center, second.center).length() <= tolerance
               && qAbs(first.radius - second.radius) <= tolerance;
    }

    return false;
}

bool entitiesDirectlyTouch(const PathEndpointInfo &firstInfo, const PathEndpointInfo &secondInfo)
{
    return pointsNear(firstInfo.startPoint, secondInfo.startPoint)
           || pointsNear(firstInfo.startPoint, secondInfo.endPoint)
           || pointsNear(firstInfo.endPoint, secondInfo.startPoint)
           || pointsNear(firstInfo.endPoint, secondInfo.endPoint);
}

QList<int> sameFamilyNextEntities(const DxfDocument &document,
                                  const QList<DetectedBridge> &bridges,
                                  int currentEntityIndex,
                                  int previousEntityIndex,
                                  const QList<int> &blockedIndexes,
                                  double tolerance)
{
    QList<int> nextIndexes;
    if (currentEntityIndex < 0 || currentEntityIndex >= document.entities.size()) {
        return nextIndexes;
    }

    const PathEndpointInfo currentInfo = endpointInfoForEntity(document.entities.at(currentEntityIndex));
    if (!currentInfo.valid) {
        return nextIndexes;
    }

    for (int index = 0; index < document.entities.size(); ++index) {
        if (index == currentEntityIndex || index == previousEntityIndex || blockedIndexes.contains(index)) {
            continue;
        }

        const PathEndpointInfo candidateInfo = endpointInfoForEntity(document.entities.at(index));
        if (!candidateInfo.valid
            || !entitiesShareSelectableFamily(document, currentEntityIndex, index, tolerance)) {
            continue;
        }

        if (entitiesDirectlyTouch(currentInfo, candidateInfo)
            || findBridgeIndexForEntities(bridges, currentEntityIndex, index) >= 0) {
            nextIndexes.append(index);
        }
    }

    return nextIndexes;
}

QList<int> sameFamilyNeighbors(const DxfDocument &document,
                               const QList<DetectedBridge> &bridges,
                               int currentEntityIndex,
                               const QList<int> &allowedIndexes,
                               double tolerance)
{
    QList<int> neighbors;
    if (currentEntityIndex < 0 || currentEntityIndex >= document.entities.size()) {
        return neighbors;
    }

    const PathEndpointInfo currentInfo = endpointInfoForEntity(document.entities.at(currentEntityIndex));
    if (!currentInfo.valid) {
        return neighbors;
    }

    for (int candidateIndex : allowedIndexes) {
        if (candidateIndex == currentEntityIndex
            || candidateIndex < 0
            || candidateIndex >= document.entities.size()) {
            continue;
        }

        const PathEndpointInfo candidateInfo = endpointInfoForEntity(document.entities.at(candidateIndex));
        if (!candidateInfo.valid
            || !entitiesShareSelectableFamily(document, currentEntityIndex, candidateIndex, tolerance)) {
            continue;
        }

        if (entitiesDirectlyTouch(currentInfo, candidateInfo)
            || findBridgeIndexForEntities(bridges, currentEntityIndex, candidateIndex) >= 0) {
            neighbors.append(candidateIndex);
        }
    }

    return neighbors;
}

QList<int> buildCandidateChain(const DxfDocument &document,
                               const QList<DetectedBridge> &bridges,
                               int previousEntityIndex,
                               int seedEntityIndex,
                               const QList<int> &blockedIndexes,
                               double tolerance)
{
    QList<int> component;
    if (seedEntityIndex < 0 || seedEntityIndex >= document.entities.size()) {
        return component;
    }

    component.append(seedEntityIndex);
    for (int queueIndex = 0; queueIndex < component.size(); ++queueIndex) {
        const int currentEntityIndex = component.at(queueIndex);
        const QList<int> nextIndexes = sameFamilyNextEntities(document,
                                                              bridges,
                                                              currentEntityIndex,
                                                              -1,
                                                              mergeUniqueIndexes(blockedIndexes, component),
                                                              tolerance);
        for (int nextIndex : nextIndexes) {
            if (!component.contains(nextIndex)) {
                component.append(nextIndex);
            }
        }
    }

    if (component.size() <= 1) {
        return component;
    }

    int anchorEntityIndex = seedEntityIndex;
    if (previousEntityIndex >= 0 && previousEntityIndex < document.entities.size()) {
        const PathEndpointInfo previousInfo = endpointInfoForEntity(document.entities.at(previousEntityIndex));
        for (int candidateIndex : std::as_const(component)) {
            const PathEndpointInfo candidateInfo = endpointInfoForEntity(document.entities.at(candidateIndex));
            if (candidateInfo.valid
                && entitiesConnectedAtAnyEndpoint(bridges,
                                                  previousEntityIndex,
                                                  previousInfo,
                                                  candidateIndex,
                                                  candidateInfo)) {
                anchorEntityIndex = candidateIndex;
                break;
            }
        }
    }

    QList<int> endpoints;
    for (int candidateIndex : std::as_const(component)) {
        const QList<int> neighbors = sameFamilyNeighbors(document,
                                                         bridges,
                                                         candidateIndex,
                                                         component,
                                                         tolerance);
        if (neighbors.size() <= 1) {
            endpoints.append(candidateIndex);
        }
    }

    int chainStartIndex = anchorEntityIndex;
    if (!endpoints.contains(anchorEntityIndex) && !endpoints.isEmpty()) {
        double bestDistance = std::numeric_limits<double>::max();
        const QPointF anchorPoint = entityMidPointForOrdering(document.entities.at(anchorEntityIndex));
        for (int endpointIndex : std::as_const(endpoints)) {
            const double distance = QLineF(anchorPoint,
                                           entityMidPointForOrdering(document.entities.at(endpointIndex))).length();
            if (distance < bestDistance) {
                bestDistance = distance;
                chainStartIndex = endpointIndex;
            }
        }
    }

    QList<int> chain;
    chain.append(chainStartIndex);
    int previousChainIndex = -1;
    while (true) {
        const QList<int> neighbors = sameFamilyNeighbors(document,
                                                         bridges,
                                                         chain.last(),
                                                         component,
                                                         tolerance);
        QList<int> nextChoices;
        for (int neighbor : neighbors) {
            if (neighbor != previousChainIndex) {
                nextChoices.append(neighbor);
            }
        }

        if (nextChoices.size() != 1) {
            break;
        }

        previousChainIndex = chain.last();
        chain.append(nextChoices.first());
    }

    if (chain.size() != component.size()) {
        for (int candidateIndex : std::as_const(component)) {
            if (!chain.contains(candidateIndex)) {
                chain.append(candidateIndex);
            }
        }
    }

    return chain;
}

QList<int> candidateRuleProfileIndexes(const DxfDocument &document,
                                     const QStringList &pendingEntityIds,
                                     const QList<DetectedBridge> &bridges)
{
    QList<int> seedIndexes;
    if (pendingEntityIds.isEmpty()) {
        return seedIndexes;
    }

    QList<int> pendingIndexes = resolveRuleProfileIndexes(document, pendingEntityIds);
    if (pendingIndexes.isEmpty()) {
        return seedIndexes;
    }

    if (pendingEntityIds.size() == 1) {
        const int firstIndex = pendingIndexes.first();
        if (firstIndex < 0 || firstIndex >= document.entities.size()) {
            return seedIndexes;
        }

        const PathEndpointInfo firstInfo = endpointInfoForEntity(document.entities.at(firstIndex));
        if (!firstInfo.valid) {
            return seedIndexes;
        }

        for (int index = 0; index < document.entities.size(); ++index) {
            if (index == firstIndex || pendingIndexes.contains(index)) {
                continue;
            }

            const PathEndpointInfo candidateInfo = endpointInfoForEntity(document.entities.at(index));
            if (!candidateInfo.valid) {
                continue;
            }

            QPointF unusedOpenPoint;
            if (tryConnectToNextEntity(bridges, firstIndex, firstInfo, firstInfo.startPoint, index, candidateInfo, &unusedOpenPoint)
                || tryConnectToNextEntity(bridges, firstIndex, firstInfo, firstInfo.endPoint, index, candidateInfo, &unusedOpenPoint)) {
                seedIndexes.append(index);
            }
        }
    } else {
        const PendingRuleProfileState state = pendingRuleProfileState(document, pendingEntityIds, bridges);
        if (!state.valid || state.openEntityIndex < 0 || state.openEntityIndex >= document.entities.size()) {
            return seedIndexes;
        }

        const PathEndpointInfo openInfo = endpointInfoForEntity(document.entities.at(state.openEntityIndex));
        if (!openInfo.valid) {
            return seedIndexes;
        }

        for (int index = 0; index < document.entities.size(); ++index) {
            if (pendingIndexes.contains(index)) {
                continue;
            }

            const PathEndpointInfo candidateInfo = endpointInfoForEntity(document.entities.at(index));
            if (!candidateInfo.valid) {
                continue;
            }

            QPointF nextOpenPoint;
            if (tryConnectToNextEntity(bridges,
                                       state.openEntityIndex,
                                       openInfo,
                                       state.openPoint,
                                       index,
                                       candidateInfo,
                                       &nextOpenPoint)) {
                seedIndexes.append(index);
            }
        }
    }

    return expandBridgeCandidates(bridges, pendingIndexes, seedIndexes);
}

QList<QList<int>> candidateRuleProfileChains(const DxfDocument &document,
                                           const QStringList &pendingEntityIds,
                                           const QList<DetectedBridge> &bridges,
                                           double tolerance)
{
    QList<QList<int>> chains;
    const QList<int> pendingIndexes = resolveRuleProfileIndexes(document, pendingEntityIds);
    const QList<int> seedIndexes = candidateRuleProfileIndexes(document, pendingEntityIds, bridges);
    if (seedIndexes.isEmpty()) {
        return chains;
    }

    int previousEntityIndex = -1;
    if (pendingIndexes.size() >= 2) {
        previousEntityIndex = pendingIndexes.at(pendingIndexes.size() - 2);
    }

    for (int seedIndex : seedIndexes) {
        QList<int> chain = buildCandidateChain(document,
                                               bridges,
                                               previousEntityIndex,
                                               seedIndex,
                                               pendingIndexes,
                                               tolerance);
        if (chain.isEmpty()) {
            continue;
        }

        bool duplicate = false;
        for (const QList<int> &existing : std::as_const(chains)) {
            if (existing == chain) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            chains.append(chain);
        }
    }

    return chains;
}

PendingRuleProfileState pendingRuleProfileState(const DxfDocument &document,
                                            const QStringList &pendingEntityIds,
                                            const QList<DetectedBridge> &bridges)
{
    PendingRuleProfileState state;
    if (pendingEntityIds.isEmpty()) {
        return state;
    }

    QList<int> entityIndexes;
    QList<PathEndpointInfo> infos;
    infos.reserve(pendingEntityIds.size());
    for (const QString &entityId : pendingEntityIds) {
        int entityIndex = -1;
        for (int index = 0; index < document.entities.size(); ++index) {
            if (document.entities.at(index).id == entityId) {
                entityIndex = index;
                break;
            }
        }
        if (entityIndex < 0) {
            return state;
        }
        entityIndexes.append(entityIndex);

        const PathEndpointInfo info = endpointInfoForEntity(document.entities.at(entityIndex));
        if (!info.valid) {
            return state;
        }
        infos.append(info);
    }

    state.valid = true;
    if (infos.size() == 1) {
        return state;
    }

    const PathEndpointInfo &first = infos.at(0);
    const PathEndpointInfo &second = infos.at(1);
    QPointF currentOpenPoint;
    bool oriented = false;

    QPointF nextOpenPoint;
    if (tryConnectToNextEntity(bridges,
                               entityIndexes.at(0),
                               first,
                               first.endPoint,
                               entityIndexes.at(1),
                               second,
                               &nextOpenPoint)) {
        state.startPoint = first.startPoint;
        state.nextPoint = first.endPoint;
        currentOpenPoint = nextOpenPoint;
        oriented = true;
    } else if (tryConnectToNextEntity(bridges,
                                      entityIndexes.at(0),
                                      first,
                                      first.startPoint,
                                      entityIndexes.at(1),
                                      second,
                                      &nextOpenPoint)) {
        state.startPoint = first.endPoint;
        state.nextPoint = first.startPoint;
        currentOpenPoint = nextOpenPoint;
        oriented = true;
    }

    if (!oriented) {
        state.valid = false;
        return state;
    }

    state.hasDirection = true;
    for (int infoIndex = 2; infoIndex < infos.size(); ++infoIndex) {
        const PathEndpointInfo &info = infos.at(infoIndex);
        QPointF propagatedOpenPoint;
        if (!tryConnectToNextEntity(bridges,
                                    entityIndexes.at(infoIndex - 1),
                                    infos.at(infoIndex - 1),
                                    currentOpenPoint,
                                    entityIndexes.at(infoIndex),
                                    info,
                                    &propagatedOpenPoint)) {
            state.valid = false;
            return state;
        }
        currentOpenPoint = propagatedOpenPoint;
    }

    state.openPoint = currentOpenPoint;
    state.openEntityIndex = entityIndexes.last();
    return state;
}

QList<DirectedPendingEntityInfo> directedPendingEntityInfos(const DxfDocument &document,
                                                            const QStringList &pendingEntityIds,
                                                            const QList<DetectedBridge> &bridges)
{
    QList<DirectedPendingEntityInfo> result;
    if (pendingEntityIds.isEmpty()) {
        return result;
    }

    QList<int> entityIndexes;
    QList<PathEndpointInfo> infos;
    infos.reserve(pendingEntityIds.size());
    for (const QString &entityId : pendingEntityIds) {
        int entityIndex = -1;
        for (int index = 0; index < document.entities.size(); ++index) {
            if (document.entities.at(index).id == entityId) {
                entityIndex = index;
                break;
            }
        }
        if (entityIndex < 0) {
            return {};
        }

        const PathEndpointInfo info = endpointInfoForEntity(document.entities.at(entityIndex));
        if (!info.valid) {
            return {};
        }

        entityIndexes.append(entityIndex);
        infos.append(info);
    }

    if (entityIndexes.size() == 1) {
        result.append({entityIndexes.first(), infos.first().startPoint, infos.first().endPoint});
        return result;
    }

    QPointF nextOpenPoint;
    QPointF currentOpenPoint;
    if (tryConnectToNextEntity(bridges,
                               entityIndexes.at(0),
                               infos.at(0),
                               infos.at(0).endPoint,
                               entityIndexes.at(1),
                               infos.at(1),
                               &nextOpenPoint)) {
        result.append({entityIndexes.at(0), infos.at(0).startPoint, infos.at(0).endPoint});
        const QPointF secondStart = pointsNear(nextOpenPoint, infos.at(1).startPoint)
                                        ? infos.at(1).endPoint
                                        : infos.at(1).startPoint;
        result.append({entityIndexes.at(1), secondStart, nextOpenPoint});
        currentOpenPoint = nextOpenPoint;
    } else if (tryConnectToNextEntity(bridges,
                                      entityIndexes.at(0),
                                      infos.at(0),
                                      infos.at(0).startPoint,
                                      entityIndexes.at(1),
                                      infos.at(1),
                                      &nextOpenPoint)) {
        result.append({entityIndexes.at(0), infos.at(0).endPoint, infos.at(0).startPoint});
        const QPointF secondStart = pointsNear(nextOpenPoint, infos.at(1).startPoint)
                                        ? infos.at(1).endPoint
                                        : infos.at(1).startPoint;
        result.append({entityIndexes.at(1), secondStart, nextOpenPoint});
        currentOpenPoint = nextOpenPoint;
    } else {
        return {};
    }

    for (int infoIndex = 2; infoIndex < infos.size(); ++infoIndex) {
        QPointF propagatedOpenPoint;
        if (!tryConnectToNextEntity(bridges,
                                    entityIndexes.at(infoIndex - 1),
                                    infos.at(infoIndex - 1),
                                    currentOpenPoint,
                                    entityIndexes.at(infoIndex),
                                    infos.at(infoIndex),
                                    &propagatedOpenPoint)) {
            return {};
        }

        const QPointF pathStart = pointsNear(propagatedOpenPoint, infos.at(infoIndex).startPoint)
                                      ? infos.at(infoIndex).endPoint
                                      : infos.at(infoIndex).startPoint;
        result.append({entityIndexes.at(infoIndex), pathStart, propagatedOpenPoint});
        currentOpenPoint = propagatedOpenPoint;
    }

    return result;
}

BreakResult createLineBreak(const DxfDocument &sourceDocument,
                            int entityIndex,
                            const QPointF &scenePos)
{
    BreakResult result;
    result.document = sourceDocument;

    if (entityIndex < 0 || entityIndex >= sourceDocument.entities.size()) {
        result.errorMessage = QStringLiteral("Invalid entity for break.");
        return result;
    }

    DxfEntity &lineEntity = result.document.entities[entityIndex];
    if (lineEntity.type != DxfEntityType::Line || lineEntity.points.size() != 2) {
        result.errorMessage = QStringLiteral("Break currently supports only LINE entities.");
        return result;
    }

    const QPointF start = lineEntity.points.at(0);
    const QPointF end = lineEntity.points.at(1);
    const QPointF lineVector = end - start;
    const double lineLengthSquared = dotProduct(lineVector, lineVector);
    if (qFuzzyIsNull(lineLengthSquared)) {
        result.errorMessage = QStringLiteral("Cannot break a zero-length line.");
        return result;
    }

    const double t = dotProduct(scenePos - start, lineVector) / lineLengthSquared;
    if (t <= 0.001 || t >= 0.999) {
        result.errorMessage = QStringLiteral("Break point must be inside the line, not at an endpoint.");
        return result;
    }

    const QPointF breakPoint = start + lineVector * t;
    lineEntity.points[1] = breakPoint;

    DxfEntity secondLine = lineEntity;
    secondLine.id = nextBreakId(result.document);
    secondLine.points[0] = breakPoint;
    secondLine.points[1] = end;

    result.document.entities.insert(entityIndex + 1, secondLine);
    result.success = true;
    return result;
}

BreakResult createArcBreak(const DxfDocument &sourceDocument,
                           int entityIndex,
                           const QPointF &scenePos)
{
    BreakResult result;
    result.document = sourceDocument;

    if (entityIndex < 0 || entityIndex >= sourceDocument.entities.size()) {
        result.errorMessage = QStringLiteral("Invalid entity for break.");
        return result;
    }

    DxfEntity &arcEntity = result.document.entities[entityIndex];
    if (arcEntity.type != DxfEntityType::Arc || qFuzzyIsNull(arcEntity.radius)) {
        result.errorMessage = QStringLiteral("Break currently supports only LINE and ARC entities.");
        return result;
    }

    double breakAngleDeg = qRadiansToDegrees(std::atan2(scenePos.y() - arcEntity.center.y(),
                                                        scenePos.x() - arcEntity.center.x()));
    breakAngleDeg = normalizeAngle360(breakAngleDeg);

    const double sweepDeg = normalizedArcSweepDegrees(arcEntity.startAngleDeg, arcEntity.endAngleDeg);
    double breakSweepDeg = breakAngleDeg - arcEntity.startAngleDeg;
    while (breakSweepDeg < 0.0) {
        breakSweepDeg += 360.0;
    }

    if (breakSweepDeg <= 0.001 || breakSweepDeg >= sweepDeg - 0.001) {
        result.errorMessage = QStringLiteral("Break point must be inside the arc, not at an endpoint.");
        return result;
    }

    arcEntity.endAngleDeg = breakAngleDeg;

    DxfEntity secondArc = arcEntity;
    secondArc.id = nextBreakId(result.document);
    secondArc.startAngleDeg = breakAngleDeg;
    secondArc.endAngleDeg = sourceDocument.entities.at(entityIndex).endAngleDeg;

    result.document.entities.insert(entityIndex + 1, secondArc);
    result.success = true;
    return result;
}

BreakResult createBreakForEntity(const DxfDocument &sourceDocument,
                                 int entityIndex,
                                 const QPointF &scenePos)
{
    if (entityIndex < 0 || entityIndex >= sourceDocument.entities.size()) {
        BreakResult result;
        result.document = sourceDocument;
        result.errorMessage = QStringLiteral("Invalid entity for break.");
        return result;
    }

    const DxfEntity &entity = sourceDocument.entities.at(entityIndex);
    if (entity.type == DxfEntityType::Line) {
        return createLineBreak(sourceDocument, entityIndex, scenePos);
    }
    if (entity.type == DxfEntityType::Arc) {
        return createArcBreak(sourceDocument, entityIndex, scenePos);
    }

    BreakResult result;
    result.document = sourceDocument;
    result.errorMessage = QStringLiteral("Break currently supports only LINE and ARC entities.");
    return result;
}

FilletResult createLineFillet(const DxfDocument &sourceDocument,
                              int firstIndex,
                              int secondIndex,
                              double radius)
{
    FilletResult result;
    result.document = sourceDocument;

    if (firstIndex < 0 || secondIndex < 0
        || firstIndex >= sourceDocument.entities.size()
        || secondIndex >= sourceDocument.entities.size()
        || firstIndex == secondIndex) {
        result.errorMessage = QStringLiteral("Select exactly two different entities.");
        return result;
    }

    DxfEntity &firstLine = result.document.entities[firstIndex];
    DxfEntity &secondLine = result.document.entities[secondIndex];

    if (firstLine.type != DxfEntityType::Line || secondLine.type != DxfEntityType::Line
        || firstLine.points.size() != 2 || secondLine.points.size() != 2) {
        result.errorMessage = QStringLiteral("Fillet currently supports only two LINE entities.");
        return result;
    }

    QLineF firstInfinite(firstLine.points.at(0), firstLine.points.at(1));
    QLineF secondInfinite(secondLine.points.at(0), secondLine.points.at(1));
    QPointF intersectionPoint;
    if (firstInfinite.intersects(secondInfinite, &intersectionPoint) == QLineF::NoIntersection) {
        result.errorMessage = QStringLiteral("Selected lines do not intersect.");
        return result;
    }

    const int firstNearIndex = QLineF(firstLine.points.at(0), intersectionPoint).length()
                                   <= QLineF(firstLine.points.at(1), intersectionPoint).length()
                               ? 0
                               : 1;
    const int secondNearIndex = QLineF(secondLine.points.at(0), intersectionPoint).length()
                                    <= QLineF(secondLine.points.at(1), intersectionPoint).length()
                                ? 0
                                : 1;
    const int firstFarIndex = firstNearIndex == 0 ? 1 : 0;
    const int secondFarIndex = secondNearIndex == 0 ? 1 : 0;

    if (qFuzzyIsNull(radius)) {
        firstLine.points[firstNearIndex] = intersectionPoint;
        secondLine.points[secondNearIndex] = intersectionPoint;
        result.success = true;
        return result;
    }

    const QPointF firstDirection = normalizedVector(firstLine.points.at(firstFarIndex) - intersectionPoint);
    const QPointF secondDirection = normalizedVector(secondLine.points.at(secondFarIndex) - intersectionPoint);
    if (qFuzzyIsNull(std::hypot(firstDirection.x(), firstDirection.y()))
        || qFuzzyIsNull(std::hypot(secondDirection.x(), secondDirection.y()))) {
        result.errorMessage = QStringLiteral("Could not determine line directions for fillet.");
        return result;
    }

    const double rawDot = std::clamp(dotProduct(firstDirection, secondDirection), -1.0, 1.0);
    const double theta = std::acos(rawDot);
    if (theta <= qDegreesToRadians(1.0) || theta >= qDegreesToRadians(179.0)) {
        result.errorMessage = QStringLiteral("Selected lines do not form a usable corner for fillet.");
        return result;
    }

    const double trimDistance = radius / std::tan(theta * 0.5);
    const double firstAvailable = QLineF(intersectionPoint, firstLine.points.at(firstFarIndex)).length();
    const double secondAvailable = QLineF(intersectionPoint, secondLine.points.at(secondFarIndex)).length();
    if (trimDistance >= firstAvailable || trimDistance >= secondAvailable) {
        result.errorMessage = QStringLiteral("Fillet radius is too large for the selected line lengths.");
        return result;
    }

    const QPointF firstTangent = intersectionPoint + firstDirection * trimDistance;
    const QPointF secondTangent = intersectionPoint + secondDirection * trimDistance;
    const QPointF bisector = normalizedVector(firstDirection + secondDirection);
    if (qFuzzyIsNull(std::hypot(bisector.x(), bisector.y()))) {
        result.errorMessage = QStringLiteral("Could not compute fillet bisector.");
        return result;
    }

    const double centerDistance = radius / std::sin(theta * 0.5);
    const QPointF center = intersectionPoint + bisector * centerDistance;

    firstLine.points[firstNearIndex] = firstTangent;
    secondLine.points[secondNearIndex] = secondTangent;

    DxfEntity arc;
    arc.id = nextFilletId(result.document);
    arc.type = DxfEntityType::Arc;
    arc.layerName = firstLine.layerName;
    arc.color = firstLine.color;
    arc.center = center;
    arc.radius = radius;

    const double firstTangentAngle = qRadiansToDegrees(std::atan2(firstTangent.y() - center.y(),
                                                                  firstTangent.x() - center.x()));
    const double secondTangentAngle = qRadiansToDegrees(std::atan2(secondTangent.y() - center.y(),
                                                                   secondTangent.x() - center.x()));
    if (crossProduct(firstDirection, secondDirection) > 0.0) {
        arc.startAngleDeg = secondTangentAngle;
        arc.endAngleDeg = firstTangentAngle;
    } else {
        arc.startAngleDeg = firstTangentAngle;
        arc.endAngleDeg = secondTangentAngle;
    }

    result.document.entities.append(arc);
    result.success = true;
    return result;
}

bool documentsEqual(const DxfDocument &left, const DxfDocument &right)
{
    if (left.entities.size() != right.entities.size() || left.layers.size() != right.layers.size()) {
        return false;
    }

    for (int index = 0; index < left.entities.size(); ++index) {
        const DxfEntity &lhs = left.entities.at(index);
        const DxfEntity &rhs = right.entities.at(index);

        if (lhs.type != rhs.type
            || lhs.id != rhs.id
            || lhs.layerName != rhs.layerName
            || lhs.points != rhs.points
            || lhs.center != rhs.center
            || !qFuzzyCompare(lhs.radius + 1.0, rhs.radius + 1.0)
            || !qFuzzyCompare(lhs.startAngleDeg + 1.0, rhs.startAngleDeg + 1.0)
            || !qFuzzyCompare(lhs.endAngleDeg + 1.0, rhs.endAngleDeg + 1.0)) {
            return false;
        }
    }

    return true;
}

class GeometryEditCommand final : public QUndoCommand
{
public:
    GeometryEditCommand(MainWindow *window,
                        DxfDocument before,
                        DxfDocument after,
                        const QString &text)
        : QUndoCommand(text)
        , m_window(window)
        , m_before(std::move(before))
        , m_after(std::move(after))
    {
    }

    void undo() override;
    void redo() override;

private:
    MainWindow *m_window;
    DxfDocument m_before;
    DxfDocument m_after;
    bool m_firstRedo = true;
};

class ProjectDocumentEditCommand final : public QUndoCommand
{
public:
    ProjectDocumentEditCommand(MainWindow *window,
                               ProjectDocument before,
                               ProjectDocument after,
                               const QString &text)
        : QUndoCommand(text)
        , m_window(window)
        , m_before(std::move(before))
        , m_after(std::move(after))
    {
    }

    void undo() override;
    void redo() override;

private:
    MainWindow *m_window;
    ProjectDocument m_before;
    ProjectDocument m_after;
    bool m_firstRedo = true;
};

int ruleProfileIndexContainingEntityId(const ProjectDocument &projectDocument, const QString &entityId)
{
    if (entityId.isEmpty()) {
        return -1;
    }

    for (int index = 0; index < projectDocument.ruleProfiles.size(); ++index) {
        if (projectDocument.ruleProfiles.at(index).contains(entityId)) {
            return index;
        }
    }

    return -1;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_undoStack(new QUndoStack(this))
{
    ui->setupUi(this);
    loadAppSettings();
    setupWindow();
    loadDemoGeometry();
    populateToolTable();
    populateBendTables();
    updateBendParametersStatusUi();
    refreshBendParametersFileList();
    populateLayerTree();
    populateEntityProperties();
    updateProjectSummary();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_appSettings.mainWindowGeometry =
        QString::fromLatin1(saveGeometry().toBase64(QByteArray::Base64Encoding));
    m_appSettings.mainWindowState =
        QString::fromLatin1(saveState().toBase64(QByteArray::Base64Encoding));
    m_appSettings.layerTreeHeaderState =
        QString::fromLatin1(ui->layerTree->header()->saveState().toBase64(QByteArray::Base64Encoding));
    m_appSettings.entityPropertiesHeaderState =
        QString::fromLatin1(ui->entityPropertiesTree->header()->saveState().toBase64(QByteArray::Base64Encoding));
    m_appSettings.ruleProfileTreeHeaderState =
        QString::fromLatin1(ui->bladeLinePropertiesTree->header()->saveState().toBase64(QByteArray::Base64Encoding));
    saveAppSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::setupWindow()
{
    setWindowTitle(QStringLiteral("FoxBender"));
    statusBar()->showMessage(QStringLiteral("Project shell ready"));
    m_statusEntityLabel = new QLabel(this);
    m_statusPointerLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_statusEntityLabel, 1);
    statusBar()->addPermanentWidget(m_statusPointerLabel, 0);
    updateStatusBarDetails();

    ui->toolStationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->toolStationTable->verticalHeader()->setVisible(false);
    ui->toolStationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->toolStationTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->angleBendTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->angleBendTableWidget->verticalHeader()->setVisible(false);
    ui->angleBendTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->angleBendTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->arcBendTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->arcBendTableWidget->verticalHeader()->setVisible(false);
    ui->arcBendTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->arcBendTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->operationList->addItem(QStringLiteral("Operation list will be generated from RuleProfile data."));
    ui->logList->addItem(QStringLiteral("Log pane initialized."));
    ui->logList->addItem(QStringLiteral("Demo DXF model loaded into the view shell."));
    ui->layerTree->setHeaderLabels({QStringLiteral("Layer"), QStringLiteral("State")});
    ui->entityPropertiesTree->setHeaderLabels({QStringLiteral("Property"), QStringLiteral("Value")});
    ui->bladeLinePropertiesTree->setHeaderLabels({QStringLiteral("Property"), QStringLiteral("Value")});
    ui->bladeLinePropertiesTree->setEditTriggers(QAbstractItemView::DoubleClicked
                                                 | QAbstractItemView::EditKeyPressed
                                                 | QAbstractItemView::SelectedClicked);
    ui->simulationEventList->addItem(QStringLiteral("Simulation skeleton waiting for ToolAction data."));
    ui->menuEdit->addAction(m_undoStack->createUndoAction(this, QStringLiteral("Undo")));
    ui->menuEdit->addAction(m_undoStack->createRedoAction(this, QStringLiteral("Redo")));
    ui->actionStartNewBladeLine->setShortcut(QKeySequence());

    m_recentProjectsMenu = new QMenu(QStringLiteral("Previous projects"), this);
    ui->menuFile->insertMenu(ui->actionOpenDxf, m_recentProjectsMenu);
    connect(m_recentProjectsMenu, &QMenu::triggered, this, [this](QAction *action) {
        if (action != nullptr) {
            const QString filePath = action->data().toString();
            if (!filePath.isEmpty()) {
                openProjectFilePath(filePath);
            }
        }
    });

    m_recentDxfFilesMenu = new QMenu(QStringLiteral("Previous files"), this);
    ui->menuFile->insertMenu(ui->actionLastDxfDirectory, m_recentDxfFilesMenu);
    connect(m_recentDxfFilesMenu, &QMenu::triggered, this, [this](QAction *action) {
        if (action != nullptr) {
            const QString filePath = action->data().toString();
            if (!filePath.isEmpty()) {
                openDxfFilePath(filePath);
            }
        }
    });

    updateViewColorUi();
    updateFileMenuState();

    ui->menuView->addSeparator();
    m_dockingMenu = ui->menuView->addMenu(QStringLiteral("Docking"));
    m_dockingMenu->addAction(ui->layerDock->toggleViewAction());
    m_dockingMenu->addAction(ui->entityPropertiesDock->toggleViewAction());
    m_dockingMenu->addAction(ui->bladeLineDock->toggleViewAction());
    m_dockingMenu->addAction(ui->operationDock->toggleViewAction());
    m_dockingMenu->addAction(ui->logDock->toggleViewAction());
    m_dockingMenu->addSeparator();
    QAction *resetDockLayoutAction = m_dockingMenu->addAction(QStringLiteral("Reset Dock Layout"));
    connect(resetDockLayoutAction, &QAction::triggered, this, &MainWindow::resetDockLayout);

    m_defaultMainWindowState = saveState();

    if (!m_appSettings.mainWindowGeometry.isEmpty()) {
        restoreGeometry(QByteArray::fromBase64(m_appSettings.mainWindowGeometry.toLatin1(),
                                               QByteArray::Base64Encoding));
    }
    if (!m_appSettings.mainWindowState.isEmpty()) {
        restoreState(QByteArray::fromBase64(m_appSettings.mainWindowState.toLatin1(),
                                            QByteArray::Base64Encoding));
    }
    if (!m_appSettings.layerTreeHeaderState.isEmpty()) {
        ui->layerTree->header()->restoreState(
            QByteArray::fromBase64(m_appSettings.layerTreeHeaderState.toLatin1(),
                                   QByteArray::Base64Encoding));
    }
    if (!m_appSettings.entityPropertiesHeaderState.isEmpty()) {
        ui->entityPropertiesTree->header()->restoreState(
            QByteArray::fromBase64(m_appSettings.entityPropertiesHeaderState.toLatin1(),
                                   QByteArray::Base64Encoding));
    }
    if (!m_appSettings.ruleProfileTreeHeaderState.isEmpty()) {
        ui->bladeLinePropertiesTree->header()->restoreState(
            QByteArray::fromBase64(m_appSettings.ruleProfileTreeHeaderState.toLatin1(),
                                   QByteArray::Base64Encoding));
    }
    ui->mainTabWidget->setCurrentWidget(ui->dxfTab);

    auto *spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(spaceShortcut, &QShortcut::activated,
            this, &MainWindow::acceptHoveredCandidateChain);
    auto *returnShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(returnShortcut, &QShortcut::activated, this, [this] {
        if (m_toolMode == ToolMode::RuleProfileBuild && !m_pendingRuleProfileEntityIds.isEmpty()) {
            buildRuleProfile();
        }
    });
    auto *enterShortcut = new QShortcut(QKeySequence(Qt::Key_Enter), this);
    connect(enterShortcut, &QShortcut::activated, this, [this] {
        if (m_toolMode == ToolMode::RuleProfileBuild && !m_pendingRuleProfileEntityIds.isEmpty()) {
            buildRuleProfile();
        }
    });
    auto *escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escapeShortcut, &QShortcut::activated,
            this, &MainWindow::clearUiSelections);
    auto *deleteShortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(deleteShortcut, &QShortcut::activated,
            this, &MainWindow::deleteSelectedEntity);

    connect(ui->actionOpenProject, &QAction::triggered, this, &MainWindow::openProjectFile);
    connect(ui->actionSaveProject, &QAction::triggered, this, &MainWindow::saveProjectFile);
    connect(ui->actionSaveProjectAs, &QAction::triggered, this, &MainWindow::saveProjectFileAs);
    connect(ui->actionOpenDxf, &QAction::triggered, this, &MainWindow::openDxfFile);
    connect(ui->actionStartNewBladeLine, &QAction::triggered, this, &MainWindow::buildRuleProfile);
    connect(ui->actionAddSelectedToBladeLine, &QAction::triggered, this, &MainWindow::startRuleProfilePicking);
    connect(ui->actionRemoveLastFromBladeLine, &QAction::triggered, this, &MainWindow::removeLastFromRuleProfile);
    connect(ui->actionClearBladeLine, &QAction::triggered, this, &MainWindow::removeActiveRuleProfile);
    connect(ui->actionCreateBreak, &QAction::triggered, this, &MainWindow::createBreak);
    connect(ui->actionJoinLinesToIntersection, &QAction::triggered, this, &MainWindow::joinLinesToIntersection);
    connect(ui->actionFitView, &QAction::triggered, this, &MainWindow::fitDxfToView);
    connect(ui->actionCreateFillet, &QAction::triggered, this, &MainWindow::createFillet);
    connect(ui->dxfView, &DxfViewWidget::entityClicked,
            this, &MainWindow::handleEntityClicked);
    connect(ui->dxfView, &DxfViewWidget::entityHovered,
            this, &MainWindow::handleEntityHovered);
    connect(ui->dxfView, &DxfViewWidget::pointerMoved,
            this, &MainWindow::handlePointerMoved);
    connect(ui->dxfView, &DxfViewWidget::cancelRequested,
            this, &MainWindow::cancelActiveTool);
    connect(ui->dxfView, &DxfViewWidget::entitySelected,
            this, &MainWindow::handleEntitySelectionChanged);
    connect(ui->dxfView, &DxfViewWidget::documentEdited,
            this, &MainWindow::handleDocumentEdited);
    connect(ui->dxfView, &DxfViewWidget::documentEditCommitted,
            this, &MainWindow::handleDocumentEditCommitted);
    connect(ui->bladeLinePropertiesTree, &QTreeWidget::itemClicked,
            this, &MainWindow::handleRuleProfileItemClicked);
    connect(ui->bladeLinePropertiesTree, &QTreeWidget::itemChanged,
            this, &MainWindow::handleRuleProfileTreeItemChanged);
    connect(ui->layerTree, &QTreeWidget::itemChanged,
            this, &MainWindow::handleLayerTreeItemChanged);
    connect(ui->toolStationTable, &QTableWidget::cellChanged,
            this, &MainWindow::handleToolStationCellChanged);
    connect(ui->angleBendTableWidget, &QTableWidget::cellChanged,
            this, &MainWindow::handleAngleBendTableCellChanged);
    connect(ui->arcBendTableWidget, &QTableWidget::cellChanged,
            this, &MainWindow::handleArcBendTableCellChanged);
    connect(ui->addAngleBendRowButton, &QPushButton::clicked,
            this, &MainWindow::addAngleBendRow);
    connect(ui->addArcBendRowButton, &QPushButton::clicked,
            this, &MainWindow::addArcBendRow);
    connect(ui->newBendParametersButton, &QPushButton::clicked,
            this, &MainWindow::createNewBendParametersSet);
    connect(ui->openBendParametersButton, &QPushButton::clicked,
            this, &MainWindow::openBendParametersFile);
    connect(ui->loadBendParametersButton, &QPushButton::clicked,
            this, &MainWindow::loadSelectedBendParametersFile);
    connect(ui->saveBendParametersButton, &QPushButton::clicked,
            this, &MainWindow::saveBendParametersFile);
    connect(ui->saveBendParametersAsButton, &QPushButton::clicked,
            this, &MainWindow::saveBendParametersFileAs);
    connect(ui->bendParametersNameEdit, &QLineEdit::editingFinished,
            this, &MainWindow::handleBendParametersNameEditingFinished);
    connect(ui->bendParametersDescriptionEdit, &QLineEdit::editingFinished,
            this, &MainWindow::handleBendParametersDescriptionEditingFinished);
    connect(ui->bendParametersNotesEdit, &QPlainTextEdit::textChanged,
            this, &MainWindow::handleBendParametersNotesChanged);
    connect(ui->selectedEntityColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->selectedEntityColorButton, &m_appSettings.selectedEntityColor,
                    QStringLiteral("Choose Entity Selection Color"), QStringLiteral("Entity"));
    });
    connect(ui->segmentSelectionColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->segmentSelectionColorButton, &m_appSettings.segmentSelectionColor,
                    QStringLiteral("Choose Segment Selection Color"), QStringLiteral("Segment"));
    });
    connect(ui->hoverEntityColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->hoverEntityColorButton, &m_appSettings.hoverEntityColor,
                    QStringLiteral("Choose Hover Entity Color"), QStringLiteral("Hover"));
    });
    connect(ui->armedEntityColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->armedEntityColorButton, &m_appSettings.armedEntityColor,
                    QStringLiteral("Choose Armed Entity Color"), QStringLiteral("Armed"));
    });
    connect(ui->bladeLineColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->bladeLineColorButton, &m_appSettings.bladeLineColor,
                    QStringLiteral("Choose RuleProfile Color"), QStringLiteral("RuleProfile"));
    });
    connect(ui->activeBladeLineColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->activeBladeLineColorButton, &m_appSettings.activeBladeLineColor,
                    QStringLiteral("Choose Active RuleProfile Color"), QStringLiteral("Active"));
    });
    connect(ui->handleColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->handleColorButton, &m_appSettings.handleColor,
                    QStringLiteral("Choose Handle Color"), QStringLiteral("Handle"));
    });
    connect(ui->handleHoverColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->handleHoverColorButton, &m_appSettings.handleHoverColor,
                    QStringLiteral("Choose Handle Hover Stroke Color"), QStringLiteral("Handle Hover"));
    });
    connect(ui->handleHoverFillColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->handleHoverFillColorButton, &m_appSettings.handleHoverFillColor,
                    QStringLiteral("Choose Handle Hover Fill Color"), QStringLiteral("Handle Fill"));
    });
    connect(ui->snapPreviewColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->snapPreviewColorButton, &m_appSettings.snapPreviewColor,
                    QStringLiteral("Choose Snap Preview Color"), QStringLiteral("Snap"));
    });
    connect(ui->breakPreviewColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->breakPreviewColorButton, &m_appSettings.breakPreviewColor,
                    QStringLiteral("Choose Break Preview Color"), QStringLiteral("Break"));
    });
    connect(ui->viewBackgroundColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->viewBackgroundColorButton, &m_appSettings.viewBackgroundColor,
                    QStringLiteral("Choose View Background Color"), QStringLiteral("Background"));
    });
    connect(ui->startFlagColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->startFlagColorButton, &m_appSettings.startFlagColor,
                    QStringLiteral("Choose Start Flag Color"), QStringLiteral("Start Flag"));
    });
    connect(ui->endFlagColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->endFlagColorButton, &m_appSettings.endFlagColor,
                    QStringLiteral("Choose End Flag Color"), QStringLiteral("End Flag"));
    });
    connect(ui->bridgeColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->bridgeColorButton, &m_appSettings.bridgeColor,
                    tr("Choose Bridge Color"), tr("Bridge"));
    });
    connect(ui->fillFlagsCheckBox, &QCheckBox::toggled,
            this, &MainWindow::handleFlagFillChanged);
    connect(ui->flagScaleSlider, &QSlider::valueChanged,
            this, &MainWindow::handleFlagScaleChanged);
    connect(ui->handleScaleSlider, &QSlider::valueChanged,
            this, &MainWindow::handleHandleScaleChanged);
    connect(ui->portMinLengthSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double) { handleBridgeLengthChanged(); });
    connect(ui->portMaxLengthSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double) { handleBridgeLengthChanged(); });
}

void MainWindow::resetDockLayout()
{
    if (!m_defaultMainWindowState.isEmpty()) {
        restoreState(m_defaultMainWindowState);
    }

    ui->layerDock->show();
    ui->entityPropertiesDock->show();
    ui->bladeLineDock->show();
    ui->operationDock->show();
    ui->logDock->show();

    m_appSettings.mainWindowState =
        QString::fromLatin1(saveState().toBase64(QByteArray::Base64Encoding));
    m_appSettings.layerTreeHeaderState =
        QString::fromLatin1(ui->layerTree->header()->saveState().toBase64(QByteArray::Base64Encoding));
    m_appSettings.entityPropertiesHeaderState =
        QString::fromLatin1(ui->entityPropertiesTree->header()->saveState().toBase64(QByteArray::Base64Encoding));
    m_appSettings.ruleProfileTreeHeaderState =
        QString::fromLatin1(ui->bladeLinePropertiesTree->header()->saveState().toBase64(QByteArray::Base64Encoding));
    saveAppSettings();
    statusBar()->showMessage(QStringLiteral("Dock layout reset"), 3000);
}

void MainWindow::loadDemoGeometry()
{
    applyLoadedDocument(m_dxfReader.createDemoDocument(),
                        QStringLiteral("demo_bracket.dxf"),
                        QStringLiteral("Demo DXF model loaded into the view shell."));
}

void MainWindow::openDxfFile()
{
    QString initialPath;
    if (m_projectDocument.hasDxfFile()) {
        initialPath = m_projectDocument.dxfFilePath;
    } else if (!m_appSettings.lastDxfDirectory.isEmpty()) {
        initialPath = m_appSettings.lastDxfDirectory;
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open DXF File"),
        initialPath,
        QStringLiteral("DXF Files (*.dxf);;All Files (*)"));

    if (filePath.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("DXF open canceled"), 3000);
        return;
    }

    openDxfFilePath(filePath);
}

void MainWindow::openProjectFile()
{
    QString initialPath = m_projectDocument.projectFilePath;
    if (initialPath.isEmpty() && !m_appSettings.recentProjects.isEmpty()) {
        initialPath = m_appSettings.recentProjects.first();
    }
    if (initialPath.isEmpty() && !m_projectDocument.dxfFilePath.isEmpty()) {
        initialPath = QFileInfo(m_projectDocument.dxfFilePath).absolutePath();
    }

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open Project File"),
        initialPath,
        QStringLiteral("FoxBender Project (*.foxjob.json);;JSON Files (*.json);;All Files (*)"));

    if (filePath.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Project open canceled"), 3000);
        return;
    }

    openProjectFilePath(filePath);
}

void MainWindow::saveProjectFile()
{
    if (!m_projectDocument.projectFilePath.isEmpty()) {
        saveProjectFilePath(m_projectDocument.projectFilePath);
        return;
    }

    saveProjectFileAs();
}

void MainWindow::saveProjectFileAs()
{
    QString initialPath = m_projectDocument.projectFilePath;
    if (initialPath.isEmpty() && !m_projectDocument.dxfFilePath.isEmpty()) {
        const QFileInfo dxfInfo(m_projectDocument.dxfFilePath);
        initialPath = dxfInfo.absolutePath() + QStringLiteral("/")
                      + dxfInfo.completeBaseName() + QStringLiteral(".foxjob.json");
    } else if (initialPath.isEmpty() && !m_appSettings.recentProjects.isEmpty()) {
        initialPath = m_appSettings.recentProjects.first();
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Save Project File"),
        initialPath,
        QStringLiteral("FoxBender Project (*.foxjob.json);;JSON Files (*.json);;All Files (*)"));

    if (filePath.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Project save canceled"), 3000);
        return;
    }

    if (!filePath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
        filePath += QStringLiteral(".foxjob.json");
    }

    saveProjectFilePath(filePath);
}

void MainWindow::openDxfFilePath(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    const FoxDxfReader::Result result = m_dxfReader.loadFile(filePath);
    if (!result.success) {
        ui->logList->addItem(QStringLiteral("DXF load failed: %1").arg(result.errorMessage));
        QMessageBox::warning(this, QStringLiteral("DXF Load Failed"), result.errorMessage);
        statusBar()->showMessage(QStringLiteral("DXF load failed"), 5000);
        return;
    }

    DxfDocument document = result.document;
    const QStringList hiddenLayers = hiddenLayersFromSettings(m_appSettings);
    if (!hiddenLayers.isEmpty()) {
        for (LayerDefinition &layer : document.layers) {
            layer.visible = !hiddenLayers.contains(layer.name);
        }
    }

    applyLoadedDocument(document,
                        filePath,
                        QStringLiteral("Loaded DXF: %1").arg(filePath));
    m_appSettings.lastDxfDirectory = QFileInfo(filePath).absolutePath();
    m_appSettings.recentDxfFiles.removeAll(filePath);
    m_appSettings.recentDxfFiles.prepend(filePath);
    while (m_appSettings.recentDxfFiles.size() > MaxRecentFiles) {
        m_appSettings.recentDxfFiles.removeLast();
    }
    updateFileMenuState();
    saveAppSettings();
}

bool MainWindow::saveProjectFilePath(const QString &filePath)
{
    if (filePath.trimmed().isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Project file path is empty"), 4000);
        return false;
    }

    m_projectDocument.projectFilePath = filePath;
    m_projectDocument.geometrySnapshot = m_geometryModel.dxfDocument;
    m_projectDocument.hasGeometrySnapshot = !m_geometryModel.dxfDocument.isEmpty();

    QString errorMessage;
    if (!JsonProjectStore::save(filePath, m_projectDocument, &errorMessage)) {
        ui->logList->addItem(QStringLiteral("Project save failed: %1").arg(errorMessage));
        QMessageBox::warning(this, QStringLiteral("Project Save Failed"), errorMessage);
        statusBar()->showMessage(QStringLiteral("Project save failed"), 5000);
        return false;
    }

    m_appSettings.recentProjects.removeAll(filePath);
    m_appSettings.recentProjects.prepend(filePath);
    while (m_appSettings.recentProjects.size() > MaxRecentFiles) {
        m_appSettings.recentProjects.removeLast();
    }
    updateFileMenuState();
    saveAppSettings();
    updateProjectSummary();
    ui->logList->addItem(QStringLiteral("Project saved: %1").arg(filePath));
    statusBar()->showMessage(QStringLiteral("Project saved"), 3000);
    return true;
}

void MainWindow::openProjectFilePath(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return;
    }

    ProjectDocument loadedProject;
    QString errorMessage;
    if (!JsonProjectStore::load(filePath, &loadedProject, &errorMessage)) {
        ui->logList->addItem(QStringLiteral("Project load failed: %1").arg(errorMessage));
        QMessageBox::warning(this, QStringLiteral("Project Load Failed"), errorMessage);
        statusBar()->showMessage(QStringLiteral("Project load failed"), 5000);
        return;
    }

    if (!loadedProject.hasGeometrySnapshot && loadedProject.dxfFilePath.trimmed().isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("Project Load Failed"),
                             QStringLiteral("Project does not contain geometry or a DXF file path."));
        statusBar()->showMessage(QStringLiteral("Project load failed"), 5000);
        return;
    }

    DxfDocument document;
    if (loadedProject.hasGeometrySnapshot) {
        document = loadedProject.geometrySnapshot;
    } else {
        const FoxDxfReader::Result dxfResult = m_dxfReader.loadFile(loadedProject.dxfFilePath);
        if (!dxfResult.success) {
            ui->logList->addItem(QStringLiteral("Project DXF load failed: %1").arg(dxfResult.errorMessage));
            QMessageBox::warning(this, QStringLiteral("Project DXF Load Failed"), dxfResult.errorMessage);
            statusBar()->showMessage(QStringLiteral("Project DXF load failed"), 5000);
            return;
        }

        document = dxfResult.document;
        const QStringList hiddenLayers = hiddenLayersFromSettings(m_appSettings);
        if (!hiddenLayers.isEmpty()) {
            for (LayerDefinition &layer : document.layers) {
                layer.visible = !hiddenLayers.contains(layer.name);
            }
        }
    }

    applyGeometryDocument(document, true);
    m_projectDocument = loadedProject;
    m_projectDocument.projectFilePath = filePath;
    m_projectDocument.geometrySnapshot = m_geometryModel.dxfDocument;
    m_projectDocument.hasGeometrySnapshot = !m_geometryModel.dxfDocument.isEmpty();
    updateRuleProfileViewState();
    populateLayerTree();
    populateEntityProperties();
    updateProjectSummary();
    updateStatusBarDetails();

    m_appSettings.lastDxfDirectory = QFileInfo(loadedProject.dxfFilePath).absolutePath();
    m_appSettings.recentDxfFiles.removeAll(loadedProject.dxfFilePath);
    m_appSettings.recentDxfFiles.prepend(loadedProject.dxfFilePath);
    while (m_appSettings.recentDxfFiles.size() > MaxRecentFiles) {
        m_appSettings.recentDxfFiles.removeLast();
    }
    m_appSettings.recentProjects.removeAll(filePath);
    m_appSettings.recentProjects.prepend(filePath);
    while (m_appSettings.recentProjects.size() > MaxRecentFiles) {
        m_appSettings.recentProjects.removeLast();
    }
    updateFileMenuState();
    saveAppSettings();

    ui->logList->addItem(QStringLiteral("Loaded project: %1").arg(filePath));
    statusBar()->showMessage(QStringLiteral("Project loaded"), 5000);
}

void MainWindow::openRecentDxfAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == nullptr) {
        return;
    }

    const QString filePath = action->data().toString();
    if (!filePath.isEmpty()) {
        openDxfFilePath(filePath);
    }
}

void MainWindow::openRecentProjectAction()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == nullptr) {
        return;
    }

    const QString filePath = action->data().toString();
    if (!filePath.isEmpty()) {
        openProjectFilePath(filePath);
    }
}

void MainWindow::updateRuleProfileViewState()
{
    QList<int> allIndexes = resolveAllRuleProfileIndexes(m_geometryModel.dxfDocument, m_projectDocument.ruleProfiles);
    QList<int> activeIndexes;
    QList<int> candidateIndexes;
    QList<int> modifiedSegmentIndexes;
    QList<int> highlightedBridgeIndexes;
    PendingRuleProfileState pendingState;
    m_candidateChains.clear();

    auto segmentOptionsModified = [](const RuleProfileSegmentOptions &options) {
        return !qFuzzyCompare(options.angleCorrectionDeg + 1.0, 1.0)
               || !qFuzzyCompare(options.lengthCorrectionMm + 1.0, 1.0)
               || options.openMode != QStringLiteral("Normal");
    };

    for (int pathIndex = 0; pathIndex < m_projectDocument.ruleProfiles.size(); ++pathIndex) {
        const QStringList &ruleProfile = m_projectDocument.ruleProfiles.at(pathIndex);
        const QList<int> resolvedIndexes = resolveRuleProfileIndexes(m_geometryModel.dxfDocument, ruleProfile);
        int familyIndex = -1;
        int previousResolvedIndex = -1;
        QList<int> currentFamilyEntityIndexes;

        auto flushModifiedFamily = [&]() {
            if (familyIndex < 0 || currentFamilyEntityIndexes.isEmpty()
                || pathIndex < 0 || pathIndex >= m_projectDocument.ruleProfileSegmentOptions.size()
                || familyIndex >= m_projectDocument.ruleProfileSegmentOptions.at(pathIndex).size()) {
                currentFamilyEntityIndexes.clear();
                return;
            }

            if (segmentOptionsModified(m_projectDocument.ruleProfileSegmentOptions.at(pathIndex).at(familyIndex))) {
                modifiedSegmentIndexes = mergeUniqueIndexes(modifiedSegmentIndexes, currentFamilyEntityIndexes);
            }
            currentFamilyEntityIndexes.clear();
        };

        for (int resolvedIndex : resolvedIndexes) {
            const bool sameFamilyAsPrevious =
                !currentFamilyEntityIndexes.isEmpty()
                && previousResolvedIndex >= 0
                && entitiesShareSelectableFamily(m_geometryModel.dxfDocument,
                                                 previousResolvedIndex,
                                                 resolvedIndex,
                                                 qMax(0.05, m_appSettings.defaultToleranceMm));
            if (!sameFamilyAsPrevious) {
                flushModifiedFamily();
                ++familyIndex;
            }
            currentFamilyEntityIndexes.append(resolvedIndex);
            previousResolvedIndex = resolvedIndex;
        }
        flushModifiedFamily();
    }

    if (m_toolMode == ToolMode::RuleProfileBuild) {
        activeIndexes = resolveRuleProfileIndexes(m_geometryModel.dxfDocument, m_pendingRuleProfileEntityIds);
        pendingState = pendingRuleProfileState(m_geometryModel.dxfDocument,
                                             m_pendingRuleProfileEntityIds,
                                             m_geometryModel.detectedBridges);
        m_candidateChains = candidateRuleProfileChains(m_geometryModel.dxfDocument,
                                                     m_pendingRuleProfileEntityIds,
                                                     m_geometryModel.detectedBridges,
                                                     qMax(0.05, m_appSettings.defaultToleranceMm));
        for (const QList<int> &chain : std::as_const(m_candidateChains)) {
            candidateIndexes = mergeUniqueIndexes(candidateIndexes, chain);
            const QList<int> pendingIndexes = resolveRuleProfileIndexes(m_geometryModel.dxfDocument,
                                                                      m_pendingRuleProfileEntityIds);
            int previousIndex = pendingIndexes.isEmpty() ? -1 : pendingIndexes.last();
            for (int entityIndex : chain) {
                const int bridgeIndex = findBridgeIndexForEntities(m_geometryModel.detectedBridges,
                                                                   previousIndex,
                                                                   entityIndex);
                if (bridgeIndex >= 0 && !highlightedBridgeIndexes.contains(bridgeIndex)) {
                    highlightedBridgeIndexes.append(bridgeIndex);
                }
                previousIndex = entityIndex;
            }
        }
        allIndexes = mergeUniqueIndexes(allIndexes, activeIndexes);
        allIndexes = mergeUniqueIndexes(allIndexes, candidateIndexes);
    } else {
        const QStringList *ruleProfile = activeRuleProfile(&m_projectDocument);
        activeIndexes = ruleProfile == nullptr ? QList<int>{}
                                               : resolveRuleProfileIndexes(m_geometryModel.dxfDocument, *ruleProfile);
        if (ruleProfile != nullptr) {
            pendingState = pendingRuleProfileState(m_geometryModel.dxfDocument,
                                                   *ruleProfile,
                                                   m_geometryModel.detectedBridges);
        }
    }

    ui->dxfView->setRuleProfileEntityIndexes(allIndexes);
    ui->dxfView->setActiveRuleProfileEntityIndexes(activeIndexes);
    ui->dxfView->setCandidateEntityIndexes(candidateIndexes);
    ui->dxfView->setModifiedSegmentEntityIndexes(modifiedSegmentIndexes);
    ui->dxfView->setHighlightedBridgeIndexes(highlightedBridgeIndexes);
    if (m_toolMode != ToolMode::RuleProfileBuild) {
        ui->dxfView->setHoveredCandidateEntityIndexes({});
        ui->dxfView->setHoveredBridgeIndexes({});
    } else if (!candidateIndexes.contains(m_hoveredEntityIndex)) {
        ui->dxfView->setHoveredCandidateEntityIndexes({});
        ui->dxfView->setHoveredBridgeIndexes({});
    }
    ui->dxfView->setRuleProfileGuide(pendingState.startPoint,
                                     pendingState.nextPoint,
                                     pendingState.hasDirection,
                                     pendingState.openPoint,
                                     pendingState.hasDirection);
}

void MainWindow::startRuleProfilePicking()
{
    cancelActiveTool();
    m_toolMode = ToolMode::RuleProfileBuild;
    m_pendingRuleProfileEntityIds.clear();
    ui->dxfView->setArmedEntityIndex(-1);
    updateRuleProfileViewState();
    populateEntityProperties();
    updateProjectSummary();
    updateToolStatus();
}

void MainWindow::removeLastFromRuleProfile()
{
    if (m_toolMode == ToolMode::RuleProfileBuild) {
        if (m_pendingRuleProfileEntityIds.isEmpty()) {
            statusBar()->showMessage(QStringLiteral("Pending RuleProfile is already empty"), 3000);
            return;
        }

        const QString removedEntityId = m_pendingRuleProfileEntityIds.takeLast();
        updateRuleProfileViewState();
        populateEntityProperties();
        updateProjectSummary();
        ui->logList->addItem(QStringLiteral("Removed last pending entity %1 from RuleProfile build")
                                 .arg(removedEntityId));
        statusBar()->showMessage(QStringLiteral("Removed last pending RuleProfile entity"), 3000);
        return;
    }

    if (m_projectDocument.ruleProfiles.isEmpty()
        || m_projectDocument.activeRuleProfileIndex < 0
        || m_projectDocument.activeRuleProfileIndex >= m_projectDocument.ruleProfiles.size()) {
        statusBar()->showMessage(QStringLiteral("No active RuleProfile"), 3000);
        return;
    }

    QStringList &ruleProfile = m_projectDocument.ruleProfiles[m_projectDocument.activeRuleProfileIndex];
    if (ruleProfile.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Active RuleProfile is already empty"), 3000);
        return;
    }

    const QString removedEntityId = ruleProfile.takeLast();
    updateRuleProfileViewState();
    populateEntityProperties();
    updateProjectSummary();
    ui->logList->addItem(QStringLiteral("Removed last entity %1 from RuleProfile %2")
                             .arg(removedEntityId)
                             .arg(m_projectDocument.activeRuleProfileIndex + 1));
    statusBar()->showMessage(QStringLiteral("Removed last entity from active RuleProfile"), 3000);
}

void MainWindow::buildRuleProfile()
{
    if (m_toolMode != ToolMode::RuleProfileBuild) {
        statusBar()->showMessage(QStringLiteral("Press A to start RuleProfile picking first"), 3000);
        return;
    }

    if (m_pendingRuleProfileEntityIds.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("No pending RuleProfile entities to build"), 3000);
        return;
    }

    m_projectDocument.ruleProfiles.append(m_pendingRuleProfileEntityIds);
    m_projectDocument.ruleProfileOptions.append(RuleProfileOptions());
    m_projectDocument.ruleProfileSegmentOptions.append(QList<RuleProfileSegmentOptions>());
    m_projectDocument.activeRuleProfileIndex = m_projectDocument.ruleProfiles.size() - 1;
    const int ruleProfileNumber = m_projectDocument.activeRuleProfileIndex + 1;
    const int segmentCount = m_pendingRuleProfileEntityIds.size();
    m_pendingRuleProfileEntityIds.clear();
    m_toolMode = ToolMode::None;
    updateRuleProfileViewState();
    populateEntityProperties();
    updateProjectSummary();
    ui->logList->addItem(QStringLiteral("Built RuleProfile %1 with %2 entities")
                             .arg(ruleProfileNumber)
                             .arg(segmentCount));
    updateToolStatus();
}

void MainWindow::removeActiveRuleProfile()
{
    if (m_projectDocument.ruleProfiles.isEmpty()
        || m_projectDocument.activeRuleProfileIndex < 0
        || m_projectDocument.activeRuleProfileIndex >= m_projectDocument.ruleProfiles.size()) {
        statusBar()->showMessage(QStringLiteral("No RuleProfile to remove"), 3000);
        return;
    }

    const ProjectDocument beforeProject = m_projectDocument;
    ProjectDocument afterProject = m_projectDocument;
    const int removedIndex = afterProject.activeRuleProfileIndex;
    afterProject.ruleProfiles.removeAt(removedIndex);
    if (removedIndex >= 0 && removedIndex < afterProject.ruleProfileOptions.size()) {
        afterProject.ruleProfileOptions.removeAt(removedIndex);
    }
    if (removedIndex >= 0 && removedIndex < afterProject.ruleProfileSegmentOptions.size()) {
        afterProject.ruleProfileSegmentOptions.removeAt(removedIndex);
    }
    if (afterProject.ruleProfiles.isEmpty()) {
        afterProject.activeRuleProfileIndex = -1;
    } else if (removedIndex >= afterProject.ruleProfiles.size()) {
        afterProject.activeRuleProfileIndex = afterProject.ruleProfiles.size() - 1;
    } else {
        afterProject.activeRuleProfileIndex = removedIndex;
    }

    applyUndoRedoProjectDocument(afterProject);
    m_undoStack->push(new ProjectDocumentEditCommand(this,
                                                     beforeProject,
                                                     afterProject,
                                                     QStringLiteral("Remove RuleProfile")));
    ui->logList->addItem(QStringLiteral("Removed RuleProfile %1").arg(removedIndex + 1));
    statusBar()->showMessage(QStringLiteral("RuleProfile removed"), 3000);
}

void MainWindow::deleteSelectedEntity()
{
    QTreeWidgetItem *currentRuleProfileItem = ui->bladeLinePropertiesTree->currentItem();
    if (currentRuleProfileItem != nullptr) {
        const int pathIndex = currentRuleProfileItem->data(0, BladeTreePathIndexRole).toInt();
        if (pathIndex >= 0 && pathIndex < m_projectDocument.ruleProfiles.size()) {
            m_projectDocument.activeRuleProfileIndex = pathIndex;
            removeActiveRuleProfile();
            return;
        }
    }

    if (m_selectedEntityIndex < 0 || m_selectedEntityIndex >= m_geometryModel.dxfDocument.entities.size()) {
        statusBar()->showMessage(QStringLiteral("No entity selected"), 3000);
        return;
    }

    const DxfEntity &selectedEntity = m_geometryModel.dxfDocument.entities.at(m_selectedEntityIndex);

    if (m_pendingRuleProfileEntityIds.contains(selectedEntity.id)) {
        const int pendingCount = m_pendingRuleProfileEntityIds.size();
        m_pendingRuleProfileEntityIds.clear();
        if (m_toolMode == ToolMode::RuleProfileBuild) {
            m_toolMode = ToolMode::None;
        }
        updateRuleProfileViewState();
        populateEntityProperties();
        updateProjectSummary();
        updateToolStatus();
        ui->logList->addItem(QStringLiteral("Removed pending RuleProfile with %1 entities")
                                 .arg(pendingCount));
        statusBar()->showMessage(QStringLiteral("Pending RuleProfile removed"), 3000);
        return;
    }

    const int owningRuleProfileIndex = ruleProfileIndexContainingEntityId(m_projectDocument, selectedEntity.id);
    if (owningRuleProfileIndex >= 0) {
        m_projectDocument.activeRuleProfileIndex = owningRuleProfileIndex;
        removeActiveRuleProfile();
        return;
    }

    const DxfDocument beforeDocument = m_geometryModel.dxfDocument;
    DxfDocument afterDocument = beforeDocument;
    afterDocument.entities.removeAt(m_selectedEntityIndex);
    applyGeometryDocument(afterDocument, false);
    m_undoStack->push(new GeometryEditCommand(this,
                                              beforeDocument,
                                              afterDocument,
                                              QStringLiteral("Delete DXF entity")));
    ui->logList->addItem(QStringLiteral("Deleted entity %1").arg(selectedEntity.id));
    statusBar()->showMessage(QStringLiteral("Entity deleted"), 3000);
}

void MainWindow::createBreak()
{
    cancelActiveTool();
    m_toolMode = ToolMode::BreakPickEntity;
    m_pendingBreakEntity = -1;
    ui->dxfView->setArmedEntityIndex(-1);
    ui->dxfView->setBreakPreviewEnabled(false);
    updateToolStatus();
}

void MainWindow::createFillet()
{
    cancelActiveTool();
    bool accepted = false;
    const double radius = QInputDialog::getDouble(this,
                                                  QStringLiteral("Create Fillet"),
                                                  QStringLiteral("Fillet radius (mm)"),
                                                  2.0,
                                                  0.001,
                                                  100000.0,
                                                  3,
                                                  &accepted);
    if (!accepted) {
        return;
    }

    m_pendingFilletRadius = radius;
    m_pendingBreakEntity = -1;
    m_pendingFilletFirstEntity = -1;
    m_toolMode = ToolMode::FilletPickFirst;
    ui->dxfView->setArmedEntityIndex(-1);
    ui->dxfView->setBreakPreviewEnabled(false);
    updateToolStatus();
}

void MainWindow::joinLinesToIntersection()
{
    cancelActiveTool();
    m_pendingFilletRadius = 0.0;
    m_pendingBreakEntity = -1;
    m_pendingFilletFirstEntity = -1;
    m_toolMode = ToolMode::FilletPickFirst;
    ui->dxfView->setArmedEntityIndex(-1);
    ui->dxfView->setBreakPreviewEnabled(false);
    updateToolStatus();
}

void MainWindow::handleEntityClicked(int entityIndex, const QPointF &scenePos)
{
    if (entityIndex < 0 || entityIndex >= m_geometryModel.dxfDocument.entities.size()) {
        return;
    }

    if (m_toolMode == ToolMode::None) {
        return;
    }

    const DxfEntity &clickedEntity = m_geometryModel.dxfDocument.entities.at(entityIndex);

    if (m_toolMode == ToolMode::RuleProfileBuild) {
        if (clickedEntity.id.isEmpty()) {
            statusBar()->showMessage(QStringLiteral("Selected entity has no stable ID"), 3000);
            return;
        }

        if (!m_pendingRuleProfileEntityIds.isEmpty()) {
            const int existingIndex = m_pendingRuleProfileEntityIds.indexOf(clickedEntity.id);
            if (existingIndex >= 0) {
                const QList<DirectedPendingEntityInfo> directedInfos =
                    directedPendingEntityInfos(m_geometryModel.dxfDocument,
                                               m_pendingRuleProfileEntityIds,
                                               m_geometryModel.detectedBridges);
                if (existingIndex >= directedInfos.size()) {
                    statusBar()->showMessage(QStringLiteral("Could not resolve pending RuleProfile direction"), 3000);
                    return;
                }

                const DirectedPendingEntityInfo &directedInfo = directedInfos.at(existingIndex);
                const double startDistance = QLineF(scenePos, directedInfo.pathStartPoint).length();
                const double endDistance = QLineF(scenePos, directedInfo.pathEndPoint).length();
                const double endpointToleranceMm = qMax(1.0, m_appSettings.defaultToleranceMm * 8.0);
                const bool nearStart = startDistance <= endpointToleranceMm;
                const bool nearEnd = endDistance <= endpointToleranceMm;
                if (!nearStart && !nearEnd) {
                    statusBar()->showMessage(QStringLiteral("Click a RuleProfile endpoint to shorten it"), 3000);
                    return;
                }

                const int keepCount =
                    (nearStart && startDistance <= endDistance) ? existingIndex : existingIndex + 1;
                const int removedCount = m_pendingRuleProfileEntityIds.size() - keepCount;
                while (m_pendingRuleProfileEntityIds.size() > keepCount) {
                    m_pendingRuleProfileEntityIds.removeLast();
                }

                updateRuleProfileViewState();
                populateEntityProperties();
                updateProjectSummary();
                if (removedCount > 0) {
                    ui->logList->addItem(QStringLiteral("Trimmed pending RuleProfile back to %1")
                                             .arg(clickedEntity.id));
                    statusBar()->showMessage(QStringLiteral("Pending RuleProfile shortened by %1 entities")
                                                 .arg(removedCount),
                                             3000);
                } else {
                    statusBar()->showMessage(QStringLiteral("Pending RuleProfile already ends at %1")
                                                 .arg(clickedEntity.id),
                                             3000);
                }
                return;
            }

            if (!acceptCandidateChainByEntityIndex(entityIndex)) {
                statusBar()->showMessage(QStringLiteral("Pick a connected next entity for RuleProfile"), 4000);
            }
            return;
        }

        m_pendingRuleProfileEntityIds.append(clickedEntity.id);
        updateRuleProfileViewState();
        populateEntityProperties();
        updateProjectSummary();
        ui->logList->addItem(QStringLiteral("Picked %1 for pending RuleProfile")
                                 .arg(clickedEntity.id));
        statusBar()->showMessage(QStringLiteral("RuleProfile picking: %1 entities queued, right click or Enter to build")
                                     .arg(m_pendingRuleProfileEntityIds.size()),
                                 3000);
        return;
    }

    if (m_toolMode == ToolMode::BreakPickEntity) {
        if (clickedEntity.type != DxfEntityType::Line && clickedEntity.type != DxfEntityType::Arc) {
            statusBar()->showMessage(QStringLiteral("Break currently supports only LINE and ARC entities"), 4000);
            return;
        }

        m_pendingBreakEntity = entityIndex;
        m_toolMode = ToolMode::BreakPickPoint;
        ui->dxfView->setArmedEntityIndex(entityIndex);
        updateToolStatus();
        return;
    }

    if (m_toolMode == ToolMode::BreakPickPoint) {
        if (clickedEntity.type != DxfEntityType::Line && clickedEntity.type != DxfEntityType::Arc) {
            statusBar()->showMessage(QStringLiteral("Break currently supports only LINE and ARC entities"), 4000);
            return;
        }

        if (m_pendingBreakEntity >= 0 && entityIndex != m_pendingBreakEntity) {
            m_pendingBreakEntity = entityIndex;
            ui->dxfView->setArmedEntityIndex(entityIndex);
            statusBar()->showMessage(QStringLiteral("Break: selected a different LINE/ARC, now pick break point"), 4000);
            return;
        }

        const BreakResult result = createBreakForEntity(m_geometryModel.dxfDocument, entityIndex, scenePos);
        if (!result.success) {
            QMessageBox::warning(this, QStringLiteral("Break"), result.errorMessage);
            return;
        }

        m_undoStack->push(new GeometryEditCommand(this,
                                                  m_geometryModel.dxfDocument,
                                                  result.document,
                                                  QStringLiteral("Break line")));
        applyGeometryDocument(result.document, false);
        ui->logList->addItem(QStringLiteral("Created break in selected line"));
        cancelActiveTool();
        return;
    }

    if (clickedEntity.type != DxfEntityType::Line) {
        statusBar()->showMessage(QStringLiteral("Fillet currently supports only LINE entities"), 4000);
        return;
    }

    if (m_toolMode == ToolMode::FilletPickFirst) {
        m_pendingFilletFirstEntity = entityIndex;
        m_toolMode = ToolMode::FilletPickSecond;
        ui->dxfView->setArmedEntityIndex(entityIndex);
        updateToolStatus();
        return;
    }

    if (m_toolMode == ToolMode::FilletPickSecond) {
        if (entityIndex == m_pendingFilletFirstEntity) {
            statusBar()->showMessage(QStringLiteral("Pick a different second entity for fillet"), 4000);
            return;
        }

        const FilletResult result = createLineFillet(m_geometryModel.dxfDocument,
                                                     m_pendingFilletFirstEntity,
                                                     entityIndex,
                                                     m_pendingFilletRadius);
        if (!result.success) {
            QMessageBox::warning(this, QStringLiteral("Create Fillet"), result.errorMessage);
            cancelActiveTool();
            return;
        }

        m_undoStack->push(new GeometryEditCommand(this,
                                                  m_geometryModel.dxfDocument,
                                                  result.document,
                                                  qFuzzyIsNull(m_pendingFilletRadius)
                                                      ? QStringLiteral("Join lines to intersection")
                                                      : QStringLiteral("Create fillet")));
        applyGeometryDocument(result.document, false);
        if (qFuzzyIsNull(m_pendingFilletRadius)) {
            ui->logList->addItem(QStringLiteral("Joined selected lines to intersection"));
        } else {
            ui->logList->addItem(QStringLiteral("Created fillet with radius %1 mm")
                                     .arg(QString::number(m_pendingFilletRadius, 'f', 3)));
        }
        cancelActiveTool();
    }
}

void MainWindow::acceptHoveredCandidateChain()
{
    if (m_toolMode != ToolMode::RuleProfileBuild) {
        return;
    }

    int targetEntityIndex = -1;
    if (m_hoveredEntityIndex >= 0) {
        targetEntityIndex = m_hoveredEntityIndex;
    } else if (m_selectedEntityIndex >= 0) {
        targetEntityIndex = m_selectedEntityIndex;
    } else if (m_candidateChains.size() == 1 && !m_candidateChains.first().isEmpty()) {
        targetEntityIndex = m_candidateChains.first().first();
    }

    if (targetEntityIndex < 0) {
        statusBar()->showMessage(QStringLiteral("Hover or select a candidate segment first"), 3000);
        return;
    }

    if (m_pendingRuleProfileEntityIds.isEmpty()) {
        handleEntityClicked(targetEntityIndex, m_lastPointerScenePos);
        return;
    }

    if (!acceptCandidateChainByEntityIndex(targetEntityIndex)) {
        statusBar()->showMessage(QStringLiteral("Hover or select a candidate segment first"), 3000);
    }
}

bool MainWindow::acceptCandidateChainByEntityIndex(int entityIndex)
{
    QList<int> matchedChain;
    for (const QList<int> &chain : std::as_const(m_candidateChains)) {
        if (chain.contains(entityIndex)) {
            matchedChain = chain;
            break;
        }
    }

    if (matchedChain.isEmpty()) {
        return false;
    }

    for (int chainEntityIndex : std::as_const(matchedChain)) {
        const QString chainEntityId = m_geometryModel.dxfDocument.entities.at(chainEntityIndex).id;
        if (!m_pendingRuleProfileEntityIds.contains(chainEntityId)) {
            m_pendingRuleProfileEntityIds.append(chainEntityId);
        }
    }

    updateRuleProfileViewState();
    populateEntityProperties();
    updateProjectSummary();
    ui->logList->addItem(QStringLiteral("Picked candidate chain with %1 entities for pending RuleProfile")
                             .arg(matchedChain.size()));
    statusBar()->showMessage(QStringLiteral("RuleProfile picking: %1 entities queued, right click or Enter to build")
                                 .arg(m_pendingRuleProfileEntityIds.size()),
                             3000);
    return true;
}

void MainWindow::cancelActiveTool()
{
    if (m_toolMode == ToolMode::RuleProfileBuild && !m_pendingRuleProfileEntityIds.isEmpty()) {
        buildRuleProfile();
        return;
    }

    if (m_toolMode == ToolMode::None) {
        return;
    }

    m_toolMode = ToolMode::None;
    m_pendingBreakEntity = -1;
    m_pendingFilletRadius = 0.0;
    m_pendingFilletFirstEntity = -1;
    m_pendingRuleProfileEntityIds.clear();
    ui->dxfView->setArmedEntityIndex(-1);
    ui->dxfView->setBreakPreviewEnabled(false);
    ui->dxfView->setPendingTrimPreview({}, false);
    updateRuleProfileViewState();
    populateEntityProperties();
    updateProjectSummary();
    updateToolStatus();
}

void MainWindow::applyLoadedDocument(const DxfDocument &document,
                                     const QString &sourcePath,
                                     const QString &logMessage)
{
    m_projectDocument.projectFilePath.clear();
    m_projectDocument.ruleProfiles.clear();
    m_projectDocument.ruleProfileOptions.clear();
    m_projectDocument.ruleProfileSegmentOptions.clear();
    m_projectDocument.activeRuleProfileIndex = 0;
    applyGeometryDocument(document, true);
    m_projectDocument.dxfFilePath = sourcePath;
    m_projectDocument.geometrySnapshot = m_geometryModel.dxfDocument;
    m_projectDocument.hasGeometrySnapshot = !m_geometryModel.dxfDocument.isEmpty();
    ui->logList->addItem(logMessage);
    statusBar()->showMessage(logMessage, 5000);
}

void MainWindow::applyGeometryDocument(const DxfDocument &document, bool fitView)
{
    m_geometryModel.dxfDocument = document;
    m_projectDocument.geometrySnapshot = m_geometryModel.dxfDocument;
    m_projectDocument.hasGeometrySnapshot = !m_geometryModel.dxfDocument.isEmpty();
    recomputeDetectedBridges();
    if (!m_isApplyingUndoRedo) {
        ui->dxfView->setDocument(m_geometryModel.dxfDocument, fitView);
    } else {
        ui->dxfView->setDocument(m_geometryModel.dxfDocument, false);
    }
    ui->dxfView->setDetectedBridges(m_geometryModel.detectedBridges);
    updateRuleProfileViewState();
    populateLayerTree();
    populateEntityProperties();
    updateProjectSummary();
    updateStatusBarDetails();
}

void MainWindow::handleEntitySelectionChanged(int entityIndex)
{
    m_selectedEntityIndex = entityIndex;
    populateSelectedEntityPropertiesTree();
    updateStatusBarDetails();

    if (entityIndex >= 0 && entityIndex < m_geometryModel.dxfDocument.entities.size()) {
        const DxfEntity &entity = m_geometryModel.dxfDocument.entities.at(entityIndex);
        statusBar()->showMessage(
            QStringLiteral("Selected %1 on layer %2").arg(entity.typeName(), entity.layerName), 4000);
    }
}

void MainWindow::handleEntityHovered(int entityIndex)
{
    m_hoveredEntityIndex = entityIndex;
    QList<int> hoveredChain;
    QList<int> hoveredBridgeIndexes;
    if (m_toolMode == ToolMode::RuleProfileBuild && entityIndex >= 0) {
        for (const QList<int> &chain : std::as_const(m_candidateChains)) {
            if (!chain.contains(entityIndex)) {
                continue;
            }

            hoveredChain = chain;
            const QList<int> pendingIndexes = resolveRuleProfileIndexes(m_geometryModel.dxfDocument,
                                                                      m_pendingRuleProfileEntityIds);
            int previousIndex = pendingIndexes.isEmpty() ? -1 : pendingIndexes.last();
            for (int chainEntityIndex : chain) {
                const int bridgeIndex = findBridgeIndexForEntities(m_geometryModel.detectedBridges,
                                                                   previousIndex,
                                                                   chainEntityIndex);
                if (bridgeIndex >= 0 && !hoveredBridgeIndexes.contains(bridgeIndex)) {
                    hoveredBridgeIndexes.append(bridgeIndex);
                }
                previousIndex = chainEntityIndex;
            }
            break;
        }
    }

    ui->dxfView->setHoveredCandidateEntityIndexes(hoveredChain);
    ui->dxfView->setHoveredBridgeIndexes(hoveredBridgeIndexes);
    updatePendingTrimPreview();
    updateStatusBarDetails();
}

void MainWindow::handlePointerMoved(const QPointF &scenePos)
{
    m_lastPointerScenePos = scenePos;
    updatePendingTrimPreview();
    updateStatusBarDetails();
}

void MainWindow::handleRuleProfileItemClicked(QTreeWidgetItem *item, int column)
{
    if (item == nullptr) {
        return;
    }

    const QVariant optionKeyData = item->data(0, BladeTreeOptionKeyRole);
    if (optionKeyData.isValid()) {
        QTreeWidgetItem *pathItem = item->parent();
        while (pathItem != nullptr && pathItem->data(0, BladeTreeItemTypeRole).toInt() != BladeTreeItemTypePath) {
            pathItem = pathItem->parent();
        }
        if (pathItem != nullptr) {
            const int pathIndex = pathItem->data(0, BladeTreePathIndexRole).toInt();
            if (pathIndex >= 0 && pathIndex < m_projectDocument.ruleProfiles.size()
                && m_projectDocument.activeRuleProfileIndex != pathIndex) {
                m_projectDocument.activeRuleProfileIndex = pathIndex;
                updateRuleProfileViewState();
                updateProjectSummary();
            }
        }

        if (column == 1
            && optionKeyData.toString() != QLatin1String(RuleProfileOptionMirrorKey)
            && optionKeyData.toString() != QLatin1String(RuleProfileOptionStartLipKey)
            && optionKeyData.toString() != QLatin1String(RuleProfileOptionEndLipKey)) {
            ui->bladeLinePropertiesTree->editItem(item, 1);
        }
        return;
    }

    QTreeWidgetItem *typedItem = item;
    while (typedItem != nullptr && !typedItem->data(0, BladeTreeItemTypeRole).isValid()) {
        typedItem = typedItem->parent();
    }
    if (typedItem == nullptr) {
        return;
    }

    const int itemType = typedItem->data(0, BladeTreeItemTypeRole).toInt();
    const int pathIndex = typedItem->data(0, BladeTreePathIndexRole).toInt();
    if (itemType == BladeTreeItemTypePath) {
        if (pathIndex >= 0 && pathIndex < m_projectDocument.ruleProfiles.size()) {
            m_projectDocument.activeRuleProfileIndex = pathIndex;
            updateRuleProfileViewState();
            ui->dxfView->selectEntityIndex(-1);
            ui->dxfView->setTreeSelectionEntityIndexes({});
            ui->dxfView->setTreeSelectionBridgeIndexes({});
            populateEntityProperties();
            updateProjectSummary();
            ui->logList->addItem(QStringLiteral("Activated RuleProfile %1").arg(pathIndex + 1));
            statusBar()->showMessage(QStringLiteral("Activated RuleProfile %1").arg(pathIndex + 1), 3000);
        }
        return;
    }

    if (itemType == BladeTreeItemTypeFamily) {
        const QList<int> familyEntityIndexes = stringListToIntList(
            typedItem->data(0, BladeTreeEntityIdsRole).toStringList());
        const QList<int> familyBridgeIndexes = stringListToIntList(
            typedItem->data(0, BladeTreeBridgeIndexesRole).toStringList());
        ui->dxfView->selectEntityIndex(-1);
        ui->dxfView->setTreeSelectionEntityIndexes(familyEntityIndexes);
        ui->dxfView->setTreeSelectionBridgeIndexes(familyBridgeIndexes);
        statusBar()->showMessage(QStringLiteral("Selected %1")
                                     .arg(typedItem->text(0)),
                                 3000);
        return;
    }

    if (itemType == BladeTreeItemTypeBridge) {
        const int bridgeIndex = typedItem->data(0, BladeTreeBridgeIndexRole).toInt();
        ui->dxfView->selectEntityIndex(-1);
        ui->dxfView->setTreeSelectionEntityIndexes({});
        ui->dxfView->setTreeSelectionBridgeIndexes(bridgeIndex >= 0 ? QList<int>{bridgeIndex} : QList<int>{});
        statusBar()->showMessage(QStringLiteral("Selected %1").arg(typedItem->text(0)), 3000);
        return;
    }

    if (itemType == BladeTreeItemTypeSegment) {
        if (pathIndex >= 0 && pathIndex < m_projectDocument.ruleProfiles.size()
            && m_projectDocument.activeRuleProfileIndex != pathIndex) {
            m_projectDocument.activeRuleProfileIndex = pathIndex;
            updateRuleProfileViewState();
            populateEntityProperties();
            updateProjectSummary();
        }

        const int entityIndex = typedItem->data(0, BladeTreeEntityIndexRole).toInt();
        if (entityIndex >= 0 && entityIndex < m_geometryModel.dxfDocument.entities.size()) {
            ui->dxfView->selectEntityIndex(entityIndex);
            ui->dxfView->setTreeSelectionEntityIndexes({});
            ui->dxfView->setTreeSelectionBridgeIndexes({});
            statusBar()->showMessage(QStringLiteral("RuleProfile %1, segment %2 selected")
                                         .arg(pathIndex + 1)
                                         .arg(typedItem->text(0)),
                                     3000);
        }
    }
}

void MainWindow::handleRuleProfileTreeItemChanged(QTreeWidgetItem *item, int column)
{
    if (m_isPopulatingRuleProfileTree || item == nullptr || column != 1) {
        return;
    }

    const QVariant optionKeyData = item->data(0, BladeTreeOptionKeyRole);
    if (!optionKeyData.isValid()) {
        return;
    }

    QTreeWidgetItem *typedItem = item;
    while (typedItem != nullptr && !typedItem->data(0, BladeTreeItemTypeRole).isValid()) {
        typedItem = typedItem->parent();
    }
    if (typedItem == nullptr || typedItem->data(0, BladeTreeItemTypeRole).toInt() != BladeTreeItemTypePath) {
        return;
    }

    const int pathIndex = typedItem->data(0, BladeTreePathIndexRole).toInt();
    if (pathIndex < 0 || pathIndex >= m_projectDocument.ruleProfileOptions.size()) {
        return;
    }

    RuleProfileOptions &options = m_projectDocument.ruleProfileOptions[pathIndex];
    const QString optionKey = optionKeyData.toString();
    bool updated = false;
    bool valid = true;

    if (optionKey == QLatin1String(RuleProfileOptionCountKey)) {
        const QString text = item->text(1).trimmed();
        bool ok = false;
        const int value = text.toInt(&ok);
        valid = ok && value >= 1;
        if (valid) {
            options.count = value;
            item->setText(1, QString::number(options.count));
            updated = true;
        }
    } else if (optionKey == QLatin1String(RuleProfileOptionMirrorKey)) {
        options.mirror = item->checkState(1) == Qt::Checked;
        updated = true;
    } else if (optionKey == QLatin1String(RuleProfileOptionStartCorrectionKey)) {
        const QString text = item->text(1).trimmed();
        bool ok = false;
        const double value = text.toDouble(&ok);
        valid = ok;
        if (valid) {
            options.startCorrectionMm = value;
            item->setText(1, QString::number(options.startCorrectionMm, 'f', 2));
            updated = true;
        }
    } else if (optionKey == QLatin1String(RuleProfileOptionEndCorrectionKey)) {
        const QString text = item->text(1).trimmed();
        bool ok = false;
        const double value = text.toDouble(&ok);
        valid = ok;
        if (valid) {
            options.endCorrectionMm = value;
            item->setText(1, QString::number(options.endCorrectionMm, 'f', 2));
            updated = true;
        }
    } else if (optionKey == QLatin1String(RuleProfileOptionStartLipKey)) {
        options.startLip = item->checkState(1) == Qt::Checked;
        updated = true;
    } else if (optionKey == QLatin1String(RuleProfileOptionEndLipKey)) {
        options.endLip = item->checkState(1) == Qt::Checked;
        updated = true;
    }

    if (!valid) {
        m_isPopulatingRuleProfileTree = true;
        const RuleProfileOptions current = options;
        if (optionKey == QLatin1String(RuleProfileOptionCountKey)) {
            item->setText(1, QString::number(current.count));
        } else if (optionKey == QLatin1String(RuleProfileOptionStartCorrectionKey)) {
            item->setText(1, QString::number(current.startCorrectionMm, 'f', 2));
        } else if (optionKey == QLatin1String(RuleProfileOptionEndCorrectionKey)) {
            item->setText(1, QString::number(current.endCorrectionMm, 'f', 2));
        }
        m_isPopulatingRuleProfileTree = false;
        statusBar()->showMessage(QStringLiteral("Invalid RuleProfile option value"), 3000);
        return;
    }

    if (updated) {
        const RuleProfileOptions defaultOptions;
        bool highlight = false;
        if (optionKey == QLatin1String(RuleProfileOptionCountKey)) {
            highlight = options.count != defaultOptions.count;
        } else if (optionKey == QLatin1String(RuleProfileOptionMirrorKey)) {
            highlight = options.mirror != defaultOptions.mirror;
        } else if (optionKey == QLatin1String(RuleProfileOptionStartCorrectionKey)) {
            highlight = !qFuzzyCompare(options.startCorrectionMm + 1.0, defaultOptions.startCorrectionMm + 1.0);
        } else if (optionKey == QLatin1String(RuleProfileOptionEndCorrectionKey)) {
            highlight = !qFuzzyCompare(options.endCorrectionMm + 1.0, defaultOptions.endCorrectionMm + 1.0);
        } else if (optionKey == QLatin1String(RuleProfileOptionStartLipKey)) {
            highlight = options.startLip != defaultOptions.startLip;
        } else if (optionKey == QLatin1String(RuleProfileOptionEndLipKey)) {
            highlight = options.endLip != defaultOptions.endLip;
        }
        const QBrush changedBrush(highlight ? QColor(255, 247, 210) : Qt::transparent);
        item->setBackground(0, changedBrush);
        item->setBackground(1, changedBrush);
        updateProjectSummary();
        statusBar()->showMessage(QStringLiteral("RuleProfile option updated"), 2000);
    }
}

void MainWindow::clearUiSelections()
{
    if (m_toolMode != ToolMode::None) {
        cancelActiveTool();
    }

    ui->bladeLinePropertiesTree->clearSelection();
    ui->dxfView->selectEntityIndex(-1);
    ui->dxfView->setTreeSelectionEntityIndexes({});
    ui->dxfView->setTreeSelectionBridgeIndexes({});
    ui->dxfView->setHoveredCandidateEntityIndexes({});
    ui->dxfView->setHoveredBridgeIndexes({});
    ui->dxfView->setPendingTrimPreview({}, false);
    ui->dxfView->setArmedEntityIndex(-1);
    m_selectedEntityIndex = -1;
    m_hoveredEntityIndex = -1;
    populateEntityProperties();
    updateStatusBarDetails();
    statusBar()->showMessage(QStringLiteral("Selections cleared"), 2000);
}

void MainWindow::handleDocumentEdited()
{
    m_geometryModel.dxfDocument = ui->dxfView->document();
    populateEntityProperties();
    updateProjectSummary();
}

void MainWindow::handleDocumentEditCommitted(const DxfDocument &before,
                                             const DxfDocument &after,
                                             const QString &description)
{
    if (documentsEqual(before, after)) {
        return;
    }

    m_undoStack->push(new GeometryEditCommand(this, before, after, description));
    ui->logList->addItem(QStringLiteral("Geometry edited: %1").arg(description));
}

void MainWindow::fitDxfToView()
{
    ui->dxfView->fitAll();
    statusBar()->showMessage(QStringLiteral("DXF fit to view"), 3000);
}

void MainWindow::handleToolStationCellChanged(int row, int column)
{
    if (m_isPopulatingToolTable) {
        return;
    }

    if (row < 0 || column < 0 || column >= ui->toolStationTable->columnCount()) {
        return;
    }

    QTableWidgetItem *item = ui->toolStationTable->item(row, column);
    QTableWidgetItem *idSourceItem = ui->toolStationTable->item(row, 0);
    if (item == nullptr || idSourceItem == nullptr) {
        return;
    }

    const QString stationId = idSourceItem->data(ToolStationIdRole).toString();
    const int stationIndex = findToolStationIndexById(m_appSettings.toolStations, stationId);
    if (stationIndex < 0 || stationIndex >= m_appSettings.toolStations.size()) {
        statusBar()->showMessage(QStringLiteral("Tool station metadata mismatch"), 3000);
        return;
    }

    const QSignalBlocker blocker(ui->toolStationTable);
    ToolStation &station = m_appSettings.toolStations[stationIndex];
    bool ok = true;

    switch (column) {
    case 0:
        station.enabled = item->checkState() == Qt::Checked;
        break;
    case 1:
        station.name = item->text().trimmed();
        if (station.name.isEmpty()) {
            station.name = ToolStation::createDefaultStations().value(row).name;
            item->setText(station.name);
        }
        break;
    case 2: {
        const double value = item->text().toDouble(&ok);
        if (!ok) {
            item->setText(QString::number(station.centerOffsetMm, 'f', 1));
            statusBar()->showMessage(QStringLiteral("Invalid offset value"), 3000);
            return;
        }
        station.centerOffsetMm = value;
        item->setText(QString::number(station.centerOffsetMm, 'f', 1));
        break;
    }
    case 3: {
        const double value = item->text().toDouble(&ok);
        if (!ok) {
            item->setText(QString::number(station.widthMm, 'f', 1));
            statusBar()->showMessage(QStringLiteral("Invalid width value"), 3000);
            return;
        }
        station.widthMm = value;
        item->setText(QString::number(station.widthMm, 'f', 1));
        break;
    }
    case 4: {
        const int value = item->text().toInt(&ok);
        if (!ok) {
            item->setText(QString::number(station.actionTimeoutMs));
            statusBar()->showMessage(QStringLiteral("Invalid timeout value"), 3000);
            return;
        }
        station.actionTimeoutMs = value;
        item->setText(QString::number(station.actionTimeoutMs));
        break;
    }
    default:
        return;
    }

    saveAppSettings();
    statusBar()->showMessage(QStringLiteral("Tool settings saved"), 2000);
}

void MainWindow::handleAngleBendTableCellChanged(int row, int column)
{
    if (m_isPopulatingBendTables) {
        return;
    }

    if (row < 0 || row >= m_appSettings.angleBendTable.size()
        || column < 0 || column >= ui->angleBendTableWidget->columnCount()) {
        return;
    }

    QTableWidgetItem *item = ui->angleBendTableWidget->item(row, column);
    if (item == nullptr) {
        return;
    }

    AngleBendTableRow &tableRow = m_appSettings.angleBendTable[row];
    bool ok = false;
    const double value = item->text().toDouble(&ok);
    if (!ok) {
        populateBendTables();
        statusBar()->showMessage(QStringLiteral("Invalid angle bend table value"), 3000);
        return;
    }

    switch (column) {
    case 0:
        tableRow.angleDeg = value;
        sortAngleBendTable(&m_appSettings.angleBendTable);
        break;
    case 1:
        tableRow.left = value;
        break;
    case 2:
        tableRow.right = value;
        break;
    case 3:
        tableRow.startCorrection = value;
        break;
    case 4:
        tableRow.endCorrection = value;
        break;
    default:
        return;
    }

    saveAppSettings();
    populateBendTables();
    statusBar()->showMessage(QStringLiteral("Angle bend table saved"), 2000);
}

void MainWindow::handleArcBendTableCellChanged(int row, int column)
{
    if (m_isPopulatingBendTables) {
        return;
    }

    if (row < 0 || row >= m_appSettings.arcBendTable.size()
        || column < 0 || column >= ui->arcBendTableWidget->columnCount() - 1) {
        return;
    }

    QTableWidgetItem *item = ui->arcBendTableWidget->item(row, column);
    if (item == nullptr) {
        return;
    }

    ArcBendTableRow &tableRow = m_appSettings.arcBendTable[row];
    bool ok = false;

    if (column == 2) {
        const int segments = item->text().toInt(&ok);
        if (!ok || segments <= 0) {
            populateBendTables();
            statusBar()->showMessage(QStringLiteral("Invalid segments value"), 3000);
            return;
        }
        tableRow.segments = segments;
    } else {
        const double value = item->text().toDouble(&ok);
        if (!ok) {
            populateBendTables();
            statusBar()->showMessage(QStringLiteral("Invalid arc bend table value"), 3000);
            return;
        }

        switch (column) {
        case 0:
            tableRow.radiusMm = value;
            sortArcBendTable(&m_appSettings.arcBendTable);
            break;
        case 1:
            if (value <= 0.0) {
                populateBendTables();
                statusBar()->showMessage(QStringLiteral("Segment length must be greater than zero"), 3000);
                return;
            }
            tableRow.segmentLengthMm = value;
            break;
        case 3:
            return;
        case 4:
            tableRow.right90 = qRound(value);
            break;
        case 5:
            tableRow.right45 = qRound(value);
            break;
        case 6:
            tableRow.left90 = qRound(value);
            break;
        case 7:
            tableRow.left45 = qRound(value);
            break;
        case 8:
            tableRow.startCorrection = value;
            break;
        case 9:
            tableRow.endCorrection = value;
            break;
        case 10:
            tableRow.bridgeReduction = value;
            break;
        default:
            return;
        }
    }

    saveAppSettings();
    populateBendTables();
    statusBar()->showMessage(QStringLiteral("Arc bend table saved"), 2000);
}

QString MainWindow::defaultBendParametersFilePath() const
{
    return bendParametersFilePath();
}

QString MainWindow::bendParametersDirectoryPath() const
{
    const QString currentPath = m_currentBendParametersFilePath.trimmed();
    if (!currentPath.isEmpty()) {
        const QFileInfo currentInfo(currentPath);
        return currentInfo.absolutePath();
    }

    return QFileInfo(defaultBendParametersFilePath()).absolutePath();
}

QString MainWindow::sanitizeBendParametersName(const QString &name) const
{
    const QString trimmed = name.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("Default") : trimmed;
}

QString MainWindow::bendParametersFilePathForName(const QString &name) const
{
    const QString directoryPath = bendParametersDirectoryPath();
    const QString fileName = sanitizeBendParametersFileBase(name);
    return QDir(directoryPath).filePath(fileName);
}

bool MainWindow::saveCurrentBendParametersWithNamePrompt()
{
    m_appSettings.bendParametersName = sanitizeBendParametersName(m_appSettings.bendParametersName);

    const QString currentPath = m_currentBendParametersFilePath.trimmed().isEmpty()
                                    ? defaultBendParametersFilePath()
                                    : m_currentBendParametersFilePath;
    const QString targetPath = bendParametersFilePathForName(m_appSettings.bendParametersName);

    if (QFileInfo(currentPath).fileName() == QFileInfo(targetPath).fileName()) {
        return saveBendParametersFilePath(currentPath, true);
    }

    const QMessageBox::StandardButton answer =
        QMessageBox::question(this,
                              QStringLiteral("Save Bend Parameters As New"),
                              QStringLiteral("Name changed to \"%1\".\nSave bend parameters as a new file?\n\n%2")
                                  .arg(m_appSettings.bendParametersName, QFileInfo(targetPath).fileName()),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::Yes);

    if (answer == QMessageBox::Yes) {
        return saveBendParametersFilePath(targetPath, true);
    }

    return saveBendParametersFilePath(currentPath, true);
}

void MainWindow::loadAppSettings()
{
    AppSettings loadedSettings;
    QString errorMessage;
    const bool settingsLoaded = JsonSettingsStore::load(settingsFilePath(), &loadedSettings, &errorMessage);
    if (!settingsLoaded) {
        JsonSettingsStore::save(settingsFilePath(), m_appSettings, nullptr);
        loadedSettings = m_appSettings;
    }

    m_appSettings = loadedSettings;

    m_currentBendParametersFilePath =
        m_appSettings.lastBendParametersFilePath.trimmed().isEmpty()
            ? defaultBendParametersFilePath()
            : m_appSettings.lastBendParametersFilePath.trimmed();

    AppSettings bendSettings = m_appSettings;
    QString bendErrorMessage;
    if (!JsonBendParametersStore::load(m_currentBendParametersFilePath, &bendSettings, &bendErrorMessage)) {
        const QString legacyPath = QCoreApplication::applicationDirPath() + QStringLiteral("/bendparameters.json");
        if (m_currentBendParametersFilePath != legacyPath
            && JsonBendParametersStore::load(legacyPath, &bendSettings, nullptr)) {
            m_currentBendParametersFilePath = legacyPath;
            m_appSettings.lastBendParametersFilePath = legacyPath;
            m_appSettings.angleBendTable = bendSettings.angleBendTable;
            m_appSettings.arcBendTable = bendSettings.arcBendTable;
            m_appSettings.bendParametersName = bendSettings.bendParametersName;
            m_appSettings.bendParametersDescription = bendSettings.bendParametersDescription;
            m_appSettings.bendParametersNotes = bendSettings.bendParametersNotes;
        } else {
            JsonBendParametersStore::save(m_currentBendParametersFilePath, m_appSettings, nullptr);
        }
    } else {
        m_appSettings.angleBendTable = bendSettings.angleBendTable;
        m_appSettings.arcBendTable = bendSettings.arcBendTable;
        m_appSettings.bendParametersName = bendSettings.bendParametersName;
        m_appSettings.bendParametersDescription = bendSettings.bendParametersDescription;
        m_appSettings.bendParametersNotes = bendSettings.bendParametersNotes;
    }

    m_appSettings.bendParametersName = sanitizeBendParametersName(m_appSettings.bendParametersName);
}

bool MainWindow::saveAppSettings(bool logSuccess)
{
    m_appSettings.lastBendParametersFilePath = m_currentBendParametersFilePath;

    QString errorMessage;
    if (!JsonSettingsStore::save(settingsFilePath(), m_appSettings, &errorMessage)) {
        if (ui != nullptr && ui->logList != nullptr) {
            ui->logList->addItem(QStringLiteral("Settings save failed: %1").arg(errorMessage));
        }
        if (statusBar() != nullptr) {
            statusBar()->showMessage(QStringLiteral("Settings save failed"), 5000);
        }
        return false;
    }

    QString bendErrorMessage;
    if (!m_currentBendParametersFilePath.isEmpty()
        && !JsonBendParametersStore::save(m_currentBendParametersFilePath, m_appSettings, &bendErrorMessage)) {
        if (ui != nullptr && ui->logList != nullptr) {
            ui->logList->addItem(QStringLiteral("Bend parameters save failed: %1").arg(bendErrorMessage));
        }
        if (statusBar() != nullptr) {
            statusBar()->showMessage(QStringLiteral("Bend parameters save failed"), 5000);
        }
        return false;
    }

    if (logSuccess && ui != nullptr && ui->logList != nullptr) {
        ui->logList->addItem(QStringLiteral("Settings saved: %1").arg(settingsFilePath()));
        if (!m_currentBendParametersFilePath.isEmpty()) {
            ui->logList->addItem(QStringLiteral("Bend parameters saved: %1").arg(m_currentBendParametersFilePath));
        }
    }
    updateBendParametersStatusUi();
    refreshBendParametersFileList();
    return true;
}

bool MainWindow::loadBendParametersFilePath(const QString &filePath)
{
    if (filePath.trimmed().isEmpty()) {
        return false;
    }

    AppSettings bendSettings = m_appSettings;
    QString errorMessage;
    if (!JsonBendParametersStore::load(filePath, &bendSettings, &errorMessage)) {
        ui->logList->addItem(QStringLiteral("Bend parameters load failed: %1").arg(errorMessage));
        QMessageBox::warning(this, QStringLiteral("Bend Parameters Load Failed"), errorMessage);
        statusBar()->showMessage(QStringLiteral("Bend parameters load failed"), 5000);
        return false;
    }

    m_appSettings.angleBendTable = bendSettings.angleBendTable;
    m_appSettings.arcBendTable = bendSettings.arcBendTable;
    m_appSettings.bendParametersName = sanitizeBendParametersName(bendSettings.bendParametersName);
    m_appSettings.bendParametersDescription = bendSettings.bendParametersDescription;
    m_appSettings.bendParametersNotes = bendSettings.bendParametersNotes;
    m_currentBendParametersFilePath = filePath;
    updateBendParametersStatusUi();
    populateBendTables();
    saveAppSettings();
    ui->logList->addItem(QStringLiteral("Loaded bend parameters: %1").arg(filePath));
    statusBar()->showMessage(QStringLiteral("Bend parameters loaded"), 3000);
    return true;
}

bool MainWindow::saveBendParametersFilePath(const QString &filePath, bool updateCurrentPath)
{
    if (filePath.trimmed().isEmpty()) {
        return false;
    }

    m_appSettings.bendParametersName = sanitizeBendParametersName(m_appSettings.bendParametersName);

    QString errorMessage;
    if (!JsonBendParametersStore::save(filePath, m_appSettings, &errorMessage)) {
        ui->logList->addItem(QStringLiteral("Bend parameters save failed: %1").arg(errorMessage));
        QMessageBox::warning(this, QStringLiteral("Bend Parameters Save Failed"), errorMessage);
        statusBar()->showMessage(QStringLiteral("Bend parameters save failed"), 5000);
        return false;
    }

    if (updateCurrentPath) {
        m_currentBendParametersFilePath = filePath;
        updateBendParametersStatusUi();
        saveAppSettings();
    } else {
        updateBendParametersStatusUi();
    }

    ui->logList->addItem(QStringLiteral("Saved bend parameters: %1").arg(filePath));
    statusBar()->showMessage(QStringLiteral("Bend parameters saved"), 3000);
    return true;
}

void MainWindow::updateBendParametersStatusUi()
{
    if (ui == nullptr) {
        return;
    }

    const QString filePath = m_currentBendParametersFilePath.trimmed().isEmpty()
                                 ? defaultBendParametersFilePath()
                                 : m_currentBendParametersFilePath;
    const QFileInfo fileInfo(filePath);
    m_isUpdatingBendParametersUi = true;
    ui->bendParametersNameEdit->setText(sanitizeBendParametersName(m_appSettings.bendParametersName));
    ui->bendParametersDescriptionEdit->setText(m_appSettings.bendParametersDescription);
    ui->bendParametersNotesEdit->setPlainText(m_appSettings.bendParametersNotes);
    ui->bendParametersFileValue->setText(fileInfo.fileName().isEmpty()
                                             ? QStringLiteral("(unnamed)")
                                             : fileInfo.fileName());
    ui->bendParametersModifiedValue->setText(fileInfo.exists()
                                                 ? fileInfo.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                                                 : QStringLiteral("-"));
    m_isUpdatingBendParametersUi = false;
}

void MainWindow::refreshBendParametersFileList()
{
    if (ui == nullptr) {
        return;
    }

    const QSignalBlocker blocker(ui->bendParametersFileComboBox);
    ui->bendParametersFileComboBox->clear();

    const QDir directory(bendParametersDirectoryPath());
    const QFileInfoList entries =
        directory.entryInfoList({QStringLiteral("bend_*.json")}, QDir::Files, QDir::Name);

    int currentIndex = -1;
    for (const QFileInfo &entry : entries) {
        AppSettings metadata;
        QString displayName = entry.completeBaseName();
        if (JsonBendParametersStore::load(entry.absoluteFilePath(), &metadata, nullptr)) {
            displayName = sanitizeBendParametersName(metadata.bendParametersName);
        }

        ui->bendParametersFileComboBox->addItem(displayName, entry.absoluteFilePath());
        if (QFileInfo(m_currentBendParametersFilePath).absoluteFilePath() == entry.absoluteFilePath()) {
            currentIndex = ui->bendParametersFileComboBox->count() - 1;
        }
    }

    if (currentIndex < 0 && !m_currentBendParametersFilePath.trimmed().isEmpty()) {
        const QFileInfo currentInfo(m_currentBendParametersFilePath);
        ui->bendParametersFileComboBox->addItem(sanitizeBendParametersName(m_appSettings.bendParametersName),
                                                currentInfo.absoluteFilePath());
        currentIndex = ui->bendParametersFileComboBox->count() - 1;
    }

    if (currentIndex >= 0) {
        ui->bendParametersFileComboBox->setCurrentIndex(currentIndex);
    }
}

void MainWindow::openBendParametersFile()
{
    const QString initialPath = m_currentBendParametersFilePath.isEmpty()
                                    ? defaultBendParametersFilePath()
                                    : m_currentBendParametersFilePath;
    const QString filePath = QFileDialog::getOpenFileName(this,
                                                          QStringLiteral("Open Bend Parameters"),
                                                          initialPath,
                                                          QStringLiteral("Bend Parameters (*.json)"));
    if (filePath.isEmpty()) {
        return;
    }

    loadBendParametersFilePath(filePath);
}

void MainWindow::loadSelectedBendParametersFile()
{
    const QString filePath = ui->bendParametersFileComboBox->currentData().toString();
    if (filePath.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("No bend parameters file selected"), 3000);
        return;
    }

    loadBendParametersFilePath(filePath);
}

void MainWindow::saveBendParametersFile()
{
    saveCurrentBendParametersWithNamePrompt();
}

void MainWindow::saveBendParametersFileAs()
{
    const QString initialPath = bendParametersFilePathForName(m_appSettings.bendParametersName);
    const QString filePath = QFileDialog::getSaveFileName(this,
                                                          QStringLiteral("Save Bend Parameters As"),
                                                          initialPath,
                                                          QStringLiteral("Bend Parameters (*.json)"));
    if (filePath.isEmpty()) {
        return;
    }

    saveBendParametersFilePath(filePath, true);
}

void MainWindow::handleBendParametersNameEditingFinished()
{
    if (m_isUpdatingBendParametersUi) {
        return;
    }

    const QString newName = sanitizeBendParametersName(ui->bendParametersNameEdit->text());
    if (newName == m_appSettings.bendParametersName) {
        updateBendParametersStatusUi();
        return;
    }

    m_appSettings.bendParametersName = newName;
    saveCurrentBendParametersWithNamePrompt();
}

void MainWindow::handleBendParametersDescriptionEditingFinished()
{
    if (m_isUpdatingBendParametersUi) {
        return;
    }

    m_appSettings.bendParametersDescription = ui->bendParametersDescriptionEdit->text();
    saveAppSettings();
}

void MainWindow::handleBendParametersNotesChanged()
{
    if (m_isUpdatingBendParametersUi) {
        return;
    }

    m_appSettings.bendParametersNotes = ui->bendParametersNotesEdit->toPlainText();
    saveAppSettings();
}

void MainWindow::addAngleBendRow()
{
    bool ok = false;
    const double angleDeg = QInputDialog::getDouble(this,
                                                    QStringLiteral("Add Angle"),
                                                    QStringLiteral("Angle"),
                                                    0.0,
                                                    -100000.0,
                                                    100000.0,
                                                    2,
                                                    &ok);
    if (!ok) {
        return;
    }

    for (const AngleBendTableRow &existingRow : std::as_const(m_appSettings.angleBendTable)) {
        if (qAbs(existingRow.angleDeg - angleDeg) < 1e-9) {
            statusBar()->showMessage(QStringLiteral("Angle already exists"), 3000);
            return;
        }
    }

    AngleBendTableRow newRow;
    newRow.angleDeg = angleDeg;
    sortAngleBendTable(&m_appSettings.angleBendTable);
    int insertIndex = 0;
    while (insertIndex < m_appSettings.angleBendTable.size()
           && m_appSettings.angleBendTable.at(insertIndex).angleDeg < angleDeg) {
        ++insertIndex;
    }

    if (m_appSettings.angleBendTable.isEmpty()) {
        newRow.left = angleDeg;
        newRow.right = angleDeg;
    } else if (insertIndex <= 0) {
        const AngleBendTableRow &nearest = m_appSettings.angleBendTable.first();
        newRow.left = nearest.left;
        newRow.right = nearest.right;
        newRow.startCorrection = nearest.startCorrection;
        newRow.endCorrection = nearest.endCorrection;
    } else if (insertIndex >= m_appSettings.angleBendTable.size()) {
        const AngleBendTableRow &nearest = m_appSettings.angleBendTable.last();
        newRow.left = nearest.left;
        newRow.right = nearest.right;
        newRow.startCorrection = nearest.startCorrection;
        newRow.endCorrection = nearest.endCorrection;
    } else {
        const AngleBendTableRow &leftRow = m_appSettings.angleBendTable.at(insertIndex - 1);
        const AngleBendTableRow &rightRow = m_appSettings.angleBendTable.at(insertIndex);
        newRow.left = interpolateLinear(angleDeg, leftRow.angleDeg, leftRow.left, rightRow.angleDeg, rightRow.left);
        newRow.right = interpolateLinear(angleDeg, leftRow.angleDeg, leftRow.right, rightRow.angleDeg, rightRow.right);
        newRow.startCorrection = interpolateLinear(angleDeg,
                                                   leftRow.angleDeg,
                                                   leftRow.startCorrection,
                                                   rightRow.angleDeg,
                                                   rightRow.startCorrection);
        newRow.endCorrection = interpolateLinear(angleDeg,
                                                 leftRow.angleDeg,
                                                 leftRow.endCorrection,
                                                 rightRow.angleDeg,
                                                 rightRow.endCorrection);
    }

    m_appSettings.angleBendTable.append(newRow);
    sortAngleBendTable(&m_appSettings.angleBendTable);
    saveAppSettings();
    populateBendTables();
    statusBar()->showMessage(QStringLiteral("Angle bend row added"), 2000);
}

void MainWindow::addArcBendRow()
{
    bool ok = false;
    const double radiusMm = QInputDialog::getDouble(this,
                                                    QStringLiteral("Add Radius"),
                                                    QStringLiteral("Radius"),
                                                    0.0,
                                                    -100000.0,
                                                    100000.0,
                                                    2,
                                                    &ok);
    if (!ok) {
        return;
    }

    for (const ArcBendTableRow &existingRow : std::as_const(m_appSettings.arcBendTable)) {
        if (qAbs(existingRow.radiusMm - radiusMm) < 1e-9) {
            statusBar()->showMessage(QStringLiteral("Radius already exists"), 3000);
            return;
        }
    }

    ArcBendTableRow newRow;
    newRow.radiusMm = radiusMm;
    newRow.segmentLengthMm = bendArcSegmentLengthMm(radiusMm);
    sortArcBendTable(&m_appSettings.arcBendTable);
    int insertIndex = 0;
    while (insertIndex < m_appSettings.arcBendTable.size()
           && m_appSettings.arcBendTable.at(insertIndex).radiusMm < radiusMm) {
        ++insertIndex;
    }

    if (m_appSettings.arcBendTable.isEmpty()) {
        newRow = defaultArcBendRow(radiusMm);
    } else if (insertIndex <= 0) {
        newRow = m_appSettings.arcBendTable.first();
        newRow.radiusMm = radiusMm;
        newRow.segmentLengthMm = bendArcSegmentLengthMm(radiusMm);
    } else if (insertIndex >= m_appSettings.arcBendTable.size()) {
        newRow = m_appSettings.arcBendTable.last();
        newRow.radiusMm = radiusMm;
        newRow.segmentLengthMm = bendArcSegmentLengthMm(radiusMm);
    } else {
        const ArcBendTableRow &leftRow = m_appSettings.arcBendTable.at(insertIndex - 1);
        const ArcBendTableRow &rightRow = m_appSettings.arcBendTable.at(insertIndex);
        newRow.segmentLengthMm = interpolateLinear(radiusMm,
                                                   leftRow.radiusMm,
                                                   leftRow.segmentLengthMm,
                                                   rightRow.radiusMm,
                                                   rightRow.segmentLengthMm);
        newRow.segments = qRound(interpolateLinear(radiusMm,
                                                   leftRow.radiusMm,
                                                   leftRow.segments,
                                                   rightRow.radiusMm,
                                                   rightRow.segments));
        newRow.right90 = interpolateLinear(radiusMm, leftRow.radiusMm, leftRow.right90, rightRow.radiusMm, rightRow.right90);
        newRow.right45 = interpolateLinear(radiusMm, leftRow.radiusMm, leftRow.right45, rightRow.radiusMm, rightRow.right45);
        newRow.left90 = interpolateLinear(radiusMm, leftRow.radiusMm, leftRow.left90, rightRow.radiusMm, rightRow.left90);
        newRow.left45 = interpolateLinear(radiusMm, leftRow.radiusMm, leftRow.left45, rightRow.radiusMm, rightRow.left45);
        newRow.startCorrection = interpolateLinear(radiusMm,
                                                   leftRow.radiusMm,
                                                   leftRow.startCorrection,
                                                   rightRow.radiusMm,
                                                   rightRow.startCorrection);
        newRow.endCorrection = interpolateLinear(radiusMm,
                                                 leftRow.radiusMm,
                                                 leftRow.endCorrection,
                                                 rightRow.radiusMm,
                                                 rightRow.endCorrection);
        newRow.bridgeReduction = interpolateLinear(radiusMm,
                                                   leftRow.radiusMm,
                                                   leftRow.bridgeReduction,
                                                   rightRow.radiusMm,
                                                   rightRow.bridgeReduction);
    }

    m_appSettings.arcBendTable.append(newRow);
    sortArcBendTable(&m_appSettings.arcBendTable);
    saveAppSettings();
    populateBendTables();
    statusBar()->showMessage(QStringLiteral("Arc bend row added"), 2000);
}

void MainWindow::createNewBendParametersSet()
{
    bool ok = false;
    const QString suggestedName =
        sanitizeBendParametersName(ui->bendParametersNameEdit->text().trimmed().isEmpty()
                                       ? m_appSettings.bendParametersName
                                       : ui->bendParametersNameEdit->text());
    const QString name = QInputDialog::getText(this,
                                               QStringLiteral("New Bend Parameters"),
                                               QStringLiteral("Name"),
                                               QLineEdit::Normal,
                                               suggestedName,
                                               &ok).trimmed();
    if (!ok) {
        return;
    }

    const QString sanitizedName = sanitizeBendParametersName(name);
    m_appSettings.bendParametersName = sanitizedName;
    m_appSettings.bendParametersDescription.clear();
    m_appSettings.bendParametersNotes.clear();
    m_appSettings.angleBendTable = AppSettings::createDefaultAngleBendTable();
    m_appSettings.arcBendTable = AppSettings::createDefaultArcBendTable();
    const QString filePath = bendParametersFilePathForName(sanitizedName);
    saveBendParametersFilePath(filePath, true);
    populateBendTables();
    statusBar()->showMessage(QStringLiteral("New bend parameter set created"), 3000);
}

void MainWindow::handleFlagScaleChanged(int value)
{
    const double newScale = qBound(0.5, static_cast<double>(value) / 100.0, 3.0);
    if (qFuzzyCompare(m_appSettings.flagScale + 1.0, newScale + 1.0)) {
        updateFlagPreview();
        return;
    }

    m_appSettings.flagScale = newScale;
    updateViewColorUi();
    saveAppSettings();
}

void MainWindow::handleHandleScaleChanged(int value)
{
    const double newScale = qBound(0.5, static_cast<double>(value) / 100.0, 3.0);
    if (qFuzzyCompare(m_appSettings.handleScale + 1.0, newScale + 1.0)) {
        updateHandlePreview();
        return;
    }

    m_appSettings.handleScale = newScale;
    updateViewColorUi();
    saveAppSettings();
}

void MainWindow::handleFlagFillChanged(bool checked)
{
    if (m_appSettings.fillFlags == checked) {
        updateFlagPreview();
        return;
    }

    m_appSettings.fillFlags = checked;
    updateViewColorUi();
    saveAppSettings();
}

void MainWindow::handleBridgeLengthChanged()
{
    {
        const QSignalBlocker minBlocker(ui->portMinLengthSpinBox);
        const QSignalBlocker maxBlocker(ui->portMaxLengthSpinBox);
        if (ui->portMaxLengthSpinBox->value() < ui->portMinLengthSpinBox->value()) {
            ui->portMaxLengthSpinBox->setValue(ui->portMinLengthSpinBox->value());
        }
    }

    const double minValue = ui->portMinLengthSpinBox->value();
    const double maxValue = ui->portMaxLengthSpinBox->value();
    if (qFuzzyCompare(m_appSettings.bridgeMinLengthMm + 1.0, minValue + 1.0)
        && qFuzzyCompare(m_appSettings.bridgeMaxLengthMm + 1.0, maxValue + 1.0)) {
        return;
    }

    m_appSettings.bridgeMinLengthMm = minValue;
    m_appSettings.bridgeMaxLengthMm = maxValue;
    recomputeDetectedBridges();
    ui->dxfView->setDetectedBridges(m_geometryModel.detectedBridges);
    populateEntityProperties();
    updateProjectSummary();
    saveAppSettings();
}

void MainWindow::chooseColor(QPushButton *button,
                             QColor *targetColor,
                             const QString &title,
                             const QString &buttonText)
{
    if (button == nullptr || targetColor == nullptr) {
        return;
    }

    const QColor color = QColorDialog::getColor(*targetColor, this, title);
    if (!color.isValid()) {
        return;
    }

    *targetColor = color;
    updateColorButton(button, color, buttonText);
    updateViewColorUi();
    saveAppSettings();
    populateEntityProperties();
}

void MainWindow::updateStatusBarDetails()
{
    if (m_statusPointerLabel != nullptr) {
        m_statusPointerLabel->setText(
            QStringLiteral("X %1  Y %2")
                .arg(QString::number(m_lastPointerScenePos.x(), 'f', 3),
                     QString::number(m_lastPointerScenePos.y(), 'f', 3)));
    }

    if (m_statusEntityLabel == nullptr) {
        return;
    }

    int infoEntityIndex = m_selectedEntityIndex >= 0 ? m_selectedEntityIndex : m_hoveredEntityIndex;
    QString entityText = QStringLiteral("No entity");
    if (infoEntityIndex >= 0 && infoEntityIndex < m_geometryModel.dxfDocument.entities.size()) {
        const DxfEntity &entity = m_geometryModel.dxfDocument.entities.at(infoEntityIndex);
        const QString prefix = m_selectedEntityIndex >= 0 ? QStringLiteral("Selected") : QStringLiteral("Hover");
        entityText = QStringLiteral("%1: %2 on %3")
                         .arg(prefix, entity.typeName(), entity.layerName);
    }

    m_statusEntityLabel->setText(entityText);
}

void MainWindow::updateFileMenuState()
{
    if (m_recentProjectsMenu != nullptr) {
        m_recentProjectsMenu->clear();
        for (const QString &filePath : std::as_const(m_appSettings.recentProjects)) {
            const QFileInfo fileInfo(filePath);
            const QString actionText = fileInfo.fileName().isEmpty()
                                           ? filePath
                                           : QStringLiteral("%1  [%2]")
                                                 .arg(fileInfo.fileName(), fileInfo.absolutePath());
            QAction *action = m_recentProjectsMenu->addAction(actionText);
            action->setData(filePath);
            action->setToolTip(filePath);
        }
        m_recentProjectsMenu->setEnabled(!m_appSettings.recentProjects.isEmpty());
    }

    if (m_recentDxfFilesMenu != nullptr) {
        m_recentDxfFilesMenu->clear();
        for (const QString &filePath : std::as_const(m_appSettings.recentDxfFiles)) {
            const QFileInfo fileInfo(filePath);
            const QString actionText = fileInfo.fileName().isEmpty()
                                           ? filePath
                                           : QStringLiteral("%1  [%2]")
                                                 .arg(fileInfo.fileName(), fileInfo.absolutePath());
            QAction *action = m_recentDxfFilesMenu->addAction(actionText);
            action->setData(filePath);
            action->setToolTip(filePath);
        }
        m_recentDxfFilesMenu->setEnabled(!m_appSettings.recentDxfFiles.isEmpty());
    }

    if (ui->actionLastDxfDirectory != nullptr) {
        if (m_appSettings.lastDxfDirectory.isEmpty()) {
            ui->actionLastDxfDirectory->setText(QStringLiteral("Last DXF directory: (none)"));
        } else {
            ui->actionLastDxfDirectory->setText(QStringLiteral("Last DXF directory: %1")
                                                    .arg(m_appSettings.lastDxfDirectory));
        }
    }

    if (ui->actionSaveProject != nullptr) {
        ui->actionSaveProject->setEnabled(!m_projectDocument.dxfFilePath.trimmed().isEmpty());
    }
}

void MainWindow::updateColorButton(QPushButton *button, const QColor &color, const QString &fallbackText)
{
    if (button == nullptr) {
        return;
    }

    QPixmap swatch(34, 20);
    swatch.fill(Qt::transparent);
    {
        QPainter painter(&swatch);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(235, 235, 235), 1.0));
        painter.setBrush(color);
        painter.drawRect(0, 0, swatch.width() - 1, swatch.height() - 1);
    }

    button->setIcon(QIcon(swatch));
    button->setIconSize(swatch.size());
    button->setText(QString());
    button->setMinimumSize(44, 26);
    button->setToolTip(QStringLiteral("%1: %2")
                           .arg(fallbackText, color.name(QColor::HexRgb)));
    button->setStyleSheet(QString());
}

void MainWindow::updateViewColorUi()
{
    updateColorButton(ui->selectedEntityColorButton, m_appSettings.selectedEntityColor, QStringLiteral("Entity"));
    updateColorButton(ui->segmentSelectionColorButton, m_appSettings.segmentSelectionColor, QStringLiteral("Segment"));
    updateColorButton(ui->hoverEntityColorButton, m_appSettings.hoverEntityColor, QStringLiteral("Hover"));
    updateColorButton(ui->armedEntityColorButton, m_appSettings.armedEntityColor, QStringLiteral("Armed"));
    updateColorButton(ui->bladeLineColorButton, m_appSettings.bladeLineColor, QStringLiteral("RuleProfile"));
    updateColorButton(ui->activeBladeLineColorButton, m_appSettings.activeBladeLineColor, QStringLiteral("Active"));
    updateColorButton(ui->handleColorButton, m_appSettings.handleColor, QStringLiteral("Handle"));
    updateColorButton(ui->handleHoverColorButton, m_appSettings.handleHoverColor, QStringLiteral("Handle Hover"));
    updateColorButton(ui->handleHoverFillColorButton, m_appSettings.handleHoverFillColor, QStringLiteral("Handle Fill"));
    updateColorButton(ui->snapPreviewColorButton, m_appSettings.snapPreviewColor, QStringLiteral("Snap"));
    updateColorButton(ui->breakPreviewColorButton, m_appSettings.breakPreviewColor, QStringLiteral("Break"));
    updateColorButton(ui->viewBackgroundColorButton, m_appSettings.viewBackgroundColor, QStringLiteral("Background"));
    updateColorButton(ui->startFlagColorButton, m_appSettings.startFlagColor, QStringLiteral("Start Flag"));
    updateColorButton(ui->endFlagColorButton, m_appSettings.endFlagColor, QStringLiteral("End Flag"));
    updateColorButton(ui->bridgeColorButton, m_appSettings.bridgeColor, tr("Bridge"));

    {
        const QSignalBlocker fillBlocker(ui->fillFlagsCheckBox);
        ui->fillFlagsCheckBox->setChecked(m_appSettings.fillFlags);
    }
    {
        const QSignalBlocker sliderBlocker(ui->flagScaleSlider);
        ui->flagScaleSlider->setValue(qRound(m_appSettings.flagScale * 100.0));
    }
    {
        const QSignalBlocker sliderBlocker(ui->handleScaleSlider);
        ui->handleScaleSlider->setValue(qRound(m_appSettings.handleScale * 100.0));
    }
    {
        const QSignalBlocker minBlocker(ui->portMinLengthSpinBox);
        ui->portMinLengthSpinBox->setValue(m_appSettings.bridgeMinLengthMm);
    }
    {
        const QSignalBlocker maxBlocker(ui->portMaxLengthSpinBox);
        ui->portMaxLengthSpinBox->setValue(m_appSettings.bridgeMaxLengthMm);
    }
    ui->flagScaleValueLabel->setText(QStringLiteral("%1 %")
                                         .arg(QString::number(qRound(m_appSettings.flagScale * 100.0))));
    ui->handleScaleValueLabel->setText(QStringLiteral("%1 %")
                                           .arg(QString::number(qRound(m_appSettings.handleScale * 100.0))));

    DxfViewWidget::ViewColors colors;
    colors.backgroundColor = m_appSettings.viewBackgroundColor;
    colors.selectedEntityColor = m_appSettings.selectedEntityColor;
    colors.segmentSelectionColor = m_appSettings.segmentSelectionColor;
    colors.hoverEntityColor = m_appSettings.hoverEntityColor;
    colors.armedEntityColor = m_appSettings.armedEntityColor;
    colors.ruleProfileColor = m_appSettings.bladeLineColor;
    colors.activeRuleProfileColor = m_appSettings.activeBladeLineColor;
    colors.handleColor = m_appSettings.handleColor;
    colors.handleHoverColor = m_appSettings.handleHoverColor;
    colors.handleHoverFillColor = m_appSettings.handleHoverFillColor;
    colors.snapPreviewColor = m_appSettings.snapPreviewColor;
    colors.breakPreviewColor = m_appSettings.breakPreviewColor;
    colors.startFlagColor = m_appSettings.startFlagColor;
    colors.endFlagColor = m_appSettings.endFlagColor;
    colors.bridgeColor = m_appSettings.bridgeColor;
    colors.fillFlags = m_appSettings.fillFlags;
    colors.flagScale = m_appSettings.flagScale;
    colors.handleScale = m_appSettings.handleScale;
    ui->dxfView->setViewColors(colors);
    ui->dxfView->setDetectedBridges(m_geometryModel.detectedBridges);
    updateFlagPreview();
    updateHandlePreview();
}

void MainWindow::updateFlagPreview()
{
    if (ui->flagPreviewLabel == nullptr) {
        return;
    }

    const QSize previewSize = ui->flagPreviewLabel->size().expandedTo(QSize(220, 120));
    QPixmap pixmap(previewSize);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(210, 210, 210), 1.0));
    painter.drawRect(pixmap.rect().adjusted(0, 0, -1, -1));

    const QPointF baselineLeft(42.0, previewSize.height() * 0.68);
    const QPointF baselineRight(previewSize.width() - 42.0, previewSize.height() * 0.42);
    painter.setPen(QPen(QColor(180, 180, 180), 1.0, Qt::DashLine));
    painter.drawLine(baselineLeft, baselineRight);

    drawPreviewFlag(painter,
                    QPointF(58.0, previewSize.height() * 0.68),
                    QPointF(1.0, -0.25),
                    m_appSettings.startFlagColor,
                    m_appSettings.fillFlags,
                    m_appSettings.flagScale,
                    false);
    drawPreviewFlag(painter,
                    QPointF(previewSize.width() - 58.0, previewSize.height() * 0.42),
                    QPointF(1.0, -0.25),
                    m_appSettings.endFlagColor,
                    m_appSettings.fillFlags,
                    m_appSettings.flagScale,
                    true);

    ui->flagPreviewLabel->setPixmap(pixmap);
}

void MainWindow::updateHandlePreview()
{
    if (ui->handlePreviewLabel == nullptr) {
        return;
    }

    const QSize previewSize = ui->handlePreviewLabel->size().expandedTo(QSize(220, 120));
    QPixmap pixmap(previewSize);
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(210, 210, 210), 1.0));
    painter.drawRect(pixmap.rect().adjusted(0, 0, -1, -1));

    const QPointF guideLeft(32.0, previewSize.height() * 0.72);
    const QPointF guideRight(previewSize.width() - 32.0, previewSize.height() * 0.32);
    painter.setPen(QPen(QColor(180, 180, 180), 1.0, Qt::DashLine));
    painter.drawLine(guideLeft, guideRight);

    const qreal handleScale = std::clamp(m_appSettings.handleScale, 0.5, 3.0);
    const qreal pointHalfSize = 2.5 * handleScale;
    const qreal midRadius = 3.0 * handleScale;
    const QPointF startPoint(58.0, previewSize.height() * 0.68);
    const QPointF endPoint(previewSize.width() - 58.0, previewSize.height() * 0.36);
    const QPointF midPoint = (startPoint + endPoint) * 0.5;

    QPen linePen(QColor(120, 120, 120), 1.0);
    linePen.setCosmetic(true);
    painter.setPen(linePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawLine(startPoint, endPoint);

    QPen handlePen(m_appSettings.handleColor, 1.4);
    handlePen.setCosmetic(true);
    painter.setPen(handlePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(startPoint.x() - pointHalfSize,
                            startPoint.y() - pointHalfSize,
                            pointHalfSize * 2.0,
                            pointHalfSize * 2.0));
    painter.drawRect(QRectF(endPoint.x() - pointHalfSize,
                            endPoint.y() - pointHalfSize,
                            pointHalfSize * 2.0,
                            pointHalfSize * 2.0));
    painter.drawEllipse(midPoint, midRadius, midRadius);

    QPen hoverPen(m_appSettings.handleHoverColor, 1.4);
    hoverPen.setCosmetic(true);
    painter.setPen(hoverPen);
    painter.setBrush(QBrush(m_appSettings.handleHoverFillColor));
    painter.drawRect(QRectF(endPoint.x() - pointHalfSize,
                            endPoint.y() - pointHalfSize,
                            pointHalfSize * 2.0,
                            pointHalfSize * 2.0));
    painter.drawEllipse(midPoint, midRadius, midRadius);

    ui->handlePreviewLabel->setPixmap(pixmap);
}

void MainWindow::updateToolStatus()
{
    switch (m_toolMode) {
    case ToolMode::None:
        ui->dxfView->setBreakPreviewEnabled(false);
        statusBar()->showMessage(QStringLiteral("Ready"), 3000);
        break;
    case ToolMode::RuleProfileBuild:
        ui->dxfView->setBreakPreviewEnabled(false);
        statusBar()->showMessage(
            QStringLiteral("RuleProfile: pick connected entities, Space accepts hover, right click or Enter builds (%1 queued)")
                .arg(m_pendingRuleProfileEntityIds.size()));
        break;
    case ToolMode::BreakPickEntity:
        ui->dxfView->setBreakPreviewEnabled(false);
        statusBar()->showMessage(QStringLiteral("Break: pick LINE or ARC, right click to cancel"));
        break;
    case ToolMode::BreakPickPoint:
        ui->dxfView->setBreakPreviewEnabled(true);
        statusBar()->showMessage(QStringLiteral("Break: pick point on selected LINE/ARC, right click to cancel"));
        break;
    case ToolMode::FilletPickFirst:
        ui->dxfView->setBreakPreviewEnabled(false);
        if (qFuzzyIsNull(m_pendingFilletRadius)) {
            statusBar()->showMessage(QStringLiteral("Join lines: pick first LINE, right click to cancel"));
        } else {
            statusBar()->showMessage(
                QStringLiteral("Fillet r=%1 mm: pick first LINE, right click to cancel")
                    .arg(QString::number(m_pendingFilletRadius, 'f', 3)));
        }
        break;
    case ToolMode::FilletPickSecond:
        ui->dxfView->setBreakPreviewEnabled(false);
        if (qFuzzyIsNull(m_pendingFilletRadius)) {
            statusBar()->showMessage(QStringLiteral("Join lines: pick second LINE, right click to cancel"));
        } else {
            statusBar()->showMessage(
                QStringLiteral("Fillet r=%1 mm: pick second LINE, right click to cancel")
                    .arg(QString::number(m_pendingFilletRadius, 'f', 3)));
        }
        break;
    }
}

void MainWindow::updatePendingTrimPreview()
{
    if (m_toolMode != ToolMode::RuleProfileBuild
        || m_pendingRuleProfileEntityIds.isEmpty()
        || m_hoveredEntityIndex < 0) {
        ui->dxfView->setPendingTrimPreview({}, false);
        return;
    }

    const QList<int> pendingIndexes = resolveRuleProfileIndexes(m_geometryModel.dxfDocument,
                                                              m_pendingRuleProfileEntityIds);
    const int existingIndex = pendingIndexes.indexOf(m_hoveredEntityIndex);
    if (existingIndex < 0) {
        ui->dxfView->setPendingTrimPreview({}, false);
        return;
    }

    const QList<DirectedPendingEntityInfo> directedInfos =
        directedPendingEntityInfos(m_geometryModel.dxfDocument,
                                   m_pendingRuleProfileEntityIds,
                                   m_geometryModel.detectedBridges);
    if (existingIndex >= directedInfos.size()) {
        ui->dxfView->setPendingTrimPreview({}, false);
        return;
    }

    const DirectedPendingEntityInfo &directedInfo = directedInfos.at(existingIndex);
    const double startDistance = QLineF(m_lastPointerScenePos, directedInfo.pathStartPoint).length();
    const double endDistance = QLineF(m_lastPointerScenePos, directedInfo.pathEndPoint).length();
    const double endpointToleranceMm = qMax(1.0, m_appSettings.defaultToleranceMm * 8.0);

    if (startDistance <= endpointToleranceMm || endDistance <= endpointToleranceMm) {
        ui->dxfView->setPendingTrimPreview(startDistance <= endDistance
                                               ? directedInfo.pathStartPoint
                                               : directedInfo.pathEndPoint,
                                           true);
    } else {
        ui->dxfView->setPendingTrimPreview({}, false);
    }
}

void MainWindow::applyUndoRedoDocument(const DxfDocument &document)
{
    m_isApplyingUndoRedo = true;
    applyGeometryDocument(document, false);
    m_isApplyingUndoRedo = false;
}

void MainWindow::applyUndoRedoProjectDocument(const ProjectDocument &document)
{
    m_isApplyingUndoRedo = true;
    m_projectDocument = document;
    const DxfDocument geometry = m_projectDocument.hasGeometrySnapshot
                                     ? m_projectDocument.geometrySnapshot
                                     : m_geometryModel.dxfDocument;
    applyGeometryDocument(geometry, false);
    m_isApplyingUndoRedo = false;
}

void GeometryEditCommand::undo()
{
    m_window->applyUndoRedoDocument(m_before);
}

void GeometryEditCommand::redo()
{
    if (m_firstRedo) {
        m_firstRedo = false;
        return;
    }

    m_window->applyUndoRedoDocument(m_after);
}

void ProjectDocumentEditCommand::undo()
{
    m_window->applyUndoRedoProjectDocument(m_before);
}

void ProjectDocumentEditCommand::redo()
{
    if (m_firstRedo) {
        m_firstRedo = false;
        return;
    }

    m_window->applyUndoRedoProjectDocument(m_after);
}

void MainWindow::populateToolTable()
{
    m_isPopulatingToolTable = true;
    const QSignalBlocker blocker(ui->toolStationTable);
    ui->toolStationTable->setRowCount(m_appSettings.toolStations.size());

    int row = 0;
    for (const ToolStation &station : m_appSettings.toolStations) {
        auto *enabledItem = new QTableWidgetItem();
        enabledItem->setFlags((enabledItem->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
        enabledItem->setCheckState(station.enabled ? Qt::Checked : Qt::Unchecked);
        enabledItem->setData(ToolStationIdRole, station.id);
        auto *nameItem = new QTableWidgetItem(station.name);
        nameItem->setData(ToolStationIdRole, station.id);
        auto *offsetItem = new QTableWidgetItem(QString::number(station.centerOffsetMm, 'f', 1));
        offsetItem->setData(ToolStationIdRole, station.id);
        auto *widthItem = new QTableWidgetItem(QString::number(station.widthMm, 'f', 1));
        widthItem->setData(ToolStationIdRole, station.id);
        auto *timeoutItem = new QTableWidgetItem(QString::number(station.actionTimeoutMs));
        timeoutItem->setData(ToolStationIdRole, station.id);

        ui->toolStationTable->setItem(row, 0, enabledItem);
        ui->toolStationTable->setItem(row, 1, nameItem);
        ui->toolStationTable->setItem(row, 2, offsetItem);
        ui->toolStationTable->setItem(row, 3, widthItem);
        ui->toolStationTable->setItem(row, 4, timeoutItem);
        ++row;
    }
    m_isPopulatingToolTable = false;
}

void MainWindow::populateBendTables()
{
    m_isPopulatingBendTables = true;
    sortAngleBendTable(&m_appSettings.angleBendTable);
    sortArcBendTable(&m_appSettings.arcBendTable);

    {
        const QSignalBlocker blocker(ui->angleBendTableWidget);
        ui->angleBendTableWidget->setRowCount(m_appSettings.angleBendTable.size());
        for (int row = 0; row < m_appSettings.angleBendTable.size(); ++row) {
            const AngleBendTableRow &tableRow = m_appSettings.angleBendTable.at(row);
            ui->angleBendTableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(tableRow.angleDeg, 'f', 2)));
            ui->angleBendTableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(tableRow.left, 'f', 2)));
            ui->angleBendTableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(tableRow.right, 'f', 2)));
            ui->angleBendTableWidget->setItem(row, 3, new QTableWidgetItem(QString::number(tableRow.startCorrection, 'f', 2)));
            ui->angleBendTableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(tableRow.endCorrection, 'f', 2)));
        }
    }

    {
        const QSignalBlocker blocker(ui->arcBendTableWidget);
        ui->arcBendTableWidget->setRowCount(m_appSettings.arcBendTable.size());
        for (int row = 0; row < m_appSettings.arcBendTable.size(); ++row) {
            const ArcBendTableRow &tableRow = m_appSettings.arcBendTable.at(row);
            ui->arcBendTableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(tableRow.radiusMm, 'f', 2)));
            ui->arcBendTableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(tableRow.segmentLengthMm, 'f', 2)));
            ui->arcBendTableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(tableRow.segments)));
            auto *degreesItem = new QTableWidgetItem(QString::number(bendArcDegreesFor90(tableRow.segments), 'f', 2));
            degreesItem->setFlags(degreesItem->flags() & ~Qt::ItemIsEditable);
            ui->arcBendTableWidget->setItem(row, 3, degreesItem);
            ui->arcBendTableWidget->setItem(row, 4, new QTableWidgetItem(QString::number(tableRow.right90)));
            ui->arcBendTableWidget->setItem(row, 5, new QTableWidgetItem(QString::number(tableRow.right45)));
            ui->arcBendTableWidget->setItem(row, 6, new QTableWidgetItem(QString::number(tableRow.left90)));
            ui->arcBendTableWidget->setItem(row, 7, new QTableWidgetItem(QString::number(tableRow.left45)));
            ui->arcBendTableWidget->setItem(row, 8, new QTableWidgetItem(QString::number(tableRow.startCorrection, 'f', 2)));
            ui->arcBendTableWidget->setItem(row, 9, new QTableWidgetItem(QString::number(tableRow.endCorrection, 'f', 2)));
            ui->arcBendTableWidget->setItem(row, 10, new QTableWidgetItem(QString::number(tableRow.bridgeReduction, 'f', 2)));

            auto *button = new QPushButton(QStringLiteral("Recalculate"), ui->arcBendTableWidget);
            connect(button, &QPushButton::clicked, this, [this, row] {
                if (row < 0 || row >= m_appSettings.arcBendTable.size()) {
                    return;
                }
                ArcBendTableRow &tableRow = m_appSettings.arcBendTable[row];
                if (tableRow.segmentLengthMm <= 0.0) {
                    statusBar()->showMessage(QStringLiteral("Segment length must be greater than zero"), 3000);
                    return;
                }
                const double radiusMm = qAbs(tableRow.radiusMm);
                const double segmentLengthMm = tableRow.segmentLengthMm;
                tableRow.segments = bendArcSegmentCount(radiusMm, 90.0, segmentLengthMm);
                tableRow.right90 = bendArcValue(radiusMm, 90.0, segmentLengthMm);
                tableRow.right45 = bendArcValue(radiusMm, 45.0, segmentLengthMm);
                tableRow.left90 = bendArcValue(radiusMm, 90.0, segmentLengthMm);
                tableRow.left45 = bendArcValue(radiusMm, 45.0, segmentLengthMm);
                saveAppSettings();
                populateBendTables();
                statusBar()->showMessage(QStringLiteral("Arc bend row recalculated"), 2000);
            });
            ui->arcBendTableWidget->setCellWidget(row, 11, button);
        }
    }

    m_isPopulatingBendTables = false;
}

void MainWindow::populateLayerTree()
{
    const QSignalBlocker blocker(ui->layerTree);
    ui->layerTree->clear();

    for (const LayerDefinition &layer : m_geometryModel.dxfDocument.layers) {
        int entityCount = 0;
        for (const DxfEntity &entity : m_geometryModel.dxfDocument.entities) {
            if (entity.layerName == layer.name) {
                ++entityCount;
            }
        }

        auto *item = new QTreeWidgetItem(ui->layerTree);
        item->setText(0, layer.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, layer.visible ? Qt::Checked : Qt::Unchecked);
        item->setData(0, LayerNameRole, layer.name);
        item->setText(1, layer.visible
                             ? QStringLiteral("Visible (%1 entities)").arg(entityCount)
                             : QStringLiteral("Hidden"));
        item->setForeground(0, QBrush(layer.color));
    }
}

void MainWindow::handleLayerTreeItemChanged(QTreeWidgetItem *item, int column)
{
    if (item == nullptr || column != 0) {
        return;
    }

    const QString layerName = item->data(0, LayerNameRole).toString();
    if (layerName.isEmpty()) {
        return;
    }

    bool changed = false;
    int entityCount = 0;
    for (LayerDefinition &layer : m_geometryModel.dxfDocument.layers) {
        if (layer.name == layerName) {
            const bool visible = item->checkState(0) == Qt::Checked;
            if (layer.visible != visible) {
                layer.visible = visible;
                changed = true;
            }
            break;
        }
    }

    for (const DxfEntity &entity : m_geometryModel.dxfDocument.entities) {
        if (entity.layerName == layerName) {
            ++entityCount;
        }
    }

    if (!changed) {
        return;
    }

    {
        const QSignalBlocker blocker(ui->layerTree);
        item->setText(1, item->checkState(0) == Qt::Checked
                             ? QStringLiteral("Visible (%1 entities)").arg(entityCount)
                             : QStringLiteral("Hidden"));
    }

    ui->dxfView->setDocument(m_geometryModel.dxfDocument, false);
    ui->dxfView->setDetectedBridges(m_geometryModel.detectedBridges);
    updateRuleProfileViewState();
    populateEntityProperties();
    updateProjectSummary();
    updateStatusBarDetails();
    storeHiddenLayersToSettings(&m_appSettings, m_geometryModel.dxfDocument);
    saveAppSettings();
}

void MainWindow::populateSelectedEntityPropertiesTree()
{
    ui->entityPropertiesTree->clear();

    if (m_geometryModel.dxfDocument.entities.isEmpty()) {
        auto *item = new QTreeWidgetItem(ui->entityPropertiesTree);
        item->setText(0, QStringLiteral("State"));
        item->setText(1, QStringLiteral("No supported entities loaded"));
        return;
    }

    const int entityIndex = (m_selectedEntityIndex >= 0
                             && m_selectedEntityIndex < m_geometryModel.dxfDocument.entities.size())
                                ? m_selectedEntityIndex
                                : 0;
    const DxfEntity &entity = m_geometryModel.dxfDocument.entities.at(entityIndex);
    QList<QPair<QString, QString>> entityRows = {
        {QStringLiteral("Selected index"), QString::number(entityIndex)},
        {QStringLiteral("Entity ID"), entity.id},
        {QStringLiteral("Source handle"), entity.sourceHandle},
        {QStringLiteral("Type"), entity.typeName()},
        {QStringLiteral("Layer"), entity.layerName},
        {QStringLiteral("Summary"), entity.summary()},
        {QStringLiteral("Point count"), QString::number(entity.points.size())}
    };

    if (entity.type == DxfEntityType::Arc) {
        const QPointF startPoint(entity.center.x() + entity.radius * qCos(qDegreesToRadians(entity.startAngleDeg)),
                                 entity.center.y() + entity.radius * qSin(qDegreesToRadians(entity.startAngleDeg)));
        const QPointF endPoint(entity.center.x() + entity.radius * qCos(qDegreesToRadians(entity.endAngleDeg)),
                               entity.center.y() + entity.radius * qSin(qDegreesToRadians(entity.endAngleDeg)));
        double endDeg = entity.endAngleDeg;
        while (endDeg < entity.startAngleDeg) {
            endDeg += 360.0;
        }
        const double sweepDeg = endDeg - entity.startAngleDeg;
        const double arcLength = qDegreesToRadians(sweepDeg) * entity.radius;

        entityRows.append({QStringLiteral("Start point"), pointText(startPoint)});
        entityRows.append({QStringLiteral("End point"), pointText(endPoint)});
        entityRows.append({QStringLiteral("Center"), pointText(entity.center)});
        entityRows.append({QStringLiteral("Radius"), QString::number(entity.radius, 'f', 3)});
        entityRows.append({QStringLiteral("Start angle"), QStringLiteral("%1 deg").arg(QString::number(entity.startAngleDeg, 'f', 3))});
        entityRows.append({QStringLiteral("End angle"), QStringLiteral("%1 deg").arg(QString::number(entity.endAngleDeg, 'f', 3))});
        entityRows.append({QStringLiteral("Sweep"), QStringLiteral("%1 deg").arg(QString::number(sweepDeg, 'f', 3))});
        entityRows.append({QStringLiteral("Arc length"), QString::number(arcLength, 'f', 3)});
    } else if (!entity.points.isEmpty()) {
        entityRows.append({QStringLiteral("Start point"), pointText(entity.points.first())});
        entityRows.append({QStringLiteral("End point"), pointText(entity.points.last())});

        if (entity.points.size() >= 2) {
            const QLineF baseLine(entity.points.first(), entity.points.last());
            const double length = entity.type == DxfEntityType::Polyline
                                      ? polylineLength(entity.points)
                                      : baseLine.length();
            entityRows.append({QStringLiteral("Length"), QString::number(length, 'f', 3)});
            entityRows.append({QStringLiteral("Angle"), QStringLiteral("%1 deg").arg(QString::number(-baseLine.angle(), 'f', 3))});
        }
    }

    for (const auto &row : entityRows) {
        auto *item = new QTreeWidgetItem(ui->entityPropertiesTree);
        item->setText(0, row.first);
        item->setText(1, row.second);
    }
}

void MainWindow::populateEntityProperties()
{
    populateSelectedEntityPropertiesTree();
    const QSet<QString> expandedKeys = expandedTreeKeys(ui->bladeLinePropertiesTree);
    m_isPopulatingRuleProfileTree = true;
    ui->bladeLinePropertiesTree->clear();

    auto appendBridgeDetails = [this](QTreeWidgetItem *parent, const DetectedBridge &bridge) {
        if (parent == nullptr) {
            return;
        }

        auto *typeItem = new QTreeWidgetItem(parent);
        typeItem->setText(0, QStringLiteral("Type"));
        typeItem->setText(1, bridge.type == DetectedBridgeType::Arc ? QStringLiteral("Arc") : QStringLiteral("Line"));

        auto *lengthItem = new QTreeWidgetItem(parent);
        lengthItem->setText(0, QStringLiteral("Length"));
        lengthItem->setText(1, QStringLiteral("%1 mm").arg(QString::number(bridge.lengthMm, 'f', 3)));

        auto *startItem = new QTreeWidgetItem(parent);
        startItem->setText(0, QStringLiteral("Start point"));
        startItem->setText(1, pointText(bridge.startPoint));

        auto *endItem = new QTreeWidgetItem(parent);
        endItem->setText(0, QStringLiteral("End point"));
        endItem->setText(1, pointText(bridge.endPoint));

        auto *entityLinkItem = new QTreeWidgetItem(parent);
        entityLinkItem->setText(0, QStringLiteral("Linked entities"));
        entityLinkItem->setText(1, QStringLiteral("%1")
                                       .arg(QStringList{
                                           bridge.relatedEntityIndexes.isEmpty() ? QStringLiteral("-")
                                                                                 : QString::number(bridge.relatedEntityIndexes.value(0)),
                                           bridge.relatedEntityIndexes.size() < 2 ? QStringLiteral("-")
                                                                                  : QString::number(bridge.relatedEntityIndexes.value(1))}
                                                .join(QStringLiteral(" -> "))));

        if (bridge.type == DetectedBridgeType::Arc) {
            auto *centerItem = new QTreeWidgetItem(parent);
            centerItem->setText(0, QStringLiteral("Center"));
            centerItem->setText(1, pointText(bridge.center));

            auto *radiusItem = new QTreeWidgetItem(parent);
            radiusItem->setText(0, QStringLiteral("Radius"));
            radiusItem->setText(1, QString::number(bridge.radius, 'f', 3));

            auto *startAngleItem = new QTreeWidgetItem(parent);
            startAngleItem->setText(0, QStringLiteral("Start angle"));
            startAngleItem->setText(1, QStringLiteral("%1 deg").arg(QString::number(bridge.startAngleDeg, 'f', 3)));

            auto *endAngleItem = new QTreeWidgetItem(parent);
            endAngleItem->setText(0, QStringLiteral("End angle"));
            endAngleItem->setText(1, QStringLiteral("%1 deg").arg(QString::number(bridge.endAngleDeg, 'f', 3)));
        }
    };

    auto appendEntityDetails = [this](QTreeWidgetItem *parent, const DxfEntity &segmentEntity) {
        if (parent == nullptr) {
            return;
        }

        auto addRow = [parent](const QString &name, const QString &value) {
            auto *item = new QTreeWidgetItem(parent);
            item->setText(0, name);
            item->setText(1, value);
        };

        addRow(QStringLiteral("Entity ID"), segmentEntity.id);
        addRow(QStringLiteral("Type"), segmentEntity.typeName());
        addRow(QStringLiteral("Layer"), segmentEntity.layerName);

        if (segmentEntity.type == DxfEntityType::Arc) {
            const QPointF startPoint(segmentEntity.center.x() + segmentEntity.radius * qCos(qDegreesToRadians(segmentEntity.startAngleDeg)),
                                     segmentEntity.center.y() + segmentEntity.radius * qSin(qDegreesToRadians(segmentEntity.startAngleDeg)));
            const QPointF endPoint(segmentEntity.center.x() + segmentEntity.radius * qCos(qDegreesToRadians(segmentEntity.endAngleDeg)),
                                   segmentEntity.center.y() + segmentEntity.radius * qSin(qDegreesToRadians(segmentEntity.endAngleDeg)));
            double endDeg = segmentEntity.endAngleDeg;
            while (endDeg < segmentEntity.startAngleDeg) {
                endDeg += 360.0;
            }
            const double sweepDeg = endDeg - segmentEntity.startAngleDeg;
            const double arcLength = qDegreesToRadians(sweepDeg) * segmentEntity.radius;

            addRow(QStringLiteral("Start point"), pointText(startPoint));
            addRow(QStringLiteral("End point"), pointText(endPoint));
            addRow(QStringLiteral("Center"), pointText(segmentEntity.center));
            addRow(QStringLiteral("Radius"), QString::number(segmentEntity.radius, 'f', 3));
            addRow(QStringLiteral("Start angle"), QStringLiteral("%1 deg").arg(QString::number(segmentEntity.startAngleDeg, 'f', 3)));
            addRow(QStringLiteral("End angle"), QStringLiteral("%1 deg").arg(QString::number(segmentEntity.endAngleDeg, 'f', 3)));
            addRow(QStringLiteral("Sweep"), QStringLiteral("%1 deg").arg(QString::number(sweepDeg, 'f', 3)));
            addRow(QStringLiteral("Arc length"), QStringLiteral("%1 mm").arg(QString::number(arcLength, 'f', 3)));
            return;
        }

        if (!segmentEntity.points.isEmpty()) {
            addRow(QStringLiteral("Start point"), pointText(segmentEntity.points.first()));
            addRow(QStringLiteral("End point"), pointText(segmentEntity.points.last()));

            if (segmentEntity.points.size() >= 2) {
                const QLineF baseLine(segmentEntity.points.first(), segmentEntity.points.last());
                const double length = segmentEntity.type == DxfEntityType::Polyline
                                          ? polylineLength(segmentEntity.points)
                                          : baseLine.length();
                addRow(QStringLiteral("Length"), QStringLiteral("%1 mm").arg(QString::number(length, 'f', 3)));
                addRow(QStringLiteral("Angle"), QStringLiteral("%1 deg").arg(QString::number(-baseLine.angle(), 'f', 3)));
            }
        }
    };

    auto entityLengthMm = [](const DxfEntity &segmentEntity) {
        if (segmentEntity.type == DxfEntityType::Arc) {
            double endDeg = segmentEntity.endAngleDeg;
            while (endDeg < segmentEntity.startAngleDeg) {
                endDeg += 360.0;
            }
            return qDegreesToRadians(endDeg - segmentEntity.startAngleDeg) * segmentEntity.radius;
        }

        if (segmentEntity.type == DxfEntityType::Polyline) {
            return polylineLength(segmentEntity.points);
        }

        if (segmentEntity.points.size() >= 2) {
            return QLineF(segmentEntity.points.first(), segmentEntity.points.last()).length();
        }

        return 0.0;
    };

    auto appendFamilyDetails = [&](QTreeWidgetItem *propertiesRoot,
                                   const QList<int> &familyEntityIndexes,
                                   const QList<int> &familyBridgeIndexes) {
        if (propertiesRoot == nullptr || familyEntityIndexes.isEmpty()) {
            return;
        }

        auto addRow = [propertiesRoot](const QString &name, const QString &value) {
            auto *item = new QTreeWidgetItem(propertiesRoot);
            item->setText(0, name);
            item->setText(1, value);
        };

        double geometryLength = 0.0;
        for (int entityIndex : familyEntityIndexes) {
            if (entityIndex >= 0 && entityIndex < m_geometryModel.dxfDocument.entities.size()) {
                geometryLength += entityLengthMm(m_geometryModel.dxfDocument.entities.at(entityIndex));
            }
        }

        double bridgeLength = 0.0;
        for (int bridgeIndex : familyBridgeIndexes) {
            if (bridgeIndex >= 0 && bridgeIndex < m_geometryModel.detectedBridges.size()) {
                bridgeLength += m_geometryModel.detectedBridges.at(bridgeIndex).lengthMm;
            }
        }

        const DxfEntity &firstEntity = m_geometryModel.dxfDocument.entities.at(familyEntityIndexes.first());
        addRow(QStringLiteral("Type"),
               firstEntity.type == DxfEntityType::Arc
                   ? QStringLiteral("Arc segment")
                   : QStringLiteral("Line segment"));
        addRow(QStringLiteral("Entity count"), QString::number(familyEntityIndexes.size()));
        addRow(tr("Bridge count"), QString::number(familyBridgeIndexes.size()));
        addRow(QStringLiteral("Geometry length"),
               QStringLiteral("%1 mm").arg(QString::number(geometryLength, 'f', 3)));
        addRow(tr("Bridge length"),
               QStringLiteral("%1 mm").arg(QString::number(bridgeLength, 'f', 3)));
        addRow(QStringLiteral("Total length"),
               QStringLiteral("%1 mm").arg(QString::number(geometryLength + bridgeLength, 'f', 3)));

        if (firstEntity.type == DxfEntityType::Arc) {
            addRow(QStringLiteral("Center"), pointText(firstEntity.center));
            addRow(QStringLiteral("Radius"), QStringLiteral("%1 mm").arg(QString::number(firstEntity.radius, 'f', 3)));
        } else if (firstEntity.points.size() >= 2) {
            const QLineF baseLine(firstEntity.points.first(), firstEntity.points.last());
            addRow(QStringLiteral("Angle"),
                   QStringLiteral("%1 deg").arg(QString::number(-baseLine.angle(), 'f', 3)));
        }
    };

    QString continuityText = QStringLiteral("Empty");
    int resolvedCount = 0;
    int connectedPairs = 0;
    bool allResolved = true;
    bool allConnected = true;

    int totalPathCount = m_projectDocument.ruleProfiles.size();
    int totalSegmentCount = totalRuleProfileEntityCount(m_projectDocument.ruleProfiles);

    if (m_toolMode == ToolMode::RuleProfileBuild) {
        auto *pendingHeader = new QTreeWidgetItem(ui->bladeLinePropertiesTree);
        pendingHeader->setText(0, QStringLiteral("Pending RuleProfile"));
        pendingHeader->setText(1, QStringLiteral("%1 entities | %2 candidate chains")
                                      .arg(m_pendingRuleProfileEntityIds.size())
                                      .arg(m_candidateChains.size()));
        QFont pendingFont = pendingHeader->font(0);
        pendingFont.setBold(true);
        pendingHeader->setFont(0, pendingFont);
        pendingHeader->setFont(1, pendingFont);
        for (int pendingIndex = 0; pendingIndex < m_pendingRuleProfileEntityIds.size(); ++pendingIndex) {
            const QString &entityId = m_pendingRuleProfileEntityIds.at(pendingIndex);
            const int resolvedIndex = findEntityIndexById(entityId);
            QString valueText = entityId;
            if (resolvedIndex >= 0 && resolvedIndex < m_geometryModel.dxfDocument.entities.size()) {
                const DxfEntity &pendingEntity = m_geometryModel.dxfDocument.entities.at(resolvedIndex);
                valueText = QStringLiteral("%1 | %2 | %3")
                                .arg(entityId, pendingEntity.typeName(), pendingEntity.layerName);
            }

            if (pendingIndex > 0) {
                const int previousIndex = findEntityIndexById(m_pendingRuleProfileEntityIds.at(pendingIndex - 1));
                const int bridgeIndex = findBridgeIndexForEntities(m_geometryModel.detectedBridges,
                                                                   previousIndex,
                                                                   resolvedIndex);
                if (bridgeIndex >= 0 && bridgeIndex < m_geometryModel.detectedBridges.size()) {
                    const DetectedBridge &bridge = m_geometryModel.detectedBridges.at(bridgeIndex);
                    auto *bridgeItem = new QTreeWidgetItem(pendingHeader);
                    bridgeItem->setData(0, BladeTreeItemTypeRole, BladeTreeItemTypeBridge);
                    bridgeItem->setData(0, BladeTreeBridgeIndexRole, bridgeIndex);
                    bridgeItem->setText(0, tr("Bridge %1").arg(pendingIndex));
                    bridgeItem->setText(1, QStringLiteral("%1 mm").arg(QString::number(bridge.lengthMm, 'f', 3)));
                    appendBridgeDetails(bridgeItem, bridge);
                }
            }

            auto *pendingItem = new QTreeWidgetItem(pendingHeader);
            pendingItem->setText(0, resolvedIndex >= 0 && resolvedIndex < m_geometryModel.dxfDocument.entities.size()
                                        ? QStringLiteral("%1 %2")
                                              .arg(m_geometryModel.dxfDocument.entities.at(resolvedIndex).typeName())
                                              .arg(pendingIndex + 1)
                                        : QStringLiteral("Pending %1").arg(pendingIndex + 1));
            pendingItem->setText(1, valueText);
            if (resolvedIndex >= 0 && resolvedIndex < m_geometryModel.dxfDocument.entities.size()) {
                appendEntityDetails(pendingItem, m_geometryModel.dxfDocument.entities.at(resolvedIndex));
            }
        }

        for (int chainIndex = 0; chainIndex < m_candidateChains.size(); ++chainIndex) {
            const QList<int> &chain = m_candidateChains.at(chainIndex);
            auto *chainRoot = new QTreeWidgetItem(pendingHeader);
            chainRoot->setText(0, QStringLiteral("Candidate %1").arg(chainIndex + 1));
            chainRoot->setText(1, QStringLiteral("%1 entities").arg(chain.size()));
            auto *linesRoot = new QTreeWidgetItem(chainRoot);
            linesRoot->setText(0, QStringLiteral("Lines"));
            auto *arcsRoot = new QTreeWidgetItem(chainRoot);
            arcsRoot->setText(0, QStringLiteral("Arcs"));
            auto *bridgesRoot = new QTreeWidgetItem(chainRoot);
            bridgesRoot->setText(0, tr("Bridges"));

            int lineCount = 0;
            int arcCount = 0;
            int bridgeCount = 0;

            for (int entityIndex : chain) {
                if (entityIndex < 0 || entityIndex >= m_geometryModel.dxfDocument.entities.size()) {
                    continue;
                }

                const DxfEntity &candidateEntity = m_geometryModel.dxfDocument.entities.at(entityIndex);
                if (candidateEntity.type == DxfEntityType::Line) {
                    ++lineCount;
                    auto *item = new QTreeWidgetItem(linesRoot);
                    item->setText(0, QStringLiteral("Line %1").arg(lineCount));
                    item->setText(1, candidateEntity.summary());
                } else if (candidateEntity.type == DxfEntityType::Arc) {
                    ++arcCount;
                    auto *item = new QTreeWidgetItem(arcsRoot);
                    item->setText(0, QStringLiteral("Arc %1").arg(arcCount));
                    item->setText(1, candidateEntity.summary());
                }
            }

            const QList<int> pendingIndexes = resolveRuleProfileIndexes(m_geometryModel.dxfDocument, m_pendingRuleProfileEntityIds);
            int previousIndex = pendingIndexes.isEmpty() ? -1 : pendingIndexes.last();
            for (int entityIndex : chain) {
                const int bridgeIndex = findBridgeIndexForEntities(m_geometryModel.detectedBridges, previousIndex, entityIndex);
                previousIndex = entityIndex;
                if (bridgeIndex < 0 || bridgeIndex >= m_geometryModel.detectedBridges.size()) {
                    continue;
                }

                ++bridgeCount;
                const DetectedBridge &bridge = m_geometryModel.detectedBridges.at(bridgeIndex);
                auto *item = new QTreeWidgetItem(bridgesRoot);
                item->setData(0, BladeTreeItemTypeRole, BladeTreeItemTypeBridge);
                item->setData(0, BladeTreeBridgeIndexRole, bridgeIndex);
                item->setText(0, tr("Bridge %1").arg(bridgeCount));
                item->setText(1, QStringLiteral("%1 mm").arg(QString::number(bridge.lengthMm, 'f', 3)));
                appendBridgeDetails(item, bridge);
            }

            if (lineCount == 0) {
                linesRoot->setText(1, QStringLiteral("0"));
            }
            if (arcCount == 0) {
                arcsRoot->setText(1, QStringLiteral("0"));
            }
            if (bridgeCount == 0) {
                bridgesRoot->setText(1, QStringLiteral("0"));
            }
        }
    }

    for (int pathIndex = 0; pathIndex < m_projectDocument.ruleProfiles.size(); ++pathIndex) {
        const QStringList &ruleProfile = m_projectDocument.ruleProfiles.at(pathIndex);
        const RuleProfileOptions ruleProfileOptions =
            pathIndex >= 0 && pathIndex < m_projectDocument.ruleProfileOptions.size()
                ? m_projectDocument.ruleProfileOptions.at(pathIndex)
                : RuleProfileOptions();
        const bool isActiveRuleProfile = pathIndex == m_projectDocument.activeRuleProfileIndex;
        const PendingRuleProfileState ruleProfileState =
            pendingRuleProfileState(m_geometryModel.dxfDocument,
                                    ruleProfile,
                                    m_geometryModel.detectedBridges);
        auto *headerItem = new QTreeWidgetItem(ui->bladeLinePropertiesTree);
        headerItem->setData(0, BladeTreeItemTypeRole, BladeTreeItemTypePath);
        headerItem->setData(0, BladeTreePathIndexRole, pathIndex);
        headerItem->setText(0, QStringLiteral("RuleProfile %1").arg(pathIndex + 1));
        headerItem->setText(1, isActiveRuleProfile
                                   ? QStringLiteral("Active (%1 entities)").arg(ruleProfile.size())
                                   : QStringLiteral("%1 entities").arg(ruleProfile.size()));
        QFont headerFont = headerItem->font(0);
        headerFont.setBold(isActiveRuleProfile);
        headerItem->setFont(0, headerFont);
        headerItem->setFont(1, headerFont);

        auto addEditableRuleProfileOption = [headerItem](const QString &name,
                                                         const QString &value,
                                                         const char *optionKey,
                                                         bool highlighted) {
            auto *item = new QTreeWidgetItem(headerItem);
            item->setText(0, name);
            item->setText(1, value);
            item->setData(0, BladeTreeOptionKeyRole, QString::fromLatin1(optionKey));
            item->setFlags(item->flags() | Qt::ItemIsEditable);
            const QBrush changedBrush(highlighted ? QColor(255, 247, 210) : Qt::transparent);
            item->setBackground(0, changedBrush);
            item->setBackground(1, changedBrush);
        };

        auto addBooleanRuleProfileOption = [headerItem](const QString &name,
                                                        bool value,
                                                        const char *optionKey,
                                                        bool highlighted) {
            auto *item = new QTreeWidgetItem(headerItem);
            item->setText(0, name);
            item->setText(1, QString());
            item->setData(0, BladeTreeOptionKeyRole, QString::fromLatin1(optionKey));
            item->setFlags((item->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
            item->setCheckState(1, value ? Qt::Checked : Qt::Unchecked);
            const QBrush changedBrush(highlighted ? QColor(255, 247, 210) : Qt::transparent);
            item->setBackground(0, changedBrush);
            item->setBackground(1, changedBrush);
        };

        addEditableRuleProfileOption(QStringLiteral("Count"),
                                     QString::number(ruleProfileOptions.count),
                                     RuleProfileOptionCountKey,
                                     ruleProfileOptions.count != RuleProfileOptions().count);
        addBooleanRuleProfileOption(QStringLiteral("Mirror"),
                                    ruleProfileOptions.mirror,
                                    RuleProfileOptionMirrorKey,
                                    ruleProfileOptions.mirror != RuleProfileOptions().mirror);
        addEditableRuleProfileOption(QStringLiteral("Start correction"),
                                     QString::number(ruleProfileOptions.startCorrectionMm, 'f', 2),
                                     RuleProfileOptionStartCorrectionKey,
                                     !qFuzzyCompare(ruleProfileOptions.startCorrectionMm + 1.0, RuleProfileOptions().startCorrectionMm + 1.0));
        addEditableRuleProfileOption(QStringLiteral("End correction"),
                                     QString::number(ruleProfileOptions.endCorrectionMm, 'f', 2),
                                     RuleProfileOptionEndCorrectionKey,
                                     !qFuzzyCompare(ruleProfileOptions.endCorrectionMm + 1.0, RuleProfileOptions().endCorrectionMm + 1.0));
        addBooleanRuleProfileOption(QStringLiteral("Start Lip"),
                                    ruleProfileOptions.startLip,
                                    RuleProfileOptionStartLipKey,
                                    ruleProfileOptions.startLip != RuleProfileOptions().startLip);
        addBooleanRuleProfileOption(QStringLiteral("End Lip"),
                                    ruleProfileOptions.endLip,
                                    RuleProfileOptionEndLipKey,
                                    ruleProfileOptions.endLip != RuleProfileOptions().endLip);

        double bladeGeometryLength = 0.0;
        double bladeBridgeLength = 0.0;
        for (const QString &entityId : ruleProfile) {
            const int resolvedIndex = findEntityIndexById(entityId);
            if (resolvedIndex >= 0 && resolvedIndex < m_geometryModel.dxfDocument.entities.size()) {
                bladeGeometryLength += entityLengthMm(m_geometryModel.dxfDocument.entities.at(resolvedIndex));
            }
        }
        for (int ruleProfileIndex = 1; ruleProfileIndex < ruleProfile.size(); ++ruleProfileIndex) {
            const int previousIndex = findEntityIndexById(ruleProfile.at(ruleProfileIndex - 1));
            const int resolvedIndex = findEntityIndexById(ruleProfile.at(ruleProfileIndex));
            const int bridgeIndex = findBridgeIndexForEntities(m_geometryModel.detectedBridges,
                                                               previousIndex,
                                                               resolvedIndex);
            if (bridgeIndex >= 0 && bridgeIndex < m_geometryModel.detectedBridges.size()) {
                bladeBridgeLength += m_geometryModel.detectedBridges.at(bridgeIndex).lengthMm;
            }
        }

        auto *bladePropertiesItem = new QTreeWidgetItem(headerItem);
        bladePropertiesItem->setText(0, QStringLiteral("Properties"));

        auto addBladeProperty = [bladePropertiesItem](const QString &name, const QString &value) {
            auto *item = new QTreeWidgetItem(bladePropertiesItem);
            item->setText(0, name);
            item->setText(1, value);
        };

        addBladeProperty(QStringLiteral("Start point"),
                         ruleProfileState.valid ? pointText(ruleProfileState.startPoint)
                                              : QStringLiteral("-"));
        addBladeProperty(QStringLiteral("End point"),
                         ruleProfileState.valid
                             ? pointText(ruleProfileState.hasDirection
                                             ? ruleProfileState.openPoint
                                             : ruleProfileState.startPoint)
                             : QStringLiteral("-"));
        addBladeProperty(QStringLiteral("Geometry length"),
                         QStringLiteral("%1 mm").arg(QString::number(bladeGeometryLength, 'f', 3)));
        addBladeProperty(tr("Bridge length"),
                         QStringLiteral("%1 mm").arg(QString::number(bladeBridgeLength, 'f', 3)));
        addBladeProperty(QStringLiteral("Total length"),
                         QStringLiteral("%1 mm").arg(QString::number(bladeGeometryLength + bladeBridgeLength, 'f', 3)));

        QTreeWidgetItem *familyItem = nullptr;
        QTreeWidgetItem *familyPropertiesItem = nullptr;
        int familyCount = 0;
        int previousResolvedIndex = -1;
        QList<int> currentFamilyEntityIndexes;
        QList<int> currentFamilyBridgeIndexes;
        bool previousFamilyHasExitTangent = false;
        QPointF previousFamilyExitTangent;
        bool currentFamilyHasTangents = false;
        QPointF currentFamilyEntryTangent;
        QPointF currentFamilyExitTangent;
        const QList<DirectedPendingEntityInfo> directedInfos =
            directedPendingEntityInfos(m_geometryModel.dxfDocument,
                                       ruleProfile,
                                       m_geometryModel.detectedBridges);

        while (m_projectDocument.ruleProfileSegmentOptions.size() <= pathIndex) {
            m_projectDocument.ruleProfileSegmentOptions.append(QList<RuleProfileSegmentOptions>());
        }

        auto applyOptionRowHighlight = [](QTreeWidgetItem *rowItem,
                                          QWidget *editorWidget,
                                          bool highlighted) {
            if (rowItem == nullptr) {
                return;
            }
            const QBrush changedBrush(highlighted ? QColor(255, 247, 210) : Qt::transparent);
            rowItem->setBackground(0, changedBrush);
            rowItem->setBackground(1, changedBrush);
            if (editorWidget != nullptr) {
                editorWidget->setStyleSheet(highlighted
                                                ? QStringLiteral("background-color: rgb(255, 247, 210);")
                                                : QString());
            }
        };

        auto findDirectedInfoForEntity = [&](int entityIndex) -> std::optional<DirectedPendingEntityInfo> {
            for (const DirectedPendingEntityInfo &info : directedInfos) {
                if (info.entityIndex == entityIndex) {
                    return info;
                }
            }
            return std::nullopt;
        };

        auto familyBoundaryTangents = [&](const QList<int> &familyEntityIndexes,
                                          QPointF *entryTangent,
                                          QPointF *exitTangent) {
            if (entryTangent == nullptr || exitTangent == nullptr || familyEntityIndexes.isEmpty()) {
                return false;
            }

            const std::optional<DirectedPendingEntityInfo> firstInfo =
                findDirectedInfoForEntity(familyEntityIndexes.first());
            const std::optional<DirectedPendingEntityInfo> lastInfo =
                findDirectedInfoForEntity(familyEntityIndexes.last());
            if (!firstInfo.has_value() || !lastInfo.has_value()) {
                return false;
            }

            const DxfEntity &firstEntity = m_geometryModel.dxfDocument.entities.at(firstInfo->entityIndex);
            const DxfEntity &lastEntity = m_geometryModel.dxfDocument.entities.at(lastInfo->entityIndex);
            QPointF firstEntryTangent;
            QPointF firstExitTangent;
            QPointF lastEntryTangent;
            QPointF lastExitTangent;
            if (!directedTangentsForEntity(firstEntity,
                                           firstInfo->pathStartPoint,
                                           firstInfo->pathEndPoint,
                                           &firstEntryTangent,
                                           &firstExitTangent)
                || !directedTangentsForEntity(lastEntity,
                                              lastInfo->pathStartPoint,
                                              lastInfo->pathEndPoint,
                                              &lastEntryTangent,
                                              &lastExitTangent)) {
                return false;
            }

            *entryTangent = firstEntryTangent;
            *exitTangent = lastExitTangent;
            return true;
        };

        auto finalizeFamily = [&]() {
            if (familyItem != nullptr && !currentFamilyEntityIndexes.isEmpty()) {
                double geometryLength = 0.0;
                double bridgeLength = 0.0;
                for (int entityIndex : currentFamilyEntityIndexes) {
                    if (entityIndex >= 0 && entityIndex < m_geometryModel.dxfDocument.entities.size()) {
                        geometryLength += entityLengthMm(m_geometryModel.dxfDocument.entities.at(entityIndex));
                    }
                }
                for (int bridgeIndex : currentFamilyBridgeIndexes) {
                    if (bridgeIndex >= 0 && bridgeIndex < m_geometryModel.detectedBridges.size()) {
                        bridgeLength += m_geometryModel.detectedBridges.at(bridgeIndex).lengthMm;
                    }
                }
                familyItem->setText(1,
                                    tr("%1 entities | %2 bridges | %3 mm")
                                        .arg(currentFamilyEntityIndexes.size())
                                        .arg(currentFamilyBridgeIndexes.size())
                                        .arg(QString::number(geometryLength + bridgeLength, 'f', 3)));
                const QVariant angleData = familyItem->data(0, BladeTreeFamilyAngleRole);
                if (angleData.isValid()) {
                    const double familyAngleDeg = angleData.toDouble();
                    if (qAbs(familyAngleDeg) > 1e-6) {
                        familyItem->setText(1,
                                            tr("%1 entities | %2 bridges | %3 mm | α %4 deg")
                                                .arg(currentFamilyEntityIndexes.size())
                                                .arg(currentFamilyBridgeIndexes.size())
                                                .arg(QString::number(geometryLength + bridgeLength, 'f', 3))
                                                .arg(QString::number(familyAngleDeg, 'f', 2)));
                    }
                }
                familyItem->setData(0, BladeTreeItemTypeRole, BladeTreeItemTypeFamily);
                familyItem->setData(0, BladeTreePathIndexRole, pathIndex);
                familyItem->setData(0, BladeTreeEntityIdsRole, intListToStringList(currentFamilyEntityIndexes));
                familyItem->setData(0, BladeTreeBridgeIndexesRole, intListToStringList(currentFamilyBridgeIndexes));
            }
            appendFamilyDetails(familyPropertiesItem, currentFamilyEntityIndexes, currentFamilyBridgeIndexes);
            QPointF computedEntryTangent;
            QPointF computedExitTangent;
            if (familyBoundaryTangents(currentFamilyEntityIndexes, &computedEntryTangent, &computedExitTangent)) {
                previousFamilyHasExitTangent = true;
                previousFamilyExitTangent = computedExitTangent;
            } else if (currentFamilyHasTangents) {
                previousFamilyHasExitTangent = true;
                previousFamilyExitTangent = currentFamilyExitTangent;
            }
            currentFamilyEntityIndexes.clear();
            currentFamilyBridgeIndexes.clear();
            currentFamilyHasTangents = false;
            currentFamilyEntryTangent = {};
            currentFamilyExitTangent = {};
        };

        auto ensureFamilyItem = [&](int resolvedIndex, int ruleProfileIndex) {
            if (resolvedIndex < 0 || resolvedIndex >= m_geometryModel.dxfDocument.entities.size()) {
                finalizeFamily();
                familyItem = nullptr;
                familyPropertiesItem = nullptr;
                return;
            }

            const DxfEntity &currentEntity = m_geometryModel.dxfDocument.entities.at(resolvedIndex);
            const bool sameFamilyAsPrevious =
                familyItem != nullptr
                && previousResolvedIndex >= 0
                && entitiesShareSelectableFamily(m_geometryModel.dxfDocument,
                                                 previousResolvedIndex,
                                                 resolvedIndex,
                                                 qMax(0.05, m_appSettings.defaultToleranceMm));

            if (sameFamilyAsPrevious) {
                currentFamilyEntityIndexes.append(resolvedIndex);
                if (ruleProfileIndex >= 0 && ruleProfileIndex < directedInfos.size()) {
                    QPointF entryTangent;
                    QPointF exitTangent;
                    if (directedTangentsForEntity(currentEntity,
                                                  directedInfos.at(ruleProfileIndex).pathStartPoint,
                                                  directedInfos.at(ruleProfileIndex).pathEndPoint,
                                                  &entryTangent,
                                                  &exitTangent)) {
                        currentFamilyHasTangents = true;
                        currentFamilyExitTangent = exitTangent;
                    }
                }
                return;
            }

            finalizeFamily();
            ++familyCount;
            const int familyIndex = familyCount - 1;
            while (m_projectDocument.ruleProfileSegmentOptions[pathIndex].size() <= familyIndex) {
                m_projectDocument.ruleProfileSegmentOptions[pathIndex].append(RuleProfileSegmentOptions());
            }
            RuleProfileSegmentOptions &segmentOptions =
                m_projectDocument.ruleProfileSegmentOptions[pathIndex][familyIndex];

            familyItem = new QTreeWidgetItem(headerItem);
            familyItem->setText(0, currentEntity.type == DxfEntityType::Arc
                                       ? QStringLiteral("Arc segment %1").arg(familyCount)
                                       : QStringLiteral("Line segment %1").arg(familyCount));
            familyItem->setText(1, QStringLiteral("..."));
            familyItem->setData(0, BladeTreeFamilyIndexRole, familyIndex);

            bool hasAngle = false;
            double angleDeg = 0.0;
            QPointF entryTangent;
            QPointF exitTangent;
            if (ruleProfileIndex >= 0 && ruleProfileIndex < directedInfos.size()
                && directedTangentsForEntity(currentEntity,
                                             directedInfos.at(ruleProfileIndex).pathStartPoint,
                                             directedInfos.at(ruleProfileIndex).pathEndPoint,
                                             &entryTangent,
                                             &exitTangent)) {
                currentFamilyHasTangents = true;
                currentFamilyEntryTangent = entryTangent;
                currentFamilyExitTangent = exitTangent;
                if (previousFamilyHasExitTangent) {
                    hasAngle = true;
                    angleDeg = signedTurnDegrees(previousFamilyExitTangent, currentFamilyEntryTangent);
                }
            }
            familyItem->setData(0, BladeTreeFamilyAngleRole, hasAngle ? angleDeg : 0.0);

            if (!hasAngle && !qFuzzyCompare(segmentOptions.angleCorrectionDeg + 1.0, 1.0)) {
                segmentOptions.angleCorrectionDeg = 0.0;
            }

            auto *angleItem = new QTreeWidgetItem(familyItem);
            angleItem->setText(0, QStringLiteral("Angle"));
            angleItem->setText(1, hasAngle
                                      ? QStringLiteral("%1 deg").arg(QString::number(angleDeg, 'f', 2))
                                      : QStringLiteral("-"));

            auto *angleCorrectionItem = new QTreeWidgetItem(familyItem);
            angleCorrectionItem->setText(0, QStringLiteral("Angle correction"));
            angleCorrectionItem->setData(0, BladeTreeOptionKeyRole, QString::fromLatin1(RuleProfileSegmentAngleCorrectionKey));
            angleCorrectionItem->setData(0, BladeTreePathIndexRole, pathIndex);
            angleCorrectionItem->setData(0, BladeTreeFamilyIndexRole, familyIndex);
            auto *angleCorrectionSpin = new QDoubleSpinBox(ui->bladeLinePropertiesTree);
            angleCorrectionSpin->setDecimals(2);
            angleCorrectionSpin->setSingleStep(0.10);
            angleCorrectionSpin->setRange(-100000.0, 100000.0);
            const double clampedAngleCorrection = hasAngle
                                                      ? qMax(-qAbs(angleDeg), segmentOptions.angleCorrectionDeg)
                                                      : 0.0;
            if (!qFuzzyCompare(segmentOptions.angleCorrectionDeg + 1.0, clampedAngleCorrection + 1.0)) {
                segmentOptions.angleCorrectionDeg = clampedAngleCorrection;
            }
            angleCorrectionSpin->setValue(clampedAngleCorrection);
            angleCorrectionSpin->setEnabled(hasAngle);
            ui->bladeLinePropertiesTree->setItemWidget(angleCorrectionItem, 1, angleCorrectionSpin);
            applyOptionRowHighlight(angleCorrectionItem,
                                    angleCorrectionSpin,
                                    !qFuzzyCompare(clampedAngleCorrection + 1.0, 1.0));
            connect(angleCorrectionSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                    [this,
                     pathIndex,
                     familyIndex,
                     angleDeg,
                     angleCorrectionItem,
                     angleCorrectionSpin,
                     applyOptionRowHighlight](double value) {
                        if (pathIndex < 0 || pathIndex >= m_projectDocument.ruleProfileSegmentOptions.size()
                            || familyIndex < 0
                            || familyIndex >= m_projectDocument.ruleProfileSegmentOptions[pathIndex].size()) {
                            return;
                        }
                        const double correctedValue = qMax(-qAbs(angleDeg), value);
                        if (!qFuzzyCompare(correctedValue + 1.0, value + 1.0)) {
                            const QSignalBlocker blocker(angleCorrectionSpin);
                            angleCorrectionSpin->setValue(correctedValue);
                        }
                        m_projectDocument.ruleProfileSegmentOptions[pathIndex][familyIndex].angleCorrectionDeg = correctedValue;
                        applyOptionRowHighlight(angleCorrectionItem,
                                                angleCorrectionSpin,
                                                !qFuzzyCompare(correctedValue + 1.0, 1.0));
                        updateProjectSummary();
                        statusBar()->showMessage(QStringLiteral("Segment angle correction updated"), 2000);
                    });

            auto *lengthCorrectionItem = new QTreeWidgetItem(familyItem);
            lengthCorrectionItem->setText(0, QStringLiteral("Length correction"));
            lengthCorrectionItem->setData(0, BladeTreeOptionKeyRole, QString::fromLatin1(RuleProfileSegmentLengthCorrectionKey));
            lengthCorrectionItem->setData(0, BladeTreePathIndexRole, pathIndex);
            lengthCorrectionItem->setData(0, BladeTreeFamilyIndexRole, familyIndex);
            auto *lengthCorrectionSpin = new QDoubleSpinBox(ui->bladeLinePropertiesTree);
            lengthCorrectionSpin->setDecimals(2);
            lengthCorrectionSpin->setRange(-100000.0, 100000.0);
            lengthCorrectionSpin->setSingleStep(0.10);
            lengthCorrectionSpin->setValue(segmentOptions.lengthCorrectionMm);
            ui->bladeLinePropertiesTree->setItemWidget(lengthCorrectionItem, 1, lengthCorrectionSpin);
            applyOptionRowHighlight(lengthCorrectionItem,
                                    lengthCorrectionSpin,
                                    !qFuzzyCompare(segmentOptions.lengthCorrectionMm + 1.0, 1.0));
            connect(lengthCorrectionSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                    [this,
                     pathIndex,
                     familyIndex,
                     lengthCorrectionItem,
                     lengthCorrectionSpin,
                     applyOptionRowHighlight](double value) {
                        if (pathIndex < 0 || pathIndex >= m_projectDocument.ruleProfileSegmentOptions.size()
                            || familyIndex < 0
                            || familyIndex >= m_projectDocument.ruleProfileSegmentOptions[pathIndex].size()) {
                            return;
                        }
                        m_projectDocument.ruleProfileSegmentOptions[pathIndex][familyIndex].lengthCorrectionMm = value;
                        applyOptionRowHighlight(lengthCorrectionItem,
                                                lengthCorrectionSpin,
                                                !qFuzzyCompare(value + 1.0, 1.0));
                        updateProjectSummary();
                        statusBar()->showMessage(QStringLiteral("Segment length correction updated"), 2000);
                    });

            if (currentEntity.type == DxfEntityType::Arc) {
                auto *openItem = new QTreeWidgetItem(familyItem);
                openItem->setText(0, QStringLiteral("Open"));
                openItem->setData(0, BladeTreeOptionKeyRole, QString::fromLatin1(RuleProfileSegmentOpenModeKey));
                openItem->setData(0, BladeTreePathIndexRole, pathIndex);
                openItem->setData(0, BladeTreeFamilyIndexRole, familyIndex);
                auto *openCombo = new QComboBox(ui->bladeLinePropertiesTree);
                openCombo->addItems(arcOpenModeOptions());
                const int openIndex = qMax(0, openCombo->findText(segmentOptions.openMode));
                openCombo->setCurrentIndex(openIndex);
                ui->bladeLinePropertiesTree->setItemWidget(openItem, 1, openCombo);
                applyOptionRowHighlight(openItem,
                                        openCombo,
                                        segmentOptions.openMode != QStringLiteral("Normal"));
                connect(openCombo, &QComboBox::currentTextChanged, this,
                        [this,
                         pathIndex,
                         familyIndex,
                         openItem,
                         openCombo,
                         applyOptionRowHighlight](const QString &value) {
                            if (pathIndex < 0 || pathIndex >= m_projectDocument.ruleProfileSegmentOptions.size()
                                || familyIndex < 0
                                || familyIndex >= m_projectDocument.ruleProfileSegmentOptions[pathIndex].size()) {
                                return;
                            }
                            m_projectDocument.ruleProfileSegmentOptions[pathIndex][familyIndex].openMode = value;
                            applyOptionRowHighlight(openItem,
                                                    openCombo,
                                                    value != QStringLiteral("Normal"));
                            updateProjectSummary();
                            statusBar()->showMessage(QStringLiteral("Arc open mode updated"), 2000);
                        });
            }

            familyPropertiesItem = new QTreeWidgetItem(familyItem);
            familyPropertiesItem->setText(0, QStringLiteral("Properties"));
            currentFamilyEntityIndexes = {resolvedIndex};
            currentFamilyBridgeIndexes.clear();
        };

        for (int ruleProfileIndex = 0; ruleProfileIndex < ruleProfile.size(); ++ruleProfileIndex) {
            const QString &entityId = ruleProfile.at(ruleProfileIndex);
            const int resolvedIndex = findEntityIndexById(entityId);

            QString valueText = entityId;
            if (resolvedIndex >= 0 && resolvedIndex < m_geometryModel.dxfDocument.entities.size()) {
                ++resolvedCount;
                const DxfEntity &bladeEntity = m_geometryModel.dxfDocument.entities.at(resolvedIndex);
                valueText = QStringLiteral("%1 | %2 | %3")
                                .arg(entityId, bladeEntity.typeName(), bladeEntity.layerName);
            } else {
                allResolved = false;
                valueText = QStringLiteral("%1 | missing").arg(entityId);
            }

            ensureFamilyItem(resolvedIndex, ruleProfileIndex);
            QTreeWidgetItem *segmentParent = familyItem != nullptr ? familyItem : headerItem;

            if (ruleProfileIndex > 0) {
                const int previousIndex = findEntityIndexById(ruleProfile.at(ruleProfileIndex - 1));
                const int bridgeIndex = findBridgeIndexForEntities(m_geometryModel.detectedBridges,
                                                                   previousIndex,
                                                                   resolvedIndex);
                if (bridgeIndex >= 0 && bridgeIndex < m_geometryModel.detectedBridges.size()) {
                    const DetectedBridge &bridge = m_geometryModel.detectedBridges.at(bridgeIndex);
                    if (!currentFamilyBridgeIndexes.contains(bridgeIndex)) {
                        currentFamilyBridgeIndexes.append(bridgeIndex);
                    }
                    auto *bridgeItem = new QTreeWidgetItem(segmentParent);
                    bridgeItem->setData(0, BladeTreeItemTypeRole, BladeTreeItemTypeBridge);
                    bridgeItem->setData(0, BladeTreePathIndexRole, pathIndex);
                    bridgeItem->setData(0, BladeTreeBridgeIndexRole, bridgeIndex);
                    bridgeItem->setText(0, tr("Bridge %1").arg(ruleProfileIndex));
                    bridgeItem->setText(1, QStringLiteral("%1 mm").arg(QString::number(bridge.lengthMm, 'f', 3)));
                    appendBridgeDetails(bridgeItem, bridge);
                }
            }

            auto *pathItem = new QTreeWidgetItem(segmentParent);
            pathItem->setData(0, BladeTreeItemTypeRole, BladeTreeItemTypeSegment);
            pathItem->setData(0, BladeTreePathIndexRole, pathIndex);
            pathItem->setData(0, BladeTreeEntityIndexRole, resolvedIndex);
            pathItem->setText(0, resolvedIndex >= 0 && resolvedIndex < m_geometryModel.dxfDocument.entities.size()
                                     ? QStringLiteral("%1 %2")
                                           .arg(m_geometryModel.dxfDocument.entities.at(resolvedIndex).typeName())
                                           .arg(ruleProfileIndex + 1)
                                     : QStringLiteral("Segment %1").arg(ruleProfileIndex + 1));
            pathItem->setText(1, valueText);
            if (resolvedIndex >= 0 && resolvedIndex < m_geometryModel.dxfDocument.entities.size()) {
                appendEntityDetails(pathItem, m_geometryModel.dxfDocument.entities.at(resolvedIndex));
            }
            previousResolvedIndex = resolvedIndex;
        }

        finalizeFamily();
    }

    for (const QStringList &ruleProfile : m_projectDocument.ruleProfiles) {
        if (ruleProfile.size() < 2) {
            continue;
        }

        for (int ruleProfileIndex = 1; ruleProfileIndex < ruleProfile.size(); ++ruleProfileIndex) {
            const int previousIndex = findEntityIndexById(ruleProfile.at(ruleProfileIndex - 1));
            const int currentIndex = findEntityIndexById(ruleProfile.at(ruleProfileIndex));

            if (previousIndex < 0 || currentIndex < 0) {
                allConnected = false;
                continue;
            }

            const PathEndpointInfo previousInfo = endpointInfoForEntity(m_geometryModel.dxfDocument.entities.at(previousIndex));
            const PathEndpointInfo currentInfo = endpointInfoForEntity(m_geometryModel.dxfDocument.entities.at(currentIndex));
            if (!previousInfo.valid || !currentInfo.valid) {
                allConnected = false;
                continue;
            }

            if (entitiesConnectedAtAnyEndpoint(m_geometryModel.detectedBridges,
                                               previousIndex,
                                               previousInfo,
                                               currentIndex,
                                               currentInfo)) {
                ++connectedPairs;
            } else {
                allConnected = false;
            }
        }
    }

    if (totalSegmentCount == 0) {
        continuityText = QStringLiteral("Empty");
    } else if (!allResolved) {
        continuityText = QStringLiteral("Contains missing entities");
    } else if (totalSegmentCount == 1) {
        continuityText = QStringLiteral("Single segment");
    } else if (allConnected) {
        continuityText = QStringLiteral("Connected (%1/%2 joins)")
                             .arg(connectedPairs)
                             .arg(qMax(0, totalSegmentCount - totalPathCount));
    } else {
        continuityText = QStringLiteral("Broken (%1/%2 joins)")
                             .arg(connectedPairs)
                             .arg(qMax(0, totalSegmentCount - totalPathCount));
    }

    const QString activeRuleProfileText =
        (m_projectDocument.activeRuleProfileIndex >= 0
         && m_projectDocument.activeRuleProfileIndex < m_projectDocument.ruleProfiles.size())
            ? QString::number(m_projectDocument.activeRuleProfileIndex + 1)
            : QStringLiteral("None");

    const QList<QPair<QString, QString>> bladeRows = {
        {QStringLiteral("RuleProfile count"), QString::number(totalPathCount)},
        {QStringLiteral("Active RuleProfile"), activeRuleProfileText},
        {QStringLiteral("Path count"), QString::number(totalSegmentCount)},
        {QStringLiteral("Resolved entities"), QStringLiteral("%1 / %2").arg(resolvedCount).arg(totalSegmentCount)},
        {QStringLiteral("Continuity"), continuityText},
        {tr("Detected bridges"), QString::number(m_geometryModel.detectedBridges.size())},
        {QStringLiteral("Modifiers"), QStringLiteral("%1").arg(m_projectDocument.modifiers.size())},
        {QStringLiteral("Source DXF"), m_projectDocument.dxfFilePath}
    };

    for (const auto &row : bladeRows) {
        auto *item = new QTreeWidgetItem(ui->bladeLinePropertiesTree);
        item->setText(0, row.first);
        item->setText(1, row.second);
    }

    restoreExpandedTreeKeys(ui->bladeLinePropertiesTree, expandedKeys);
    m_isPopulatingRuleProfileTree = false;
}

void MainWindow::updateProjectSummary()
{
    ui->projectNameValue->setText(m_projectDocument.displayName());
    ui->projectVersionValue->setText(QString::number(m_projectDocument.projectVersion));
    ui->dxfPathValue->setText(m_projectDocument.hasDxfFile()
                                  ? m_projectDocument.dxfFilePath
                                  : QStringLiteral("No DXF loaded"));
    ui->summaryValue->setText(
        tr("Entities: %1 | Layers: %2 | Bridges: %3 | RuleProfiles: %4 | Undo commands: %5")
            .arg(m_geometryModel.dxfDocument.entities.size())
            .arg(m_geometryModel.dxfDocument.layers.size())
            .arg(m_geometryModel.detectedBridges.size())
            .arg(m_projectDocument.ruleProfiles.size())
            .arg(m_undoStack->count()));
}

void MainWindow::recomputeDetectedBridges()
{
    const double minGap = qMax(0.0, m_appSettings.bridgeMinLengthMm);
    const double maxGap = qMax(minGap, m_appSettings.bridgeMaxLengthMm);
    const double tolerance = qMax(0.05, m_appSettings.defaultToleranceMm);
    m_geometryModel.detectedBridges = detectBridges(m_geometryModel.dxfDocument,
                                                minGap,
                                                maxGap,
                                                tolerance);
}

int MainWindow::findEntityIndexById(const QString &entityId) const
{
    for (int index = 0; index < m_geometryModel.dxfDocument.entities.size(); ++index) {
        if (m_geometryModel.dxfDocument.entities.at(index).id == entityId) {
            return index;
        }
    }

    return -1;
}
