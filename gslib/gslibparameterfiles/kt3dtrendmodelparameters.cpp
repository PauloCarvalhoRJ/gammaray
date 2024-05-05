#include "kt3dtrendmodelparameters.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"

Kt3dTrendModelParameters::Kt3dTrendModelParameters() :
    GSLibParameterFile({
                          "<double>         -x²",  // 0
                          "<double>         -xy",  // 1
                          "<double>         -xz",  // 2
                          "<double>         -y²",  // 3
                          "<double>         -yz",  // 4
                          "<double>         -z²",  // 5
                          "<double>         -x",   // 6
                          "<double>         -y",   // 7
                          "<double>         -z",   // 8
                      })
{
    getParameter<GSLibParDouble*>(0)->_value = 0.0;
    getParameter<GSLibParDouble*>(1)->_value = 0.0;
    getParameter<GSLibParDouble*>(2)->_value = 0.0;
    getParameter<GSLibParDouble*>(3)->_value = 0.0;
    getParameter<GSLibParDouble*>(4)->_value = 0.0;
    getParameter<GSLibParDouble*>(5)->_value = 0.0;
    getParameter<GSLibParDouble*>(6)->_value = 0.0;
    getParameter<GSLibParDouble*>(7)->_value = 0.0;
    getParameter<GSLibParDouble*>(8)->_value = 0.0;
}

double Kt3dTrendModelParameters::getA()
{
    return getParameter<GSLibParDouble*>(0)->_value;
}

double Kt3dTrendModelParameters::getB()
{
    return getParameter<GSLibParDouble*>(1)->_value;
}

double Kt3dTrendModelParameters::getC()
{
    return getParameter<GSLibParDouble*>(2)->_value;
}

double Kt3dTrendModelParameters::getD()
{
    return getParameter<GSLibParDouble*>(3)->_value;
}

double Kt3dTrendModelParameters::getE()
{
    return getParameter<GSLibParDouble*>(4)->_value;
}

double Kt3dTrendModelParameters::getF()
{
    return getParameter<GSLibParDouble*>(5)->_value;
}

double Kt3dTrendModelParameters::getG()
{
    return getParameter<GSLibParDouble*>(6)->_value;
}

double Kt3dTrendModelParameters::getH()
{
    return getParameter<GSLibParDouble*>(7)->_value;
}

double Kt3dTrendModelParameters::getI()
{
    return getParameter<GSLibParDouble*>(8)->_value;
}

void Kt3dTrendModelParameters::setA(double value)
{
    getParameter<GSLibParDouble*>(0)->_value = value;
}

void Kt3dTrendModelParameters::setB(double value)
{
    getParameter<GSLibParDouble*>(1)->_value = value;
}

void Kt3dTrendModelParameters::setC(double value)
{
    getParameter<GSLibParDouble*>(2)->_value = value;
}

void Kt3dTrendModelParameters::setD(double value)
{
    getParameter<GSLibParDouble*>(3)->_value = value;
}

void Kt3dTrendModelParameters::setE(double value)
{
    getParameter<GSLibParDouble*>(4)->_value = value;
}

void Kt3dTrendModelParameters::setF(double value)
{
    getParameter<GSLibParDouble*>(5)->_value = value;
}

void Kt3dTrendModelParameters::setG(double value)
{
    getParameter<GSLibParDouble*>(6)->_value = value;
}

void Kt3dTrendModelParameters::setH(double value)
{
    getParameter<GSLibParDouble*>(7)->_value = value;
}

void Kt3dTrendModelParameters::setI(double value)
{
    getParameter<GSLibParDouble*>(8)->_value = value;
}
