#ifndef VIEW3DBUILDERS_H
#define VIEW3DBUILDERS_H

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkImageActor.h>
#include "view3dviewdata.h"

class ProjectComponent;
class PointSet;
class Attribute;
class CartesianGrid;

/**
 * This class groups static functions to build VTK actors for the several domain objects.
 * In other words, if you need an object visible in the 3D Viewer, then you need to create a build() function
 * for the object's class.  You also need to implement the ProjectComponent::buildVTKActor() in the target domain class.
 */
class View3DBuilders
{
public:
    View3DBuilders();

    /** Generic builder (fallback implementation). */
    static View3DViewData build(ProjectComponent *object );

    //@{
    /** Specific overrides. */
    static View3DViewData build( PointSet* object ); //point set geometry only
    static View3DViewData build( Attribute* object ); //attribute (can be of point set, cartesian grid, etc.)
    static View3DViewData build( CartesianGrid* object );  //cartesian grid geometry only
    //@}

private:
    static View3DViewData buildForAttributeFromPointSet( PointSet* pointSet, Attribute* attribute );

    /** Specific builder for a Cartesian grid that represents a 2D map (nZ < 2).
     *  The grid is displayed in the XY plane (Z=0).
    */
    static View3DViewData buildForMapCartesianGrid( CartesianGrid* cartesianGrid );

    /** Specific builder for an Attribute in a Cartesian grid that represents a 2D map (nZ < 2).
     *  The grid is displayed in the XY plane (Z=0).
     */
    static View3DViewData buildForAttributeInMapCartesianGrid(
            CartesianGrid* cartesianGrid,
            Attribute* attribute );

    /** Specific builder for a Cartesian grid that represents a generic 3D volume (nZ >= 2).
     */
    static View3DViewData buildFor3DCartesianGrid( CartesianGrid* cartesianGrid );

    /** Specific builder for an Attribute in a generic 3D Cartesian grid.
     */
    static View3DViewData buildForAttribute3DCartesianGrid(
            CartesianGrid* cartesianGrid,
            Attribute* attribute );

    /** Specific builder for an Attribute in a generic 3D Cartesian grid (with clipping planes
     *  along de I, J and K planes)
     */
    static View3DViewData buildForAttribute3DCartesianGridWithIJKClipping(
            CartesianGrid* cartesianGrid,
            Attribute* attribute );

    /** Specific builder for a stratigraphic grid (kept for future reference).
     */
    static View3DViewData buildForStratGrid( ProjectComponent* toBeSpecified );
};

#endif // VIEW3DBUILDERS_H
