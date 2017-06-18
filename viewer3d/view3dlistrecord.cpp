#include "view3dlistrecord.h"

View3DListRecord::View3DListRecord() :
    objectLocator( "" ),
    instance( 0 )
{

}

View3DListRecord::View3DListRecord(QString p_object_locator, uint p_instance) :
    objectLocator( p_object_locator ),
    instance( p_instance )
{
}

QString View3DListRecord::getDescription() const
{
    return objectLocator + "[" + QString::number( instance ) + "]";
}

