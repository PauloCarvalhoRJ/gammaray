#ifndef DRIFTANALYSIS_H
#define DRIFTANALYSIS_H

#include "util.h"

#include <stdint.h>

#include <QString>

class DataFile;
class Attribute;

class DriftAnalysis
{
public:

    typedef double coordX;
    typedef double coordY;
    typedef double coordZ;
    typedef double Mean;

    DriftAnalysis();

    //@{
    /** The input dataset. */
    DataFile *getInputDataFile() const;
    void setInputDataFile(DataFile *inputDataFile);
    //@}

    //@{
    /** The input variable. */
    Attribute *getAttribute() const;
    void setAttribute(Attribute *attribute);
    //@}

    //@{
    /** The number of steps in which divide the south-north, west-east and vertical extents.
     * The greater this number, the greater the resolution of the output data.
     */
    uint16_t getNumberOfSteps() const;
    void setNumberOfSteps(const uint16_t &numberOfSteps);
    //@}

    /** The input dataset. */
    QString getLastError() const;

    //@{
    /** The results of the drift analysis. */
    std::vector< std::pair< DriftAnalysis::coordX, DriftAnalysis::Mean > > getResultsWestEast() const;
    std::vector< std::pair< DriftAnalysis::coordY, DriftAnalysis::Mean > > getResultsSouthNorth() const;
    std::vector< std::pair< DriftAnalysis::coordZ, DriftAnalysis::Mean > > getResultsVertical() const;
    //@}

    /** Executes the algorithm.  isOKtoRun() is called to check whether all parameters are set properly.
     * @return True if the algorithm finishes without issues (check getLastError() otherwise).
     */
    bool run();

private:

    /**
     * Verifies whether everything is correct to execute the algorithm.
     * If any, the reason for last check failure must be checked with getLastError().
     * This is called by run().
     * @return True if the algorithm is all set to start.
     */
    bool isOKtoRun();

    //-------------agorithm parameters-------------
    DataFile*  m_inputDataFile;
    Attribute* m_attribute;
    uint16_t m_NumberOfSteps;
    //---------------------------------------------

    QString m_lastError;
    DataSetType m_inputDataType;

    //stores the drift analysis results, that is, pairs of X/Y/Z values and mean of
    //the input variable in the three directions (vertical will be empty for 2D datasets).
    std::vector< std::pair< DriftAnalysis::coordX, DriftAnalysis::Mean > > m_resultsWestEast;
    std::vector< std::pair< DriftAnalysis::coordY, DriftAnalysis::Mean > > m_resultsSouthNorth;
    std::vector< std::pair< DriftAnalysis::coordZ, DriftAnalysis::Mean > > m_resultsVertical;
};

#endif // DRIFTANALYSIS_H
