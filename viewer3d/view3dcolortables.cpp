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
        case ColorTable::RAINBOW: return getClassicRainbow( min, max ); break;
        default:
            Application::instance()->logError("View3dColorTables::getColorTable(): unknown color table code.  Returning a default.");
            return getClassicRainbow( min, max );
    }
}

QString View3dColorTables::getColorTableName(ColorTable ct)
{
    switch( ct ){
        case ColorTable::RAINBOW: return "Rainbow"; break;
        default:
            return "UNKNOWN";
    }
}

vtkSmartPointer<vtkLookupTable> View3dColorTables::getCategoricalColorTable( CategoryDefinition *cd, bool useGSLibColors )
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
            //WARNING: avoid using different transparency levels
            //         weird transparency effects have been observed.
            //         set the same alpha for all colors in the table
            lut->SetTableValue(i, rgb[0], rgb[1], rgb[2], 1.0);
            lut->SetAnnotation(i, QString::number(i).toStdString() );
        } else {
            lut->SetTableValue(i, 0.0, 0.0, 0.0, 1.0);
            lut->SetAnnotation(i, "UNKNOWN CATEGORY" );
        }
    }
    lut->IndexedLookupOn();
    lut->SetNanColor( 0.0, 0.0, 0.0, 1.0 ); //ilegal color codes are rendered as 100% transparent.
    lut->Build();

    return lut;

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
    vtkSmartPointer<vtkColorTransferFunction> ctf =
            vtkSmartPointer<vtkColorTransferFunction>::New();
    ctf->SetColorSpaceToRGB();
    ctf->AddRGBPoint(0.00, 0.000, 0.000, 1.000);
    ctf->AddRGBPoint(0.25, 0.000, 1.000, 1.000);
    ctf->AddRGBPoint(0.50, 0.000, 1.000, 0.000);
    ctf->AddRGBPoint(0.75, 1.000, 1.000, 0.000);
    ctf->AddRGBPoint(1.00, 1.000, 0.000, 0.000);

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
