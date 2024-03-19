#ifndef COMMONSIMULATIONPARAMETERS_H
#define COMMONSIMULATIONPARAMETERS_H

#include "gslib/gslibparameterfiles/gslibparameterfile.h"

/** Specialization of GSLibParameterFile used to store common simulation parameters
 * such as random number generator seed, number of realizations and search parameters.
 */
class CommonSimulationParameters : public GSLibParameterFile
{
public:
    CommonSimulationParameters();

    void setBaseNameForRealizationVariables( const QString baseName );

    QString getBaseNameForRealizationVariables();
    uint    getSeed();
    uint    getNumberOfRealizations();
    double  getSearchEllipHMax();
    double  getSearchEllipHMin();
    double  getSearchEllipHVert();
    double  getSearchEllipAzimuth();
    double  getSearchEllipDip();
    double  getSearchEllipRoll();
    uint    getNumberOfSamples();
    uint    getMinNumberOfSamples();
    uint    getNumberOfSectors();
    uint    getMinNumberOfSamplesPerSector();
    uint    getMaxNumberOfSamplesPerSector();
    double  getMinDistanceBetweenSecondaryDataSamples();
    uint    getNumberOfSimulatedNodesForConditioning();
    uint    getSearchAlgorithmOptionForSimGrid();
    uint    getSaveRealizationsOption();
    QString getSaveRealizationsPath();

    /** Returns a multi-line text listing the values of the parameters.
     * This is useful so the user can check the parameters.
     */
    QString print();
};

#endif // COMMONSIMULATIONPARAMETERS_H
