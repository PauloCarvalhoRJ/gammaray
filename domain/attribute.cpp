#include "attribute.h"
#include "file.h"
#include "pointset.h"
#include "util.h"
#include "viewer3d/view3dconfigwidgetsbuilder.h"
#include "cartesiangrid.h"
#include "viewer3d/view3dbuilders.h"
#include "domain/segmentset.h"

Attribute::Attribute(QString name, int index_in_file, bool categorical) :
    IJAbstractVariable(), ICalcProperty()
{
    this->_name = name;
    this->_index = index_in_file;
    _categorical = categorical;
}

File *Attribute::getContainingFile() const
{
    const ProjectComponent *pc = this;
    while( pc->hasParent() ){
        pc = pc->getParent();
        if( pc->isFile() )
            return (File*)pc; //surely pc is not this if execution reaches this line, so it is safe to cast const ponter to non-const poitner here.
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


QString Attribute::getName() const
{
    return this->_name;
}

QIcon Attribute::getIcon()
{
	if( this->getParent()->isFile() ){ //most attributes have a File as parent, but not always
        File* parent_file = (File*)this->getParent();
        if( parent_file){
            if( parent_file->getFileType() == "POINTSET" || parent_file->getFileType() == "SEGMENTSET" ){
                PointSet* ps = (PointSet*)parent_file; //SegmentSet is a subclass of PointSet
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
            if( parent_file->getFileType() == "SEGMENTSET" ){
                SegmentSet* ps = static_cast<SegmentSet*>(parent_file);
                if( this->_index ==  ps->getXFinalIndex() ) {
                    return QIcon(":icons32/xcoordf32");
                }
                if( this->_index ==  ps->getYFinalIndex() ){
                    return QIcon(":icons32/ycoordf32");
                }
                if( this->_index ==  ps->getZFinalIndex() ){
                    return QIcon(":icons32/zcoordf32");
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
        return QIcon(":icons32/var32");
}


bool Attribute::isFile() const
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
	return nullptr;
}

int Attribute::getIndexInParentGrid() const
{
    return getAttributeGEOEASgivenIndex() - 1;
}

QIcon Attribute::getVariableIcon()
{
    return getIcon();
}

QString Attribute::getVariableName()
{
	return getName();
}
