#ifndef CATEGORYPDF_H
#define CATEGORYPDF_H

#include "valuepairs.h"

typedef ValuePairs<int,double> IntDoublePairs;

class CategoryDefinition;

class CategoryPDF : public IntDoublePairs
{
public:
    CategoryPDF( CategoryDefinition *cd, QString path );
    /** Constructor with the name of a CategoryDefinition (delayed load).
      * @param categoryDefinitionName The name of a CategoryDefinition object existing in the project.
      */
    CategoryPDF( const QString categoryDefinitionName, QString path );

    /** Returns the CategoryDefinition object this object was based on.
        Returns nullptr if this object was not created from a CategoryDefinition (older versions of GammaRay)*/
    CategoryDefinition *getCategoryDefinition();

    /** Returns whether the probabilities sum up to 1.0. */
    bool sumsToOne( double tolerance = 0.001 ) const;

    /** Returns whether there is one (or more) zero (or less) probabilities. */
    bool hasZeroOrLessProb() const;

    /** Returns whether there is one (or more) negative probabilities. */
    bool hasNegativeProbabilities() const;

    /**
     * Returns a category code given a cumulative frequency.
     * The passed cumulative frequency is normally drawn from a random number generator.
     * This method is typically called from facies drawing code in simulation algorithms.
     * Returns -1 if somehow the method fails to find the category code corresponding to passed
     * cumulative frequency.
     */
    int getFaciesFromCumulativeFrequency( double cumulativeProbability );

    /** Returns the sum of the probabilities. */
    double sumProbs() const;

    // ProjectComponent interface
public:
    QIcon getIcon();
    void save(QTextStream *txt_stream);

    // File interface
public:
    bool canHaveMetaData(){ return false; }
    QString getFileType() const { return "CATEGORYPDF"; }
    void updateMetaDataFile(){}
    virtual bool isEditable(){ return true; }
    bool isDataFile(){ return false; }
	bool isDistribution(){ return false; } //Athough a PDF is technically a distribution, it doesn't inherit Distribution

private:
    CategoryDefinition *m_categoryDefinition;

    /** This value does not need to be updated after m_categoryDefinition pointer is set. */
    QString m_categoryDefinitionNameForDelayedLoad;

    /** Sets m_categoryDefinition by searching a CategoryDefinition with the name given by
     * m_categoryDefinitionNameForDelayedLoad.
     * Returns true if the search was successful.
     */
    bool setCategoryDefinition();
};

#endif // CATEGORYPDF_H
