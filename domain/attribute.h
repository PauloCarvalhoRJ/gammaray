#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H
#include "projectcomponent.h"
#include <QString>

class File;

class Attribute : public ProjectComponent
{
public:
    /** Constructor. Index in GEO-EAS format begins with 1, not zero. */
    Attribute( QString name, int index_in_file, bool categorical = false );

    /** Returns the File object containing this Attribute.
     *  The File can be a parent, grand-parent, etc. object.
     *  Returns a null pointer if the search goes all the way up to the ProjectRoot object.
     */
    File* getContainingFile();

    /**
     * Returns the GEO-EAS index passed in the constructor,
     * so this is not necessarily read from a GEO-EAS file.
     */
    int getAttributeGEOEASgivenIndex(){ return _index; }

    /** Returns whether this attribute was considered as a categorical variable. */
    bool isCategorical();

    /** Sets whether this attribute was considered as a categorical variable. */
    void setCategorical( bool value );

    // ProjectComponent interface
public:
    QString getName();
    QIcon getIcon();
    bool isFile();
    bool isAttribute();
    QString getPresentationName();
    virtual QString getObjectLocator();
    virtual vtkSmartPointer<vtkProp> buildVTKActor();

private:
    QString _name;
    int _index; //index in GEO-EAS format begins with 1, not zero.
    bool _categorical;
};

#endif // ATTRIBUTE_H
