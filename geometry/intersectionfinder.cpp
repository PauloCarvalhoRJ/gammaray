#include "intersectionfinder.h"
#include "viewer3d/view3dbuilders.h"
#include "domain/cartesiangrid.h"
#include "domain/application.h"

#include <vtkSmartPointer.h>
#include <vtkOBBTree.h>
#include <vtkUnstructuredGrid.h>

IntersectionFinder::IntersectionFinder() :
    m_tree( nullptr )
{

}

void IntersectionFinder::initWithSurface( CartesianGrid *cg, int variableIndexOfZ )
{
    // Get the variable with the z values.
    Attribute* attributeWithZvalues = cg->getAttributeFromGEOEASIndex( variableIndexOfZ + 1 );

    // Make a surface object.
    vtkSmartPointer< vtkUnstructuredGrid > surface =
            View3DBuilders::makeSurfaceFrom2DGridWithZvalues( cg, attributeWithZvalues );

    if( ! surface ){
        Application::instance()->logError( "IntersectionFinder::initWithSurface(): null surface."
                                           " Check console for error messages." );
        return;
    }

    // Create the VTK locator
    m_tree = vtkSmartPointer<vtkOBBTree>::New();
    m_tree->SetDataSet( surface );
    m_tree->BuildLocator();
}
