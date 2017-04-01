#include "ghostscript.h"
#include <QtGlobal>
#include <QProcess>
#include "../domain/application.h"
#include <QStringList>
#include <QDir>
#include "exceptions/externalprogramexception.h"

Ghostscript::Ghostscript()
{
}

QString Ghostscript::getGsProgramName(QDir &bin_dir )
{
    #ifdef Q_OS_WIN
        //GhostScript Windows executable can be gswin64c.exe or gswin32c.exe.
        //Try gswin64c.exe preferentially.
        QFile gsExePath( bin_dir.filePath( "gswin64c.exe" ) );
        if( gsExePath.exists() ){
            Application::instance()->logInfo( "GhostScript 64-bit found." );
            return "gswin64c.exe";
        }
        Application::instance()->logInfo( "Trying GhostScript 32-bit." );
        return "gswin32c.exe";
    #else
        static_cast<void>(bin_dir); //avoid compiler warning (no-use) in non-Windows platforms
        return "gs";
    #endif
}

void Ghostscript::makePNG(const QString input_ps_file_path, const QString output_png_file_path, int resolution)
{
    QProcess cmd;
    QDir gs_home = QDir(Application::instance()->getGhostscriptPathSetting());
    QDir gs_bin = QDir(gs_home.filePath("bin"));
    QString gs_program = gs_bin.filePath( Ghostscript::getGsProgramName( gs_bin ) );

    gs_program = QString("\"").append(gs_program).append("\"");

    //for some reason calling the program this way does not work, also no error is risen
    //cmd.setWorkingDirectory( gs_bin.canonicalPath() );
    //cmd.setProgram( QString("\"").append(gs_program).append("\"") );
    //QStringList args;
    //args << "-dSAFER" << "-dBATCH" << "-dNOPAUSE";
    //args << QString("-r").append( QString::number( resolution ) );
    //args << "-sDEVICE=png16m";
    //args << QString("-sOutputFile=\"").append(output_png_file_path).append("\"");
    //args << input_ps_file_path;
    //cmd.setArguments( args );

    //refer to http://www.ghostscript.com/doc/current/Use.htm for all GhostScript options.

    QString command = gs_program.append( " -dSAFER -dBATCH -dNOPAUSE -r").\
            append( QString::number(resolution) ).\
            append(" -sDEVICE=png16m -sOutputFile=\"").\
            append( output_png_file_path ).\
            append("\" ").\
            append("\"").\
            append( input_ps_file_path ).append("\"");

    cmd.start( command );
    if(! cmd.waitForFinished() )
        throw ExternalProgramException( cmd.error() );

}
