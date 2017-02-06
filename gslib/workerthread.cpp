#include "workerthread.h"
#include <QDir>
#include <domain/application.h>

WorkerThread::WorkerThread(QObject *client, const QString program_name, const QString par_file_path) :
    QThread( client ),
    m_program_name( program_name ),
    m_par_file_path( par_file_path )
{
    //this is necessary so one of the signal-slot connections works.
    qRegisterMetaType< QProcess::ExitStatus >("QProcess::ExitStatus");
}

void WorkerThread::run()
{
    m_process = new QProcess();
    QDir gslib_home = QDir(Application::instance()->getGSLibPathSetting());
    QString gslib_program = gslib_home.filePath( m_program_name );

    //TODO: test whether running a program with quotation marks works in Unix
    gslib_program = QString("\"").append(gslib_program).append("\"");

    QString command = gslib_program.append(" \"").append( m_par_file_path ).append("\"");

    connect (m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(onProgramOutput()));
    connect (m_process, SIGNAL(readyReadStandardError()), this, SLOT(onProgramOutput()));
    connect (m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onProgramFinished(int,QProcess::ExitStatus)));

    m_process->start( command );
    //start event processing loop
    this->exec();
}

void WorkerThread::onProgramOutput()
{
    QString stderr_text(m_process->readAllStandardError());
    QString stdout_text(m_process->readAllStandardOutput());
    if( ! stderr_text.trimmed().isEmpty() )
        Application::instance()->logError( stderr_text );
    if( ! stdout_text.trimmed().isEmpty() )
        Application::instance()->logInfo( stdout_text );
}

void WorkerThread::onProgramFinished(int exit_code, QProcess::ExitStatus exit_status)
{
    Application::instance()->logInfo( QString("WorkerThread::onProgramFinished(): Program terminated with exit code = ").append( QString::number(exit_code) ) );
    delete m_process;
    emit completed();
}
