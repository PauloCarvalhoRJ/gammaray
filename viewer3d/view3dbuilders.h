#ifndef VIEW3DBUILDERS_H
#define VIEW3DBUILDERS_H

#include "view3dviewdata.h"

class ProjectComponent;
class PointSet;
class Attribute;
class CartesianGrid;
class View3DWidget;

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
    static View3DViewData build(ProjectComponent *object, View3DWidget * widget3D  );

    //@{
    /** Specific overrides. */
    static View3DViewData build( PointSet* object, View3DWidget * widget3D ); //point set geometry only
    static View3DViewData build( Attribute* object, View3DWidget * widget3D ); //attribute (can be of point set, cartesian grid, etc.)
    static View3DViewData build( CartesianGrid* object, View3DWidget * widget3D );  //cartesian grid geometry only
    //@}

private:
    static View3DViewData buildForAttributeFromPointSet( PointSet* pointSet, Attribute* attribute, View3DWidget * widget3D );

    /** Specific builder for a Cartesian grid that represents a 2D map (nZ < 2).
     *  The grid is displayed in the XY plane (Z=0).
    */
    static View3DViewData buildForMapCartesianGrid( CartesianGrid* cartesianGrid, View3DWidget * widget3D );

    /** Specific builder for an Attribute in a Cartesian grid that represents a 2D map (nZ < 2).
     *  The grid is displayed in the XY plane (Z=0).  The VTK model used is the vtkPlaneSource,
     *  which as an algorithm that generates a polygonal mesh.
     * @note Not used. Kept for future reference as vtkPlaneSource showed to be less efficient than vtkStructuredGrid
     *       to render regular grids.
     */
    static View3DViewData buildForAttributeInMapCartesianGridWithVtkPlaneSource(
            CartesianGrid* cartesianGrid,
            Attribute* attribute,
            View3DWidget * widget3D);

    /** Specific builder for an Attribute in a Cartesian grid that represents a 2D map (nZ < 2).
     *  The grid is displayed in the XY plane (Z=0).  The VTK model used is the vtkStructuredGrid,
     *  which as a data structure that is regular grid and level-of-detail management.
     * @note Not used. Kept for future reference as LOD is not working properly yet.
     */
    static View3DViewData buildForAttributeInMapCartesianGridWithVtkStructuredGridAndLOD(
            CartesianGrid* cartesianGrid,
            Attribute* attribute,
            View3DWidget * widget3D);

    /** Specific builder for an Attribute in a Cartesian grid that represents a 2D map (nZ < 2).
     *  The grid is displayed in the XY plane (Z=0).  The VTK model used is the vtkStructuredGrid,
     *  which as a data structure that is regular grid.
     */
    static View3DViewData buildForAttributeInMapCartesianGridWithVtkStructuredGrid(
            CartesianGrid* cartesianGrid,
            Attribute* attribute,
            View3DWidget * widget3D);

    /** Specific builder for a Cartesian grid that represents a generic 3D volume (nZ >= 2).
     */
    static View3DViewData buildFor3DCartesianGrid( CartesianGrid* cartesianGrid, View3DWidget * widget3D );

    /** Specific builder for an Attribute in a generic 3D Cartesian grid (with clipping planes
     *  along de I, J and K planes)
     */
    static View3DViewData buildForAttribute3DCartesianGridWithIJKClipping(
            CartesianGrid* cartesianGrid,
            Attribute* attribute,
            View3DWidget * widget3D);

    /** Specific builder for a stratigraphic grid (kept for future reference).
     */
    static View3DViewData buildForStratGrid( ProjectComponent* toBeSpecified, View3DWidget * widget3D );
};

#endif // VIEW3DBUILDERS_H
