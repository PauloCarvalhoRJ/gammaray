#include "commonsimulationparameters.h"
#include "gslib/gslibparameterfiles/gslibparamtypes.h"

#include <QStringBuilder> //for the += and % operators on QString in the print() method.

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
                          "<double><double><double>                              -Search ellipsoid: angles (az, dip, roll)",                    // 9
                          "<option [0:generic rtree based] [1:tuned for large data sets] [2:tuned for Cartesian grids]>   -Search algorithm option for the simulation grid",  // 10
                          "<option [0:simulation grid] [1:separate grid files in project] [2:separate grid files elsewhere (specify path)]>   -Save realizations option",   // 11
                          "<dir>                                                 -Path if option above is the third",                          // 12
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
    GSLibParOption* par10 = getParameter<GSLibParOption*>(10);
    par10->_selected_value = 2;
    GSLibParOption* par11 = getParameter<GSLibParOption*>(11);
    par11->_selected_value = 0;
    getParameter<GSLibParDir*>(12)->_path = "";
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

uint CommonSimulationParameters::getSearchAlgorithmOptionForSimGrid()
{
    return getParameter<GSLibParOption*>(10)->_selected_value;
}

uint CommonSimulationParameters::getSaveRealizationsOption()
{
    return getParameter<GSLibParOption*>(11)->_selected_value;
}

QString CommonSimulationParameters::getSaveRealizationsPath()
{
    return getParameter<GSLibParDir*>(12)->_path;
}

QString CommonSimulationParameters::print()
{
    QString result;
    result += "   Base name for realizations = " % getBaseNameForRealizationVariables() % '\n';
    result += "   Seed for random number generator = " % QString::number(getSeed()) % '\n';
    result += "   Number of realizations = " % QString::number(getNumberOfRealizations()) % '\n';
    result += "   Search ellipsoid:\n";
    result += "      range max.  = " % QString::number(getSearchEllipHMax()) % '\n';
    result += "      range min.  = " % QString::number(getSearchEllipHMin()) % '\n';
    result += "      range vert. = " % QString::number(getSearchEllipHVert()) % '\n';
    result += "      azimuth     = " % QString::number(getSearchEllipAzimuth()) % '\n';
    result += "      dip         = " % QString::number(getSearchEllipDip()) % '\n';
    result += "      roll        = " % QString::number(getSearchEllipRoll()) % '\n';
    result += "   Max. number of prim. data = " % QString::number(getNumberOfSamples()) % '\n';
    result += "   Min. number of prim. data = " % QString::number(getMinNumberOfSamples()) % '\n';
    result += "   Number of sectors = " % QString::number(getNumberOfSectors()) % '\n';
    result += "   Min. number of prim. data per sector = " % QString::number(getMinNumberOfSamplesPerSector()) % '\n';
    result += "   Max. number of prim. data per sector = " % QString::number(getMaxNumberOfSamplesPerSector()) % '\n';
    result += "   Min. distance between sec. data = " % QString::number(getMinDistanceBetweenSecondaryDataSamples()) % '\n';
    result += "   Max. number of previously simulated nodes to use = " % QString::number(getNumberOfSimulatedNodesForConditioning()) % '\n';
    result += "   Search algorithm option = " % QString::number(getSearchAlgorithmOptionForSimGrid()) % " (0=generic rtree based 1=tuned for large data sets 2=tuned for Cartesian grids)\n";
    result += "   Save realizations as = " % QString::number(getSaveRealizationsOption()) % " (0=variables in simulation grid 1=separate grid files in project 2=separate grid files elsewhere (specify path below))\n";
    result += "   Save realizations path (needed if option above is 2) = " % getSaveRealizationsPath() % '\n';
    return result;
}
