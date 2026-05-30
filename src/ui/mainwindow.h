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
class QMenu;
class QPushButton;
class QTableWidget;
class QUndoStack;
class QTreeWidgetItem;
class QShortcut;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void applyUndoRedoDocument(const DxfDocument &document);

private slots:
    void openDxfFile();
    void openProjectFile();
    void saveProjectFile();
    void saveProjectFileAs();
    void startRuleProfilePicking();
    void removeLastFromRuleProfile();
    void buildRuleProfile();
    void removeActiveRuleProfile();
    void createBreak();
    void createFillet();
    void handleEntityClicked(int entityIndex, const QPointF &scenePos);
    void handleEntityHovered(int entityIndex);
    void handlePointerMoved(const QPointF &scenePos);
    void handleRuleProfileItemClicked(QTreeWidgetItem *item, int column);
    void cancelActiveTool();
    void handleEntitySelectionChanged(int entityIndex);
    void handleDocumentEdited();
    void handleDocumentEditCommitted(const DxfDocument &before,
                                     const DxfDocument &after,
                                     const QString &description);
    void fitDxfToView();
    void handleToolStationCellChanged(int row, int column);
    void handleFlagScaleChanged(int value);
    void handleHandleScaleChanged(int value);
    void handleFlagFillChanged(bool checked);
    void handlePortLengthChanged();
    void acceptHoveredCandidateChain();
    void handleLayerTreeItemChanged(QTreeWidgetItem *item, int column);
    void openRecentDxfAction();
    void openRecentProjectAction();
    void clearUiSelections();

private:
    enum class ToolMode
    {
        None,
        RuleProfileBuild,
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
    void populateSelectedEntityPropertiesTree();
    void populateEntityProperties();
    void updateProjectSummary();
    void updateStatusBarDetails();
    void updateRuleProfileViewState();
    void updateFileMenuState();
    void openDxfFilePath(const QString &filePath);
    bool saveProjectFilePath(const QString &filePath);
    void openProjectFilePath(const QString &filePath);
    void loadAppSettings();
    bool saveAppSettings(bool logSuccess = false);
    void updateColorButton(QPushButton *button, const QColor &color, const QString &fallbackText);
    void chooseColor(QPushButton *button, QColor *targetColor, const QString &title, const QString &buttonText);
    void updateViewColorUi();
    void updateFlagPreview();
    void updateHandlePreview();
    void recomputeDetectedPorts();
    bool acceptCandidateChainByEntityIndex(int entityIndex);
    int findEntityIndexById(const QString &entityId) const;
    void updatePendingTrimPreview();

    Ui::MainWindow *ui;
    QUndoStack *m_undoStack;
    QLabel *m_statusEntityLabel = nullptr;
    QLabel *m_statusPointerLabel = nullptr;
    QMenu *m_recentDxfFilesMenu = nullptr;
    QMenu *m_recentProjectsMenu = nullptr;
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
    QStringList m_pendingRuleProfileEntityIds;
    QList<QList<int>> m_candidateChains;
    bool m_isPopulatingToolTable = false;
};

#endif // FOXBENDER_MAINWINDOW_H
