#include "nestedvariogramstructuresparameters.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"
#include "gslib/gslibparams/widgets/widgetgslibparrepeat.h"
#include "util.h"
#include "domain/application.h"

NestedVariogramStructuresParameters::NestedVariogramStructuresParameters() :
    GSLibParameterFile({
                          "<repeat>                              ",                                        // 0
                          "   <double><double><double><double>   -aMax, aMin, azimuth and contribution",
                       }),
    m_lastReturnedEditorWidget( nullptr )
{
    getParameter<GSLibParRepeat*>(0)->setCount( 1 );
    GSLibParMultiValuedFixed* varPars = getParameter<GSLibParRepeat*>(0)->getParameter<GSLibParMultiValuedFixed*>( 0, 0 );
    varPars->getParameter<GSLibParDouble*>( 0 )->_value = 10.0;
    varPars->getParameter<GSLibParDouble*>( 1 )->_value = 10.0;
    varPars->getParameter<GSLibParDouble*>( 2 )->_value = 0.0;
    varPars->getParameter<GSLibParDouble*>( 3 )->_value = 1.0;
}

void NestedVariogramStructuresParameters::setNumberOfNestedStructures(int number)
{
    getParameter<GSLibParRepeat*>(0)->setCount( number );
}

QWidget *NestedVariogramStructuresParameters::getEditorWidget()
{
    GSLibParRepeat* par = getParameter<GSLibParRepeat*>(0);
    return m_lastReturnedEditorWidget = par->getWidget();
}

double NestedVariogramStructuresParameters::getSemiMajorAxis(int nNestedStructure)
{
    updateValuesFromEditorWidget();
    GSLibParMultiValuedFixed* varPars = getParameter<GSLibParRepeat*>(0)
            ->getParameter<GSLibParMultiValuedFixed*>( nNestedStructure, 0 );
    return varPars->getParameter<GSLibParDouble*>( 0 )->_value;
}

double NestedVariogramStructuresParameters::getSemiMinorSemiMajorAxesRatio(int nNestedStructure)
{
    updateValuesFromEditorWidget();
    GSLibParMultiValuedFixed* varPars = getParameter<GSLibParRepeat*>(0)
            ->getParameter<GSLibParMultiValuedFixed*>( nNestedStructure, 0 );
    return varPars->getParameter<GSLibParDouble*>( 1 )->_value / varPars->getParameter<GSLibParDouble*>( 0 )->_value;
}

double NestedVariogramStructuresParameters::getAzimuthAsRadians(int nNestedStructure)
{
    updateValuesFromEditorWidget();
    GSLibParMultiValuedFixed* varPars = getParameter<GSLibParRepeat*>(0)
            ->getParameter<GSLibParMultiValuedFixed*>( nNestedStructure, 0 );
    return Util::azimuthToRadians( varPars->getParameter<GSLibParDouble*>( 2 )->_value );
}

double NestedVariogramStructuresParameters::getContribution(int nNestedStructure)
{
    updateValuesFromEditorWidget();
    GSLibParMultiValuedFixed* varPars = getParameter<GSLibParRepeat*>(0)
            ->getParameter<GSLibParMultiValuedFixed*>( nNestedStructure, 0 );
    return varPars->getParameter<GSLibParDouble*>( 3 )->_value;
}

void NestedVariogramStructuresParameters::updateValuesFromEditorWidget()
{
    WidgetGSLibParRepeat* wgsparrepeat = dynamic_cast< WidgetGSLibParRepeat* >( m_lastReturnedEditorWidget );
    if( wgsparrepeat ){
        wgsparrepeat->updateValue( getParameter<GSLibParRepeat*>(0) );
    } else {
        Application::instance()->logError( "NestedVariogramStructuresParameters::updateValuesFromEditorWidget(): widget is not a WidgetGSLibParRepeat or it is null." );
    }
}

