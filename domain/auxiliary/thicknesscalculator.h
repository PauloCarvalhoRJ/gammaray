#ifndef THICKNESSCALCULATOR_H
#define THICKNESSCALCULATOR_H

#include "util.h"

#include <limits>

/** Adapters for the different data files.
 *  See the avaliable specializations in the cpp source file.  Add more as needed for
 *  object types other than those already implemented.*/
namespace ThicknessCalculatorAdapters {

    /** Returns the number of interval units. An interval unit is any entity
     * possessing extent in vertical axis that has a single value. A cell of a
     * cell-centered grid is an example as it has vertical extent and has a
     * single value per variable in it.
     */
    template <typename Klass> double getNumberOfIntervalUnits( Klass* dataFile );

    /** Returns the thickness of the given interval unit. */
    template <typename Klass> double getThickness( Klass* dataFile,
                                                   int intervalUnitIndex );

    /** The value of the given variable in the given interval unit. */
    template <typename Klass> double getValueInIntervalUnit( Klass* dataFile,
                                                             int variableIndex,
                                                             int intervalUnitIndex );


}

/** This is a template class used to make thickness calculations for different
 *  domain objects and with different parameters.
 */
template <class Klass>
class ThicknessCalculator
{
public:
    ThicknessCalculator( Klass* dataFileWithFacies, int variableIndex ) :
        m_dataFileWithFacies( dataFileWithFacies ),
        m_variableIndex( variableIndex )
    {
    }

    /** Returns the mean thickness which contains the given value.
     * Normally this is for categorical values (e.g. facies).
     * Returns std::numeric_limits::quiet_NaN() if the value is not found anywhere.
     */
    double getMeanThicknessForSingleValue( double value ){
        int n = ThicknessCalculatorAdapters::getNumberOfIntervalUnits( m_dataFileWithFacies );
        double sumThickness = 0.0;
        int count = 0;
        for( int i = 0; i < n; ++i ){
            double valueInIntervalUnit = ThicknessCalculatorAdapters::getValueInIntervalUnit( m_dataFileWithFacies,
                                                                                              m_variableIndex,
                                                                                              i );
            if( Util::almostEqual2sComplement( valueInIntervalUnit, value, 1 ) ){
                valueInIntervalUnit += ThicknessCalculatorAdapters::getThickness( m_dataFileWithFacies, i );
                ++count;
            }
        }
        if( count )
            return sumThickness / count;
        else
            std::numeric_limits<double>::quiet_NaN();
    }

private:
    Klass* m_dataFileWithFacies;
    int m_variableIndex;
};

#endif // THICKNESSCALCULATOR_H
