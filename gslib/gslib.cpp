#include "gslib.h"

#include <QDir>
#include "../domain/application.h"
#include <QMessageBox>
#include <QThread>
#include "workerthread.h"

/*static*/ GSLib* GSLib::s_instance = nullptr;

GSLib::GSLib() : QObject(),
    m_process( nullptr ),
    m_stderr_assync_count( 0 )
{
}

GSLib *GSLib::instance()
{
    if( ! GSLib::s_instance )
        s_instance = new GSLib();
    return s_instance;
}

void GSLib::runProgram(const QString program_name, const QString par_file_path, bool parFromStdIn )
{
    QProcess process;
    QDir gslib_home = QDir(Application::instance()->getGSLibPathSetting());
    QString gslib_program = gslib_home.filePath( program_name );

    gslib_program = QString("\"").append(gslib_program).append("\"");

    QString command = gslib_program;
    if( !parFromStdIn )
        command.append(" \"").append( par_file_path ).append("\"");

    process.setWorkingDirectory( Application::instance()->getGSLibPathSetting() );

    process.start( command );
    if( parFromStdIn ){
        process.write( QString(par_file_path).append('\n').toStdString().c_str() );
    }
    if(! process.waitForFinished(-1) ){
        Application::instance()->logError(QString("ERROR: call to external program ").append(program_name).append(" abnormally ended."));
        QString errorCause;
        switch( process.error() ){
            case QProcess::FailedToStart:
                errorCause = "Program file is missing or you lack execution permission on it.";
                break;
            case QProcess::Crashed:
                errorCause = "Program crashed.";
                break;
            case QProcess::Timedout:
                errorCause = "Execution timeout.";
                break;
            case QProcess::WriteError:
                errorCause = "Could not write to process' input stream.";
                break;
            case QProcess::ReadError:
                errorCause = "Could not read from process' output stream.";
                break;
            default:
                errorCause = "Unknown.";
        }
        Application::instance()->logError(QString("      Cause: ").append(errorCause));
    }

    m_last_output = process.readAllStandardOutput();
    QString stdErrMessages( process.readAllStandardError() );
    Application::instance()->logError( stdErrMessages );
    Application::instance()->logInfo( QString( m_last_output ) );
    if( ! stdErrMessages.trimmed().isEmpty() )
        QMessageBox::critical( nullptr, "Errors to stderr", program_name + " program output error messages. Please, check the Output Message panel for recent messages in red.");
}

void GSLib::runProgramAsync(const QString program_name,
                            const QString par_file_path,
                            bool parFromStdIn,
                            const QString working_directory )
{
    m_stderr_assync_count = 0;
    m_process = new QProcess();

    //get the GSLib home dir
    QDir gslib_home = QDir(Application::instance()->getGSLibPathSetting());

    //build the entire path to the program executable
    QString gslib_program = gslib_home.filePath( program_name );

    //surround the complete path with double quotes (path may contain whitespaces)
    gslib_program = QString("\"").append(gslib_program).append("\"");

    //tell whether the program name already has a path
    bool programNameHasPath = program_name.contains('/') || program_name.contains('\\') ;

    //construct the final command to start the external process
    QString command;
    if( ! programNameHasPath )
        command = gslib_program;
    else
        command = program_name;

    //test whether the executable exists (for some reason, some GSLib executables simply causes the program
    // to hang if missing).  It is necessary to remove any quotation marks.
    QString exePath = command.replace('\"', ' ').trimmed();
#ifdef Q_OS_WIN
		exePath += ".exe";
#endif
	QFile exeFile( exePath );
    if( ! ( exeFile.exists() ) ){
		Application::instance()->logError("GSLib program not found or with permission denied: " + exePath);
		return;
	}

    if( !parFromStdIn )
        command.append(" \"").append( par_file_path ).append("\"");

    m_last_output = "";

    //connect the signals to the slots to keep track of the assynchronous process
    connect (m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onProgramOutput()));
    connect (m_process, SIGNAL(readyReadStandardError()), this, SLOT(onProgramOutput()));
    connect (m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onProgramFinished(int,QProcess::ExitStatus)));

    //set the process' working directory (if applicable)
    if( ! working_directory.isEmpty() )
        m_process->setWorkingDirectory( working_directory );

    //start the program, passing the path to parameter file if the program only accepts it via std in.
    m_process->start( command );
    if( parFromStdIn ){
        m_process->write( QString(par_file_path).append('\n').append('y').append('\n').toStdString().c_str() );
    //Sometimes the newcokb3d program asks a "are you sure" question.^^^
    }
}

void GSLib::runProgramThread(const QString program_name, const QString par_file_path)
{
    WorkerThread *workerThread = new WorkerThread( this, program_name, par_file_path );
    connect(workerThread, SIGNAL(completed()), this, SLOT(onProgramThreadFinished()));
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));
    m_thread_running = true;
    workerThread->start();
    //FIXME: this wait is actually blocking...
    // QThread::wait() does not work too.
    while( m_thread_running )
        QThread::msleep( 100 );
}

QString GSLib::getLastOutput()
{
    return m_last_output;
}

void GSLib::onProgramOutput()
{
    QString std_out_text( m_process->readAllStandardOutput() );
    m_last_output += std_out_text;
    QString stderr_text(m_process->readAllStandardError());
    QString stdout_text( std_out_text );
    if( ! stderr_text.trimmed().isEmpty() ){
        Application::instance()->logError( stderr_text );
        ++m_stderr_assync_count;
    }
    if( ! stdout_text.trimmed().isEmpty() )
        Application::instance()->logInfo( stdout_text );
}

void GSLib::onProgramFinished(int exit_code, QProcess::ExitStatus /*exit_status*/)
{
    Application::instance()->logInfo( QString("GSLib::onProgramFinished(): Program terminated with exit code = ").append( QString::number(exit_code) ) );
    if( m_stderr_assync_count > 0 )
        QMessageBox::critical( nullptr, "Errors to stderr", "GSLib program output error messages. Please, check the Output Message panel for recent messages in red.");
    emit programFinished();
}

void GSLib::onProgramThreadFinished()
{
    Application::instance()->logInfo( "GSLib::onProgramThreadFinished(): Program thread finished work." );
    m_thread_running = false;
}
