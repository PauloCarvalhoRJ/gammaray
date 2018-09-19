#ifndef APPLICATION_H
#define APPLICATION_H

#include <QString>
#include <QByteArray>
#include <vector>

class Project;
class MainWindow;

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
    void logInfo(const QString text, bool showMessageBox = false );

    /**
     * @brief Treats the text as an warning text.
     */
    void logWarn(const QString text, bool showMessageBox = false );

    /**
     * @brief Treats the text as an error text.
     */
    void logError(const QString text, bool showMessageBox = false );

    /** Updates the currently opened project display. */
    void refreshProjectTree();

    /** Generates a path to a temporary file in the GSLib directory. */
    QString generateUniqueFilePathInGSLibDir(const QString file_extension);

    /** Adds a data file to the project.  Does nothing of there is no open project. */
    void addDataFile( const QString path );

    /** Disables warning logging. */
    void logWarningOff(){ logWarn("WARNING! Warning messages disabled! "); _logWarnings = false; }
    /** Enables warning logging. */
    void logWarningOn();

    /** Disables error logging. */
    void logErrorOff(){ logWarn("WARNING! Error messages disabled! "); _logErrors = false; }
    /** Enables error logging. */
    void logErrorOn();

    /** Disables information messages logging. */
    void logInfoOff(){ logWarn("WARNING! Information messages disabled! "); _logInfo = false; }
    /** Enables information messages logging. */
    void logInfoOn();

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

    /** Flag that enables/disables error logging/printing. */
    bool _logErrors;

    /** Flag that enables/disables information logging/printing. */
    bool _logInfo;

    /** Warning messages are stored here while their display is disabled. */
    std::vector<QString> _warningBuffer;

    /** Error messages are stored here while their display is disabled. */
    std::vector<QString> _errorBuffer;

    /** Information messages are stored here while their display is disabled. */
    std::vector<QString> _infoBuffer;
};

#endif // APPLICATION_H
