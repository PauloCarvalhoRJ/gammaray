#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class QAction;
class QMenu;
class File; //GammaRay API
class Attribute; //GammaRay API
class CartesianGrid;
class PointSet;
class VariogramModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

friend class Application;

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    /** Updates UI with current application state. */
    void displayApplicationInfo();

    /** Prints messages with the given style to the message text box.
    * Supported styles: information, warning, error.  A wrong or empty style alias defaults to error.
    */
    void log_message( const QString message, const QString style );

public slots:
    void showAbout();
    void showSetup();
    void newProject();
    void closeProject();
    void openRecentProject();
    void openKriging();
    void openIKContinuous();
    void openIKCategorical();

private:
    Ui::MainWindow *ui;
    //this is called upon quiting.
    void closeEvent(QCloseEvent *);
    //the most recently used project actions
    //to make MRU menu in File menu
    enum { MaxRecentProjects = 10};
    QAction *_MRUactions[ MaxRecentProjects ];
    //opens the given project (directory path)
    void openProject( const QString path );
    //updates the MRU project list.  The given project is the most recently opened.
    void setCurrentMRU( const QString path );
    //returns the directory name (without the path)
    //the directory name is considered the project name
    QString strippedName(const QString &fullDirPath);
    //update MRU menus according to the saved list
    //of recently opened projects in OS registry
    void updateRecentProjectActions();
    //saves project tree UI state
    void saveProjectTreeUIState();
    //restores project tree UI state
    void restoreProjectTreeUIState();
    //restores project tree UI state
    void restoreProjectTreeUIState2();
    //updates project tree style sheet (updates visualization)
    //must be called upon changes in project objects hierarchy
    void refreshTreeStyle();
    //project tree context menu
    QMenu *_projectContextMenu;
    //project header context menu
    QMenu *_projectHeaderContextMenu;
    //pointer to right clicked file (set in onProjectContextMenu() slot)
    File *_right_clicked_file;
    //pointer to right clicked attribute (set in onProjectContextMenu() slot)
    Attribute *_right_clicked_attribute;
    //pointer to the second right clicked attribute (set in onProjectContextMenu() slot)
    Attribute *_right_clicked_attribute2;
    //pointer to right clicked Cartesian Grid (set in onProjectContextMenu() slot)
    CartesianGrid* _right_clicked_cartesian_grid;
    //pointer to right clicked Point Set (set in onProjectContextMenu() slot)
    PointSet* _right_clicked_point_set;

private slots:
    void onProjectContextMenu(const QPoint &mouse_location);
    void onProjectHeaderContextMenu(const QPoint &mouse_location);
    void onAddDataFile();
    void onRemoveFile();
    void onRemoveAndDeleteFile();
    void onSeeMetadata();
    void onHistogram();
    void onLocMap();
    void onXPlot();
    void onPixelPlt();
    void onProbPlt();
    void onQpplt();
    void onSeeProjectPath();
    void onOpenProjectPath();
    void onVariogramAnalysis();
    void onDecluster();
    void onSetNDV();
    void onGetPoints( );
    void onNScore();
    void onDisplayPlot();
    void onDisplayExperimentalVariogram();
    void onFitVModelToExperimentalVariogram();
    void onDisplayVariogramModel();
    void onCreateVariogramModel();
    void onCreateGrid();
    void onCleanTmpFiles();
    void onAddCoord();
    void onDistrModel();
    void onBidistrModel();
    void onCreateThresholdCDF();
    void onEdit();
    void onCreateCategoryPDF();
    void onCreateCategoryDefinition();
private:
    /**
     * This method is used to create (vm == nullptr) or review (vm != nullptr)
     * an existing variogram model.
     */
    void createOrReviewVariogramModel( VariogramModel* vm );
    /**
      * Returns a list of attributes selected by the user.
      * If none was selected, returns an empty list.
      */
    QList<Attribute*> getSelectedAttributes();
};

#endif // MAINWINDOW_H
