#include "commonsimulationparameters.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"

CommonSimulationParameters::CommonSimulationParameters() :
    GSLibParameterFile({
                          "<string>                                              -Basename for realization variables",                          // 0
                          "<uint>                                                -Number of realizations",                                      // 1
                          "<uint>                                                -Seed for random number generator",                            // 2
                          "<uint><uint>                                          -Min. and max. primary data for conditioning",                 // 3
                          "<double>                                              -Min. distance between secondary data for conditioning",       // 4
                          "<uint>                                                -Number of simulated nodes for conditioning",                  // 5
                          "<uint>                                                -Number of sectors (8==octants; 1==no sectors actually)",      // 6
                          "<uint><uint>                                          -Data per sector: min. and max. (0=not used)",                 // 7
                          "<double><double><double>                              -Search ellipsoid: radii (hmax,hmin,vert)",                    // 8
                          "<double><double><double>                              -Search ellipsoid: angles (az, dip, roll)"                     // 9
                          //"<option [0:no] [1:yes]>                             -Assign data to nodes",                                        // --
                          //"<option [0:no] [1:yes]><uint>                       -Use multigrid search (0=no, 1=yes), number",                  // --
                      })
{
    getParameter<GSLibParString*>(0)->_value = "noname_";
    getParameter<GSLibParUInt*>(1)->_value = 1;
    getParameter<GSLibParUInt*>(2)->_value = 69069;
    GSLibParMultiValuedFixed* par3 = getParameter<GSLibParMultiValuedFixed*>(3);{
        par3->getParameter<GSLibParUInt*>(0)->_value = 4;
        par3->getParameter<GSLibParUInt*>(1)->_value = 8;
    }
    getParameter<GSLibParDouble*>(4)->_value = 0.0;
    getParameter<GSLibParUInt*>(5)->_value = 16;
    getParameter<GSLibParUInt*>(6)->_value = 1;
    GSLibParMultiValuedFixed* par7 = getParameter<GSLibParMultiValuedFixed*>(7);{
        par7->getParameter<GSLibParUInt*>(0)->_value = 0;
        par7->getParameter<GSLibParUInt*>(1)->_value = 0;
    }
    GSLibParMultiValuedFixed* par8 = getParameter<GSLibParMultiValuedFixed*>(8);{
        par8->getParameter<GSLibParDouble*>(0)->_value = 10.0;
        par8->getParameter<GSLibParDouble*>(1)->_value = 10.0;
        par8->getParameter<GSLibParDouble*>(2)->_value = 1.0;
    }
    GSLibParMultiValuedFixed* par9 = getParameter<GSLibParMultiValuedFixed*>(9);{
        par9->getParameter<GSLibParDouble*>(0)->_value = 0.0;
        par9->getParameter<GSLibParDouble*>(1)->_value = 0.0;
        par9->getParameter<GSLibParDouble*>(2)->_value = 0.0;
    }
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

double CommonSimulationParameters::getSearchEllipHMax()
{
    GSLibParMultiValuedFixed* par8 = getParameter<GSLibParMultiValuedFixed*>(8);
    return par8->getParameter<GSLibParDouble*>(0)->_value;
}

double CommonSimulationParameters::getSearchEllipHMin()
{
    GSLibParMultiValuedFixed* par8 = getParameter<GSLibParMultiValuedFixed*>(8);
    return par8->getParameter<GSLibParDouble*>(1)->_value;
}

double CommonSimulationParameters::getSearchEllipHVert()
{
    GSLibParMultiValuedFixed* par8 = getParameter<GSLibParMultiValuedFixed*>(8);
    return par8->getParameter<GSLibParDouble*>(2)->_value;
}

double CommonSimulationParameters::getSearchEllipAzimuth()
{
    GSLibParMultiValuedFixed* par9 = getParameter<GSLibParMultiValuedFixed*>(9);
    return par9->getParameter<GSLibParDouble*>(0)->_value;
}

double CommonSimulationParameters::getSearchEllipDip()
{
    GSLibParMultiValuedFixed* par9 = getParameter<GSLibParMultiValuedFixed*>(9);
    return par9->getParameter<GSLibParDouble*>(1)->_value;
}

double CommonSimulationParameters::getSearchEllipRoll()
{
    GSLibParMultiValuedFixed* par9 = getParameter<GSLibParMultiValuedFixed*>(9);
    return par9->getParameter<GSLibParDouble*>(2)->_value;
}

uint CommonSimulationParameters::getNumberOfSamples()
{
    GSLibParMultiValuedFixed* par3 = getParameter<GSLibParMultiValuedFixed*>(3);
    return par3->getParameter<GSLibParUInt*>(1)->_value;
}

uint CommonSimulationParameters::getMinNumberOfSamples()
{
    GSLibParMultiValuedFixed* par3 = getParameter<GSLibParMultiValuedFixed*>(3);
    return par3->getParameter<GSLibParUInt*>(0)->_value;
}

uint CommonSimulationParameters::getNumberOfSectors()
{
    return getParameter<GSLibParUInt*>(6)->_value;
}

uint CommonSimulationParameters::getMinNumberOfSamplesPerSector()
{
    GSLibParMultiValuedFixed* par7 = getParameter<GSLibParMultiValuedFixed*>(7);
    return par7->getParameter<GSLibParUInt*>(0)->_value;
}

uint CommonSimulationParameters::getMaxNumberOfSamplesPerSector()
{
    GSLibParMultiValuedFixed* par7 = getParameter<GSLibParMultiValuedFixed*>(7);
    return par7->getParameter<GSLibParUInt*>(1)->_value;
}

double CommonSimulationParameters::getMinDistanceBetweenSecondaryDataSamples()
{
    return getParameter<GSLibParDouble*>(4)->_value;
}

uint CommonSimulationParameters::getNumberOfSimulatedNodesForConditioning()
{
    return getParameter<GSLibParUInt*>(5)->_value;
}
