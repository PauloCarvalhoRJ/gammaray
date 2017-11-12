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

    /** Shortcut getter methods. */
    uint getNX();
    double getDX();
    double getX0();
    uint getNY();
    double getDY();
    double getY0();
    uint getNZ();
    double getDZ();
    double getZ0();

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "grid";}
    QWidget* getWidget();
    bool update();
    bool isCollection() { return true; }
};

#endif // GSLIBPARGRID_H
