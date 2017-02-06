#ifndef GSLIBPARLIMITSDOUBLE_H
#define GSLIBPARLIMITSDOUBLE_H
#include "gslibpartype.h"

/**
 * @brief The GSLibParLimitsDouble class represents a pair of doubles
 * to stabilish an interval or limits such as trimming limits.
 * First value is the minimum.  Example:
 *
 *  0.0      20.0               -attribute minimum and maximum
 */
class GSLibParLimitsDouble : public GSLibParType
{
public:
    GSLibParLimitsDouble(const QString name, const QString label, const QString description);
    double _min;
    double _max;

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "limits_double";}
    QWidget* getWidget();
    bool update();
    bool isCollection() { return false; }
};

#endif // GSLIBPARLIMITSDOUBLE_H
