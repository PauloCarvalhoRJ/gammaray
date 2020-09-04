#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H
#include "projectcomponent.h"
#include "imagejockey/ijabstractvariable.h"
#include "calculator/icalcproperty.h"
#include <QString>

class File;

class Attribute : public ProjectComponent, public IJAbstractVariable, public ICalcProperty
{
public:
    /** Constructor. Index in GEO-EAS format begins with 1, not zero. */
    Attribute( QString name, int index_in_file, bool categorical = false );

    /** Returns the File object containing this Attribute.
     *  The File can be a parent, grand-parent, etc. object.
     *  Returns a null pointer if the search goes all the way up to the ProjectRoot object.
     */
    File* getContainingFile() const;

    /**
     * Returns the GEO-EAS index passed in the constructor,
     * so this is not necessarily read from a GEO-EAS file.
     */
	int getAttributeGEOEASgivenIndex() const { return _index; }

    /** Returns whether this attribute was considered as a categorical variable. */
    bool isCategorical();

    /** Sets whether this attribute was considered as a categorical variable. */
    void setCategorical( bool value );

    // ProjectComponent interface
public:
	virtual QString getName() const;
	virtual QIcon getIcon();
    virtual bool isFile() const;
	virtual bool isAttribute();
	virtual QString getPresentationName();
    virtual QString getObjectLocator();
    virtual View3DViewData build3DViewObjects( View3DWidget * widget3D );
    virtual QString getTypeName() const { return "Attribute"; }
    virtual View3DConfigWidget* build3DViewerConfigWidget(View3DViewData viewObjects);

//IJAbstractVariable interface
public:
    /** Returns null pointer if parent object is not a Cartesian grid. */
    virtual IJAbstractCartesianGrid* getParentGrid();
	virtual int getIndexInParentGrid() const;
	virtual QString getVariableName();
    virtual QIcon getVariableIcon();

// ICalcProperty interface
public:
    virtual QString getCalcPropertyName(){ return getName(); }
    virtual QIcon getCalcPropertyIcon(){ return getIcon(); }
	virtual int getCalcPropertyIndex(){ return getIndexInParent(); }

private:
    QString _name;
    int _index; //index in GEO-EAS format begins with 1, not zero.
    bool _categorical;

};

#endif // ATTRIBUTE_H
