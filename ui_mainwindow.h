/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "src/ui/widgets/dxfviewwidget.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionOpenDxf;
    QAction *actionExit;
    QAction *actionFitView;
    QAction *actionCreateFillet;
    QAction *actionCreateBreak;
    QAction *actionAddSelectedToBladeLine;
    QAction *actionClearBladeLine;
    QAction *actionRemoveLastFromBladeLine;
    QAction *actionStartNewBladeLine;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QTabWidget *mainTabWidget;
    QWidget *dxfTab;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *projectSummaryGroup;
    QGridLayout *gridLayout;
    QLabel *labelProjectName;
    QLabel *projectNameValue;
    QLabel *labelDxfPath;
    QLabel *dxfPathValue;
    QLabel *labelProjectVersion;
    QLabel *projectVersionValue;
    QLabel *labelSummary;
    QLabel *summaryValue;
    DxfViewWidget *dxfView;
    QWidget *settingsTab;
    QVBoxLayout *verticalLayout_3;
    QTabWidget *settingsTabWidget;
    QWidget *toolSettingsTab;
    QVBoxLayout *verticalLayout_12;
    QGroupBox *toolStationsGroup;
    QVBoxLayout *verticalLayout_5;
    QTableWidget *toolStationTable;
    QPlainTextEdit *settingsNotes;
    QWidget *colorSettingsTab;
    QVBoxLayout *verticalLayout_13;
    QGroupBox *viewColorsGroup;
    QGridLayout *gridLayout_2;
    QLabel *labelSelectedEntityColor;
    QPushButton *snapPreviewColorButton;
    QPushButton *armedEntityColorButton;
    QLabel *labelArmedEntityColor;
    QPushButton *hoverEntityColorButton;
    QLabel *labelBladeLineColor;
    QLabel *labelActiveBladeLineColor;
    QPushButton *viewBackgroundColorButton;
    QLabel *labelSnapPreviewColor;
    QPushButton *handleHoverFillColorButton;
    QLabel *labelHoverEntityColor;
    QPushButton *activeBladeLineColorButton;
    QPushButton *handleHoverColorButton;
    QLabel *labelViewBackgroundColor;
    QPushButton *selectedEntityColorButton;
    QLabel *labelHandleHoverColor;
    QLabel *labelBreakPreviewColor;
    QPushButton *breakPreviewColorButton;
    QPushButton *handleColorButton;
    QLabel *labelHandleColor;
    QPushButton *bladeLineColorButton;
    QLabel *labelHandleHoverFillColor;
    QSpacerItem *horizontalSpacer;
    QGroupBox *flagSettingsGroup;
    QGridLayout *gridLayout_3;
    QLabel *labelStartFlagColor;
    QPushButton *startFlagColorButton;
    QLabel *labelEndFlagColor;
    QPushButton *endFlagColorButton;
    QLabel *labelFlagScale;
    QHBoxLayout *horizontalLayout_2;
    QSlider *flagScaleSlider;
    QLabel *flagScaleValueLabel;
    QLabel *labelFlagFill;
    QCheckBox *fillFlagsCheckBox;
    QLabel *labelFlagPreview;
    QLabel *flagPreviewLabel;
    QGroupBox *portSettingsGroup;
    QGridLayout *gridLayout_4;
    QLabel *labelPortMinLength;
    QDoubleSpinBox *portMinLengthSpinBox;
    QLabel *labelPortMaxLength;
    QDoubleSpinBox *portMaxLengthSpinBox;
    QLabel *labelPortColor;
    QPushButton *portColorButton;
    QWidget *simulationTab;
    QVBoxLayout *verticalLayout_4;
    QGroupBox *simulationOverviewGroup;
    QVBoxLayout *verticalLayout_6;
    QLabel *simulationStatusLabel;
    QListWidget *simulationEventList;
    QDockWidget *layerDock;
    QWidget *dockWidgetContents;
    QVBoxLayout *verticalLayout_7;
    QTreeWidget *layerTree;
    QDockWidget *entityPropertiesDock;
    QWidget *dockWidgetContents_2;
    QVBoxLayout *verticalLayout_8;
    QTreeWidget *entityPropertiesTree;
    QDockWidget *bladeLineDock;
    QWidget *dockWidgetContents_3;
    QVBoxLayout *verticalLayout_9;
    QTreeWidget *bladeLinePropertiesTree;
    QDockWidget *operationDock;
    QWidget *dockWidgetContents_4;
    QVBoxLayout *verticalLayout_10;
    QListWidget *operationList;
    QDockWidget *logDock;
    QWidget *dockWidgetContents_5;
    QVBoxLayout *verticalLayout_11;
    QListWidget *logList;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuEdit;
    QMenu *menuView;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1360, 1075);
        actionOpenDxf = new QAction(MainWindow);
        actionOpenDxf->setObjectName("actionOpenDxf");
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        actionFitView = new QAction(MainWindow);
        actionFitView->setObjectName("actionFitView");
        actionCreateFillet = new QAction(MainWindow);
        actionCreateFillet->setObjectName("actionCreateFillet");
        actionCreateBreak = new QAction(MainWindow);
        actionCreateBreak->setObjectName("actionCreateBreak");
        actionAddSelectedToBladeLine = new QAction(MainWindow);
        actionAddSelectedToBladeLine->setObjectName("actionAddSelectedToBladeLine");
        actionClearBladeLine = new QAction(MainWindow);
        actionClearBladeLine->setObjectName("actionClearBladeLine");
        actionRemoveLastFromBladeLine = new QAction(MainWindow);
        actionRemoveLastFromBladeLine->setObjectName("actionRemoveLastFromBladeLine");
        actionStartNewBladeLine = new QAction(MainWindow);
        actionStartNewBladeLine->setObjectName("actionStartNewBladeLine");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        mainTabWidget = new QTabWidget(centralwidget);
        mainTabWidget->setObjectName("mainTabWidget");
        dxfTab = new QWidget();
        dxfTab->setObjectName("dxfTab");
        verticalLayout_2 = new QVBoxLayout(dxfTab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        projectSummaryGroup = new QGroupBox(dxfTab);
        projectSummaryGroup->setObjectName("projectSummaryGroup");
        gridLayout = new QGridLayout(projectSummaryGroup);
        gridLayout->setObjectName("gridLayout");
        labelProjectName = new QLabel(projectSummaryGroup);
        labelProjectName->setObjectName("labelProjectName");

        gridLayout->addWidget(labelProjectName, 0, 0, 1, 1);

        projectNameValue = new QLabel(projectSummaryGroup);
        projectNameValue->setObjectName("projectNameValue");

        gridLayout->addWidget(projectNameValue, 0, 1, 1, 1);

        labelDxfPath = new QLabel(projectSummaryGroup);
        labelDxfPath->setObjectName("labelDxfPath");

        gridLayout->addWidget(labelDxfPath, 1, 0, 1, 1);

        dxfPathValue = new QLabel(projectSummaryGroup);
        dxfPathValue->setObjectName("dxfPathValue");
        dxfPathValue->setWordWrap(true);

        gridLayout->addWidget(dxfPathValue, 1, 1, 1, 1);

        labelProjectVersion = new QLabel(projectSummaryGroup);
        labelProjectVersion->setObjectName("labelProjectVersion");

        gridLayout->addWidget(labelProjectVersion, 2, 0, 1, 1);

        projectVersionValue = new QLabel(projectSummaryGroup);
        projectVersionValue->setObjectName("projectVersionValue");

        gridLayout->addWidget(projectVersionValue, 2, 1, 1, 1);

        labelSummary = new QLabel(projectSummaryGroup);
        labelSummary->setObjectName("labelSummary");

        gridLayout->addWidget(labelSummary, 3, 0, 1, 1);

        summaryValue = new QLabel(projectSummaryGroup);
        summaryValue->setObjectName("summaryValue");
        summaryValue->setWordWrap(true);

        gridLayout->addWidget(summaryValue, 3, 1, 1, 1);


        verticalLayout_2->addWidget(projectSummaryGroup);

        dxfView = new DxfViewWidget(dxfTab);
        dxfView->setObjectName("dxfView");

        verticalLayout_2->addWidget(dxfView);

        mainTabWidget->addTab(dxfTab, QString());
        settingsTab = new QWidget();
        settingsTab->setObjectName("settingsTab");
        verticalLayout_3 = new QVBoxLayout(settingsTab);
        verticalLayout_3->setObjectName("verticalLayout_3");
        settingsTabWidget = new QTabWidget(settingsTab);
        settingsTabWidget->setObjectName("settingsTabWidget");
        toolSettingsTab = new QWidget();
        toolSettingsTab->setObjectName("toolSettingsTab");
        verticalLayout_12 = new QVBoxLayout(toolSettingsTab);
        verticalLayout_12->setObjectName("verticalLayout_12");
        toolStationsGroup = new QGroupBox(toolSettingsTab);
        toolStationsGroup->setObjectName("toolStationsGroup");
        verticalLayout_5 = new QVBoxLayout(toolStationsGroup);
        verticalLayout_5->setObjectName("verticalLayout_5");
        toolStationTable = new QTableWidget(toolStationsGroup);
        if (toolStationTable->columnCount() < 5)
            toolStationTable->setColumnCount(5);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        toolStationTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        toolStationTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        toolStationTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        toolStationTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        QTableWidgetItem *__qtablewidgetitem4 = new QTableWidgetItem();
        toolStationTable->setHorizontalHeaderItem(4, __qtablewidgetitem4);
        toolStationTable->setObjectName("toolStationTable");
        toolStationTable->setColumnCount(5);

        verticalLayout_5->addWidget(toolStationTable);


        verticalLayout_12->addWidget(toolStationsGroup);

        settingsNotes = new QPlainTextEdit(toolSettingsTab);
        settingsNotes->setObjectName("settingsNotes");
        settingsNotes->setReadOnly(true);

        verticalLayout_12->addWidget(settingsNotes);

        settingsTabWidget->addTab(toolSettingsTab, QString());
        colorSettingsTab = new QWidget();
        colorSettingsTab->setObjectName("colorSettingsTab");
        verticalLayout_13 = new QVBoxLayout(colorSettingsTab);
        verticalLayout_13->setObjectName("verticalLayout_13");
        viewColorsGroup = new QGroupBox(colorSettingsTab);
        viewColorsGroup->setObjectName("viewColorsGroup");
        gridLayout_2 = new QGridLayout(viewColorsGroup);
        gridLayout_2->setObjectName("gridLayout_2");
        labelSelectedEntityColor = new QLabel(viewColorsGroup);
        labelSelectedEntityColor->setObjectName("labelSelectedEntityColor");

        gridLayout_2->addWidget(labelSelectedEntityColor, 0, 0, 1, 1);

        snapPreviewColorButton = new QPushButton(viewColorsGroup);
        snapPreviewColorButton->setObjectName("snapPreviewColorButton");

        gridLayout_2->addWidget(snapPreviewColorButton, 8, 1, 1, 1);

        armedEntityColorButton = new QPushButton(viewColorsGroup);
        armedEntityColorButton->setObjectName("armedEntityColorButton");

        gridLayout_2->addWidget(armedEntityColorButton, 2, 1, 1, 1);

        labelArmedEntityColor = new QLabel(viewColorsGroup);
        labelArmedEntityColor->setObjectName("labelArmedEntityColor");

        gridLayout_2->addWidget(labelArmedEntityColor, 2, 0, 1, 1);

        hoverEntityColorButton = new QPushButton(viewColorsGroup);
        hoverEntityColorButton->setObjectName("hoverEntityColorButton");

        gridLayout_2->addWidget(hoverEntityColorButton, 1, 1, 1, 1);

        labelBladeLineColor = new QLabel(viewColorsGroup);
        labelBladeLineColor->setObjectName("labelBladeLineColor");

        gridLayout_2->addWidget(labelBladeLineColor, 3, 0, 1, 1);

        labelActiveBladeLineColor = new QLabel(viewColorsGroup);
        labelActiveBladeLineColor->setObjectName("labelActiveBladeLineColor");

        gridLayout_2->addWidget(labelActiveBladeLineColor, 4, 0, 1, 1);

        viewBackgroundColorButton = new QPushButton(viewColorsGroup);
        viewBackgroundColorButton->setObjectName("viewBackgroundColorButton");

        gridLayout_2->addWidget(viewBackgroundColorButton, 10, 1, 1, 1);

        labelSnapPreviewColor = new QLabel(viewColorsGroup);
        labelSnapPreviewColor->setObjectName("labelSnapPreviewColor");

        gridLayout_2->addWidget(labelSnapPreviewColor, 8, 0, 1, 1);

        handleHoverFillColorButton = new QPushButton(viewColorsGroup);
        handleHoverFillColorButton->setObjectName("handleHoverFillColorButton");

        gridLayout_2->addWidget(handleHoverFillColorButton, 7, 1, 1, 1);

        labelHoverEntityColor = new QLabel(viewColorsGroup);
        labelHoverEntityColor->setObjectName("labelHoverEntityColor");

        gridLayout_2->addWidget(labelHoverEntityColor, 1, 0, 1, 1);

        activeBladeLineColorButton = new QPushButton(viewColorsGroup);
        activeBladeLineColorButton->setObjectName("activeBladeLineColorButton");

        gridLayout_2->addWidget(activeBladeLineColorButton, 4, 1, 1, 1);

        handleHoverColorButton = new QPushButton(viewColorsGroup);
        handleHoverColorButton->setObjectName("handleHoverColorButton");

        gridLayout_2->addWidget(handleHoverColorButton, 6, 1, 1, 1);

        labelViewBackgroundColor = new QLabel(viewColorsGroup);
        labelViewBackgroundColor->setObjectName("labelViewBackgroundColor");

        gridLayout_2->addWidget(labelViewBackgroundColor, 10, 0, 1, 1);

        selectedEntityColorButton = new QPushButton(viewColorsGroup);
        selectedEntityColorButton->setObjectName("selectedEntityColorButton");

        gridLayout_2->addWidget(selectedEntityColorButton, 0, 1, 1, 1);

        labelHandleHoverColor = new QLabel(viewColorsGroup);
        labelHandleHoverColor->setObjectName("labelHandleHoverColor");

        gridLayout_2->addWidget(labelHandleHoverColor, 6, 0, 1, 1);

        labelBreakPreviewColor = new QLabel(viewColorsGroup);
        labelBreakPreviewColor->setObjectName("labelBreakPreviewColor");

        gridLayout_2->addWidget(labelBreakPreviewColor, 9, 0, 1, 1);

        breakPreviewColorButton = new QPushButton(viewColorsGroup);
        breakPreviewColorButton->setObjectName("breakPreviewColorButton");

        gridLayout_2->addWidget(breakPreviewColorButton, 9, 1, 1, 1);

        handleColorButton = new QPushButton(viewColorsGroup);
        handleColorButton->setObjectName("handleColorButton");

        gridLayout_2->addWidget(handleColorButton, 5, 1, 1, 1);

        labelHandleColor = new QLabel(viewColorsGroup);
        labelHandleColor->setObjectName("labelHandleColor");

        gridLayout_2->addWidget(labelHandleColor, 5, 0, 1, 1);

        bladeLineColorButton = new QPushButton(viewColorsGroup);
        bladeLineColorButton->setObjectName("bladeLineColorButton");

        gridLayout_2->addWidget(bladeLineColorButton, 3, 1, 1, 1);

        labelHandleHoverFillColor = new QLabel(viewColorsGroup);
        labelHandleHoverFillColor->setObjectName("labelHandleHoverFillColor");

        gridLayout_2->addWidget(labelHandleHoverFillColor, 7, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer, 4, 2, 1, 1);


        verticalLayout_13->addWidget(viewColorsGroup);

        flagSettingsGroup = new QGroupBox(colorSettingsTab);
        flagSettingsGroup->setObjectName("flagSettingsGroup");
        gridLayout_3 = new QGridLayout(flagSettingsGroup);
        gridLayout_3->setObjectName("gridLayout_3");
        labelStartFlagColor = new QLabel(flagSettingsGroup);
        labelStartFlagColor->setObjectName("labelStartFlagColor");

        gridLayout_3->addWidget(labelStartFlagColor, 0, 0, 1, 1);

        startFlagColorButton = new QPushButton(flagSettingsGroup);
        startFlagColorButton->setObjectName("startFlagColorButton");

        gridLayout_3->addWidget(startFlagColorButton, 0, 1, 1, 1);

        labelEndFlagColor = new QLabel(flagSettingsGroup);
        labelEndFlagColor->setObjectName("labelEndFlagColor");

        gridLayout_3->addWidget(labelEndFlagColor, 1, 0, 1, 1);

        endFlagColorButton = new QPushButton(flagSettingsGroup);
        endFlagColorButton->setObjectName("endFlagColorButton");

        gridLayout_3->addWidget(endFlagColorButton, 1, 1, 1, 1);

        labelFlagScale = new QLabel(flagSettingsGroup);
        labelFlagScale->setObjectName("labelFlagScale");

        gridLayout_3->addWidget(labelFlagScale, 2, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        flagScaleSlider = new QSlider(flagSettingsGroup);
        flagScaleSlider->setObjectName("flagScaleSlider");
        flagScaleSlider->setMinimum(50);
        flagScaleSlider->setMaximum(300);
        flagScaleSlider->setSingleStep(5);
        flagScaleSlider->setPageStep(10);
        flagScaleSlider->setValue(100);
        flagScaleSlider->setOrientation(Qt::Orientation::Horizontal);

        horizontalLayout_2->addWidget(flagScaleSlider);

        flagScaleValueLabel = new QLabel(flagSettingsGroup);
        flagScaleValueLabel->setObjectName("flagScaleValueLabel");
        flagScaleValueLabel->setMinimumSize(QSize(48, 0));

        horizontalLayout_2->addWidget(flagScaleValueLabel);


        gridLayout_3->addLayout(horizontalLayout_2, 2, 1, 1, 1);

        labelFlagFill = new QLabel(flagSettingsGroup);
        labelFlagFill->setObjectName("labelFlagFill");

        gridLayout_3->addWidget(labelFlagFill, 3, 0, 1, 1);

        fillFlagsCheckBox = new QCheckBox(flagSettingsGroup);
        fillFlagsCheckBox->setObjectName("fillFlagsCheckBox");
        fillFlagsCheckBox->setChecked(true);

        gridLayout_3->addWidget(fillFlagsCheckBox, 3, 1, 1, 1);

        labelFlagPreview = new QLabel(flagSettingsGroup);
        labelFlagPreview->setObjectName("labelFlagPreview");

        gridLayout_3->addWidget(labelFlagPreview, 4, 0, 1, 1);

        flagPreviewLabel = new QLabel(flagSettingsGroup);
        flagPreviewLabel->setObjectName("flagPreviewLabel");
        flagPreviewLabel->setMinimumSize(QSize(220, 120));
        flagPreviewLabel->setFrameShape(QFrame::Shape::StyledPanel);

        gridLayout_3->addWidget(flagPreviewLabel, 4, 1, 1, 1);


        verticalLayout_13->addWidget(flagSettingsGroup);

        portSettingsGroup = new QGroupBox(colorSettingsTab);
        portSettingsGroup->setObjectName("portSettingsGroup");
        gridLayout_4 = new QGridLayout(portSettingsGroup);
        gridLayout_4->setObjectName("gridLayout_4");
        labelPortMinLength = new QLabel(portSettingsGroup);
        labelPortMinLength->setObjectName("labelPortMinLength");

        gridLayout_4->addWidget(labelPortMinLength, 0, 0, 1, 1);

        portMinLengthSpinBox = new QDoubleSpinBox(portSettingsGroup);
        portMinLengthSpinBox->setObjectName("portMinLengthSpinBox");
        portMinLengthSpinBox->setDecimals(3);
        portMinLengthSpinBox->setMinimum(0.000000000000000);
        portMinLengthSpinBox->setMaximum(1000.000000000000000);
        portMinLengthSpinBox->setSingleStep(0.500000000000000);
        portMinLengthSpinBox->setValue(2.000000000000000);

        gridLayout_4->addWidget(portMinLengthSpinBox, 0, 1, 1, 1);

        labelPortMaxLength = new QLabel(portSettingsGroup);
        labelPortMaxLength->setObjectName("labelPortMaxLength");

        gridLayout_4->addWidget(labelPortMaxLength, 1, 0, 1, 1);

        portMaxLengthSpinBox = new QDoubleSpinBox(portSettingsGroup);
        portMaxLengthSpinBox->setObjectName("portMaxLengthSpinBox");
        portMaxLengthSpinBox->setDecimals(3);
        portMaxLengthSpinBox->setMinimum(0.000000000000000);
        portMaxLengthSpinBox->setMaximum(1000.000000000000000);
        portMaxLengthSpinBox->setSingleStep(0.500000000000000);
        portMaxLengthSpinBox->setValue(6.000000000000000);

        gridLayout_4->addWidget(portMaxLengthSpinBox, 1, 1, 1, 1);

        labelPortColor = new QLabel(portSettingsGroup);
        labelPortColor->setObjectName("labelPortColor");

        gridLayout_4->addWidget(labelPortColor, 2, 0, 1, 1);

        portColorButton = new QPushButton(portSettingsGroup);
        portColorButton->setObjectName("portColorButton");

        gridLayout_4->addWidget(portColorButton, 2, 1, 1, 1);


        verticalLayout_13->addWidget(portSettingsGroup);

        settingsTabWidget->addTab(colorSettingsTab, QString());

        verticalLayout_3->addWidget(settingsTabWidget);

        mainTabWidget->addTab(settingsTab, QString());
        simulationTab = new QWidget();
        simulationTab->setObjectName("simulationTab");
        verticalLayout_4 = new QVBoxLayout(simulationTab);
        verticalLayout_4->setObjectName("verticalLayout_4");
        simulationOverviewGroup = new QGroupBox(simulationTab);
        simulationOverviewGroup->setObjectName("simulationOverviewGroup");
        verticalLayout_6 = new QVBoxLayout(simulationOverviewGroup);
        verticalLayout_6->setObjectName("verticalLayout_6");
        simulationStatusLabel = new QLabel(simulationOverviewGroup);
        simulationStatusLabel->setObjectName("simulationStatusLabel");
        simulationStatusLabel->setWordWrap(true);

        verticalLayout_6->addWidget(simulationStatusLabel);

        simulationEventList = new QListWidget(simulationOverviewGroup);
        simulationEventList->setObjectName("simulationEventList");

        verticalLayout_6->addWidget(simulationEventList);


        verticalLayout_4->addWidget(simulationOverviewGroup);

        mainTabWidget->addTab(simulationTab, QString());

        verticalLayout->addWidget(mainTabWidget);

        MainWindow->setCentralWidget(centralwidget);
        layerDock = new QDockWidget(MainWindow);
        layerDock->setObjectName("layerDock");
        dockWidgetContents = new QWidget();
        dockWidgetContents->setObjectName("dockWidgetContents");
        verticalLayout_7 = new QVBoxLayout(dockWidgetContents);
        verticalLayout_7->setObjectName("verticalLayout_7");
        layerTree = new QTreeWidget(dockWidgetContents);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        layerTree->setHeaderItem(__qtreewidgetitem);
        layerTree->setObjectName("layerTree");

        verticalLayout_7->addWidget(layerTree);

        layerDock->setWidget(dockWidgetContents);
        MainWindow->addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, layerDock);
        entityPropertiesDock = new QDockWidget(MainWindow);
        entityPropertiesDock->setObjectName("entityPropertiesDock");
        dockWidgetContents_2 = new QWidget();
        dockWidgetContents_2->setObjectName("dockWidgetContents_2");
        verticalLayout_8 = new QVBoxLayout(dockWidgetContents_2);
        verticalLayout_8->setObjectName("verticalLayout_8");
        entityPropertiesTree = new QTreeWidget(dockWidgetContents_2);
        entityPropertiesTree->setObjectName("entityPropertiesTree");

        verticalLayout_8->addWidget(entityPropertiesTree);

        entityPropertiesDock->setWidget(dockWidgetContents_2);
        MainWindow->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, entityPropertiesDock);
        bladeLineDock = new QDockWidget(MainWindow);
        bladeLineDock->setObjectName("bladeLineDock");
        dockWidgetContents_3 = new QWidget();
        dockWidgetContents_3->setObjectName("dockWidgetContents_3");
        verticalLayout_9 = new QVBoxLayout(dockWidgetContents_3);
        verticalLayout_9->setObjectName("verticalLayout_9");
        bladeLinePropertiesTree = new QTreeWidget(dockWidgetContents_3);
        bladeLinePropertiesTree->setObjectName("bladeLinePropertiesTree");

        verticalLayout_9->addWidget(bladeLinePropertiesTree);

        bladeLineDock->setWidget(dockWidgetContents_3);
        MainWindow->addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, bladeLineDock);
        operationDock = new QDockWidget(MainWindow);
        operationDock->setObjectName("operationDock");
        dockWidgetContents_4 = new QWidget();
        dockWidgetContents_4->setObjectName("dockWidgetContents_4");
        verticalLayout_10 = new QVBoxLayout(dockWidgetContents_4);
        verticalLayout_10->setObjectName("verticalLayout_10");
        operationList = new QListWidget(dockWidgetContents_4);
        operationList->setObjectName("operationList");

        verticalLayout_10->addWidget(operationList);

        operationDock->setWidget(dockWidgetContents_4);
        MainWindow->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, operationDock);
        logDock = new QDockWidget(MainWindow);
        logDock->setObjectName("logDock");
        dockWidgetContents_5 = new QWidget();
        dockWidgetContents_5->setObjectName("dockWidgetContents_5");
        verticalLayout_11 = new QVBoxLayout(dockWidgetContents_5);
        verticalLayout_11->setObjectName("verticalLayout_11");
        logList = new QListWidget(dockWidgetContents_5);
        logList->setObjectName("logList");

        verticalLayout_11->addWidget(logList);

        logDock->setWidget(dockWidgetContents_5);
        MainWindow->addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, logDock);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1360, 22));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName("menuFile");
        menuEdit = new QMenu(menubar);
        menuEdit->setObjectName("menuEdit");
        menuView = new QMenu(menubar);
        menuView->setObjectName("menuView");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuEdit->menuAction());
        menubar->addAction(menuView->menuAction());
        menuFile->addAction(actionOpenDxf);
        menuFile->addSeparator();
        menuFile->addAction(actionExit);
        menuEdit->addAction(actionStartNewBladeLine);
        menuEdit->addAction(actionAddSelectedToBladeLine);
        menuEdit->addAction(actionRemoveLastFromBladeLine);
        menuEdit->addAction(actionClearBladeLine);
        menuEdit->addSeparator();
        menuEdit->addAction(actionCreateBreak);
        menuEdit->addAction(actionCreateFillet);
        menuView->addAction(actionFitView);

        retranslateUi(MainWindow);

        mainTabWidget->setCurrentIndex(0);
        settingsTabWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "FoxBender", nullptr));
        actionOpenDxf->setText(QCoreApplication::translate("MainWindow", "Open DXF...", nullptr));
#if QT_CONFIG(shortcut)
        actionOpenDxf->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+O", nullptr));
#endif // QT_CONFIG(shortcut)
        actionExit->setText(QCoreApplication::translate("MainWindow", "Exit", nullptr));
#if QT_CONFIG(shortcut)
        actionExit->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Q", nullptr));
#endif // QT_CONFIG(shortcut)
        actionFitView->setText(QCoreApplication::translate("MainWindow", "Fit View", nullptr));
#if QT_CONFIG(shortcut)
        actionFitView->setShortcut(QCoreApplication::translate("MainWindow", "F", nullptr));
#endif // QT_CONFIG(shortcut)
        actionCreateFillet->setText(QCoreApplication::translate("MainWindow", "Create Fillet...", nullptr));
#if QT_CONFIG(shortcut)
        actionCreateFillet->setShortcut(QCoreApplication::translate("MainWindow", "Shift+F", nullptr));
#endif // QT_CONFIG(shortcut)
        actionCreateBreak->setText(QCoreApplication::translate("MainWindow", "Break", nullptr));
#if QT_CONFIG(shortcut)
        actionCreateBreak->setShortcut(QCoreApplication::translate("MainWindow", "B", nullptr));
#endif // QT_CONFIG(shortcut)
        actionAddSelectedToBladeLine->setText(QCoreApplication::translate("MainWindow", "Add Selected To BladeLine", nullptr));
#if QT_CONFIG(shortcut)
        actionAddSelectedToBladeLine->setShortcut(QCoreApplication::translate("MainWindow", "A", nullptr));
#endif // QT_CONFIG(shortcut)
        actionClearBladeLine->setText(QCoreApplication::translate("MainWindow", "Remove Active BladeLine", nullptr));
        actionRemoveLastFromBladeLine->setText(QCoreApplication::translate("MainWindow", "Remove Last From Active BladeLine", nullptr));
#if QT_CONFIG(shortcut)
        actionRemoveLastFromBladeLine->setShortcut(QCoreApplication::translate("MainWindow", "Shift+Backspace", nullptr));
#endif // QT_CONFIG(shortcut)
        actionStartNewBladeLine->setText(QCoreApplication::translate("MainWindow", "Start New BladeLine", nullptr));
#if QT_CONFIG(shortcut)
        actionStartNewBladeLine->setShortcut(QCoreApplication::translate("MainWindow", "Shift+A", nullptr));
#endif // QT_CONFIG(shortcut)
        projectSummaryGroup->setTitle(QCoreApplication::translate("MainWindow", "Project Summary", nullptr));
        labelProjectName->setText(QCoreApplication::translate("MainWindow", "Project", nullptr));
        projectNameValue->setText(QCoreApplication::translate("MainWindow", "Untitled", nullptr));
        labelDxfPath->setText(QCoreApplication::translate("MainWindow", "DXF", nullptr));
        dxfPathValue->setText(QCoreApplication::translate("MainWindow", "No DXF loaded", nullptr));
        labelProjectVersion->setText(QCoreApplication::translate("MainWindow", "Version", nullptr));
        projectVersionValue->setText(QCoreApplication::translate("MainWindow", "1", nullptr));
        labelSummary->setText(QCoreApplication::translate("MainWindow", "Status", nullptr));
        summaryValue->setText(QCoreApplication::translate("MainWindow", "Summary placeholder", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(dxfTab), QCoreApplication::translate("MainWindow", "DXF / Polku", nullptr));
        toolStationsGroup->setTitle(QCoreApplication::translate("MainWindow", "Tool Stations", nullptr));
        QTableWidgetItem *___qtablewidgetitem = toolStationTable->horizontalHeaderItem(0);
        ___qtablewidgetitem->setText(QCoreApplication::translate("MainWindow", "Enabled", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = toolStationTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("MainWindow", "Tool", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = toolStationTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("MainWindow", "Offset mm", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = toolStationTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("MainWindow", "Width mm", nullptr));
        QTableWidgetItem *___qtablewidgetitem4 = toolStationTable->horizontalHeaderItem(4);
        ___qtablewidgetitem4->setText(QCoreApplication::translate("MainWindow", "Timeout ms", nullptr));
        settingsNotes->setPlainText(QCoreApplication::translate("MainWindow", "settings.json is now active. Tool station values are loaded at startup and saved immediately when edited. Additional calibration settings will be extended here next.", nullptr));
        settingsTabWidget->setTabText(settingsTabWidget->indexOf(toolSettingsTab), QCoreApplication::translate("MainWindow", "Ty\303\266kalut", nullptr));
        viewColorsGroup->setTitle(QCoreApplication::translate("MainWindow", "View Colors", nullptr));
        labelSelectedEntityColor->setText(QCoreApplication::translate("MainWindow", "Selected entity", nullptr));
        snapPreviewColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        armedEntityColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        labelArmedEntityColor->setText(QCoreApplication::translate("MainWindow", "Armed tool entity", nullptr));
        hoverEntityColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        labelBladeLineColor->setText(QCoreApplication::translate("MainWindow", "BladeLine", nullptr));
        labelActiveBladeLineColor->setText(QCoreApplication::translate("MainWindow", "Active BladeLine", nullptr));
        viewBackgroundColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        labelSnapPreviewColor->setText(QCoreApplication::translate("MainWindow", "Snap preview", nullptr));
        handleHoverFillColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        labelHoverEntityColor->setText(QCoreApplication::translate("MainWindow", "Hover entity", nullptr));
        activeBladeLineColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        handleHoverColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        labelViewBackgroundColor->setText(QCoreApplication::translate("MainWindow", "View background", nullptr));
        selectedEntityColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        labelHandleHoverColor->setText(QCoreApplication::translate("MainWindow", "Handle hover stroke", nullptr));
        labelBreakPreviewColor->setText(QCoreApplication::translate("MainWindow", "Break preview", nullptr));
        breakPreviewColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        handleColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        labelHandleColor->setText(QCoreApplication::translate("MainWindow", "Handle", nullptr));
        bladeLineColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        labelHandleHoverFillColor->setText(QCoreApplication::translate("MainWindow", "Handle hover fill", nullptr));
        flagSettingsGroup->setTitle(QCoreApplication::translate("MainWindow", "BladeLine Flags", nullptr));
        labelStartFlagColor->setText(QCoreApplication::translate("MainWindow", "Start flag", nullptr));
        startFlagColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        labelEndFlagColor->setText(QCoreApplication::translate("MainWindow", "End flag", nullptr));
        endFlagColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        labelFlagScale->setText(QCoreApplication::translate("MainWindow", "Flag size", nullptr));
        flagScaleValueLabel->setText(QCoreApplication::translate("MainWindow", "100 %", nullptr));
        labelFlagFill->setText(QCoreApplication::translate("MainWindow", "Fill flags", nullptr));
        fillFlagsCheckBox->setText(QCoreApplication::translate("MainWindow", "Filled", nullptr));
        labelFlagPreview->setText(QCoreApplication::translate("MainWindow", "Preview", nullptr));
        flagPreviewLabel->setText(QString());
        portSettingsGroup->setTitle(QCoreApplication::translate("MainWindow", "Port Detection", nullptr));
        labelPortMinLength->setText(QCoreApplication::translate("MainWindow", "Min port length", nullptr));
        portMinLengthSpinBox->setSuffix(QCoreApplication::translate("MainWindow", " mm", nullptr));
        labelPortMaxLength->setText(QCoreApplication::translate("MainWindow", "Max port length", nullptr));
        portMaxLengthSpinBox->setSuffix(QCoreApplication::translate("MainWindow", " mm", nullptr));
        labelPortColor->setText(QCoreApplication::translate("MainWindow", "Port highlight", nullptr));
        portColorButton->setText(QCoreApplication::translate("MainWindow", "Choose color", nullptr));
        settingsTabWidget->setTabText(settingsTabWidget->indexOf(colorSettingsTab), QCoreApplication::translate("MainWindow", "V\303\244rit", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(settingsTab), QCoreApplication::translate("MainWindow", "Asetukset", nullptr));
        simulationOverviewGroup->setTitle(QCoreApplication::translate("MainWindow", "Simulation Overview", nullptr));
        simulationStatusLabel->setText(QCoreApplication::translate("MainWindow", "2D simulation model will be attached to this tab in the next sprint.", nullptr));
        mainTabWidget->setTabText(mainTabWidget->indexOf(simulationTab), QCoreApplication::translate("MainWindow", "Simulointi", nullptr));
        layerDock->setWindowTitle(QCoreApplication::translate("MainWindow", "Layer Panel", nullptr));
        entityPropertiesDock->setWindowTitle(QCoreApplication::translate("MainWindow", "Entity Properties", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = entityPropertiesTree->headerItem();
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("MainWindow", "Value", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("MainWindow", "Property", nullptr));
        bladeLineDock->setWindowTitle(QCoreApplication::translate("MainWindow", "BladeLine Properties", nullptr));
        QTreeWidgetItem *___qtreewidgetitem1 = bladeLinePropertiesTree->headerItem();
        ___qtreewidgetitem1->setText(1, QCoreApplication::translate("MainWindow", "Value", nullptr));
        ___qtreewidgetitem1->setText(0, QCoreApplication::translate("MainWindow", "Property", nullptr));
        operationDock->setWindowTitle(QCoreApplication::translate("MainWindow", "Operation List", nullptr));
        logDock->setWindowTitle(QCoreApplication::translate("MainWindow", "Log / Warnings", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "&File", nullptr));
        menuEdit->setTitle(QCoreApplication::translate("MainWindow", "&Edit", nullptr));
        menuView->setTitle(QCoreApplication::translate("MainWindow", "&View", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
