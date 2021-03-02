#ifndef SECTION_H
#define SECTION_H

#include "domain/file.h"
#include "geometry/quadrilateral.h"

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
    PointSet* getPointSet() const { return m_PointSet; }

    /** Sets the Cartesian grid mentioned in this class' documentation.
     * The previously set point set, if any, is removed from this object's child.
     * @note ATTENTION! The previous Cartesian set, if set, is deleted!
     */
    void setCartesianGrid( CartesianGrid* cartesianGrid );
    CartesianGrid* getCartesianGrid() const { return m_CartesianGrid; }

    /** Sets point set metadata from the accompaining .md file, if it exists.
     * Nothing happens if the metadata file does not exist.  If it exists, it calls
     * #setPointSet() and setCartesianGrid() with objects built from the metadata read
     * from the .md file.*/
    void setInfoFromMetadataFile();

    /** Returns, via output parameters, the center of the cell given its topological
     * coordinates.  Recall that, for a geologic section, the number of rows is always 1,
     * So only the I and K indexes are necessary to uniquely address a given value in the section.
     */
    void IKtoXYZ( uint i, uint k, double& x, double& y, double& z ) const;

    /**
     * Creates a new PointSet object containing the data of this Section.
     * The coordinates of the point set are that of the centroids of the section's cells.
     * The function creates a new physical data file matching the newly created PointSet
     * in the project's directory using the passed name as file name.
     */
    PointSet* toPointSetCentroids(const QString &psName) const;

    /**
     * Computes the area of the cell given its address in the Section's grid.
     * Recall that nJ == 1 for a Section.
     */
    double getCellArea( uint i, uint k ) const;

    /**
     * Creates a Quadrilateral object from a cell's geometry data given the cell's index in the
     * Section's grid.
     */
    Quadrilateral makeQuadrilateral( uint i, uint k ) const;

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
    virtual QString getTypeName() const override { return "SECTION"; }
    virtual void save(QTextStream *txt_stream) override;
    virtual View3DViewData build3DViewObjects( View3DWidget * widget3D ) override;

protected:
    PointSet* m_PointSet;
    CartesianGrid* m_CartesianGrid;
};

#endif // SECTION_H
