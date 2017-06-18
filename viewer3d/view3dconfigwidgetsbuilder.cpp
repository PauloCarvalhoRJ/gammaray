#include "view3dconfigwidgetsbuilder.h"
#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/cartesiangrid.h"
#include "view3dconfigwidgets/v3dcfgwidforattributein3dcartesiangrid.h"

View3DConfigWidgetsBuilder::View3DConfigWidgetsBuilder()
{
}

View3DConfigWidget *View3DConfigWidgetsBuilder::build(ProjectComponent *pc)
{
    Application::instance()->logError("View3DConfigWidgetsBuilder::build(): objects of type \"" +
                                      pc->getTypeName()
                                      + "\" do not have a viewing configuration widget.");
    return nullptr;
}

View3DConfigWidget *View3DConfigWidgetsBuilder::build(Attribute *attribute)
{
    //get the attribute's parent data file
    File *file = attribute->getContainingFile();

    //get the parent file type
    QString fileType = file->getFileType();

    if( fileType == "CARTESIANGRID" ) {
        CartesianGrid* cg = (CartesianGrid*)file;
        if( cg->getNZ() < 2 ){
            Application::instance()->logError("View3DConfigWidgetsBuilder::build(Attribute *): Config widget unavailable for Attributes in 2D Cartesian grids.");
            return nullptr;
        } else {
            return buildForAttribute3DCartesianGrid( cg, attribute );
        }
    } else {
        Application::instance()->logError("View3DConfigWidgetsBuilder::build(Attribute *): Config widget unavailable for Attributes of file type: " + fileType);
        return nullptr;
    }
}

View3DConfigWidget *View3DConfigWidgetsBuilder::buildForAttribute3DCartesianGrid(
        CartesianGrid *cartesianGrid, Attribute *attribute )
{
    return new V3DCfgWidForAttributeIn3DCartesianGrid( cartesianGrid, attribute );
}
