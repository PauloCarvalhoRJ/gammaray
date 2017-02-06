#ifndef GSLIBPARUINT_H
#define GSLIBPARUINT_H

#include "gslibpartype.h"

/**
 * @brief The GSLibParUInt class represents a single unsigned integer parameter.
 */
class GSLibParUInt : public GSLibParType
{
public:
    GSLibParUInt(const QString name, const QString label, const QString description);
    uint _value;

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "uint";}
    QWidget *getWidget();
    bool update();
    GSLibParUInt* clone();
    bool isCollection() { return false; }
};

#endif // GSLIBPARUINT_H
