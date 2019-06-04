#include "commonsimulationparameters.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"

CommonSimulationParameters::CommonSimulationParameters() :
    GSLibParameterFile({
                          "<uint>                                                -Number of realizations",                       // 0
                          "<uint>                                                -Seed for random number generator",             // 1
                          "<uint><uint>                                          -Min. and max. primary data for conditioning",  // 2
                          "<uint>                                                -Number of simulated nodes for conditioning",   // 3
                          "<option [0:no] [1:yes]>                               -Assign data to nodes",                         // 4
                          "<option [0:no] [1:yes]><uint>                         -Use multigrid search (0=no, 1=yes), number",   // 5
                          "<uint>                                                -Max. data per octant (0=not used)",            // 6
                          "<double><double><double>                              -Search ellipsoid: radii (hmax,hmin,vert)",     // 7
                          "<double><double><double>                              -Search ellipsoid: angles"                      // 8
                      })
{
    getParameter<GSLibParUInt*>(0)->_value = 1;
    getParameter<GSLibParUInt*>(1)->_value = 69069;
    GSLibParMultiValuedFixed* par2 = getParameter<GSLibParMultiValuedFixed*>(2);{
        par2->getParameter<GSLibParUInt*>(0)->_value = 4;
        par2->getParameter<GSLibParUInt*>(1)->_value = 8;
    }
    getParameter<GSLibParUInt*>(3)->_value = 16;
    getParameter<GSLibParOption*>(4)->_selected_value = 0;
    GSLibParMultiValuedFixed* par5 = getParameter<GSLibParMultiValuedFixed*>(5);{
        par5->getParameter<GSLibParOption*>(0)->_selected_value = 0;
        par5->getParameter<GSLibParUInt*>(1)->_value = 3;
    }
    getParameter<GSLibParUInt*>(6)->_value = 0;
    GSLibParMultiValuedFixed* par7 = getParameter<GSLibParMultiValuedFixed*>(7);{
        par7->getParameter<GSLibParDouble*>(0)->_value = 10.0;
        par7->getParameter<GSLibParDouble*>(1)->_value = 10.0;
        par7->getParameter<GSLibParDouble*>(2)->_value = 1.0;
    }
    GSLibParMultiValuedFixed* par8 = getParameter<GSLibParMultiValuedFixed*>(8);{
        par8->getParameter<GSLibParDouble*>(0)->_value = 0.0;
        par8->getParameter<GSLibParDouble*>(1)->_value = 0.0;
        par8->getParameter<GSLibParDouble*>(2)->_value = 0.0;
    }
}
