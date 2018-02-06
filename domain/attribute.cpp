#include "attribute.h"
#include "file.h"
#include "pointset.h"
#include "util.h"
#include "viewer3d/view3dconfigwidgetsbuilder.h"
#include "cartesiangrid.h"

Attribute::Attribute(QString name, int index_in_file, bool categorical) :
    IJAbstractVariable()
{
    this->_name = name;
    this->_index = index_in_file;
    _categorical = categorical;
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

bool Attribute::isCategorical()
{
    return _categorical;
}

void Attribute::setCategorical(bool value)
{
    _categorical = value;
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
                if( this->_index ==  ps->getXindex() ) {
                    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
                        return QIcon(":icons/xcoord16");
                    else
                        return QIcon(":icons32/xcoord32");
                }
                if( this->_index ==  ps->getYindex() ){
                    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
                        return QIcon(":icons/ycoord16");
                    else
                        return QIcon(":icons32/ycoord32");
                }
                if( this->_index ==  ps->getZindex() ){
                    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
                        return QIcon(":icons/zcoord16");
                    else
                        return QIcon(":icons32/zcoord32");
                }
            }
        }
    }
    if( isCategorical() ){
        if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
            return QIcon(":icons/catvar16");
        else
            return QIcon(":icons32/catvar32");
    }
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI )
        return QIcon(":icons/var");
    else
        return QIcon(":icons/var32");
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

QString Attribute::getObjectLocator()
{
    return _parent->getObjectLocator() + '/' + getName();
}

View3DViewData Attribute::build3DViewObjects(View3DWidget *widget3D)
{
    return View3DBuilders::build( this, widget3D );
}

View3DConfigWidget *Attribute::build3DViewerConfigWidget( View3DViewData viewObjects )
{
	return View3DConfigWidgetsBuilder::build( this, viewObjects );
}

IJAbstractCartesianGrid *Attribute::getParentGrid()
{
    File* file = getContainingFile();
    if( file->getFileType() == "CARTESIANGRID" )
		return dynamic_cast<CartesianGrid*>(file);
    else
        return nullptr;
	return nullptr;
}

int Attribute::getIndexInParentGrid()
{
    return getAttributeGEOEASgivenIndex() - 1;
}
