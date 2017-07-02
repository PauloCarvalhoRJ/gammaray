#ifndef APPLICATION_H
#define APPLICATION_H

#include <QString>
#include <QByteArray>
#include "mainwindow.h"

class Project;

/**
 * @brief The Application class holds application state, including global variables and OS registry settings.
 * It has just one instance.
 */
class Application
{
public:
    static Application* instance();
    Project* getProject();

    /**
      * Opens the given directory as a GammaRay project.
      * If the directory has never been used as a project, then
      * some files will be created and any other
      * files and subdirectories the given directory may contain will be ignored.
     */
    void openProject( const QString directory_path );

    /**
     * Sets the main window pointer.
     */
    void setMainWindow( MainWindow* mw );

    /**
     * @brief Closes the currently opened project.
     * If there is no opened project, nothing happens.
     */
    void closeProject();

    //!@{
    //! Reads and saves the GSLib executables path from OS registry.
    QString getGSLibPathSetting();
    void setGSLibPathSetting( const QString path );
    //!@}

    //!@{
    //! Reads and saves the main window splitter state from OS registry.
    QByteArray getMainWindowSplitterSetting();
    void setMainWindowSplitterSetting( const QByteArray state );
    //!@}

    //!@{
    //! Reads and saves the contents/message splitter state from OS registry.
    QByteArray getContentsMessageSplitterSetting();
    void setContentsMessageSplitterSetting( const QByteArray state );
    //!@}

    //!@{
    //! Reads and saves the path to the opened project from OS registry.
    QString getLastlyOpenedProjectSetting();
    void setLastlyOpenedProjectSetting();
    //!@}


    /** Returns the name of the currently opened project or "no project open". */
    QString getOpenProjectName();

    /** Returns whether there is an project opened. */
    bool hasOpenProject();

    //!@{
    //! Reads and saves the Ghostscript installation path from OS registry.
    QString getGhostscriptPathSetting();
    void setGhostscriptPathSetting(const QString path);
    //!@}

    //!@{
    //! Reads and saves the maximum number of cells in a single grid for the 3D viewer.
    int getMaxGridCellCountFor3DVisualizationSetting();
    void setMaxGridCellCountFor3DVisualizationSetting(int value);
    //!@}

    /**
     * @brief Treats the text as an information text.
     */
    void logInfo(const QString text );

    /**
     * @brief Treats the text as an warning text.
     */
    void logWarn(const QString text );

    /**
     * @brief Treats the text as an error text.
     */
    void logError(const QString text );

    /** Updates the currently opened project display. */
    void refreshProjectTree();

    /** Generates a path to a temporary file in the GSLib directory. */
    QString generateUniqueFilePathInGSLibDir(const QString file_extension);

    /** Adds a data file to the project.  Does nothing of there is no open project. */
    void addDataFile( const QString path );

    /** Disables warnings. */
    void logWarningOff(){ _logWarnings = false; }
    /** Enables warnings. */
    void logWarningOn(){ _logWarnings = true; }

private:
    Application();
    ~Application();

    /** The singleton pointer */
    static Application* _instance;

    /** Pointer to the currently opened project. */
    Project* _open_project;

    /** Pointer to the main window. */
    MainWindow* _mw;

    /** Flag that enables/disables warning logging/printing. */
    bool _logWarnings;
};

#endif // APPLICATION_H
