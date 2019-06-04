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
};

#endif // COMMONSIMULATIONPARAMETERS_H
