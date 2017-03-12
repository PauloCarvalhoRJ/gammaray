#include "triads.h"

//-------------------specializations of the convert() template function---------------//
namespace triadvalueconverter {
    template <>
    double convert<double>( QString value ){
        return value.toDouble();
    }
    template <>
    float convert<float>( QString value ){
        return value.toFloat();
    }
    template <>
    int convert<int>( QString value ){
        return value.toInt();
    }
    template <>
    QString convert<QString>( QString value ){
        return value;
    }
}
//------------------------------------------------------------------------------------//
