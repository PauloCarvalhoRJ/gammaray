#ifndef VIEW3DCOLORTABLES_H
#define VIEW3DCOLORTABLES_H

#include <vtkSmartPointer.h>
#include <QString>

class vtkLookupTable;
class CategoryDefinition;

/*! The available color tables. */
enum class ColorTable : uint {
    RAINBOW = 0, /*!< The classic color table mapping low values to bluer hues and high values to redder hues. */
    SEISMIC = 1  /*!< Color table mapping low values to bluer hues and high values to redder hues with white in the middle. */
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

    /** Makes a color table for a categorical variable.
     * @param useGSLibColors If true, uses the RGB colors restriced to GSLib standard colors (GSLib color codes).
     *                       If false, uses the RGB values specified in the custom color field of
     *                       the category definition.
     * @param alphaForNDV The alpha channel value for unvalued locations. Default is 0.3 (70% transparency).
     *                    If alpha > 0.0, the unvalued locations are rendered in pink.
     */
    static vtkSmartPointer<vtkLookupTable> getCategoricalColorTable(CategoryDefinition *cd ,
                                                                    bool useGSLibColors,
                                                                    double alphaForNDV = 0.3);

private:
    /** Code example for implementing color tables with predefined colors. */
    static vtkSmartPointer<vtkLookupTable> getPredefinedColorsExample( double min, double max );

    /** Code example for implementing color tables with interpolated colors. */
    static vtkSmartPointer<vtkLookupTable> getInterpolatedColorsExample( double min, double max );

    /** The classic color table mapping lower values to bluer hues and high values to redder hues. */
    static vtkSmartPointer<vtkLookupTable> getClassicRainbow( double min, double max );

    /** The color table mapping lower values to bluer hues and high values to redder hues with white in the middle. */
    static vtkSmartPointer<vtkLookupTable> getSeismic( double min, double max );
};

#endif // VIEW3DCOLORTABLES_H
