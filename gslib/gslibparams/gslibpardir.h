#ifndef GSLIBPARDIR_H
#define GSLIBPARDIR_H

#include "gslib/gslibparams/gslibpartype.h"

class GSLibParDir : public GSLibParType
{
public:
    GSLibParDir(const QString name, const QString label, const QString description);
    QString _path;

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "dir";}
    QWidget *getWidget();
    bool update();
    GSLibParDir *clone();
    bool isCollection() { return false; }};

#endif // GSLIBPARDIR_H
