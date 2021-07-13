#ifndef EXPERIMENTALVARIOGRAM_H
#define EXPERIMENTALVARIOGRAM_H

#include "file.h"

/**
 * @brief The ExperimentalVariogram class represents a file with experimental variogram files saved by the user.
 */
class ExperimentalVariogram : public File
{
public:
    ExperimentalVariogram( const QString path );

    /** Sets experimental variogram metadata. */
    void setInfo( const QString vargplt_par_file );

    /** Sets experimental variogram metadata from the accompaining .md file, if it exists.
     Nothing happens if the metadata file does not exist.  If it exists, it calls
     #setInfo(const QString) with the metadata read from the .md file.*/
    void setInfoFromMetadataFile();

    /** Returns the path to associated vargplt par file (if exists) so it is possible
     * to plot a saved experimental variogram again without going through the variogram
     * analysis workflow.
     */
    QString getPathToVargpltPar();

// File interface
public:
    virtual void deleteFromFS();
    QString getFileType() const { return "EXPVARIOGRAM"; }
    virtual bool canHaveMetaData(){ return true; }
    virtual void updateMetaDataFile();
    bool isDataFile(){ return false; }
	bool isDistribution(){ return false; }
    virtual File* duplicatePhysicalFiles( const QString new_file_name );

// ProjectComponent interface
public:
    QIcon getIcon();
    void save(QTextStream *txt_stream);

private:
    //the path to a vargplt program parameter file used to
    //allow the user to plot it.  The experimental variogram
    //is not displayable is this file is not specified.
    QString m_path_to_vargplt_par;
};

#endif // EXPERIMENTALVARIOGRAM_H
