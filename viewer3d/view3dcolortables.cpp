#include "view3dcolortables.h"

#include <vtkColorTransferFunction.h>

View3dColorTables::View3dColorTables()
{
}

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
