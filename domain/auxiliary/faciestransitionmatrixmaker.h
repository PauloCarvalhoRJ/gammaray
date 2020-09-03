#ifndef FACIESTRANSITIONMATRIXMAKER_H
#define FACIESTRANSITIONMATRIXMAKER_H

#include "domain/faciestransitionmatrix.h"
#include "domain/categorydefinition.h"
#include <cassert>

/// Defines how a data set is trasversed so the resulting facies string has some spatial characteristic.
enum class DataSetOrderForFaciesString : uint {
    FROM_BOTTOM_TO_TOP ///Forms vertical strings of facies sorted from bottom upwards (from -Z to +Z).
};


/** Adapters for the different data files.
 *  See the avaliable specializations in the cpp source file.  Add more as needed for
 *  object types other than those already implemented.*/
namespace FTMMakerAdapters {

    /** Returns the total length of the trajectory. */
    template <typename Klass> double getTrajectoryLength( Klass* dataFile );

    /** Returns the value at given distance in trajectory.  Zero corresponds to the
     * beginning of the trajectory and the distance equalling getTrajectoryLength() corresponds
     * to the end of the trajectory.  Negative distances, distances beyond getTrajectoryLength() or
     * distances falling on gaps will cause this function to return std::numeric_limits::quiet_NaN().
     */
    template <typename Klass> double getValueInTrajectory( Klass* dataFile, int variableIndex,
                                                           double distance, double tolerance );

    /** Returns the CategoryDefinition object associated with the given variable.
     * If the variable is not categorical, it returns nullptr.
     */
    template <typename Klass> CategoryDefinition* getAssociatedCategoryDefinition( Klass* dataFile, int variableIndex );

    /**
     * Simply returns the value given a data record index.
     */
    template <typename Klass> double getValue( Klass* dataFile, int variableIndex, int dataIndex );

    /**
     * Returns a collection of strings of facies codes following a specific order.
     * @param groupByVariableIndex If set to a value different from -1, specifies the index of the variable
     *        to be used to group the data.  That is, to treat each group as a separate data set.  This is
     *        mainly useful with single files that contains data from multiple sources (e.g. drill holes).
     *        The variable is normally some interger valued id (e.g. well number).
     * @param ignoreGaps If true, samples not connected in space are treated as if they were connected
     *                   in the sequence.
     */
    template <typename Klass> std::vector< std::vector< int > > getFaciesSequence(
                                                Klass* dataFile,
                                                int dataColumnWithFaciesCodes,
                                                DataSetOrderForFaciesString dataIndexOrder,
                                                int groupByVariableIndex = -1,
                                                bool ignoreGaps = false );
}

/** This is a template class used to make facies transition matrices for different
 *  domain objects and with different parameters.
 */
template <class Klass>
class FaciesTransitionMatrixMaker
{
public:

    FaciesTransitionMatrixMaker( Klass* dataFileWithFacies, int variableIndex ) :
        m_dataFileWithFacies( dataFileWithFacies ),
        m_variableIndex( variableIndex ),
        m_groupByColumn( -1 )
    {
    }

    /**
     * Computes a facies transition matrix by counting facies changes at the given separation h along
     * the trajectory of the object passed in the constructor.  The object is assumed
     * to be a series of data (e.g. point set) ordered along a linear trajectory
     * (e.g. of a well, drillhole or countour).  Stratigraphy convention requires that the facies count
     * begins from the end (early in geologic time) to the start (late in geologic time) of the sequence.
     *
     * @param h         Separation in length units.
     * @param tolerance Tolerance in length units.  Normally this is useful when values are
     *                  located in points (zero size).
     */
    FaciesTransitionMatrix makeAlongTrajectory( double h, double tolerance ){
        //retrieve category definition
        CategoryDefinition* cd = FTMMakerAdapters::getAssociatedCategoryDefinition( m_dataFileWithFacies, m_variableIndex );
        assert( cd && "FaciesTransitionMatrixMaker::makeAlongTrajectory(): null CategoryDefinition." );
        //create an empty FTM
        FaciesTransitionMatrix ftm("");
        ftm.setInfo( cd->getName() );
        ftm.initialize();
        //traverse trajectory in steps of size h counting facies transitions
        //from end (early in geologic time) to begining (late in geologic time).
        double trajectoryLength = FTMMakerAdapters::getTrajectoryLength( m_dataFileWithFacies );
        int previousFaciesCode = -9999; //hopefully no one uses -9999 as code.
        for( double distance = trajectoryLength; distance >= 0.0; distance -= h ){
            int faciesCode = -9998;
            double readValue = FTMMakerAdapters::getValueInTrajectory
                               ( m_dataFileWithFacies, m_variableIndex, distance, tolerance );
            if( std::isfinite( readValue ) ){
                faciesCode = static_cast<int>( readValue );
                if( /*faciesCode != previousFaciesCode &&*/ previousFaciesCode != -9999 ){
                    ftm.incrementCount( previousFaciesCode, faciesCode );
                }
                previousFaciesCode = faciesCode;
            }
        }
        return ftm;
    }

    /**
     * Computes a facies transition matrix by simply counting facies changes between data elements
     * along some sequence (e.g. from last to first) of the object passed in the constructor.
     * @param ignoreGaps If true, samples not connected in space are treated as if they were connected
     *                   in the sequence.
     */
    FaciesTransitionMatrix makeSimple( DataSetOrderForFaciesString dataIndexOrder, bool ignoreGaps ){
        //retrieve category definition
        CategoryDefinition* cd = FTMMakerAdapters::getAssociatedCategoryDefinition( m_dataFileWithFacies, m_variableIndex );
        assert( cd && "FaciesTransitionMatrixMaker::makeSimple(): null CategoryDefinition." );
        //create an empty FTM
        FaciesTransitionMatrix ftm("");
        ftm.setInfo( cd->getName() );
        ftm.initialize();

        std::vector< std::vector< int > > faciesStrings =
                FTMMakerAdapters::getFaciesSequence( m_dataFileWithFacies,
                                                     m_variableIndex,
                                                     dataIndexOrder,
                                                     m_groupByColumn,
                                                     ignoreGaps );

        //each facies string is treated separately (e.g. traces of a seismic volume)
        for( const std::vector< int >& faciesString : faciesStrings ){
            int previousFaciesCode = -9999; //hopefully no one uses -9999 as code.
            for( int faciesCode : faciesString ){
                if( previousFaciesCode != -9999 )
                    ftm.incrementCount( previousFaciesCode, faciesCode );
                previousFaciesCode = faciesCode;
            }
        }

        return ftm;
    }


    /**
     * If set to a value different from -1, uses the data in the n-th column
     * to separate the data into groups.  This is typically used with single data
     * files that contain data from different sources (e.g. multiple drill holes).  The
     * data used to group is normally some kind of id (e.g. well number).
     * If this is not set, all data will be treated as a single string and, thus,
     * the resulting matrix may be different and likely wrong by treating supposedly separate samples
     * as contiguous.
     */
    void setGroupByColumn(int groupByColumn){ m_groupByColumn = groupByColumn; }
    int getGroupByColumn() const { return m_groupByColumn; }

private:
    Klass* m_dataFileWithFacies;
    int m_variableIndex;
    int m_groupByColumn;
};

#endif // FACIESTRANSITIONMATRIXMAKER_H
