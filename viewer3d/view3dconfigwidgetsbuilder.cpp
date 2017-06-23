#include "view3dconfigwidgetsbuilder.h"
#include "domain/application.h"
#include "domain/projectcomponent.h"
#include "domain/attribute.h"
#include "domain/file.h"
#include "domain/cartesiangrid.h"
#include "view3dconfigwidgets/v3dcfgwidforattributein3dcartesiangrid.h"
#include "view3dconfigwidgets/v3dcfgwidforattributeinmapcartesiangrid.h"

View3DConfigWidgetsBuilder::View3DConfigWidgetsBuilder()
{
}

View3DConfigWidget *View3DConfigWidgetsBuilder::build(ProjectComponent *pc, View3DViewData /*viewObjects*/)
{
    Application::instance()->logError("View3DConfigWidgetsBuilder::build(): objects of type \"" +
                                      pc->getTypeName()
                                      + "\" do not have a viewing configuration widget.");
    return nullptr;
}

View3DConfigWidget *View3DConfigWidgetsBuilder::build(Attribute *attribute, View3DViewData viewObjects)
{
    //get the attribute's parent data file
    File *file = attribute->getContainingFile();

    //get the parent file type
    QString fileType = file->getFileType();

    if( fileType == "CARTESIANGRID" ) {
        CartesianGrid* cg = (CartesianGrid*)file;
        if( cg->getNZ() < 2 ){
            return buildForAttributeMapCartesianGrid( cg, attribute, viewObjects );
        } else {
            return buildForAttribute3DCartesianGrid( cg, attribute, viewObjects );
        }
    } else {
        Application::instance()->logError("View3DConfigWidgetsBuilder::build(Attribute *): Config widget unavailable for Attributes of file type: " + fileType);
        return nullptr;
    }
}

View3DConfigWidget *View3DConfigWidgetsBuilder::buildForAttribute3DCartesianGrid(
        CartesianGrid *cartesianGrid, Attribute *attribute, View3DViewData viewObjects )
{
    return new V3DCfgWidForAttributeIn3DCartesianGrid( cartesianGrid, attribute, viewObjects );
}

View3DConfigWidget *View3DConfigWidgetsBuilder::buildForAttributeMapCartesianGrid(
        CartesianGrid *cartesianGrid, Attribute *attribute, View3DViewData viewObjects)
{
    return new V3DCfgWidForAttributeInMapCartesianGrid( cartesianGrid, attribute, viewObjects );
}
