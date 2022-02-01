#include "view3dcolortables.h"

#include <vtkLookupTable.h>
#include <vtkColorTransferFunction.h>

#include "domain/application.h"
#include "domain/categorydefinition.h"
#include "util.h"


View3dColorTables::View3dColorTables()
{
}

vtkSmartPointer<vtkLookupTable> View3dColorTables::getColorTable(ColorTable ct, double min, double max)
{
    switch( ct ){
        case ColorTable::RAINBOW: return getClassicRainbow( min, max );
        case ColorTable::SEISMIC: return getSeismic( min, max );
        default:
            Application::instance()->logError("View3dColorTables::getColorTable(): unknown color table code.  Returning a default.");
            return getClassicRainbow( min, max );
    }
}

vtkSmartPointer<vtkColorTransferFunction> View3dColorTables::getColorTransferFunction(ColorTable ct,
                                                                                      double min,
                                                                                      double max)
{
    switch( ct ){
        case ColorTable::RAINBOW: return getClassicRainbowCTF( min, max );
        case ColorTable::SEISMIC: return getSeismicCTF( min, max );
        default:
            Application::instance()->logError("View3dColorTables::getColorTransferFunction(): "
                                              "unknown color scheme code.  Returning a default.");
            return getClassicRainbowCTF( min, max );
    }
}

QString View3dColorTables::getColorTableName(ColorTable ct)
{
    switch( ct ){
        case ColorTable::RAINBOW: return "Rainbow";
        case ColorTable::SEISMIC: return "Seismic";
        default:
            return "UNKNOWN";
    }
}

vtkSmartPointer<vtkLookupTable> View3dColorTables::getCategoricalColorTable( CategoryDefinition *cd,
                                                                             bool useGSLibColors,
                                                                             double alphaForNDV )
{
    cd->loadQuintuplets();
    int catCount = cd->getCategoryCount();

    //determine the greatest categorical code
    int maxCatCode = 0;
    for( int i = 0; i < catCount; ++i )
        maxCatCode = std::max<int>( maxCatCode, cd->getCategoryCode(i) );

    //table color indexes must go from 0 to greatest facies code, without skipping values
    size_t tableSize = maxCatCode + 1;
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetNumberOfTableValues(tableSize);

    //but assign only the codes defined in the category definition
    //which may be less than the total number of entries in the color table
    //this is a requirement by the way VTK's LUT work for categorical color tables
    for(size_t i = 0; i < tableSize; ++i)
    {
        if( cd->codeExists( i ) ){
            double rgb[3];
            QColor color;
            int catIndex = cd->getCategoryIndex( i );
            if( useGSLibColors )
                color = Util::getGSLibColor( cd->getColorCode( catIndex ) );
            else
                color = cd->getCustomColor( catIndex );
            rgb[0] = color.redF();
            rgb[1] = color.greenF();
            rgb[2] = color.blueF();
            lut->SetTableValue(i, rgb[0], rgb[1], rgb[2], 1.0);
            lut->SetAnnotation(i, QString::number(i).toStdString() );
        } else {
            lut->SetTableValue(i, 1.0, 0.0, 1.0, alphaForNDV); //ilegal color codes are rendered as pink transparent.
            lut->SetAnnotation(i, "UNKNOWN CATEGORY" );
        }
    }
    lut->SetNanColor( 1.0, 0.0, 1.0, alphaForNDV ); //unvalued locations are rendered as pink transparent.
    lut->IndexedLookupOn();
    lut->Build();

    return lut;
}

vtkSmartPointer<vtkColorTransferFunction> View3dColorTables::getCategoricalColorTransferFunction
                                                    (CategoryDefinition *cd, bool useGSLibColors)
{
    cd->loadQuintuplets();
    int catCount = cd->getCategoryCount();

    //create a color interpolator object adequate for rendering a categorical color table
    vtkSmartPointer<vtkColorTransferFunction> ctf =
            vtkSmartPointer<vtkColorTransferFunction>::New();
    ctf->SetColorSpaceToRGB();
    for( size_t i_cat = 0; i_cat < catCount; ++i_cat )
    {
        QColor color;
        if( useGSLibColors )
            color = Util::getGSLibColor( cd->getColorCode( i_cat ) );
        else
            color = cd->getCustomColor( i_cat );
        ctf->AddRGBPoint( cd->getCategoryCode( i_cat ), color.redF(), color.greenF(), color.blueF() );
    }

    return ctf;
}

//===============private functions=====================================

vtkSmartPointer<vtkLookupTable> View3dColorTables::getPredefinedColorsExample(double min, double max)
{
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetTableRange(min, max);
    lut->SetNumberOfTableValues(2);
    lut->SetTableValue(0, 0.8, 0.8, 0.8);
    lut->SetTableValue(1, 0.0, 1.0, 1.0);
    lut->SetRampToLinear();
    lut->Build();
    return lut;
}

vtkSmartPointer<vtkLookupTable> View3dColorTables::getInterpolatedColorsExample(double min, double max)
{
    size_t tableSize = 32;

    //create a color interpolator object
    vtkSmartPointer<vtkColorTransferFunction> ctf =
            vtkSmartPointer<vtkColorTransferFunction>::New();
    ctf->SetColorSpaceToDiverging();
    ctf->AddRGBPoint(0.0, 0.085, 0.532, 0.201);
    ctf->AddRGBPoint(0.5, 0.865, 0.865, 0.865);
    ctf->AddRGBPoint(1.0, 0.677, 0.492, 0.093);

    //create the color table object
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetTableRange(min, max);
    lut->SetNumberOfTableValues(tableSize);
    for(size_t i = 0; i < tableSize; ++i)
    {
        double *rgb;
        rgb = ctf->GetColor(static_cast<double>(i)/tableSize);
        lut->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
    }
    lut->SetRampToLinear();
    lut->Build();

    return lut;
}

vtkSmartPointer<vtkLookupTable> View3dColorTables::getClassicRainbow(double min, double max)
{
    size_t tableSize = 32;

    //create a color interpolator object
    vtkSmartPointer<vtkColorTransferFunction> ctf = getClassicRainbowCTF();

    //create the color table object
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetTableRange(min, max);
    lut->SetNumberOfTableValues(tableSize);
    for(size_t i = 0; i < tableSize; ++i)
    {
        double *rgb;
        rgb = ctf->GetColor(static_cast<double>(i)/tableSize);
        lut->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
    }
    lut->SetRampToLinear();
    lut->Build();

    return lut;
}

vtkSmartPointer<vtkLookupTable> View3dColorTables::getSeismic(double min, double max)
{
    size_t tableSize = 32;

    //create a color interpolator object
    vtkSmartPointer<vtkColorTransferFunction> ctf = getSeismicCTF();

    //create the color table object
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetTableRange(min, max);
    lut->SetNumberOfTableValues(tableSize);
    for(size_t i = 0; i < tableSize; ++i)
    {
        double *rgb;
        rgb = ctf->GetColor(static_cast<double>(i)/tableSize);
        lut->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
    }
    lut->SetRampToLinear();
    lut->Build();

    return lut;
}

vtkSmartPointer<vtkColorTransferFunction> View3dColorTables::getClassicRainbowCTF(double min, double max)
{
    //create a color interpolator object
    vtkSmartPointer<vtkColorTransferFunction> ctf =
            vtkSmartPointer<vtkColorTransferFunction>::New();
    ctf->SetColorSpaceToRGB();
    double delta = max - min;
    ctf->AddRGBPoint(min               , 0.000, 0.000, 1.000);
    ctf->AddRGBPoint(min + delta * 0.25, 0.000, 1.000, 1.000);
    ctf->AddRGBPoint(min + delta * 0.50, 0.000, 1.000, 0.000);
    ctf->AddRGBPoint(min + delta * 0.75, 1.000, 1.000, 0.000);
    ctf->AddRGBPoint(max               , 1.000, 0.000, 0.000);
    return ctf;
}

vtkSmartPointer<vtkColorTransferFunction> View3dColorTables::getSeismicCTF(double min, double max)
{
    //create a color interpolator object
    vtkSmartPointer<vtkColorTransferFunction> ctf =
            vtkSmartPointer<vtkColorTransferFunction>::New();
    ctf->SetColorSpaceToRGB();
    double delta = max - min;
    ctf->AddRGBPoint(min               , 0.000, 0.000, 1.000);
    ctf->AddRGBPoint(min + delta * 0.50, 1.000, 1.000, 1.000);
    ctf->AddRGBPoint(max               , 1.000, 0.000, 0.000);
    return ctf;
}
