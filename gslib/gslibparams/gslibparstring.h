#ifndef GSLIBPARSTRING_H
#define GSLIBPARSTRING_H

#include "gslibpartype.h"
#include <QString>

/**
 * @brief The GSLibParString class represents a single textual parameter, normally a plot title.
 */
class GSLibParString : public GSLibParType
{
public:
    GSLibParString(const QString name, const QString label, const QString description);
    QString _value;

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "string";}
    QWidget* getWidget();
    bool update();
    GSLibParString* clone();
    bool isCollection() { return false; }
};

#endif // GSLIBPARSTRING_H
