#ifndef GSLIBPARVMODEL_H
#define GSLIBPARVMODEL_H

#include "gslibpartype.h"

class GSLibParMultiValuedFixed;
class GSLibParRepeat;
class VariogramModel;

class GSLibParVModel : public GSLibParType
{
public:
    GSLibParVModel(const QString name, const QString label, const QString description);
    ~GSLibParVModel();

    GSLibParMultiValuedFixed* _nst_and_nugget;
    GSLibParRepeat* _variogram_structures;

    /** Makes an isotropic variogram with unity sill and ranges without nugget effect.  */
    void makeDefault();

    /** Sets the variogram parameters according to the given VariogramModel object. */
    void setFromVariogramModel(VariogramModel *vmodel );

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "vmodel";}
    QWidget* getWidget();
    GSLibParVModel* clone();
    bool update();
    bool isCollection() { return true; }
};

#endif // GSLIBPARVMODEL_H
