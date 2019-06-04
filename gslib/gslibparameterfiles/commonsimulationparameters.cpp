#include "commonsimulationparameters.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"

CommonSimulationParameters::CommonSimulationParameters() :
    GSLibParameterFile({
                          "<string>                                              -Basename for realization variables",           // 0
                          "<uint>                                                -Number of realizations",                       // 1
                          "<uint>                                                -Seed for random number generator"              // 2
//                          "<uint><uint>                                          -Min. and max. primary data for conditioning",  // 3
//                          "<uint>                                                -Number of simulated nodes for conditioning",   // 4
//                          "<option [0:no] [1:yes]>                               -Assign data to nodes",                         // 5
//                          "<option [0:no] [1:yes]><uint>                         -Use multigrid search (0=no, 1=yes), number",   // 6
//                          "<uint>                                                -Max. data per octant (0=not used)",            // 7
//                          "<double><double><double>                              -Search ellipsoid: radii (hmax,hmin,vert)",     // 8
//                          "<double><double><double>                              -Search ellipsoid: angles"                      // 9
                      })
{
    getParameter<GSLibParString*>(0)->_value = "noname_";
    getParameter<GSLibParUInt*>(1)->_value = 1;
    getParameter<GSLibParUInt*>(2)->_value = 69069;
//    GSLibParMultiValuedFixed* par3 = getParameter<GSLibParMultiValuedFixed*>(3);{
//        par2->getParameter<GSLibParUInt*>(0)->_value = 4;
//        par2->getParameter<GSLibParUInt*>(1)->_value = 8;
//    }
//    getParameter<GSLibParUInt*>(4)->_value = 16;
//    getParameter<GSLibParOption*>(5)->_selected_value = 0;
//    GSLibParMultiValuedFixed* par6 = getParameter<GSLibParMultiValuedFixed*>(6);{
//        par5->getParameter<GSLibParOption*>(0)->_selected_value = 0;
//        par5->getParameter<GSLibParUInt*>(1)->_value = 3;
//    }
//    getParameter<GSLibParUInt*>(7)->_value = 0;
//    GSLibParMultiValuedFixed* par8 = getParameter<GSLibParMultiValuedFixed*>(8);{
//        par7->getParameter<GSLibParDouble*>(0)->_value = 10.0;
//        par7->getParameter<GSLibParDouble*>(1)->_value = 10.0;
//        par7->getParameter<GSLibParDouble*>(2)->_value = 1.0;
//    }
//    GSLibParMultiValuedFixed* par9 = getParameter<GSLibParMultiValuedFixed*>(9);{
//        par8->getParameter<GSLibParDouble*>(0)->_value = 0.0;
//        par8->getParameter<GSLibParDouble*>(1)->_value = 0.0;
//        par8->getParameter<GSLibParDouble*>(2)->_value = 0.0;
    //    }
}

void CommonSimulationParameters::setBaseNameForRealizationVariables(const QString baseName)
{
    getParameter<GSLibParString*>(0)->_value = baseName;
}

QString CommonSimulationParameters::getBaseNameForRealizationVariables()
{
    return getParameter<GSLibParString*>(0)->_value;
}

uint CommonSimulationParameters::getSeed()
{
    return getParameter<GSLibParUInt*>(2)->_value;
}

uint CommonSimulationParameters::getNumberOfRealizations()
{
    return getParameter<GSLibParUInt*>(1)->_value;
}
