#include "graphviz.h"
#include <QtGlobal>
#include <QProcess>
#include "../domain/application.h"
#include <QStringList>
#include <QDir>
#include "exceptions/externalprogramexception.h"

GraphViz::GraphViz()
{
}

void GraphViz::makePSfromDOT( const QString input_dot_file_path,
                              const QString output_ps_file_path )
{
    QProcess cmd;
    QDir gv_home = QDir(Application::instance()->getGraphVizPathSetting());
    QDir gv_bin = QDir(gv_home.filePath("bin"));
    QString dot_program = gv_bin.filePath( "dot" );

    dot_program = QString("\"").append(dot_program).append("\"");

    QString command = dot_program.append( " -Tps \"").\
            append( input_dot_file_path ).\
            append("\" -o ").\
            append("\"").\
            append( output_ps_file_path ).append("\"");

    cmd.start( command );
    if(! cmd.waitForFinished(-1) )
        throw ExternalProgramException( cmd.error() );
}
