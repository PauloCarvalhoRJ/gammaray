#ifndef DISTRIBUTIONCOLUMN_H
#define DISTRIBUTIONCOLUMN_H

#include "attribute.h"
#include "roles.h"

/** The DistributionColumn class represents the Attribute of a distribution file (uni or bivariate).*/
class DistributionColumn : public Attribute
{
public:
    DistributionColumn( QString name, int index_in_file );
    void setRole( Roles::DistributionColumnRole role );
    Roles::DistributionColumnRole getRole();

    // ProjectComponent interface
public:
    QIcon getIcon();

private:
    Roles::DistributionColumnRole m_role;
};

#endif // DISTRIBUTIONCOLUMN_H
