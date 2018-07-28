#ifndef PROJECTCOMPONENT_H
#define PROJECTCOMPONENT_H

#include <QString>
#include <QIcon>
#include <vector>

#include "viewer3d/view3dbuilders.h"
#include "viewer3d/view3dviewdata.h"

class QTextStream;
class View3DConfigWidget;
class View3DWidget;

/**
 * @brief The ProjectComponent class models any part of a project such as data files, variograms, training images,
 * workflows, data fields, parameter files, object groups, etc.
 */
class ProjectComponent
{
public:
    ProjectComponent();
    virtual ~ProjectComponent();
	virtual QString getName() const = 0; //user-given name to identify the object
    virtual QString getPresentationName(); //text composition, by default equals getName(), but may include other information such as parameter index
    virtual QIcon getIcon() = 0;
    virtual bool isFile() = 0;
    virtual bool isAttribute() = 0;
    virtual QString getTypeName() = 0;

    /** Returns a string that uniquely identify an object in the project.
     *  Use the findObject() function to fetch the object pointer given a locator.
     */
    virtual QString getObjectLocator() = 0;
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
    ProjectComponent* getChildByName( QString name, bool recurse = false );
    bool isChild( ProjectComponent* pc );
    virtual void save( QTextStream* txt_stream );
    /** Fills the given list with all ProjectComponents contained within.
     * It returns all children objects, their children, etc.  It is recursive call.
     */
    void getAllObjects( std::vector<ProjectComponent*> &result );
    /** Recursively searches for an object matching the given object locator. */
    ProjectComponent* findObject( const QString object_locator );
    void dummyCall(){ int x; x = 2; ++x; } //used to test pointer validity (TODO: not used anywhere)

    /** Builds the objects (e.g. VTKActor) to enable 3D display. This default implementation is an ineffective call.
     * Subclasses should not store the objects created, since the same domain object may be viewed multiple times,
     * possibly in different ways.  Implementations should only use the information in this object to build appropriate
     * visual objects.
     */
    virtual View3DViewData build3DViewObjects( View3DWidget *widget3D  );

    /** Builds a 3D Viewer configuration widget. This default implementation is an ineffective call.
     * viewObjects contains the objects describing the visual appearance of a domain object (mostly VTK objects).
     */
    virtual View3DConfigWidget* build3DViewerConfigWidget( View3DViewData viewObjects );

protected:
    std::vector<ProjectComponent*> _children;
    ProjectComponent* _parent;
};

#endif // PROJECTCOMPONENT_H
