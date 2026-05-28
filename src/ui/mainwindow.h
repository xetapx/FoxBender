#ifndef FOXBENDER_MAINWINDOW_H
#define FOXBENDER_MAINWINDOW_H

#include "src/app/appsettings.h"
#include "src/core/geometrymodel.h"
#include "src/core/dxfdocument.h"
#include "src/core/projectdocument.h"
#include "src/io/foxdxfreader.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class QLabel;
class QPushButton;
class QTableWidget;
class QUndoStack;
class QTreeWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void applyUndoRedoDocument(const DxfDocument &document);

private slots:
    void openDxfFile();
    void addSelectedToBladeLine();
    void removeLastFromBladeLine();
    void startNewBladeLine();
    void clearBladeLine();
    void createBreak();
    void createFillet();
    void handleEntityClicked(int entityIndex, const QPointF &scenePos);
    void handleEntityHovered(int entityIndex);
    void handlePointerMoved(const QPointF &scenePos);
    void handleBladeLineItemClicked(QTreeWidgetItem *item, int column);
    void cancelActiveTool();
    void handleEntitySelectionChanged(int entityIndex);
    void handleDocumentEdited();
    void handleDocumentEditCommitted(const DxfDocument &before,
                                     const DxfDocument &after,
                                     const QString &description);
    void fitDxfToView();
    void handleToolStationCellChanged(int row, int column);
    void handleFlagScaleChanged(int value);
    void handleFlagFillChanged(bool checked);
    void handlePortLengthChanged();

private:
    enum class ToolMode
    {
        None,
        BladeLineBuild,
        BreakPickEntity,
        BreakPickPoint,
        FilletPickFirst,
        FilletPickSecond
    };

    void applyGeometryDocument(const DxfDocument &document, bool fitView);
    void applyLoadedDocument(const DxfDocument &document, const QString &sourcePath, const QString &logMessage);
    void updateToolStatus();
    void setupWindow();
    void loadDemoGeometry();
    void populateToolTable();
    void populateLayerTree();
    void populateEntityProperties();
    void updateProjectSummary();
    void updateStatusBarDetails();
    void updateBladeLineViewState();
    void loadAppSettings();
    bool saveAppSettings(bool logSuccess = false);
    void updateColorButton(QPushButton *button, const QColor &color, const QString &fallbackText);
    void chooseColor(QPushButton *button, QColor *targetColor, const QString &title, const QString &buttonText);
    void updateViewColorUi();
    void updateFlagPreview();
    void recomputeDetectedPorts();
    int findEntityIndexById(const QString &entityId) const;

    Ui::MainWindow *ui;
    QUndoStack *m_undoStack;
    QLabel *m_statusEntityLabel = nullptr;
    QLabel *m_statusPointerLabel = nullptr;
    ProjectDocument m_projectDocument;
    AppSettings m_appSettings;
    GeometryModel m_geometryModel;
    FoxDxfReader m_dxfReader;
    int m_selectedEntityIndex = -1;
    int m_hoveredEntityIndex = -1;
    QPointF m_lastPointerScenePos;
    bool m_isApplyingUndoRedo = false;
    ToolMode m_toolMode = ToolMode::None;
    int m_pendingBreakEntity = -1;
    double m_pendingFilletRadius = 0.0;
    int m_pendingFilletFirstEntity = -1;
    QStringList m_pendingBladeLineEntityIds;
    bool m_isPopulatingToolTable = false;
};

#endif // FOXBENDER_MAINWINDOW_H
