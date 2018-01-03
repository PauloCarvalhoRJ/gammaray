#include "application.h"
#include "project.h"
#include <QDir>
#include <QSettings>
#include <QMessageBox>

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

void Application::logInfo(const QString text, bool showMessageBox)
{
    Q_ASSERT(_mw != 0);
    if( _logInfo )
        _mw->log_message( text, "information" );
    else
        _infoBuffer.push_back( text );
    if( showMessageBox )
        QMessageBox::information( nullptr, "Information", text );
}

void Application::logWarn(const QString text, bool showMessageBox)
{
    Q_ASSERT(_mw != 0);
    if( _logWarnings )
        _mw->log_message( text, "warning" );
    else
        _warningBuffer.push_back( text );
    if( showMessageBox )
        QMessageBox::warning( nullptr, "Warning", text );
}

void Application::logError(const QString text, bool showMessageBox)
{
    Q_ASSERT(_mw != 0);
    if( _logErrors )
        _mw->log_message( text, "error" );
    else
        _errorBuffer.push_back( text );
    if( showMessageBox )
        QMessageBox::critical( nullptr, "Error", text );
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

void Application::logWarningOn()
{
    _logWarnings = true;
    logInfo("Warning messages back on.");
    if( ! _warningBuffer.empty() ){
        logInfo("Buffered warning messages dump:");
        std::vector<QString>::iterator it = _warningBuffer.begin();
        QString lastMessage;
        int repeatedMessagesCount = 0;
        for(; it != _warningBuffer.end(); ++it){
            QString currentMessage = (*it);
            if( currentMessage != lastMessage ){
                if( repeatedMessagesCount > 0 )
                    logWarn( "   Followed by more " + QString::number(repeatedMessagesCount) + " message(s) like that." );
                logWarn( currentMessage );
                repeatedMessagesCount = 0;
            } else {
                ++repeatedMessagesCount;
            }
            lastMessage = currentMessage;
        }
        _warningBuffer.clear();
    }
}

void Application::logErrorOn()
{
    logInfo("Error messages back on. ");
    _logErrors = true;
    if( ! _errorBuffer.empty() ){
        logInfo("Buffered error messages dump:");
        std::vector<QString>::iterator it = _errorBuffer.begin();
        QString lastMessage;
        int repeatedMessagesCount = 0;
        for(; it != _errorBuffer.end(); ++it){
            QString currentMessage = (*it);
            if( currentMessage != lastMessage ){
                if( repeatedMessagesCount > 0 )
                    logError("   Followed by more " + QString::number(repeatedMessagesCount) + " message(s) like that." );
                logError( currentMessage );
                repeatedMessagesCount = 0;
            } else {
                ++repeatedMessagesCount;
            }
            lastMessage = currentMessage;
        }
        _errorBuffer.clear();
        if( repeatedMessagesCount > 0 )
            logError("   Followed by more " + QString::number(repeatedMessagesCount) + " message(s) like that." );
    }
}

void Application::logInfoOn()
{
    logInfo("Information messages back on. ");
    _logInfo = true;
    if( ! _infoBuffer.empty() ){
        logInfo("Buffered error messages dump:");
        std::vector<QString>::iterator it = _infoBuffer.begin();
        QString lastMessage;
        int repeatedMessagesCount = 0;
        for(; it != _infoBuffer.end(); ++it){
            QString currentMessage = (*it);
            if( currentMessage != lastMessage ){
                if( repeatedMessagesCount > 0 )
                    logError("   Followed by more " + QString::number(repeatedMessagesCount) + " message(s) like that." );
                logError( currentMessage );
                repeatedMessagesCount = 0;
            } else {
                ++repeatedMessagesCount;
            }
            lastMessage = currentMessage;
        }
        _infoBuffer.clear();
        if( repeatedMessagesCount > 0 )
            logError("   Followed by more " + QString::number(repeatedMessagesCount) + " message(s) like that." );
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
    _logWarnings( true ),
    _logErrors( true ),
    _logInfo( true )
{
    this->_open_project = nullptr;
}

Application::~Application()
{
    delete this->_open_project;
}
