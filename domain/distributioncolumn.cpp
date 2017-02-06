#include "distributioncolumn.h"

DistributionColumn::DistributionColumn(QString name, int index_in_file ) :
    Attribute( name, index_in_file ),
    m_role( Roles::DistributionColumnRole::UNDEFINED )
{
}

void DistributionColumn::setRole(Roles::DistributionColumnRole role)
{
    m_role = role;
}

Roles::DistributionColumnRole DistributionColumn::getRole()
{
    return m_role;
}

QIcon DistributionColumn::getIcon()
{
    return Roles::getRoleIcon( m_role );
}

