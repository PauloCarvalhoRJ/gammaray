#include "attribute.h"
#include "file.h"
#include "pointset.h"

Attribute::Attribute(QString name, int index_in_file)
{
    this->_name = name;
    this->_index = index_in_file;
}

File *Attribute::getContainingFile()
{
    ProjectComponent *pc = this;
    while( pc->hasParent() ){
        pc = pc->getParent();
        if( pc->isFile() )
            return (File*)pc;
    }
    return nullptr;
}


QString Attribute::getName()
{
    return this->_name;
}

QIcon Attribute::getIcon()
{
    if( this->getParent()->isFile() ){ //most attributes have a File as parent, but not always
        File* parent_file = (File*)this->getParent();
        if( parent_file){
            if( parent_file->getFileType() == "POINTSET" ){
                PointSet* ps = (PointSet*)parent_file;
                if( this->_index ==  ps->getXindex() )
                    return QIcon(":icons/xcoord16");
                if( this->_index ==  ps->getYindex() )
                    return QIcon(":icons/ycoord16");
                if( this->_index ==  ps->getZindex() )
                    return QIcon(":icons/zcoord16");
            }
        }
    }
    return QIcon(":icons/var");
}


bool Attribute::isFile()
{
    return false;
}

bool Attribute::isAttribute()
{
    return true;
}

QString Attribute::getPresentationName()
{
    return QString("[").append( QString::number(_index) ).append("]-").append( _name );
}
