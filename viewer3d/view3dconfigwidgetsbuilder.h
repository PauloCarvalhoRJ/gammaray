#ifndef VIEW3DCONFIGWIDGETSBUILDER_H
#define VIEW3DCONFIGWIDGETSBUILDER_H

class View3DConfigWidget;
class ProjectComponent;
class Attribute;
class CartesianGrid;

/**
 * This class groups static functions to build 3D viewer configuration widgets for the several domain objects.
 * In other words, if you need visualization configurations for an object visible in the 3D Viewer, then you need
 * to create a build() function for the object's class.  You also need to implement the
 * ProjectComponent::build3DViewerConfigWidget() in the target domain class.
 */
class View3DConfigWidgetsBuilder
{
public:
    View3DConfigWidgetsBuilder();

    /** Generic builder (fallback implementation). */
    static View3DConfigWidget* build( ProjectComponent *pc );

    //@{
    /** Specific overrides. */
    static View3DConfigWidget* build( Attribute *attribute ); //attribute (can be of point set, cartesian grid, etc.)
    //@}

private:
    /** Specific builder for an Attribute in a generic 3D Cartesian grid.*/
    static View3DConfigWidget* buildForAttribute3DCartesianGrid(
            CartesianGrid* cartesianGrid,
            Attribute* attribute );

};

#endif // VIEW3DCONFIGWIDGETSBUILDER_H
