#ifndef SECTION_H
#define SECTION_H

#include "domain/file.h"

class PointSet;
class CartesianGrid;

/**
 * The Section class represents arbitray geologic sections.  A Section is a 2D grid standing up (XZ panel)
 * and "folded" along a sequence of XY coordinates.
 * A Section object is actually a composition of two objects:
 *
 * 1) A PointSet containing the points that define the path of the section.  This PointSet has the following
 *    columns: X Y Z1 Z2 C -> X and Y: the areal coordinates of each "fold"; Z1 and Z2: the starting and
 *    ending depths of the "fold"; C: the column index of the 2D grid at the "fold".  This
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

    /** Sets the point set mentioned in this class' documentation.
     * The previously set point set, if any, is removed from this object's child.
     * @note ATTENTION! The previous point set, if set, is deleted!
     */
    void setPointSet( PointSet* pointSet );

    /** Sets the Cartesian grid mentioned in this class' documentation.
     * The previously set point set, if any, is removed from this object's child.
     * @note ATTENTION! The previous Cartesian set, if set, is deleted!
     */
    void setCartesianGrid( CartesianGrid* cartesianGrid );

    /** Sets point set metadata from the accompaining .md file, if it exists.
     * Nothing happens if the metadata file does not exist.  If it exists, it calls
     * #setPointSet() and setCartesianGrid() with objects built from the metadata read
     * from the .md file.*/
    void setInfoFromMetadataFile();

    //File interface
public:
    virtual void deleteFromFS() override;
    virtual void writeToFS() override;
    virtual void readFromFS() override;
    virtual bool canHaveMetaData() override { return true; }
    virtual QString getFileType() const override { return getTypeName(); }
    virtual void updateMetaDataFile() override;
    virtual bool isDataFile() override { return false; }
    virtual bool isDistribution() override { return false; }

    // ProjectComponent interface
public:
    virtual QIcon getIcon() override;
    virtual bool isFile() const override { return false; }
    virtual bool isAttribute() override { return false; }
    virtual QString getObjectLocator() override;
    virtual QString getTypeName() const override { return "SECTION"; }
    virtual void save(QTextStream *txt_stream) override;

protected:
    PointSet* m_PointSet;
    CartesianGrid* m_CartesianGrid;
};

#endif // SECTION_H
