#ifndef PROJECTROOT_H
#define PROJECTROOT_H

#include "projectcomponent.h"
#include <QIcon>
#include <QString>

/**
 * @brief The ProjectRoot class only serves as the project root for the
 * project tree model object.
 */
class ProjectRoot : public ProjectComponent
{
public:
    ProjectRoot();

    // ProjectComponent interface
public:
	QString getName() const;
    QIcon getIcon();
    bool isFile() const;
    bool isAttribute();
    virtual QString getObjectLocator();
    virtual QString getTypeName() const { return "ProjectRoot"; }
};

#endif // PROJECTROOT_H
