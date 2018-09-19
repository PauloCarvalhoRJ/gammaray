#ifndef GSLIBPARINPUTDATA_H
#define GSLIBPARINPUTDATA_H

#include "gslibpartype.h"
#include "gslibparfile.h"
#include "gslibparlimitsdouble.h"
#include <QList>

class Attribute;
class GSLibParVarWeight;

/**
 * @brief The GSLibParInputData class represents the following set of parameters found in many par files:
 *
 * ../data/cluster.dat          -file with data
 * 3   0                        -   columns for variable and weight
 * -1.0     1.0e21              -   trimming limits
 */
class GSLibParInputData : public GSLibParType
{
public:
    GSLibParInputData();
    ~GSLibParInputData();
    GSLibParFile _file_with_data;
    QList<GSLibParVarWeight*> _var_wgt_pairs;
    GSLibParLimitsDouble _trimming_limits;

    /** Sets the component parameters based on the given attribute. */
    void set( Attribute* at );

    //@{
    /** Convenient methods that return the values set for the trimming limits
     *  (_trimming_limits member). */
    double getLowerTrimmingLimit();
    double getUpperTrimmingLimit();
    //@}

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "input";}
    QWidget* getWidget();
    bool update();
    bool isCollection() { return true; }
};

#endif // GSLIBPARINPUTDATA_H
