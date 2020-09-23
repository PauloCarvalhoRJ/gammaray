#ifndef SECTION_H
#define SECTION_H

#include "domain/file.h"

/**
 * The Section class represents arbitray geologic sections.  A Section is a 2D grid standing up (XZ panel)
 * and "folded" along a sequence of XY coordinates.
 * A Section object is actually a composition of two objects:
 *
 * 1) A PointSet containing the points that define the path of the section.  This PointSet has the following
 *    columns: X Y Z1 Z2 C1 C2 -> X and Y: the areal coordinates of each "fold"; Z1 and Z2: the starting and
 *    ending depths of the "fold"; C1 and C2: the starting and ending column indexes of the 2D grid.  This
 *    PointSet must have at least two data rows of a minimum Section.
 *
 * 2) A CartesianGrid containing the data contained in the Section.  It is a 2D grid with I columns, K
 *    layers and only one row forming a "wall", which is "folded" at the XY locations defined by the PointSet
 *    object.
 *
 */
class Section : public File
{
public:
    Section( QString path );

    //File interface
//public:
//    virtual void deleteFromFS();
//    virtual void writeToFS();
//    virtual void readFromFS();

    // ProjectComponent interface
public:
    virtual QIcon getIcon();
    virtual bool isFile() const { return false; }
    virtual bool isAttribute() { return false; }
    virtual QString getObjectLocator();
    virtual QString getTypeName() const { return "SECTION"; }
};

#endif // SECTION_H
