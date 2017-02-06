#ifndef GSLIBPARRANGE_H
#define GSLIBPARRANGE_H

#include "gslibpartype.h"

/**
 * @brief The GSLibParRange class represents a paramater that can vary within
 * a given range.
 */
class GSLibParRange : public GSLibParType
{
public:
    GSLibParRange(const QString name, const QString label, const QString description);
    double _min;
    double _max;
    QString _min_label;
    QString _max_label;
    double _value;

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "range";}
    QWidget* getWidget();
    bool update();
    bool isCollection() { return false; }
};

#endif // GSLIBPARRANGE_H
