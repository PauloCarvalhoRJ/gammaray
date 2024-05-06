#ifndef KT3DTRENDMODELPARAMETERS_H
#define KT3DTRENDMODELPARAMETERS_H

#include "gslib/gslibparameterfiles/gslibparameterfile.h"

/** Specialization of GSLibParameterFile used to store kt3d's trend model's nine parameters:
 * Ax² + Bxy + Cxz + Dy² + Eyz + Fz² + Gx + Hy + Iz
 */
class Kt3dTrendModelParameters : public GSLibParameterFile
{
public:

    Kt3dTrendModelParameters();

    double getA();
    double getB();
    double getC();
    double getD();
    double getE();
    double getF();
    double getG();
    double getH();
    double getI();

    void setA( double value );
    void setB( double value );
    void setC( double value );
    void setD( double value );
    void setE( double value );
    void setF( double value );
    void setG( double value );
    void setH( double value );
    void setI( double value );
};

#endif // KT3DTRENDMODELPARAMETERS_H
