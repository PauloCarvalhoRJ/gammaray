#ifndef GSLIBPARFILE_H
#define GSLIBPARFILE_H
#include "gslibpartype.h"
#include <QString>

/**
 * @brief The GSLibParFile class represents a single file path such as an output file path.
 */
class GSLibParFile : public GSLibParType
{
public:
    GSLibParFile(const QString name, const QString label, const QString description);
    QString _path;

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "file";}
    QWidget *getWidget();
    bool update();
    GSLibParFile *clone();
    bool isCollection() { return false; }
};

#endif // GSLIBPARFILE_H
