#ifndef CONTACTANALYSIS_H
#define CONTACTANALYSIS_H

#include "geostats/datacell.h"
#include "util.h"

#include <stdint.h>
#include <QString>

class Attribute;
class DataFile;
class SearchStrategy;
class SpatialIndex;

enum class ContactAnalysisMode : uint8_t {
    LATERAL,
    VERTICAL
};

/** Enum used to avoid the slow File::getFileType() in performance-critical code. */
typedef DataSetType InputDataSetType; //DataSetType is defined in util.h

/** This class encapsulates the Contact Analysis algorithm. */
class ContactAnalysis
{
public:

    typedef double Lag;
    typedef double MeanGradeDomain1;
    typedef double MeanGradeDomain2;
    typedef std::pair
                 < ContactAnalysis::MeanGradeDomain1,
                   ContactAnalysis::MeanGradeDomain2 >
            MeanGradesBothDomains;

    ContactAnalysis();

    //@{
    /** The input dataset. */
    DataFile* getInputDataFile() const;
    void setInputDataFile(DataFile *getInputDataFile);
    //@}

    //@{
    /** The continuous variable with the grade values. */
    Attribute* getAttributeGrade() const;
    void setAttributeGrade(Attribute *getAttributeGrade);
    //@}

    //@{
    /** The categorical variable with both domains. */
    Attribute* getAttributeDomains() const;
    void setAttributeDomains(Attribute *getAttributeDomains);
    //@}

    //@{
    /** Sets whether the contact analysis should be done laterally or vertically.
     * Setting this to VERTICAL for data sets without a Z-axis is undefined behavior.
     */
    ContactAnalysisMode getMode() const;
    void setMode(const ContactAnalysisMode &mode);
    //@}

    //@{
    /** Controls the Z tolerance for non-gridded data sets when mode is LATERAL.
     * This tolerance tells whether a neighboring data sample is at the same z-level
     * of a given sample.  This setting has no effect with gridded data sets, whose
     * K index is used to tell which samples are at the same level of a given cell.
     */
    double getZtolerance() const;
    void setZtolerance(double ztolerance);
    //@}

    //@{
    /** The category code corresponding to the 1st domain. This must be a valid
     * category code according to the m_attributeDomains' CategoryDefinition object.
     */
    uint16_t getDomain1_code() const;
    void setDomain1_code(const uint16_t &domain1_code);
    //@}

    //@{
    /** The category code corresponding to the 2nd domain. This must be a valid
     * category code according to the m_attributeDomains' CategoryDefinition object.
     */
    uint16_t getDomain2_code() const;
    void setDomain2_code(const uint16_t &domain2_code);
    //@}

    //@{
    /** The minimum number of samples found around a sample in order to compute the mean
     * grade value.  The sample is marked as visited but its mean grade around it is ignored.
     */
    uint16_t getMinNumberOfSamples() const;
    void setMinNumberOfSamples(const uint16_t &minNumberOfSamples);
    //@}

    //@{
    /** The maximum number of samples found around a sample in order to compute the mean
     * grade value.  The excess samples found in the neighborhood of the sample are ignored.
     */
    uint16_t getMaxNumberOfSamples() const;
    void setMaxNumberOfSamples(const uint16_t &maxNumberOfSamples);
    //@}

    //@{
    /** The initial lag size in length units (e.g. 100m).  The lag defines a radius around each
     * visited sample within which neighboring samples are searched.
     */
    double getLagSize() const;
    void setLagSize(double lagSize);
    //@}

    //@{
    /** The number of multiples of m_lagSize used to expand the search.  For example, if m_lagSize
     * is 100m and this setting is 5, then this algorithm will be run with 100m, 200m, 300m, 400m
     * and 500m radii.  Then, a scatter plot with five points for each domain representing the mean
     * grades found for each of them is generated.
     */
    uint16_t getNumberOfLags() const;
    void setNumberOfLags(const uint16_t &numberOfLags);
    //@}


    /** Executes the algorithm.  isOKtoRun() is called to check whether all parameters are set properly.
     * @return True if the algorithm finishes without issues (check getLastError() otherwise).
     */
    bool run();

    /**
     * Returns the contact analysis results, that is, pairs of lag value and mean grades of both domains.
     * These values can be used, for example, to make plots.
     * @note Both means may be std::numeric_limits<double>::quiet_NaN if the respective search failed to find
     *       non-visited neighboring samples with non-dummy grade values within the given lag
     *       belonging to the given domain.
     */
    std::vector<
          std::pair<
              ContactAnalysis::Lag,
              ContactAnalysis::MeanGradesBothDomains
          >
    > getResults() const;

    /** Informs the last error during execution of the algorithm.  This must be checked if run()
     * returns false.
     */
    QString getLastError() const { return m_lastError; }

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
    Attribute* m_attributeGrade;
    Attribute* m_attributeDomains;
    ContactAnalysisMode m_mode;
    double m_ztolerance;
    uint16_t m_domain1_code;
    uint16_t m_domain2_code;
    uint16_t m_minNumberOfSamples;
    uint16_t m_maxNumberOfSamples;
    double m_lagSize;
    uint16_t m_numberOfLags;
    //---------------------------------------------

    QString m_lastError;
    InputDataSetType m_inputDataType;

    //stores the contact analysis results, that is, pairs of lag value and mean grades of both domains.
    std::vector<std::pair< ContactAnalysis::Lag, ContactAnalysis::MeanGradesBothDomains> > m_results;

    /** Returns a container with the input data samples around a data location to be used in the contact analysis.
     * The resulting collection depends on the SearchStrategy object set for the primary data.  Returns an empty object if any
     * required parameter for the search to work (e.g. input data) is missing.  The data cells are ordered
     * by their distance to the passed data sample.
     */
    DataCellPtrMultiset getSamplesFromInputDataSet(const DataCell& sample ,
                                                   const SearchStrategy &searchStrategyPrimary ,
                                                   const SpatialIndex &spatialIndexOfPrimaryData ) const;
};

#endif // CONTACTANALYSIS_H
