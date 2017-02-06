#include "experimentalvariogram.h"
#include <QFile>
#include <QTextStream>

ExperimentalVariogram::ExperimentalVariogram(const QString path) : File( path )
{
}

void ExperimentalVariogram::setInfo(const QString vargplt_par_file)
{
    this->m_path_to_vargplt_par = vargplt_par_file;
}

void ExperimentalVariogram::setInfoFromMetadataFile()
{
    QString md_file_path( this->_path );
    QFile md_file( md_file_path.append(".md") );
    QString vargplt_par_path;
    if( md_file.exists() ){
        md_file.open( QFile::ReadOnly | QFile::Text );
        QTextStream in(&md_file);
        for (int i = 0; !in.atEnd(); ++i)
        {
           QString line = in.readLine();
           if( line.startsWith( "VARGPLT_PAR:" ) ){
               QStringList line_parts = line.split(":");
               QString value = line_parts[1];
               //under Windows may have an extra colon in paths e.g. "C:/FOO/WOO.vargplt"
               //this extra colon is stripped and the split operation results in a three-element list
               if( line_parts.size() == 3 )
                   value.append( ":" + line_parts[2] );
               vargplt_par_path = value;
           }
        }
        md_file.close();
        this->setInfo( vargplt_par_path );
    }
}

QString ExperimentalVariogram::getPathToVargpltPar()
{
    return m_path_to_vargplt_par;
}

void ExperimentalVariogram::updateMetaDataFile()
{
    QFile file( this->getMetaDataFilePath() );
    file.open( QFile::WriteOnly | QFile::Text );
    QTextStream out(&file);
    out << APP_NAME << " metadata file.  This file is generated automatically.  Avoid editing this file.\n";
    out << "version=" << APP_VERSION << '\n';
    out << "VARGPLT_PAR:" << this->m_path_to_vargplt_par << '\n';
    file.close();
}

QIcon ExperimentalVariogram::getIcon()
{
    return QIcon(":icons/vexp16");
}

void ExperimentalVariogram::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}
