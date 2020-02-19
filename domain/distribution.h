#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include "file.h"
#include "roles.h"

/**
 * @brief The Distribution class is the base class of UnivariateDistribution and BivariateDistribution classes.
 */
class Distribution : public File
{
public:
    Distribution( const QString path );
    void updateColumnCollection();

    /** Sets distribution metadata.  It also populates the file's column collection.
     * @param varIndex_role_pairs pairs of variable GEO-EAS index (first is 1) in the distribution file and role codes.
    */
    void setInfo( const QMap<uint, Roles::DistributionColumnRole> &varIndex_role_pairs );

    /** Sets distribution metadata from the accompaining .md file, if it exists.
     Nothing happens if the metadata file does not exist.  If it exists, it calls
     #setInfo(...) with the metadata read from the .md file.*/
    void setInfoFromMetadataFile();

    /** Returns the GEO-EAS index (1 == first) of the column, if there is one, and only one, column with a
     *  value role (linear or log scale).   Returns zero if there is none or more than one.
     *  @see Roles */
    uint getTheColumnWithValueRole() const;

    /** Returns the GEO-EAS index (1 == first) of the column, if there is one, and only one, column with the
     *  probability role.   Returns zero if there is none or more than one.
     *  @see Roles */
    uint getTheColumnWithProbabilityRole() const;

// File interface
public:
    virtual bool canHaveMetaData(){ return true; }
    virtual void updateMetaDataFile();
    virtual void deleteFromFS();

// ProjectComponent interface
public:
    void save(QTextStream *txt_stream);
};

#endif // DISTRIBUTION_H
