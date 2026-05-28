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
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPlainTextEdit>
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
    QGroupBox *toolStationsGroup;
    QVBoxLayout *verticalLayout_5;
    QTableWidget *toolStationTable;
    QPlainTextEdit *settingsNotes;
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
        MainWindow->resize(1360, 860);
        actionOpenDxf = new QAction(MainWindow);
        actionOpenDxf->setObjectName("actionOpenDxf");
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
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
        toolStationsGroup = new QGroupBox(settingsTab);
        toolStationsGroup->setObjectName("toolStationsGroup");
        verticalLayout_5 = new QVBoxLayout(toolStationsGroup);
        verticalLayout_5->setObjectName("verticalLayout_5");
        toolStationTable = new QTableWidget(toolStationsGroup);
        if (toolStationTable->columnCount() < 4)
            toolStationTable->setColumnCount(4);
        QTableWidgetItem *__qtablewidgetitem = new QTableWidgetItem();
        toolStationTable->setHorizontalHeaderItem(0, __qtablewidgetitem);
        QTableWidgetItem *__qtablewidgetitem1 = new QTableWidgetItem();
        toolStationTable->setHorizontalHeaderItem(1, __qtablewidgetitem1);
        QTableWidgetItem *__qtablewidgetitem2 = new QTableWidgetItem();
        toolStationTable->setHorizontalHeaderItem(2, __qtablewidgetitem2);
        QTableWidgetItem *__qtablewidgetitem3 = new QTableWidgetItem();
        toolStationTable->setHorizontalHeaderItem(3, __qtablewidgetitem3);
        toolStationTable->setObjectName("toolStationTable");
        toolStationTable->setColumnCount(4);

        verticalLayout_5->addWidget(toolStationTable);


        verticalLayout_3->addWidget(toolStationsGroup);

        settingsNotes = new QPlainTextEdit(settingsTab);
        settingsNotes->setObjectName("settingsNotes");
        settingsNotes->setReadOnly(true);

        verticalLayout_3->addWidget(settingsNotes);

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
        menubar->setGeometry(QRect(0, 0, 1360, 30));
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

        retranslateUi(MainWindow);

        mainTabWidget->setCurrentIndex(0);


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
        ___qtablewidgetitem->setText(QCoreApplication::translate("MainWindow", "Tool", nullptr));
        QTableWidgetItem *___qtablewidgetitem1 = toolStationTable->horizontalHeaderItem(1);
        ___qtablewidgetitem1->setText(QCoreApplication::translate("MainWindow", "Offset mm", nullptr));
        QTableWidgetItem *___qtablewidgetitem2 = toolStationTable->horizontalHeaderItem(2);
        ___qtablewidgetitem2->setText(QCoreApplication::translate("MainWindow", "Width mm", nullptr));
        QTableWidgetItem *___qtablewidgetitem3 = toolStationTable->horizontalHeaderItem(3);
        ___qtablewidgetitem3->setText(QCoreApplication::translate("MainWindow", "Timeout ms", nullptr));
        settingsNotes->setPlainText(QCoreApplication::translate("MainWindow", "settings.json storage, calibration values and UI preferences will be added here next.", nullptr));
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
