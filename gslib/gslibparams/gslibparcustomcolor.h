#ifndef GSLIBPARCUSTOMCOLOR_H
#define GSLIBPARCUSTOMCOLOR_H

#include "gslibpartype.h"

/**
 * @brief The GSLibParCustomColor class represents a custom RGB color (see GSLibParColor for standard categorical GSLib colors).
 * The color can be used either for categorical or continuous values.
 */
class GSLibParCustomColor : public GSLibParType
{
public:
    GSLibParCustomColor(const QString name, const QString label, const QString description);
    int _r; //values must vary between 0 and 255
    int _g;
    int _b;

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "customcolor";}
    QWidget* getWidget();
    bool update();
    bool isCollection() { return false; }
    GSLibParCustomColor* clone();
};

#endif // GSLIBPARCUSTOMCOLOR_H
