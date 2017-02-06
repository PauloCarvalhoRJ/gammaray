#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QThread>
#include <QProcess>

/**
 * This class is used to run the GSLib programs in a separate thread.
 */
class WorkerThread : public QThread
{

    Q_OBJECT

public:
    explicit WorkerThread(QObject* client, const QString program_name, const QString par_file_path);

    void run();

signals:
    void completed();

private:
    QProcess* m_process;
    const QString m_program_name;
    const QString m_par_file_path;

private slots:
     void onProgramOutput();
     void onProgramFinished(int exit_code, QProcess::ExitStatus exit_status);
};

#endif // WORKERTHREAD_H
