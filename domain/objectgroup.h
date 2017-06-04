#ifndef OBJECTGROUP_H
#define OBJECTGROUP_H

#include "projectcomponent.h"
#include <QString>
#include <QIcon>

/**
 * @brief The ObjectGroup class models project object groups such as Variograms, Data Files, Parameters, etc.
 * An object group serves only to visually organize the project in the project tree, as it is not saved to disc.
 */
class ObjectGroup : public ProjectComponent
{
public:
    /**
     * @brief ObjectGroup constructor.
     * @param name User-friendly name.
     * @param icon Icon that represents this groups in UI.
     * @param internal_name Name by which this group is identified by internal logic (such as saved in project file).
     */
    ObjectGroup( const QString name, const QIcon icon, const QString internal_name );
private:
    QString _name;
    QIcon _icon;
    QString _internal_name;

    // ProjectComponent interface
public:
    QString getName();
    QIcon getIcon();

    // ProjectComponent interface
public:
    void save(QTextStream *txt_stream);
    bool isFile();
    bool isAttribute();
    virtual QString getObjectLocator();
};

#endif // OBJECTGROUP_H
