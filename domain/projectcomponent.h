#ifndef PROJECTCOMPONENT_H
#define PROJECTCOMPONENT_H

#include <QString>
#include <QIcon>
#include <vector>

class QTextStream;

/**
 * @brief The ProjectComponent class models any part of a project such as data files, variograms, training images,
 * workflows, data fields, parameter files, object groups, etc.
 */
class ProjectComponent
{
public:
    ProjectComponent();
    virtual ~ProjectComponent();
    virtual QString getName() = 0; //user-given name to identify the object
    virtual QString getPresentationName(); //text composition, by default equals getName(), but may include other information such as parameter index
    virtual QIcon getIcon() = 0;
    virtual bool isFile() = 0;
    virtual bool isAttribute() = 0;
    virtual bool hasChildren();
    virtual void addChild( ProjectComponent* child );
    virtual void removeChild( ProjectComponent* child );
    virtual bool hasParent();
    virtual void setParent( ProjectComponent* parent );
    virtual ProjectComponent* getChildByIndex( int index );
    virtual ProjectComponent* getParent();
    virtual int getChildCount();
    virtual int getIndexInParent();
    virtual int getChildIndex( ProjectComponent* child );
    ProjectComponent* getChildByName( QString name );
    bool isChild( ProjectComponent* pc );
    virtual void save( QTextStream* txt_stream );
    /** Fills the given list with all ProjectComponents contained within.
     * It returns all children objects, their children, etc.  It is recursive call.
     */
    void getAllObjects( std::vector<ProjectComponent*> &result );
    void dummyCall(){ int x; x = 2; ++x; } //used to test pointer validity (TODO: not used anywhere)
protected:
    std::vector<ProjectComponent*> _children;
    ProjectComponent* _parent;
};

#endif // PROJECTCOMPONENT_H
