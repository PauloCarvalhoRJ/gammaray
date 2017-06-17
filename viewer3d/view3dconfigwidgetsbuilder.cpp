#include "view3dconfigwidgetsbuilder.h"
#include "domain/application.h"
#include "domain/projectcomponent.h"

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
