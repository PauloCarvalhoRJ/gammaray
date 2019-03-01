#include "verticaltransiogrammodel.h"

VerticalTransiogramModel::VerticalTransiogramModel(QString path,
                                                   QString associatedCategoryDefinitionName ) :
        File( path ),
        m_associatedCategoryDefinitionName( associatedCategoryDefinitionName )
{

}

QIcon VerticalTransiogramModel::getIcon()
{
    return QIcon(":icons32/transiogram32");
}
