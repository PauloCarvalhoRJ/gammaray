#include "quintuplets.h"

#include <QRegularExpression>
#include <iostream>

//-------------------specializations of the convert() template function---------------//
namespace quintupletvalueconverter {
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
    template <>
    QColor convert<QColor>( QString value ){
        QRegularExpression re("(\\d+)"); //string example "(123,2,42)"
        QColor color;
        QRegularExpressionMatchIterator i = re.globalMatch( value );
        int control = 0;
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            if (match.hasMatch()) {
                QString level = match.captured(1);
                switch( control ){
                case 0: color.setRed  ( level.toInt() ); break;
                case 1: color.setGreen( level.toInt() ); break;
                case 2: color.setBlue ( level.toInt() ); break;
                }
            }
            ++control;
        }
        return color;
    }
}
//------------------------------------------------------------------------------------//

//-------------------specializations of the toText() template function---------------//
namespace quintupletvalueconverter {
    template <>
    QString toText<int>( int value ){
        return QString::number( value );
    }
    template <>
    QString toText<QString>( QString value ){
        return value;
    }
    template <>
    QString toText<QColor>( QColor value ){
        return "(" + QString::number( value.red()   ) + "," +
                     QString::number( value.green() ) + "," +
                     QString::number( value.blue()  ) + ")";
    }
}
//------------------------------------------------------------------------------------//
