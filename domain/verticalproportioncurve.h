#ifndef VERTICALPROPORTIONCURVE_H
#define VERTICALPROPORTIONCURVE_H

#include "domain/datafile.h"

/**
 * The VerticalProportionCurve class models a Vertical Proportion Curve, a series of values defining the expected proportion
 * of categories (e.g. lithofacies) along the vertical.  The vertical scale of the curve is relative, varying from 1.0 at top
 * and 0.0 at the base.  The absolute depth at which a certain proportion occurs should be computed on the fly with respect
 * to some reference such as top and base horizons or max/min depth of point sets or segment sets.  The curves are represented
 * by polygonal lines between top and base. The curve points are values between 0.0 and 1.0 and are stored as columns of a
 * GEO-EAS file (similiarly to the data sets).  These values logically must be in ascending order from first column to last
 * column. The proportion of a given category is computed by the delta between two poly lines (or between 0.0 and 1st poly line
 * or between last poly line and 1.0) at any given relative depth.
 */
class VerticalProportionCurve : public DataFile
{
public:
    VerticalProportionCurve( QString path, QString associatedCategoryDefinitionName );


    // ProjectComponent interface
public:
    virtual QIcon getIcon();
    virtual QString getTypeName();
    virtual void save(QTextStream *txt_stream);

    // File interface
public:
    virtual bool canHaveMetaData();
    virtual QString getFileType();
    virtual void updateMetaDataFile();
    virtual bool isDataFile();
    virtual bool isDistribution();
    virtual void deleteFromFS();

    //DataFile interface
public:
    bool isWeight( Attribute* at );
    Attribute* getVariableOfWeight( Attribute* weight );
    virtual void deleteVariable( uint columnToDelete );
    virtual bool isRegular();
    virtual double getDataSpatialLocation( uint line, CartesianCoord whichCoord );
    virtual void getDataSpatialLocation( uint line, double& x, double& y, double& z );
    virtual bool isTridimensional();

protected:
    ///--------------data read from metadata file------------
    QString m_associatedCategoryDefinitionName;
};

#endif // VERTICALPROPORTIONCURVE_H
