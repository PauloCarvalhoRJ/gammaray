#ifndef GSLIB_H
#define GSLIB_H

#include <QString>
#include <QObject>
#include <QProcess>

/**
 * @brief The GSLib class is the hub for interfacing with GSLib programs.
 */
class GSLib : public QObject
{

    Q_OBJECT

public:

    static GSLib* instance();

    /**
     * Runs the given GSLib program with the given parameter file.
     * It runs synchronously, meaning that the client code will block until
     * the called program finishes or crashes.
     * @param parFromStdIn If true, the paramater file path is passed via standard input. Some GSLib programs ignore
     *                     the command line argument and still wait for the user to input the path to the parameter file.
     *                     gammabar and newcokb3d are known to have this unexpected behavior.
     */
    void runProgram( const QString program_name, const QString par_file_path, bool parFromStdIn = false );

    /**
     * Does the same as runProgram(), but does not block the client code.
     * Client code can connect to the programFinished() static signal to have control over
     * program termination.
     * @param program_name The name of an executable (without extension even under Windows) or a complete path to an executable.
     *                     In the first case (most common applications), the GSLib directory as configured in GammaRay is assumed.
     * @param parFromStdIn If true, the paramater file path is passed via standard input. Some GSLib programs ignore
     *                     the command line argument and still wait for the user to input the path to the parameter file.
     *                     gammabar and newcokb3d are known to have this unexpected behavior.
     * @param working_directory A path to a directory to start the program in.  Some programs (newcokb3d) do not work when started
     *                          from GammaRay's directory.
     */
    void runProgramAsync(const QString program_name,
                         const QString par_file_path,
                         bool parFromStdIn = false ,
                         const QString working_directory = QString());

    /**
     * Does the same as runProgram(), but instead of blocking, the client code waits for
     * the program to terminate or crash.  This is the preferable way to execute external programs since
     * the client thread does not block, meaning that the user can get feedback on program execution like
     * runProgramAsync() while the client code can have control on program termination like runProgram().
     * THIS IS CURRENTLY NOT WORKING PROPERLY.
     * Use runProgramAsync() with the programFinished() signal for asynchronous run with controlled termination.
     */
    void runProgramThread( const QString program_name, const QString par_file_path );

    /**
     *  Returns the last output generated during the last run.
     */
    QString getLastOutput();

private:
     GSLib();
     QProcess* m_process;
     static GSLib* s_instance;
     bool m_thread_running;
     QString m_last_output;
     uint m_stderr_assync_count;

signals:
     void programFinished();

private slots:
     void onProgramOutput();
     void onProgramFinished(int exit_code, QProcess::ExitStatus exit_status);
     void onProgramThreadFinished( );
};

#endif // GSLIB_H
