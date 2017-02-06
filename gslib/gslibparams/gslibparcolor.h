#ifndef GSLIBPARCOLOR_H
#define GSLIBPARCOLOR_H

#include "gslibpartype.h"

/**
 * @brief The GSLibParColor class represents a single unsigned integer parameter that is a color code:
 *  1=red, 2=orange, 3=yellow, 4=light green, 5=green, 6=light blue, 7=dark blue, 8=violet, 9=white,
 *  10=black, 11=purple, 12=brown, 13=pink, 14=intermediate green, 15=gray 16=gray 10%, 17=gray 20%,
 *  18=gray 30%, 19=gray 40%, 20=gray 50%, 21=gray 60% 22=gray 70%, 23=gray 80%, 24=gray 90%.
 */
class GSLibParColor : public GSLibParType
{
public:
    GSLibParColor( const QString name, const QString label, const QString description );
    uint _color_code;

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "color";}
    QWidget *getWidget();
    //bool update();
    GSLibParColor* clone();
    bool isCollection() { return false; }
};

#endif // GSLIBPARCOLOR_H
