#ifndef GSLIBPARINT_H
#define GSLIBPARINT_H

#include "gslibpartype.h"

/**
 * @brief The GSLibParInt class represents a single integer parameter.
 */
class GSLibParInt : public GSLibParType
{
public:
    GSLibParInt(const QString name, const QString label, const QString description);
    int _value;

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "int";}
    QWidget* getWidget();
    bool update();
    bool isCollection() { return false; }
    GSLibParInt* clone();
};

#endif // GSLIBPARINT_H
