#include "experimentalvariogram.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include "util.h"
#include "domain/application.h"

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

void ExperimentalVariogram::deleteFromFS()
{
    File::deleteFromFS(); //delete the file itself.

    //also deletes the metadata file
    {
        QFile file( this->getMetaDataFilePath() );
        file.remove();
    }

    //also deletes the .vargplt file used for plotting afterwards
    {
        QFile file( getPathToVargpltPar() );
        file.remove();
    }


    //also deletes the .legend text file used for plotting legends in the variogram chart
    {
        //Get the directory where the files are
        QString dataFilePath = getPath();
        QFileInfo qfileInfoDataFile( dataFilePath );
        QString directoryPath = qfileInfoDataFile.absolutePath();

        //Get the path to the .legend file
        QString pathOfLegendFile  = directoryPath + '/' + getName() + ".legend";

        //delete it
        QFile file( pathOfLegendFile );
        file.remove();
    }
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

File *ExperimentalVariogram::duplicatePhysicalFiles(const QString new_file_name)
{
    //The usual protocol: copy the data and metadata files.
    QString duplicateFilePath = duplicateDataAndMetaDataFiles( new_file_name );

    //ExperimentalVariograms have two other files: a .legend file and a parameter file for
    // later visualization of results with the VARGPLT program of GSLib.

    //Get the directory where the additional files are
    QString originalDataFilePath = getPath();
    QFileInfo qfileInfoDataFile( originalDataFilePath );
    QString directoryPath = qfileInfoDataFile.absolutePath();

    //Get the path to the original additional files
    QString originalPathOfVargpltFile = getPathToVargpltPar();
    QString originalPathOfLegendFile  = directoryPath + '/' + getName() + ".legend";

    //Sanity checks
    QFileInfo qfileInfoVargpltFile( originalPathOfVargpltFile );
    if( ! qfileInfoVargpltFile.exists() )
        Application::instance()->logWarn("ExperimentalVariogram::duplicatePhysicalFiles(): VARGPLT parameter file for"
                                         " plotting not found.");
    QFileInfo qfileInfoLegentFile( originalPathOfLegendFile );
    if( ! qfileInfoLegentFile.exists() )
        Application::instance()->logWarn("ExperimentalVariogram::duplicatePhysicalFiles(): legend text file for"
                                         " plotting not found." + originalPathOfLegendFile );

    //Make the path to the duplicated additional files
    QString duplicatedPathOfVargpltFile = directoryPath + '/' + new_file_name + ".vargplt";
    QString duplicatedPathOfLegendFile  = directoryPath + '/' + new_file_name + ".legend";

    //Copy the two additional files
    Util::copyFile( originalPathOfVargpltFile, duplicatedPathOfVargpltFile);
    Util::copyFile( originalPathOfLegendFile,  duplicatedPathOfLegendFile);

    //Change the .vargplt file so it refers to the duplicated files.
    int count = Util::findAndReplace( duplicatedPathOfVargpltFile,
                                      originalPathOfLegendFile + "[\\s]*$",
                                      duplicatedPathOfLegendFile );
    if( count == 0 )
        Application::instance()->logWarn("ExperimentalVariogram::duplicatePhysicalFiles(): path to file with the "
                                         "legend text not found in the .vargplt file.");
    count =    Util::findAndReplace( duplicatedPathOfVargpltFile,
                                     originalDataFilePath + "[\\s]*$",
                                     duplicateFilePath );
    if( count == 0 )
        Application::instance()->logWarn("ExperimentalVariogram::duplicatePhysicalFiles(): path to file with the "
                                         "experimental variogram points not found in the .vargplt file.");

    //Create and returns a new ExperimentalVariogram object refering to the duplicated files.
    ExperimentalVariogram* exv =  new ExperimentalVariogram( duplicateFilePath );
    exv->m_path_to_vargplt_par = duplicatedPathOfVargpltFile;
    exv->updateMetaDataFile();
    return exv;
}

QIcon ExperimentalVariogram::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/vexp16");
    else
        return QIcon(":icons32/vexp32");
}

void ExperimentalVariogram::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
    //also saves the metadata file.
    this->updateMetaDataFile();
}
