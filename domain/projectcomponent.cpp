#include "projectcomponent.h"
#include "viewer3d/view3dconfigwidgetsbuilder.h"
#include "viewer3d/view3dbuilders.h"

ProjectComponent::ProjectComponent()
{
    _parent = nullptr;
}

ProjectComponent::~ProjectComponent()
{
}

QString ProjectComponent::getPresentationName()
{
    return this->getName();
}

QString ProjectComponent::getNameWithParents() const
{
    if( hasParent() )
        return getName() + " (" + _parent->getName() + ')';
    else
        return getName();
}

bool ProjectComponent::hasChildren()
{
    return !(this->_children.empty());
}

void ProjectComponent::addChild(ProjectComponent *child)
{
    this->_children.push_back( child );
}

void ProjectComponent::removeChild(ProjectComponent *child)
{
   for (std::vector<ProjectComponent*>::iterator it = _children.begin() ; it != _children.end(); ++it)
      if ( *it == child ){
          _children.erase( it );
          //TODO: delete (destroy) child and it's own children (can I ignore this memory leak?)
          break; //abandon loop since the iterator after removal point becomes invalidated
      }
}

bool ProjectComponent::hasParent() const
{
    return this->_parent;
}

void ProjectComponent::setParent(ProjectComponent *parent)
{
    this->_parent = parent;
}

ProjectComponent *ProjectComponent::getChildByIndex(int index) const
{
    return this->_children[index];
}

ProjectComponent *ProjectComponent::getParent() const
{
    return this->_parent;
}

int ProjectComponent::getChildCount() const
{
    return this->_children.size();
}

int ProjectComponent::getIndexInParent()
{
    return this->_parent->getChildIndex( this );
}

int ProjectComponent::getChildIndex(ProjectComponent *child)
{
    int i = 0;
    for (std::vector<ProjectComponent*>::iterator it = _children.begin() ; it != _children.end(); ++it, ++i)
       if ( *it == child )
          return i;
    return -1;
}

ProjectComponent *ProjectComponent::getChildByName(QString name, bool recurse )
{
    for (std::vector<ProjectComponent*>::iterator it = _children.begin() ; it != _children.end(); ++it)
       if ( (*it)->getName() == name ){
          return *it;
       } else if ( recurse ) {
           ProjectComponent* grandChild = (*it)->getChildByName( name, true );
           if( grandChild )
               return grandChild;
       }
    return nullptr;
}

bool ProjectComponent::isChild(ProjectComponent *pc)
{
    for (std::vector<ProjectComponent*>::iterator it = _children.begin() ; it != _children.end(); ++it)
       if ( (*it) == pc )
          return true;
    return false;
}

void ProjectComponent::save(QTextStream */*txt_stream*/)
{

}

void ProjectComponent::getAllObjects( std::vector<ProjectComponent *> &result ) const
{
    for (std::vector<ProjectComponent*>::const_iterator it = _children.begin() ; it != _children.end(); ++it )
    {
        result.push_back( *it );
        (*it)->getAllObjects( result );
    }
}

ProjectComponent *ProjectComponent::findObject(const QString object_locator)
{
    //if the locator matches this object's
    if( this->getObjectLocator().compare( object_locator ) == 0)
        return this; //...returns itself
    else //...otherwise, tries its children
        for (std::vector<ProjectComponent*>::iterator it = _children.begin() ; it != _children.end(); ++it){
           ProjectComponent* pc = (*it)->findObject( object_locator );
           if( pc )
               return pc;
        }
    return nullptr; //returns nullptr if no match is found
}

View3DViewData ProjectComponent::build3DViewObjects( View3DWidget * widget3D )
{
    return View3DBuilders::build( this, widget3D );
}

View3DConfigWidget *ProjectComponent::build3DViewerConfigWidget(View3DViewData viewObjects)
{
    return View3DConfigWidgetsBuilder::build( this, viewObjects );
}
