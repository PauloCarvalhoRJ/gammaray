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
    QString getName();
    QIcon getIcon();
    bool isFile();
    bool isAttribute();
    virtual QString getObjectLocator();
    virtual QString getTypeName(){ return "ProjectRoot"; }
};

#endif // PROJECTROOT_H
