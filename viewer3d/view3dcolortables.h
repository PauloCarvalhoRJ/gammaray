#ifndef VIEW3DCOLORTABLES_H
#define VIEW3DCOLORTABLES_H

#include <vtkSmartPointer.h>
#include <vtkLookupTable.h>

/** This class groups static functions to create color tables. */
class View3dColorTables
{
public:
    View3dColorTables();

    /** Code example for implementing color tables with predefined colors. */
    static vtkSmartPointer<vtkLookupTable> getPredefinedColorsExample( double min, double max );

    /** Code example for implementing color tables with interpolated colors. */
    static vtkSmartPointer<vtkLookupTable> getInterpolatedColorsExample( double min, double max );

    /** The classic color table mapping lower values to bluer hues and high values to redder hues. */
    static vtkSmartPointer<vtkLookupTable> getClassicRainbow( double min, double max );
};

#endif // VIEW3DCOLORTABLES_H
