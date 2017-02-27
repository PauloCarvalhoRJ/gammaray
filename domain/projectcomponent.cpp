#include "projectcomponent.h"

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

bool ProjectComponent::hasParent()
{
    return this->_parent;
}

void ProjectComponent::setParent(ProjectComponent *parent)
{
    this->_parent = parent;
}

ProjectComponent *ProjectComponent::getChildByIndex(int index)
{
    return this->_children[index];
}

ProjectComponent *ProjectComponent::getParent()
{
    return this->_parent;
}

int ProjectComponent::getChildCount()
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

void ProjectComponent::save(QTextStream */*txt_stream*/)
{

}

void ProjectComponent::getAllObjects( std::vector<ProjectComponent *> &result )
{
    for (std::vector<ProjectComponent*>::iterator it = _children.begin() ; it != _children.end(); ++it )
    {
        result.push_back( *it );
        (*it)->getAllObjects( result );
    }
}
