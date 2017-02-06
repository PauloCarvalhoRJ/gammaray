#ifndef GSLIBPAROPTION_H
#define GSLIBPAROPTION_H

#include "gslibpartype.h"
#include <QMap>
#include <QString>

/**
 * @brief The GSLibParOption class represents a set of option values.  Each option
 * is composed by a value accepted by a GSLib program and a description with the meaning
 * of the value.  Example:
 *
 * 0                            -0=arithmetic, 1=log scaling
 */
class GSLibParOption : public GSLibParType
{
public:
    GSLibParOption(const QString name, const QString label, const QString description);
    void addOption(const uint value, const QString description);
    QMap<uint, QString> _options; //the possible values and their descriptions
    uint _selected_value; //must be one of the vaues in _options

    // GSLibParType interface
public:
    void save(QTextStream *out);
    QString getTypeName() {return "option";}
    QWidget* getWidget();
    bool update();
    GSLibParOption *clone();
    bool isCollection() { return false; }
};

#endif // GSLIBPAROPTION_H
