#ifndef VERTICALPROPORTIONCURVEMAKER_H
#define VERTICALPROPORTIONCURVEMAKER_H

#include "domain/verticalproportioncurve.h"
#include "domain/categorydefinition.h"
#include "domain/categorypdf.h"
#include "spatialindex/spatialindex.h"
#include <cassert>

/** Adapters for the different data files.
 *  See the avaliable specializations in the cpp source file.  Add more as needed for
 *  object types other than those already implemented.*/
namespace VPCMakerAdapters {

    /** Returns the CategoryDefinition object associated with the given variable.
     * If the variable is not categorical, it returns nullptr.
     */
    template <typename Klass> CategoryDefinition* getAssociatedCategoryDefinition( Klass* dataFile,
                                                                                   int variableIndex );

    /** Builds the spatial index passed as parameter with the passed dataset. */
    template <typename Klass> void populateSpatialIndex( Klass* dataFile, SpatialIndex& spatialIndex );

    /**
     * Simply returns the value given a data record index.
     */
    template <typename Klass> double getValue( Klass* dataFile, int variableIndex, int dataIndex );

    /**
     * Returns the support size for a given data row index.  Point sets and Cartesian grids
     * have the same size for all samples and may return a constant (e.g. 1.0).  Segment sets
     * may return the length of the segment and GeoGrids may return the volume of the cell.
     * Implementations may should return a value greater than zero.
     */
    template <typename Klass> double getSupportSize( Klass* dataFile, int dataIndex );

}

/** This is a template class used to make vertical proportion curves for different
 *  domain objects and with different parameters.
 */
template <class Klass>
class VerticalProportionCurveMaker
{

public:
    VerticalProportionCurveMaker( Klass* dataFileWithFacies, int variableIndex ) :
        m_dataFileWithFacies( dataFileWithFacies ),
        m_variableIndex( variableIndex )
    {}

    /**
     * Computes a vertical proportion curve by counting facies between a top and a base depth
     * of the object passed in the constructor.
     *
     * @param resolution  Resolution in fraction between top and base (e.g.: 0.01 for 1%). The resolution
     *                    sets the number of steps between top and base.  E.g.: a 1% resulution means a curve
     *                    with 100 samples.
     * @param window      Computation window centered at each step along z axis in fraction between top and
     *                    base (e.g.: 0.03 for 3%).  The window is used to get smoother curves for high-resolution
     *                    data such as wells.
     * @param top         Initial depth.
     * @param base        Final depth.
     * @param fallBackPDF The fallback PDF sets default proportions when no data is found in a given step.
     */
    VerticalProportionCurve makeInDepthInterval( double resolution,
                                                 double window,
                                                 double top,
                                                 double base,
                                                 const CategoryPDF& fallBackPDF ){
        //retrieve category definition.
        CategoryDefinition* cd = VPCMakerAdapters::getAssociatedCategoryDefinition( m_dataFileWithFacies, m_variableIndex );
        assert( cd && "VerticalProportionCurveMaker::makeInDepthInterval(): null CategoryDefinition." );

        //ensure the fallback PDF has been loaded.
        assert( fallBackPDF.getPairCount() !=0 && "VerticalProportionCurveMaker::makeInDepthInterval(): fallback CategoryPDF not loaded." );

        //load the category definitions.
        cd->readFromFS();

        //create an empty VPC.
        VerticalProportionCurve vpc("", cd->getName() );

        //compute step and window in z units.
        double zStep = ( top - base ) * resolution;
        assert( zStep > 0.0 && "VerticalProportionCurveMaker::makeInDepthInterval(): top z lower than or equal to base z." );
        double zHalfWindowSize = ( top - base ) * window / 2.0;

        //builds the internal spatial index for the input data set.
        VPCMakerAdapters::populateSpatialIndex( m_dataFileWithFacies, m_spatialIndex );

        //create a vector to hold the summations for each category so we can compute
        //their proportions later.
        std::vector< double > sums( cd->getCategoryCount() );
        double total = 0.0;

        //traverse the z interval from the base to the top z.
        //the traversal is made by advancing in z steps from base to top
        //and the computation of proportions is made within a z window around
        //the center z of the step.
        for( double centerZ = base, center = 0.0;
             centerZ <= top;
             centerZ += zStep, center += resolution ){

            //get the data row indexes contained in the window.
            double queryMinZ = std::max( centerZ - zHalfWindowSize, base ); //cap query at base, in case the window extends below it.
            double queryMaxZ = std::min( centerZ + zHalfWindowSize, top ); //cap query at top, in case the window extends above it.
            QList<uint> rowIndexes = m_spatialIndex.getWithinZInterval( queryMinZ, queryMaxZ );

            //for each of the data found within the current z window.
            for( uint rowIndex : rowIndexes ){
                //get the value from the dtaa set (supposedly the category code).
                double value = VPCMakerAdapters::getValue( m_dataFileWithFacies, m_variableIndex, rowIndex );
                //skip uninformed data.
                if( m_dataFileWithFacies->isNDV( value ) )
                    continue;
                //get the category code.
                int categoryCode = static_cast<int>(value);
                //get its index in the category definition.
                int categoryIndex = cd->getCategoryIndex( categoryCode );
                //get the category count (1 for point sets and length for segment sets, for example).
                double count = VPCMakerAdapters::getSupportSize( m_dataFileWithFacies, rowIndex );
                //increment the category count.
                sums[ categoryIndex ] += count;
                //increment the total.
                total += count;
            }

            //adds a new all-zero entry to the VPC for the current relative depth.
            vpc.addNewEntry( center );

            //if there were data...
            if( total > 0.0 ){
                //...compute and assign the proportions for each category.
                for( int categoryIndex = 0; categoryIndex < sums.size(); ++categoryIndex ){
                    //get the category code.
                    int categoryCode = cd->getCategoryCode( categoryIndex );
                    //compute its proportion.
                    double proportion = sums[ categoryIndex ] / total;
                    //assign it to the last added entry of VPC object.
                    vpc.setProportion( vpc.getEntriesCount()-1, categoryCode, proportion );
                }
            //if no valid data were found in the z window....
            } else {
                //... assign the proportions of the fallback PDF.
                for( int categoryIndex = 0; categoryIndex < sums.size(); ++categoryIndex ){
                    //get the category code.
                    int categoryCode = cd->getCategoryCode( categoryIndex );
                    //get proportion from the fallback PDF.
                    double proportion = fallBackPDF.get2ndValue( categoryIndex );
                    //assign it to the last added entry of VPC object.
                    vpc.setProportion( vpc.getEntriesCount()-1, categoryCode, proportion );
                }
            }

        }

        return vpc;
    }

private:
    Klass* m_dataFileWithFacies;
    int m_variableIndex;
    SpatialIndex m_spatialIndex;
};

#endif // VERTICALPROPORTIONCURVEMAKER_H
