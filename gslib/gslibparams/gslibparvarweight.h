#ifndef GSLIBPARVARWEIGHT_H
#define GSLIBPARVARWEIGHT_H
#include "gslibpartype.h"
#include "../../exceptions/invalidmethodexception.h"


/**
 * @brief The GSLibParVarWeight class represents a pair of integers. The first integer is a
 * variable column index in a GEO-EAS data file.  The other is its corresponding weight, which
 * may be zero for no-weight.
 */
class GSLibParVarWeight : public GSLibParType
{
public:
    GSLibParVarWeight(const QString name, const QString label, const QString description);
    /** This constructor sets the indexes while sets name, label and description blank. */
    GSLibParVarWeight(uint var_index, uint wgt_index);
    uint _var_index;
    uint _wgt_index;

    // GSLibParType interface
public:
    /** Cannot save a pair, so this method does nothing.  It's up to a container parameter to output
     *  the variable indexes first and only then comes the respective weights per the GSLib convention.
     */
    void save(QTextStream */*out*/){ throw InvalidMethodException(); }
    QString getTypeName() {return "var_weight";}
    bool isCollection() { return false; }
};

#endif // GSLIBPARVARWEIGHT_H
