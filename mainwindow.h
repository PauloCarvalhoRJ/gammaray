#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

namespace spectral {
    struct array;
}

class QAction;
class QMenu;
class File; //GammaRay API
class Attribute; //GammaRay API
class CartesianGrid;
class PointSet;
class VariogramModel;
class UnivariateCategoryClassification;

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

    /**
     *  Adds the data file given its path to the project.
     *  Assumes there is an open project.
     */
    void doAddDataFile( const QString filePath );

    //QMainWindow interface
public:
    void dragEnterEvent( QDragEnterEvent *e );
    void dropEvent( QDropEvent *e );

public slots:
    void showAbout();
    void showSetup();
    void newProject();
    void closeProject();
    void openRecentProject();
    void openKriging();
    void openIKContinuous();
    void openIKCategorical();
    void showMessagesConsoleCustomContextMenu(const QPoint &pt);
    void openIKPostProcessing();
    void openCokriging();
    void openImageJockey();
    void openSGSIM();
    void openCokrigingNewcokb3d();

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
    //saves the right clicked attribute in the last call to onSVD() slot.
    Attribute *_right_clicked_attribute_onSVD;

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
    void onClassifyInto();
    void onPerformClassifyInto();
    void onLookForDuplicates();
    void onEditWithExternalProgram();
    void onClearMessages();
    void onClassifyWith();
    void onMapAs();
    void onSoftIndicatorCalib();
    void onFreeLoadedData();
    void onFFT();
    void onNDVEstimation();
    void onResampleGrid();
    void onMultiVariogram();
    void onHistpltsim();
    void onRFFT();
    void onUpdateStatusBar();
    void onMachineLearning();
    void onDeleteVariable();
    void onPreviewRFFTImageJockey(CartesianGrid *cgWithFFT,
                                  int indexOfVariableWithAmplitudes,
                                  int indexOfVariableWithPhases);
    void onSVD();
    void onSumOfFactorsWasComputed(spectral::array *sumOfFactors); //called to save the sum of SVD factors in the dialog called in onSVD()
    void onCalculator();
    void onNewAttribute();
	void onQuickView();

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

    /**
     * The pointer to the dynamic sub-menu "Classify into" of the project tree context menu.
     */
    QMenu* m_subMenuClassifyInto;
    /**
      * Creates the dynamic items of sub-menu "Classify into".
      */
    void makeMenuClassifyInto();
    /**
     * The pointer to the categorical classification defined by the user upon calling onClassifyInto().
     */
    UnivariateCategoryClassification *m_ucc;


    /**
     * The pointer to the dynamic sub-menu "Classify with" of the project tree context menu.
     */
    QMenu* m_subMenuClassifyWith;
    /**
      * Creates the dynamic items of sub-menu "Classify with".
      */
    void makeMenuClassifyWith();

    /**
     * The pointer to the dynamic sub-menu "Map as" of the project tree context menu.
     */
    QMenu* m_subMenuMapAs;
    /**
      * Creates the dynamic items of sub-menu "Map as".
      */
    void makeMenuMapAs();
};

#endif // MAINWINDOW_H
