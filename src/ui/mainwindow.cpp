#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "src/io/jsonstores.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFont>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QSignalBlocker>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>
#include <QUndoCommand>
#include <QUndoStack>
#include <QLineF>
#include <QtMath>

namespace
{
constexpr int BladeTreeItemTypeRole = Qt::UserRole + 1;
constexpr int BladeTreePathIndexRole = Qt::UserRole + 2;
constexpr int BladeTreeEntityIndexRole = Qt::UserRole + 3;
constexpr int ToolStationIdRole = Qt::UserRole + 10;

constexpr int BladeTreeItemTypePath = 1;
constexpr int BladeTreeItemTypeSegment = 2;

QString settingsFilePath()
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/settings.json");
}

QString pointText(const QPointF &point)
{
    return QStringLiteral("(%1, %2)")
        .arg(QString::number(point.x(), 'f', 3),
             QString::number(point.y(), 'f', 3));
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

QList<DetectedPort> detectLinePorts(const DxfDocument &document,
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

    QList<DetectedPort> ports;
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
                DetectedPort port;
                port.type = DetectedPortType::Line;
                port.startPoint = origin + axisUnit * current.maxT;
                port.endPoint = origin + axisUnit * next.minT;
                port.lengthMm = gapLength;
                port.relatedEntityIndexes = {current.rightEntityIndex, next.entityIndex};
                ports.append(port);
            }

            current.minT = next.minT;
            current.maxT = next.maxT;
            current.leftEntityIndex = next.entityIndex;
            current.rightEntityIndex = next.entityIndex;
        }
    }

    return ports;
}

QList<DetectedPort> detectArcPorts(const DxfDocument &document,
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

    QList<DetectedPort> ports;
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

            DetectedPort port;
            port.type = DetectedPortType::Arc;
            port.center = base.center;
            port.radius = base.radius;
            port.startAngleDeg = currentCoverage.endDeg;
            port.endAngleDeg = normalizeAngle360(currentCoverage.endDeg + gapDeg);
            port.lengthMm = arcLength;
            port.startPoint = arcPointAtAngle(base.center, base.radius, port.startAngleDeg);
            port.endPoint = arcPointAtAngle(base.center, base.radius, port.endAngleDeg);
            port.relatedEntityIndexes = {currentCoverage.endEntityIndex, nextCoverage.startEntityIndex};
            ports.append(port);
        }
    }

    return ports;
}

QList<DetectedPort> detectPorts(const DxfDocument &document,
                                double minGapMm,
                                double maxGapMm,
                                double tolerance)
{
    QList<DetectedPort> ports = detectLinePorts(document, minGapMm, maxGapMm, tolerance);
    const QList<DetectedPort> arcPorts = detectArcPorts(document, minGapMm, maxGapMm, tolerance);
    for (const DetectedPort &port : arcPorts) {
        ports.append(port);
    }
    return ports;
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

struct PendingBladeLineState
{
    bool valid = false;
    bool hasDirection = false;
    QPointF startPoint;
    QPointF nextPoint;
    QPointF openPoint;
    int openEntityIndex = -1;
};

PendingBladeLineState pendingBladeLineState(const DxfDocument &document,
                                            const QStringList &pendingEntityIds,
                                            const QList<DetectedPort> &ports);

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

QList<int> resolveBladeLineIndexes(const DxfDocument &document, const QStringList &entityIds)
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

const QStringList *activeBladeLine(const ProjectDocument *document)
{
    if (document == nullptr || document->bladeLines.isEmpty()) {
        return nullptr;
    }

    if (document->activeBladeLineIndex < 0 || document->activeBladeLineIndex >= document->bladeLines.size()) {
        return nullptr;
    }

    return &document->bladeLines[document->activeBladeLineIndex];
}

QList<int> resolveAllBladeLineIndexes(const DxfDocument &document, const QList<QStringList> &bladeLines)
{
    QList<int> indexes;
    for (const QStringList &bladeLine : bladeLines) {
        const QList<int> pathIndexes = resolveBladeLineIndexes(document, bladeLine);
        for (int index : pathIndexes) {
            if (!indexes.contains(index)) {
                indexes.append(index);
            }
        }
    }
    return indexes;
}

int totalBladeLineEntityCount(const QList<QStringList> &bladeLines)
{
    int count = 0;
    for (const QStringList &bladeLine : bladeLines) {
        count += bladeLine.size();
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

bool portEndpointsForEntities(const QList<DetectedPort> &ports,
                              int firstEntityIndex,
                              int secondEntityIndex,
                              QPointF *firstPortPoint,
                              QPointF *secondPortPoint)
{
    for (const DetectedPort &port : ports) {
        if (port.relatedEntityIndexes.size() != 2) {
            continue;
        }

        if (port.relatedEntityIndexes.at(0) == firstEntityIndex
            && port.relatedEntityIndexes.at(1) == secondEntityIndex) {
            if (firstPortPoint != nullptr) {
                *firstPortPoint = port.startPoint;
            }
            if (secondPortPoint != nullptr) {
                *secondPortPoint = port.endPoint;
            }
            return true;
        }

        if (port.relatedEntityIndexes.at(0) == secondEntityIndex
            && port.relatedEntityIndexes.at(1) == firstEntityIndex) {
            if (firstPortPoint != nullptr) {
                *firstPortPoint = port.endPoint;
            }
            if (secondPortPoint != nullptr) {
                *secondPortPoint = port.startPoint;
            }
            return true;
        }
    }

    return false;
}

bool entitiesConnectedAtAnyEndpoint(const QList<DetectedPort> &ports,
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

    QPointF firstPortPoint;
    QPointF secondPortPoint;
    if (!portEndpointsForEntities(ports,
                                  firstEntityIndex,
                                  secondEntityIndex,
                                  &firstPortPoint,
                                  &secondPortPoint)) {
        return false;
    }

    return (pointsNear(firstInfo.startPoint, firstPortPoint) || pointsNear(firstInfo.endPoint, firstPortPoint))
           && (pointsNear(secondInfo.startPoint, secondPortPoint) || pointsNear(secondInfo.endPoint, secondPortPoint));
}

bool tryConnectToNextEntity(const QList<DetectedPort> &ports,
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

    QPointF currentPortPoint;
    QPointF nextPortPoint;
    if (!portEndpointsForEntities(ports,
                                  currentEntityIndex,
                                  nextEntityIndex,
                                  &currentPortPoint,
                                  &nextPortPoint)
        || !pointsNear(currentOpenPoint, currentPortPoint)) {
        return false;
    }

    if (pointsNear(nextInfo.startPoint, nextPortPoint)) {
        if (nextOpenPoint != nullptr) {
            *nextOpenPoint = nextInfo.endPoint;
        }
        return true;
    }
    if (pointsNear(nextInfo.endPoint, nextPortPoint)) {
        if (nextOpenPoint != nullptr) {
            *nextOpenPoint = nextInfo.startPoint;
        }
        return true;
    }

    return false;
}

QList<int> expandPortCandidates(const QList<DetectedPort> &ports,
                                const QList<int> &pendingIndexes,
                                const QList<int> &seedIndexes)
{
    QList<int> expanded = seedIndexes;
    for (int queueIndex = 0; queueIndex < expanded.size(); ++queueIndex) {
        const int currentIndex = expanded.at(queueIndex);
        for (const DetectedPort &port : ports) {
            if (port.relatedEntityIndexes.size() != 2) {
                continue;
            }

            int otherIndex = -1;
            if (port.relatedEntityIndexes.at(0) == currentIndex) {
                otherIndex = port.relatedEntityIndexes.at(1);
            } else if (port.relatedEntityIndexes.at(1) == currentIndex) {
                otherIndex = port.relatedEntityIndexes.at(0);
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

QList<int> candidateBladeLineIndexes(const DxfDocument &document,
                                     const QStringList &pendingEntityIds,
                                     const QList<DetectedPort> &ports)
{
    QList<int> seedIndexes;
    if (pendingEntityIds.isEmpty()) {
        return seedIndexes;
    }

    QList<int> pendingIndexes = resolveBladeLineIndexes(document, pendingEntityIds);
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
            if (tryConnectToNextEntity(ports, firstIndex, firstInfo, firstInfo.startPoint, index, candidateInfo, &unusedOpenPoint)
                || tryConnectToNextEntity(ports, firstIndex, firstInfo, firstInfo.endPoint, index, candidateInfo, &unusedOpenPoint)) {
                seedIndexes.append(index);
            }
        }
    } else {
        const PendingBladeLineState state = pendingBladeLineState(document, pendingEntityIds, ports);
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
            if (tryConnectToNextEntity(ports,
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

    return expandPortCandidates(ports, pendingIndexes, seedIndexes);
}

PendingBladeLineState pendingBladeLineState(const DxfDocument &document,
                                            const QStringList &pendingEntityIds,
                                            const QList<DetectedPort> &ports)
{
    PendingBladeLineState state;
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
    if (tryConnectToNextEntity(ports,
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
    } else if (tryConnectToNextEntity(ports,
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
        if (!tryConnectToNextEntity(ports,
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
    populateLayerTree();
    populateEntityProperties();
    updateProjectSummary();
}

MainWindow::~MainWindow()
{
    delete ui;
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

    ui->operationList->addItem(QStringLiteral("Operation list will be generated from BladeLine data."));
    ui->logList->addItem(QStringLiteral("Log pane initialized."));
    ui->logList->addItem(QStringLiteral("Demo DXF model loaded into the view shell."));
    ui->layerTree->setHeaderLabels({QStringLiteral("Layer"), QStringLiteral("State")});
    ui->entityPropertiesTree->setHeaderLabels({QStringLiteral("Property"), QStringLiteral("Value")});
    ui->bladeLinePropertiesTree->setHeaderLabels({QStringLiteral("Property"), QStringLiteral("Value")});
    ui->simulationEventList->addItem(QStringLiteral("Simulation skeleton waiting for ToolAction data."));
    ui->menuEdit->addAction(m_undoStack->createUndoAction(this, QStringLiteral("Undo")));
    ui->menuEdit->addAction(m_undoStack->createRedoAction(this, QStringLiteral("Redo")));
    updateViewColorUi();

    connect(ui->actionOpenDxf, &QAction::triggered, this, &MainWindow::openDxfFile);
    connect(ui->actionStartNewBladeLine, &QAction::triggered, this, &MainWindow::startNewBladeLine);
    connect(ui->actionAddSelectedToBladeLine, &QAction::triggered, this, &MainWindow::addSelectedToBladeLine);
    connect(ui->actionRemoveLastFromBladeLine, &QAction::triggered, this, &MainWindow::removeLastFromBladeLine);
    connect(ui->actionClearBladeLine, &QAction::triggered, this, &MainWindow::clearBladeLine);
    connect(ui->actionCreateBreak, &QAction::triggered, this, &MainWindow::createBreak);
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
            this, &MainWindow::handleBladeLineItemClicked);
    connect(ui->toolStationTable, &QTableWidget::cellChanged,
            this, &MainWindow::handleToolStationCellChanged);
    connect(ui->selectedEntityColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->selectedEntityColorButton, &m_appSettings.selectedEntityColor,
                    QStringLiteral("Choose Selected Entity Color"), QStringLiteral("Selected"));
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
                    QStringLiteral("Choose BladeLine Color"), QStringLiteral("BladeLine"));
    });
    connect(ui->activeBladeLineColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->activeBladeLineColorButton, &m_appSettings.activeBladeLineColor,
                    QStringLiteral("Choose Active BladeLine Color"), QStringLiteral("Active"));
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
    connect(ui->portColorButton, &QPushButton::clicked, this, [this] {
        chooseColor(ui->portColorButton, &m_appSettings.portColor,
                    QStringLiteral("Choose Port Color"), QStringLiteral("Port"));
    });
    connect(ui->fillFlagsCheckBox, &QCheckBox::toggled,
            this, &MainWindow::handleFlagFillChanged);
    connect(ui->flagScaleSlider, &QSlider::valueChanged,
            this, &MainWindow::handleFlagScaleChanged);
    connect(ui->portMinLengthSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double) { handlePortLengthChanged(); });
    connect(ui->portMaxLengthSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, [this](double) { handlePortLengthChanged(); });
}

void MainWindow::loadDemoGeometry()
{
    applyLoadedDocument(m_dxfReader.createDemoDocument(),
                        QStringLiteral("demo_bracket.dxf"),
                        QStringLiteral("Demo DXF model loaded into the view shell."));
}

void MainWindow::openDxfFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open DXF File"),
        m_projectDocument.hasDxfFile() ? m_projectDocument.dxfFilePath : QString(),
        QStringLiteral("DXF Files (*.dxf);;All Files (*)"));

    if (filePath.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("DXF open canceled"), 3000);
        return;
    }

    const FoxDxfReader::Result result = m_dxfReader.loadFile(filePath);
    if (!result.success) {
        ui->logList->addItem(QStringLiteral("DXF load failed: %1").arg(result.errorMessage));
        QMessageBox::warning(this, QStringLiteral("DXF Load Failed"), result.errorMessage);
        statusBar()->showMessage(QStringLiteral("DXF load failed"), 5000);
        return;
    }

    applyLoadedDocument(result.document,
                        filePath,
                        QStringLiteral("Loaded DXF: %1").arg(filePath));
}

void MainWindow::updateBladeLineViewState()
{
    QList<int> allIndexes = resolveAllBladeLineIndexes(m_geometryModel.dxfDocument, m_projectDocument.bladeLines);
    QList<int> activeIndexes;
    QList<int> candidateIndexes;
    PendingBladeLineState pendingState;

    if (m_toolMode == ToolMode::BladeLineBuild) {
        activeIndexes = resolveBladeLineIndexes(m_geometryModel.dxfDocument, m_pendingBladeLineEntityIds);
        pendingState = pendingBladeLineState(m_geometryModel.dxfDocument,
                                             m_pendingBladeLineEntityIds,
                                             m_geometryModel.detectedPorts);
        candidateIndexes = candidateBladeLineIndexes(m_geometryModel.dxfDocument,
                                                     m_pendingBladeLineEntityIds,
                                                     m_geometryModel.detectedPorts);
        allIndexes = mergeUniqueIndexes(allIndexes, activeIndexes);
        allIndexes = mergeUniqueIndexes(allIndexes, candidateIndexes);
    } else {
        const QStringList *bladeLine = activeBladeLine(&m_projectDocument);
        activeIndexes = bladeLine == nullptr ? QList<int>{}
                                             : resolveBladeLineIndexes(m_geometryModel.dxfDocument, *bladeLine);
        if (bladeLine != nullptr) {
            pendingState = pendingBladeLineState(m_geometryModel.dxfDocument,
                                                 *bladeLine,
                                                 m_geometryModel.detectedPorts);
        }
    }

    ui->dxfView->setBladeLineEntityIndexes(allIndexes);
    ui->dxfView->setActiveBladeLineEntityIndexes(activeIndexes);
    ui->dxfView->setCandidateEntityIndexes(candidateIndexes);
    ui->dxfView->setBladeLineGuide(pendingState.startPoint,
                                   pendingState.nextPoint,
                                   pendingState.hasDirection,
                                   pendingState.openPoint,
                                   pendingState.hasDirection);
}

void MainWindow::addSelectedToBladeLine()
{
    cancelActiveTool();
    m_toolMode = ToolMode::BladeLineBuild;
    m_pendingBladeLineEntityIds.clear();
    ui->dxfView->setArmedEntityIndex(-1);
    updateBladeLineViewState();
    populateEntityProperties();
    updateProjectSummary();
    updateToolStatus();
}

void MainWindow::removeLastFromBladeLine()
{
    if (m_toolMode == ToolMode::BladeLineBuild) {
        if (m_pendingBladeLineEntityIds.isEmpty()) {
            statusBar()->showMessage(QStringLiteral("Pending BladeLine is already empty"), 3000);
            return;
        }

        const QString removedEntityId = m_pendingBladeLineEntityIds.takeLast();
        updateBladeLineViewState();
        populateEntityProperties();
        updateProjectSummary();
        ui->logList->addItem(QStringLiteral("Removed last pending entity %1 from BladeLine build")
                                 .arg(removedEntityId));
        statusBar()->showMessage(QStringLiteral("Removed last pending BladeLine entity"), 3000);
        return;
    }

    if (m_projectDocument.bladeLines.isEmpty()
        || m_projectDocument.activeBladeLineIndex < 0
        || m_projectDocument.activeBladeLineIndex >= m_projectDocument.bladeLines.size()) {
        statusBar()->showMessage(QStringLiteral("No active BladeLine"), 3000);
        return;
    }

    QStringList &bladeLine = m_projectDocument.bladeLines[m_projectDocument.activeBladeLineIndex];
    if (bladeLine.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("Active BladeLine is already empty"), 3000);
        return;
    }

    const QString removedEntityId = bladeLine.takeLast();
    updateBladeLineViewState();
    populateEntityProperties();
    updateProjectSummary();
    ui->logList->addItem(QStringLiteral("Removed last entity %1 from BladeLine %2")
                             .arg(removedEntityId)
                             .arg(m_projectDocument.activeBladeLineIndex + 1));
    statusBar()->showMessage(QStringLiteral("Removed last entity from active BladeLine"), 3000);
}

void MainWindow::startNewBladeLine()
{
    if (m_toolMode != ToolMode::BladeLineBuild) {
        statusBar()->showMessage(QStringLiteral("Press A to start BladeLine picking first"), 3000);
        return;
    }

    if (m_pendingBladeLineEntityIds.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("No pending BladeLine entities to build"), 3000);
        return;
    }

    m_projectDocument.bladeLines.append(m_pendingBladeLineEntityIds);
    m_projectDocument.activeBladeLineIndex = m_projectDocument.bladeLines.size() - 1;
    const int bladeLineNumber = m_projectDocument.activeBladeLineIndex + 1;
    const int segmentCount = m_pendingBladeLineEntityIds.size();
    m_pendingBladeLineEntityIds.clear();
    m_toolMode = ToolMode::None;
    updateBladeLineViewState();
    populateEntityProperties();
    updateProjectSummary();
    ui->logList->addItem(QStringLiteral("Built BladeLine %1 with %2 entities")
                             .arg(bladeLineNumber)
                             .arg(segmentCount));
    updateToolStatus();
}

void MainWindow::clearBladeLine()
{
    if (m_projectDocument.bladeLines.isEmpty()
        || m_projectDocument.activeBladeLineIndex < 0
        || m_projectDocument.activeBladeLineIndex >= m_projectDocument.bladeLines.size()) {
        statusBar()->showMessage(QStringLiteral("No BladeLine to clear"), 3000);
        return;
    }

    const int removedIndex = m_projectDocument.activeBladeLineIndex;
    m_projectDocument.bladeLines.removeAt(removedIndex);
    if (m_projectDocument.bladeLines.isEmpty()) {
        m_projectDocument.activeBladeLineIndex = -1;
    } else if (removedIndex >= m_projectDocument.bladeLines.size()) {
        m_projectDocument.activeBladeLineIndex = m_projectDocument.bladeLines.size() - 1;
    } else {
        m_projectDocument.activeBladeLineIndex = removedIndex;
    }

    ui->dxfView->setBladeLineEntityIndexes(
        resolveAllBladeLineIndexes(m_geometryModel.dxfDocument, m_projectDocument.bladeLines));
    const QStringList *activeBladeLinePath = activeBladeLine(&m_projectDocument);
    ui->dxfView->setActiveBladeLineEntityIndexes(
        activeBladeLinePath == nullptr ? QList<int>{}
                                       : resolveBladeLineIndexes(m_geometryModel.dxfDocument,
                                                                 *activeBladeLinePath));
    populateEntityProperties();
    updateProjectSummary();
    ui->logList->addItem(QStringLiteral("Removed BladeLine %1").arg(removedIndex + 1));
    statusBar()->showMessage(QStringLiteral("BladeLine removed"), 3000);
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

void MainWindow::handleEntityClicked(int entityIndex, const QPointF &scenePos)
{
    if (entityIndex < 0 || entityIndex >= m_geometryModel.dxfDocument.entities.size()) {
        return;
    }

    if (m_toolMode == ToolMode::None) {
        return;
    }

    const DxfEntity &clickedEntity = m_geometryModel.dxfDocument.entities.at(entityIndex);

    if (m_toolMode == ToolMode::BladeLineBuild) {
        if (clickedEntity.id.isEmpty()) {
            statusBar()->showMessage(QStringLiteral("Selected entity has no stable ID"), 3000);
            return;
        }

        if (!m_pendingBladeLineEntityIds.isEmpty()) {
            const QList<int> candidateIndexes =
                candidateBladeLineIndexes(m_geometryModel.dxfDocument,
                                          m_pendingBladeLineEntityIds,
                                          m_geometryModel.detectedPorts);
            if (!candidateIndexes.contains(entityIndex)) {
                statusBar()->showMessage(QStringLiteral("Pick a connected next entity for BladeLine"), 4000);
                return;
            }
        }

        m_pendingBladeLineEntityIds.append(clickedEntity.id);
        updateBladeLineViewState();
        populateEntityProperties();
        updateProjectSummary();
        ui->logList->addItem(QStringLiteral("Picked %1 for pending BladeLine")
                                 .arg(clickedEntity.id));
        statusBar()->showMessage(QStringLiteral("BladeLine picking: %1 entities queued, Shift+A to build")
                                     .arg(m_pendingBladeLineEntityIds.size()),
                                 3000);
        return;
    }

    if (m_toolMode == ToolMode::BreakPickEntity) {
        if (clickedEntity.type != DxfEntityType::Line) {
            statusBar()->showMessage(QStringLiteral("Break currently supports only LINE entities"), 4000);
            return;
        }

        m_pendingBreakEntity = entityIndex;
        m_toolMode = ToolMode::BreakPickPoint;
        ui->dxfView->setArmedEntityIndex(entityIndex);
        updateToolStatus();
        return;
    }

    if (m_toolMode == ToolMode::BreakPickPoint) {
        if (clickedEntity.type != DxfEntityType::Line) {
            statusBar()->showMessage(QStringLiteral("Break currently supports only LINE entities"), 4000);
            return;
        }

        if (m_pendingBreakEntity >= 0 && entityIndex != m_pendingBreakEntity) {
            m_pendingBreakEntity = entityIndex;
            ui->dxfView->setArmedEntityIndex(entityIndex);
            statusBar()->showMessage(QStringLiteral("Break: selected a different LINE, now pick break point"), 4000);
            return;
        }

        const BreakResult result = createLineBreak(m_geometryModel.dxfDocument, entityIndex, scenePos);
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
                                                  QStringLiteral("Create fillet")));
        applyGeometryDocument(result.document, false);
        ui->logList->addItem(QStringLiteral("Created fillet with radius %1 mm").arg(QString::number(m_pendingFilletRadius, 'f', 3)));
        cancelActiveTool();
    }
}

void MainWindow::cancelActiveTool()
{
    if (m_toolMode == ToolMode::None) {
        return;
    }

    m_toolMode = ToolMode::None;
    m_pendingBreakEntity = -1;
    m_pendingFilletRadius = 0.0;
    m_pendingFilletFirstEntity = -1;
    m_pendingBladeLineEntityIds.clear();
    ui->dxfView->setArmedEntityIndex(-1);
    ui->dxfView->setBreakPreviewEnabled(false);
    updateBladeLineViewState();
    populateEntityProperties();
    updateProjectSummary();
    updateToolStatus();
}

void MainWindow::applyLoadedDocument(const DxfDocument &document,
                                     const QString &sourcePath,
                                     const QString &logMessage)
{
    m_projectDocument.bladeLines.clear();
    m_projectDocument.activeBladeLineIndex = 0;
    applyGeometryDocument(document, true);
    m_projectDocument.dxfFilePath = sourcePath;
    ui->logList->addItem(logMessage);
    statusBar()->showMessage(logMessage, 5000);
}

void MainWindow::applyGeometryDocument(const DxfDocument &document, bool fitView)
{
    m_geometryModel.dxfDocument = document;
    recomputeDetectedPorts();
    if (!m_isApplyingUndoRedo) {
        ui->dxfView->setDocument(m_geometryModel.dxfDocument, fitView);
    } else {
        ui->dxfView->setDocument(m_geometryModel.dxfDocument, false);
    }
    ui->dxfView->setDetectedPorts(m_geometryModel.detectedPorts);
    updateBladeLineViewState();
    populateLayerTree();
    populateEntityProperties();
    updateProjectSummary();
    updateStatusBarDetails();
}

void MainWindow::handleEntitySelectionChanged(int entityIndex)
{
    m_selectedEntityIndex = entityIndex;
    populateEntityProperties();
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
    updateStatusBarDetails();
}

void MainWindow::handlePointerMoved(const QPointF &scenePos)
{
    m_lastPointerScenePos = scenePos;
    updateStatusBarDetails();
}

void MainWindow::handleBladeLineItemClicked(QTreeWidgetItem *item, int /*column*/)
{
    if (item == nullptr) {
        return;
    }

    const int itemType = item->data(0, BladeTreeItemTypeRole).toInt();
    const int pathIndex = item->data(0, BladeTreePathIndexRole).toInt();

    if (itemType == BladeTreeItemTypePath) {
        if (pathIndex >= 0 && pathIndex < m_projectDocument.bladeLines.size()) {
            m_projectDocument.activeBladeLineIndex = pathIndex;
            ui->dxfView->setActiveBladeLineEntityIndexes(
                resolveBladeLineIndexes(m_geometryModel.dxfDocument,
                                        m_projectDocument.bladeLines.at(pathIndex)));
            populateEntityProperties();
            updateProjectSummary();
            ui->logList->addItem(QStringLiteral("Activated BladeLine %1").arg(pathIndex + 1));
            statusBar()->showMessage(QStringLiteral("Activated BladeLine %1").arg(pathIndex + 1), 3000);
        }
        return;
    }

    if (itemType == BladeTreeItemTypeSegment) {
        if (pathIndex >= 0 && pathIndex < m_projectDocument.bladeLines.size()
            && m_projectDocument.activeBladeLineIndex != pathIndex) {
            m_projectDocument.activeBladeLineIndex = pathIndex;
            ui->dxfView->setActiveBladeLineEntityIndexes(
                resolveBladeLineIndexes(m_geometryModel.dxfDocument,
                                        m_projectDocument.bladeLines.at(pathIndex)));
            populateEntityProperties();
            updateProjectSummary();
        }

        const int entityIndex = item->data(0, BladeTreeEntityIndexRole).toInt();
        if (entityIndex >= 0 && entityIndex < m_geometryModel.dxfDocument.entities.size()) {
            ui->dxfView->selectEntityIndex(entityIndex);
            statusBar()->showMessage(QStringLiteral("BladeLine %1, segment %2 selected")
                                         .arg(pathIndex + 1)
                                         .arg(item->text(0)),
                                     3000);
        }
    }
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

void MainWindow::loadAppSettings()
{
    AppSettings loadedSettings;
    QString errorMessage;
    if (!JsonSettingsStore::load(settingsFilePath(), &loadedSettings, &errorMessage)) {
        return;
    }

    m_appSettings = loadedSettings;
}

bool MainWindow::saveAppSettings(bool logSuccess)
{
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

    if (logSuccess && ui != nullptr && ui->logList != nullptr) {
        ui->logList->addItem(QStringLiteral("Settings saved: %1").arg(settingsFilePath()));
    }
    return true;
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

void MainWindow::handlePortLengthChanged()
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
    if (qFuzzyCompare(m_appSettings.portMinLengthMm + 1.0, minValue + 1.0)
        && qFuzzyCompare(m_appSettings.portMaxLengthMm + 1.0, maxValue + 1.0)) {
        return;
    }

    m_appSettings.portMinLengthMm = minValue;
    m_appSettings.portMaxLengthMm = maxValue;
    recomputeDetectedPorts();
    ui->dxfView->setDetectedPorts(m_geometryModel.detectedPorts);
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

void MainWindow::updateColorButton(QPushButton *button, const QColor &color, const QString &fallbackText)
{
    if (button == nullptr) {
        return;
    }

    button->setText(QStringLiteral("%1 (%2)").arg(fallbackText, color.name(QColor::HexRgb)));
    button->setStyleSheet(QString());
}

void MainWindow::updateViewColorUi()
{
    updateColorButton(ui->selectedEntityColorButton, m_appSettings.selectedEntityColor, QStringLiteral("Selected"));
    updateColorButton(ui->hoverEntityColorButton, m_appSettings.hoverEntityColor, QStringLiteral("Hover"));
    updateColorButton(ui->armedEntityColorButton, m_appSettings.armedEntityColor, QStringLiteral("Armed"));
    updateColorButton(ui->bladeLineColorButton, m_appSettings.bladeLineColor, QStringLiteral("BladeLine"));
    updateColorButton(ui->activeBladeLineColorButton, m_appSettings.activeBladeLineColor, QStringLiteral("Active"));
    updateColorButton(ui->handleColorButton, m_appSettings.handleColor, QStringLiteral("Handle"));
    updateColorButton(ui->handleHoverColorButton, m_appSettings.handleHoverColor, QStringLiteral("Handle Hover"));
    updateColorButton(ui->handleHoverFillColorButton, m_appSettings.handleHoverFillColor, QStringLiteral("Handle Fill"));
    updateColorButton(ui->snapPreviewColorButton, m_appSettings.snapPreviewColor, QStringLiteral("Snap"));
    updateColorButton(ui->breakPreviewColorButton, m_appSettings.breakPreviewColor, QStringLiteral("Break"));
    updateColorButton(ui->viewBackgroundColorButton, m_appSettings.viewBackgroundColor, QStringLiteral("Background"));
    updateColorButton(ui->startFlagColorButton, m_appSettings.startFlagColor, QStringLiteral("Start Flag"));
    updateColorButton(ui->endFlagColorButton, m_appSettings.endFlagColor, QStringLiteral("End Flag"));
    updateColorButton(ui->portColorButton, m_appSettings.portColor, QStringLiteral("Port"));

    {
        const QSignalBlocker fillBlocker(ui->fillFlagsCheckBox);
        ui->fillFlagsCheckBox->setChecked(m_appSettings.fillFlags);
    }
    {
        const QSignalBlocker sliderBlocker(ui->flagScaleSlider);
        ui->flagScaleSlider->setValue(qRound(m_appSettings.flagScale * 100.0));
    }
    {
        const QSignalBlocker minBlocker(ui->portMinLengthSpinBox);
        ui->portMinLengthSpinBox->setValue(m_appSettings.portMinLengthMm);
    }
    {
        const QSignalBlocker maxBlocker(ui->portMaxLengthSpinBox);
        ui->portMaxLengthSpinBox->setValue(m_appSettings.portMaxLengthMm);
    }
    ui->flagScaleValueLabel->setText(QStringLiteral("%1 %")
                                         .arg(QString::number(qRound(m_appSettings.flagScale * 100.0))));

    DxfViewWidget::ViewColors colors;
    colors.backgroundColor = m_appSettings.viewBackgroundColor;
    colors.selectedEntityColor = m_appSettings.selectedEntityColor;
    colors.hoverEntityColor = m_appSettings.hoverEntityColor;
    colors.armedEntityColor = m_appSettings.armedEntityColor;
    colors.bladeLineColor = m_appSettings.bladeLineColor;
    colors.activeBladeLineColor = m_appSettings.activeBladeLineColor;
    colors.handleColor = m_appSettings.handleColor;
    colors.handleHoverColor = m_appSettings.handleHoverColor;
    colors.handleHoverFillColor = m_appSettings.handleHoverFillColor;
    colors.snapPreviewColor = m_appSettings.snapPreviewColor;
    colors.breakPreviewColor = m_appSettings.breakPreviewColor;
    colors.startFlagColor = m_appSettings.startFlagColor;
    colors.endFlagColor = m_appSettings.endFlagColor;
    colors.portColor = m_appSettings.portColor;
    colors.fillFlags = m_appSettings.fillFlags;
    colors.flagScale = m_appSettings.flagScale;
    ui->dxfView->setViewColors(colors);
    ui->dxfView->setDetectedPorts(m_geometryModel.detectedPorts);
    updateFlagPreview();
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

void MainWindow::updateToolStatus()
{
    switch (m_toolMode) {
    case ToolMode::None:
        ui->dxfView->setBreakPreviewEnabled(false);
        statusBar()->showMessage(QStringLiteral("Ready"), 3000);
        break;
    case ToolMode::BladeLineBuild:
        ui->dxfView->setBreakPreviewEnabled(false);
        statusBar()->showMessage(
            QStringLiteral("BladeLine: pick connected entities, Shift+A to build, right click to cancel (%1 queued)")
                .arg(m_pendingBladeLineEntityIds.size()));
        break;
    case ToolMode::BreakPickEntity:
        ui->dxfView->setBreakPreviewEnabled(false);
        statusBar()->showMessage(QStringLiteral("Break: pick LINE, right click to cancel"));
        break;
    case ToolMode::BreakPickPoint:
        ui->dxfView->setBreakPreviewEnabled(true);
        statusBar()->showMessage(QStringLiteral("Break: pick point on selected LINE, right click to cancel"));
        break;
    case ToolMode::FilletPickFirst:
        ui->dxfView->setBreakPreviewEnabled(false);
        statusBar()->showMessage(
            QStringLiteral("Fillet r=%1 mm: pick first LINE, right click to cancel")
                .arg(QString::number(m_pendingFilletRadius, 'f', 3)));
        break;
    case ToolMode::FilletPickSecond:
        ui->dxfView->setBreakPreviewEnabled(false);
        statusBar()->showMessage(
            QStringLiteral("Fillet r=%1 mm: pick second LINE, right click to cancel")
                .arg(QString::number(m_pendingFilletRadius, 'f', 3)));
        break;
    }
}

void MainWindow::applyUndoRedoDocument(const DxfDocument &document)
{
    m_isApplyingUndoRedo = true;
    applyGeometryDocument(document, false);
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

void MainWindow::populateLayerTree()
{
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
        item->setText(1, layer.visible
                             ? QStringLiteral("Visible (%1 entities)").arg(entityCount)
                             : QStringLiteral("Hidden"));
        item->setForeground(0, QBrush(layer.color));
    }
}

void MainWindow::populateEntityProperties()
{
    ui->entityPropertiesTree->clear();
    ui->bladeLinePropertiesTree->clear();

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

    QString continuityText = QStringLiteral("Empty");
    int resolvedCount = 0;
    int connectedPairs = 0;
    bool allResolved = true;
    bool allConnected = true;

    int totalPathCount = m_projectDocument.bladeLines.size();
    int totalSegmentCount = totalBladeLineEntityCount(m_projectDocument.bladeLines);

    if (m_toolMode == ToolMode::BladeLineBuild) {
        auto *pendingHeader = new QTreeWidgetItem(ui->bladeLinePropertiesTree);
        pendingHeader->setText(0, QStringLiteral("Pending BladeLine"));
        pendingHeader->setText(1, QStringLiteral("%1 entities | %2 candidates")
                                      .arg(m_pendingBladeLineEntityIds.size())
                                      .arg(candidateBladeLineIndexes(m_geometryModel.dxfDocument,
                                                                     m_pendingBladeLineEntityIds,
                                                                     m_geometryModel.detectedPorts).size()));
        QFont pendingFont = pendingHeader->font(0);
        pendingFont.setBold(true);
        pendingHeader->setFont(0, pendingFont);
        pendingHeader->setFont(1, pendingFont);
        pendingHeader->setExpanded(true);

        for (int pendingIndex = 0; pendingIndex < m_pendingBladeLineEntityIds.size(); ++pendingIndex) {
            const QString &entityId = m_pendingBladeLineEntityIds.at(pendingIndex);
            const int resolvedIndex = findEntityIndexById(entityId);
            QString valueText = entityId;
            if (resolvedIndex >= 0 && resolvedIndex < m_geometryModel.dxfDocument.entities.size()) {
                const DxfEntity &pendingEntity = m_geometryModel.dxfDocument.entities.at(resolvedIndex);
                valueText = QStringLiteral("%1 | %2 | %3")
                                .arg(entityId, pendingEntity.typeName(), pendingEntity.layerName);
            }

            auto *pendingItem = new QTreeWidgetItem(pendingHeader);
            pendingItem->setText(0, QStringLiteral("Pending %1").arg(pendingIndex + 1));
            pendingItem->setText(1, valueText);
        }
    }

    for (int pathIndex = 0; pathIndex < m_projectDocument.bladeLines.size(); ++pathIndex) {
        const QStringList &bladeLine = m_projectDocument.bladeLines.at(pathIndex);
        const bool isActiveBladeLine = pathIndex == m_projectDocument.activeBladeLineIndex;
        auto *headerItem = new QTreeWidgetItem(ui->bladeLinePropertiesTree);
        headerItem->setData(0, BladeTreeItemTypeRole, BladeTreeItemTypePath);
        headerItem->setData(0, BladeTreePathIndexRole, pathIndex);
        headerItem->setText(0, QStringLiteral("BladeLine %1").arg(pathIndex + 1));
        headerItem->setText(1, isActiveBladeLine
                                   ? QStringLiteral("Active (%1 entities)").arg(bladeLine.size())
                                   : QStringLiteral("%1 entities").arg(bladeLine.size()));
        QFont headerFont = headerItem->font(0);
        headerFont.setBold(isActiveBladeLine);
        headerItem->setFont(0, headerFont);
        headerItem->setFont(1, headerFont);
        headerItem->setExpanded(isActiveBladeLine);

        for (int bladeIndex = 0; bladeIndex < bladeLine.size(); ++bladeIndex) {
            const QString &entityId = bladeLine.at(bladeIndex);
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

            auto *pathItem = new QTreeWidgetItem(headerItem);
            pathItem->setData(0, BladeTreeItemTypeRole, BladeTreeItemTypeSegment);
            pathItem->setData(0, BladeTreePathIndexRole, pathIndex);
            pathItem->setData(0, BladeTreeEntityIndexRole, resolvedIndex);
            pathItem->setText(0, QStringLiteral("Segment %1").arg(bladeIndex + 1));
            pathItem->setText(1, valueText);
        }
    }

    for (const QStringList &bladeLine : m_projectDocument.bladeLines) {
        if (bladeLine.size() < 2) {
            continue;
        }

        for (int bladeIndex = 1; bladeIndex < bladeLine.size(); ++bladeIndex) {
            const int previousIndex = findEntityIndexById(bladeLine.at(bladeIndex - 1));
            const int currentIndex = findEntityIndexById(bladeLine.at(bladeIndex));

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

            if (entitiesConnectedAtAnyEndpoint(m_geometryModel.detectedPorts,
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

    const QString activeBladeLineText =
        (m_projectDocument.activeBladeLineIndex >= 0
         && m_projectDocument.activeBladeLineIndex < m_projectDocument.bladeLines.size())
            ? QString::number(m_projectDocument.activeBladeLineIndex + 1)
            : QStringLiteral("None");

    const QList<QPair<QString, QString>> bladeRows = {
        {QStringLiteral("BladeLine count"), QString::number(totalPathCount)},
        {QStringLiteral("Active BladeLine"), activeBladeLineText},
        {QStringLiteral("Path count"), QString::number(totalSegmentCount)},
        {QStringLiteral("Resolved entities"), QStringLiteral("%1 / %2").arg(resolvedCount).arg(totalSegmentCount)},
        {QStringLiteral("Continuity"), continuityText},
        {QStringLiteral("Detected ports"), QString::number(m_geometryModel.detectedPorts.size())},
        {QStringLiteral("Modifiers"), QStringLiteral("%1").arg(m_projectDocument.modifiers.size())},
        {QStringLiteral("Source DXF"), m_projectDocument.dxfFilePath}
    };

    for (const auto &row : bladeRows) {
        auto *item = new QTreeWidgetItem(ui->bladeLinePropertiesTree);
        item->setText(0, row.first);
        item->setText(1, row.second);
    }
}

void MainWindow::updateProjectSummary()
{
    ui->projectNameValue->setText(m_projectDocument.displayName());
    ui->projectVersionValue->setText(QString::number(m_projectDocument.projectVersion));
    ui->dxfPathValue->setText(m_projectDocument.hasDxfFile()
                                  ? m_projectDocument.dxfFilePath
                                  : QStringLiteral("No DXF loaded"));
    ui->summaryValue->setText(
        QStringLiteral("Entities: %1 | Layers: %2 | Ports: %3 | BladeLines: %4 | Undo commands: %5")
            .arg(m_geometryModel.dxfDocument.entities.size())
            .arg(m_geometryModel.dxfDocument.layers.size())
            .arg(m_geometryModel.detectedPorts.size())
            .arg(m_projectDocument.bladeLines.size())
            .arg(m_undoStack->count()));
}

void MainWindow::recomputeDetectedPorts()
{
    const double minGap = qMax(0.0, m_appSettings.portMinLengthMm);
    const double maxGap = qMax(minGap, m_appSettings.portMaxLengthMm);
    const double tolerance = qMax(0.05, m_appSettings.defaultToleranceMm);
    m_geometryModel.detectedPorts = detectPorts(m_geometryModel.dxfDocument,
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
