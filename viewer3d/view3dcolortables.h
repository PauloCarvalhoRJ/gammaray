#ifndef VIEW3DCOLORTABLES_H
#define VIEW3DCOLORTABLES_H

#include <vtkSmartPointer.h>
#include <QString>

class vtkLookupTable;

/*! The available color tables. */
enum class ColorTable : uint {
    RAINBOW = 0 /*!< The classic color table mapping low values to bluer hues and high values to redder hues. */
};

/** This class groups static functions to create color tables.
 *  Help needed!  GammaRay wants you!  Create new striking color tables!
 */
class View3dColorTables
{
public:
    View3dColorTables();

    /** Returns a color table object given its code. */
    static vtkSmartPointer<vtkLookupTable> getColorTable( ColorTable ct, double min, double max);

    /** Returns the name of a color table given its code. */
    static QString getColorTableName( ColorTable ct );

private:
    /** Code example for implementing color tables with predefined colors. */
    static vtkSmartPointer<vtkLookupTable> getPredefinedColorsExample( double min, double max );

    /** Code example for implementing color tables with interpolated colors. */
    static vtkSmartPointer<vtkLookupTable> getInterpolatedColorsExample( double min, double max );

    /** The classic color table mapping lower values to bluer hues and high values to redder hues. */
    static vtkSmartPointer<vtkLookupTable> getClassicRainbow( double min, double max );
};

#endif // VIEW3DCOLORTABLES_H
