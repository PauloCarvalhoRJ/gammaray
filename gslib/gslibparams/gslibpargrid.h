#ifndef GSLIBPARGRID_H
#define GSLIBPARGRID_H

#include "gslibpartype.h"

class GSLibParMultiValuedFixed;
class CartesianGrid;

/**
 * @brief The GSLibParGrid represents a grid geometry specification.
 */
class GSLibParGrid : public GSLibParType
{
public:
    GSLibParGrid(const QString name, const QString label, const QString description);
    ~GSLibParGrid();

    GSLibParMultiValuedFixed* _specs_x;
    GSLibParMultiValuedFixed* _specs_y;
    GSLibParMultiValuedFixed* _specs_z;

    //sets the parameter values from the grid parameters of the
    //Cartesian grid passed as parameter.
    void setFromCG( CartesianGrid* cg );

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "grid";}
    QWidget* getWidget();
    bool update();
    bool isCollection() { return true; }
};

#endif // GSLIBPARGRID_H
