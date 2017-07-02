#include "application.h"
#include "project.h"
#include <QDir>
#include <QSettings>

//global instance pointer in the heap.
Application* Application::_instance = nullptr;

Application *Application::instance()
{
    if( ! _instance )
        _instance = new Application();
    return _instance;
}

Project *Application::getProject()
{
    return this->_open_project;
}

void Application::openProject(const QString directory_path)
{
    if( _open_project ){
        this->_open_project->freeLoadedData(); //TODO: this is unnecessary if a ProjectComponent deletes its children in its destructor
        delete _open_project;
    }
    _open_project = new Project( directory_path );
}

void Application::setMainWindow(MainWindow *mw)
{
    this->_mw = mw;
}

void Application::closeProject()
{
    if( _open_project ){
        this->_open_project->freeLoadedData(); //TODO: this is unnecessary if a ProjectComponent deletes its children in its destructor
        delete this->_open_project;
        this->_open_project = nullptr;
    }
}

QString Application::getGSLibPathSetting()
{
    QSettings qs;
    return qs.value("gslibpath").toString();
}

void Application::setGSLibPathSetting(const QString path)
{
    QSettings qs;
    qs.setValue("gslibpath", path);
}

void Application::setGhostscriptPathSetting(const QString path)
{
    QSettings qs;
    qs.setValue("gspath", path);
}

int Application::getMaxGridCellCountFor3DVisualizationSetting()
{
    QSettings qs;
    bool ok;
    int setting = qs.value("maxcellgrid3dview").toInt( &ok );
    if( ! ok )
        return 2000000; //default
    else
        return setting;
}

void Application::setMaxGridCellCountFor3DVisualizationSetting(int value)
{
    QSettings qs;
    qs.setValue("maxcellgrid3dview", value);
}

void Application::logInfo(const QString text)
{
    Q_ASSERT(_mw != 0);
    _mw->log_message( text, "information" );
}

void Application::logWarn(const QString text)
{
    Q_ASSERT(_mw != 0);
    if( _logWarnings )
        _mw->log_message( text, "warning" );
}

void Application::logError(const QString text)
{
    Q_ASSERT(_mw != 0);
    _mw->log_message( text, "error" );
}

void Application::refreshProjectTree()
{
    _mw->refreshTreeStyle();
}

QString Application::generateUniqueFilePathInGSLibDir(const QString file_extension)
{
    while(true){
        int r = ( (int)((double)rand() / RAND_MAX * 10000000)) + 10000000;
        QString filename = QString::number(r);
        filename.append(".");
        filename.append(file_extension);
        QDir dir( getGSLibPathSetting() );
        QFile file(dir.absoluteFilePath(filename));
        if( ! file.exists() )
            return dir.absoluteFilePath(filename);
    }
}

void Application::addDataFile(const QString path)
{
    if( this->hasOpenProject() ){
        _mw->doAddDataFile( path );
    }
}

QByteArray Application::getMainWindowSplitterSetting()
{
    QSettings qs;
    if( qs.contains( "mwsplitter" ))
        return qs.value("mwsplitter").toByteArray();
    else
        return QByteArray();
}

void Application::setMainWindowSplitterSetting(const QByteArray state)
{
    QSettings qs;
    qs.setValue("mwsplitter", state);
}

QByteArray Application::getContentsMessageSplitterSetting()
{
    QSettings qs;
    if( qs.contains( "cmsplitter" ))
        return qs.value("cmsplitter").toByteArray();
    else
        return QByteArray();
}

void Application::setContentsMessageSplitterSetting(const QByteArray state)
{
    QSettings qs;
    qs.setValue("cmsplitter", state);
}

QString Application::getLastlyOpenedProjectSetting()
{
    QSettings qs;
    return qs.value("lastopenproj").toString();
}

void Application::setLastlyOpenedProjectSetting()
{
    QString path;
    if( this->_open_project )
        path = this->_open_project->getPath();
    QSettings qs;
    qs.setValue("lastopenproj", path);
}

QString Application::getOpenProjectName()
{
    if ( ! this->_open_project )
        return "<no open project>";
    else
        return this->_open_project->getName();
}

bool Application::hasOpenProject()
{
    return this->_open_project;
}

QString Application::getGhostscriptPathSetting()
{
    QSettings qs;
    return qs.value("gspath").toString();
}

Application::Application() :
    _logWarnings( true )
{
    this->_open_project = nullptr;
}

Application::~Application()
{
    delete this->_open_project;
}
