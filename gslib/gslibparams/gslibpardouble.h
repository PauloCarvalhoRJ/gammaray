#ifndef GSLIBPARDOUBLE_H
#define GSLIBPARDOUBLE_H

#include "gslibpartype.h"

/**
 * @brief The GSLibParDouble class representes a single double parameter.
 */
class GSLibParDouble : public GSLibParType
{
public:
    GSLibParDouble(const QString name, const QString label, const QString description);
    ~GSLibParDouble();
    double _value;

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "double";}
    QWidget* getWidget();
    bool update();
    GSLibParDouble *clone();
    bool isCollection() { return false; }
};

#endif // GSLIBPARDOUBLE_H
