#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H
#include "projectcomponent.h"
#include <QString>

class File;

class Attribute : public ProjectComponent
{
public:
    /** Constructor. Index in GEO-EAS format begins with 1, not zero. */
    Attribute( QString name, int index_in_file );

    /** Returns the File object containing this Attribute.
     *  The File can be a parent, grand-parent, etc. object.
     *  Returns a null pointer if the search goes all the way up to the ProjectRoot object.
     */
    File* getContainingFile();

    // ProjectComponent interface
public:
    QString getName();
    QIcon getIcon();
    bool isFile();
    bool isAttribute();
    QString getPresentationName();

private:
    QString _name;
    int _index; //index in GEO-EAS format begins with 1, not zero.
};

#endif // ATTRIBUTE_H
