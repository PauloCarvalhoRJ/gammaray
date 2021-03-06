#include "mcmcdataimputation.h"

#include "domain/segmentset.h"
#include "domain/categorydefinition.h"
#include "domain/attribute.h"
#include "domain/categorypdf.h"
#include "domain/univariatedistribution.h"
#include "domain/application.h"
#include "domain/auxiliary/faciestransitionmatrixmaker.h"
#include "domain/project.h"

#include <random>


MCMCDataImputation::MCMCDataImputation() :
    m_atVariable( nullptr ),
    m_dataSet ( nullptr ),
    m_FTM( nullptr ),
    m_distributions( std::map< int, UnivariateDistribution* >() ),
    m_atVariableGroupBy( nullptr ),
    m_pdfForImputationWithPreviousUnavailable( nullptr ),
    m_imputedData( std::vector< std::vector< std::vector<double> > >() )
{

}

bool MCMCDataImputation::run()
{
    //check whether everything is ok
    if( !isOKtoRun() )
        return false;

    //set ordering criteria to stablish the facies sequence for the Embedded Markov Chain part of the simulation
    int sortByColumnIndex = -1;
    SortingOrder ascendingOrDescending = SortingOrder::ASCENDING;
    if( m_sequenceDirection == SequenceDirection::FROM_MINUS_Z_TO_PLUS_Z ) {
        ascendingOrDescending = SortingOrder::ASCENDING;
        sortByColumnIndex = m_dataSet->getZindex()-1;
    }

    //get the index of the categorical variable to be imputed
    int indexCategoricalVariable = m_atVariable->getAttributeGEOEASgivenIndex()-1;

    //initialize the random number generator with the user-given seed
    std::mt19937 randomNumberGenerator;
    randomNumberGenerator.seed( m_seed );

    //load the thickness CDFs
    std::map<int, UnivariateDistribution*>::iterator it = m_distributions.begin();
    while( it != m_distributions.end() ){
        it->second->readFromFS();
        it++;
    }

    //for each realization
    for( int iReal = 0; iReal < m_imputedData.size(); ++iReal ){

        //get the empty realization dataframe
        std::vector< std::vector<double> >& imputedDataRealization = m_imputedData[iReal];

        //get the data set as either grouped by some variable or as is.
        std::vector< std::vector< std::vector<double> > > dataFrame;
        m_dataSet->loadData();
        if( m_atVariableGroupBy )
            dataFrame = m_dataSet->getDataGroupedBy( m_atVariableGroupBy->getAttributeGEOEASgivenIndex()-1 );
        else
            dataFrame.push_back( m_dataSet->getDataTable() );

        //keep track of the data row in the data file
        int currentDataRow = 0;

        //for each data group (may be just one)
        for( std::vector< std::vector< double > >& dataGroup : dataFrame ){

            double previousHeadX = std::numeric_limits<double>::quiet_NaN();
            double previousHeadY = std::numeric_limits<double>::quiet_NaN();
            double previousHeadZ = std::numeric_limits<double>::quiet_NaN();
            double previousTailX = std::numeric_limits<double>::quiet_NaN();
            double previousTailY = std::numeric_limits<double>::quiet_NaN();
            double previousTailZ = std::numeric_limits<double>::quiet_NaN();

            int previousFaciesCode = m_dataSet->getNoDataValueAsDouble();

            //Sort the data group to meet the sequence criterion wanted by the user.
            Util::sortDataFrame( dataGroup, sortByColumnIndex, ascendingOrDescending );

            //for each data row (for each segment)
            for( std::vector< double >& dataRow : dataGroup ){

                // get segment geometry
                double currentHeadX = dataRow[ m_dataSet->getXindex()-1 ];
                double currentHeadY = dataRow[ m_dataSet->getYindex()-1 ];
                double currentHeadZ = dataRow[ m_dataSet->getZindex()-1 ];
                double currentTailX = dataRow[ m_dataSet->getXFinalIndex()-1 ];
                double currentTailY = dataRow[ m_dataSet->getYFinalIndex()-1 ];
                double currentTailZ = dataRow[ m_dataSet->getZFinalIndex()-1 ];

                //if the current segment doesn't connect to the previous...
                if( ! Util::areConnected(  currentHeadX,  currentHeadY,  currentHeadZ,
                                           currentTailX,  currentTailY,  currentTailZ,
                                          previousHeadX, previousHeadY, previousHeadZ,
                                          previousTailX, previousTailY, previousTailZ ) ){
                    //invalidate the previous lithotype
                    previousFaciesCode = m_dataSet->getNoDataValueAsDouble();
                }

                //get the current facies code
                int currentFaciesCode = dataRow[ indexCategoricalVariable ];

                //if it is uninformed, proceed to imputation
                if( m_dataSet->isNDV( currentFaciesCode ) ){

                    //flag that signals the end of the impute process.
                    bool imputing = true;

                    //initialize the total thickness to imput with the total Z variation of the current segment
                    double remainingUninformedThickness = std::abs( currentTailZ - currentHeadZ );

                    //initialize the imputed segment's head coordinate with the base coordinate of the uninformed segment
                    double x0, y0, z0;
                    Util::getBaseCoordinate( currentHeadX, currentHeadY, currentHeadZ,
                                             currentTailX, currentTailY, currentTailZ,
                                             x0          , y0          , z0 );

                    //get the coordinate of the topmost end of the uninformed segment
                    double xTop, yTop, zTop;
                    Util::getTopCoordinate( currentHeadX, currentHeadY, currentHeadZ,
                                            currentTailX, currentTailY, currentTailZ,
                                            xTop        , yTop        , zTop );

                    while( imputing ){
                        //draw a facies code.
                        {
                            //Draw a cumulative probability from an uniform distribution
                            std::uniform_real_distribution<double> uniformDistributionBetween0and1( 0.0, 1.0 );
                            double prob = uniformDistributionBetween0and1( randomNumberGenerator );

                            //if there is a previous facies code, draw using the FTM (Markov Chains)
                            if( ! m_dataSet->isNDV( previousFaciesCode ) ) {
                                // get the next facies code from the FTM given a comulative probability drawn.
                                currentFaciesCode = m_FTM->getUpwardNextFaciesFromCumulativeFrequency( previousFaciesCode, prob );
                                if( currentFaciesCode < 0 ){
                                    m_lastError = "Simulated facies code somehow was invalid.  Check the FTM.";
                                    return false;
                                }
                            } else { //otherwhise, draw facies using the PDF (Monte Carlo)
                                if( ! m_pdfForImputationWithPreviousUnavailable ){
                                    m_lastError = "An uninformed location without a previous informed data was found (Markov "
                                                  "Chains not possible) but the user did not provide a fallback PDF.";
                                    //dump offending data line.
                                    Application::instance()->logError( "Dump of offending data line (processing order likely differs from file order):\n" + Util::dumpDataLine( dataRow ) );
                                    return false;
                                }
                                // get the next facies code from the PDF given a comulative probability drawn.
                                currentFaciesCode = m_pdfForImputationWithPreviousUnavailable->
                                        getFaciesFromCumulativeFrequency( prob );
                                if( currentFaciesCode < 0 ){
                                    m_lastError = "Simulated facies code somehow was invalid.  Check the PDF.";
                                    return false;
                                }
                            }
                        }// draw a facies code

                        //once the facies is known, draw a thickness from the facies' distribution of thickness
                        double thickness;
                        {
                            //Draw a cumulative probability from an uniform distribution
                            std::uniform_real_distribution<double> uniformDistributionBetween0and1( 0.0, 1.0 );
                            double prob = uniformDistributionBetween0and1( randomNumberGenerator );

                            thickness = m_distributions[ currentFaciesCode ]->getValueFromCumulativeFrequency( prob );

                            if( std::isnan( thickness ) ){
                                m_lastError = "An invalid thickness value was drawn.";
                                //dump offending data line.
                                Application::instance()->logError( "Dump of offending data line (processing order likely differs from file order):\n" + Util::dumpDataLine( dataRow ) );
                                return false;
                            }
                        }// draw a thickness

                        //imput a new segment
                        double x1, y1, z1;
                        {
                            double thicknessToUse = thickness;

                            //truncate the last segment so it fits in the remaining gap
                            if( thicknessToUse > remainingUninformedThickness ){
                                thicknessToUse = remainingUninformedThickness;
                                //signals there is nothing left to impute
                                imputing = false;
                            }

                            //initialize the new imputed segment with a copy of the uninformed segment
                            std::vector<double> newSegment = dataRow;

                            //compute the tail coordinate for the new imputed segment
                            z1 = z0 + thicknessToUse;
                            x1 = Util::linearInterpolation( z1, z0, zTop, x0, xTop );
                            y1 = Util::linearInterpolation( z1, z0, zTop, y0, yTop );

                            //set its geometry (resulted from the drawn thickness)
                            newSegment[ m_dataSet->getXindex()-1 ]      = x0;
                            newSegment[ m_dataSet->getYindex()-1 ]      = y0;
                            newSegment[ m_dataSet->getZindex()-1 ]      = z0;
                            newSegment[ m_dataSet->getXFinalIndex()-1 ] = x1;
                            newSegment[ m_dataSet->getYFinalIndex()-1 ] = y1;
                            newSegment[ m_dataSet->getZFinalIndex()-1 ] = z1;

                            //set the drawn facies
                            newSegment[ indexCategoricalVariable ] = currentFaciesCode;

                            //appends the imputed=yes flag to the imputed data
                            newSegment.push_back( 1 );

                            //appends the imputed data to the output
                            imputedDataRealization.push_back( newSegment );

                            //decreses the total thickness to impute
                            remainingUninformedThickness -= thicknessToUse;

                            //checks whether the imputation completed filling the entire gap
                            //the remaining thickness should be zero at the end of the process
                            if( ! imputing && ! Util::almostEqual2sComplement( remainingUninformedThickness, 0, 1 )){
                                Application::instance()->logWarn("MCMCDataImputation::run(): the remaining thickness is supposed to"
                                                                 " be zero after finishing an imputation. Got: " + QString::number( remainingUninformedThickness ) );
                                Application::instance()->logWarn("     data dump of the segment being imputed:");
                                Application::instance()->logWarn("     " + Util::dumpDataLine( dataRow ));
                            }
                        } //impute a new segment

                        //the initial coordinate of the next imputed segment will be the end of the current one
                        x0 = x1;
                        y0 = y1;
                        z0 = z1;

                    } //while imputing
                } else { //informed data is just copied to the output
                    dataRow.push_back( 0 ); //apends the imputed=no flag to the output
                    imputedDataRealization.push_back( dataRow );
                }

                // keep track of segment geometry for the next iteration to determine connectivity.
                previousHeadX = currentHeadX;
                previousHeadY = currentHeadY;
                previousHeadZ = currentHeadZ;
                previousTailX = currentTailX;
                previousTailY = currentTailY;
                previousTailZ = currentTailZ;

                previousFaciesCode = currentFaciesCode;

                ++currentDataRow;
            } //for each data row (for each segment)

        }//for each data group (may be just one)

        if( m_imputedData.empty() ){
            m_lastError = "MCMCDataImputation::run(): somehow the simulation completed with an empty data set.";
            return false;
        }

        //check wheather the realization has allowed transitions, if user wants this check.
        if( m_enforceFTM ){

            //Creates a new segment set object to house the imputed data.
            SegmentSet imputed_ss( "" );

            //Set the same metadata of the original data set.
            imputed_ss.setInfoFromAnotherSegmentSet( m_dataSet );

            //causes a population of child Attribute objects matching the ones from the original imput data set
            imputed_ss.setPath( m_dataSet->getPath() );
            imputed_ss.updateChildObjectsCollection();

            //loads the original data into the imputed data set
            imputed_ss.loadData();

            //adds a new Attribute corresponding to the imputed=1/0 flag, along with an extra data column
            imputed_ss.addEmptyDataColumn( "imputed", imputed_ss.getDataLineCount() );

            //replaces original data with the imputed data
            imputed_ss.replaceDataFrame( imputedDataRealization );

            //save the realization as temporary dataset
            QString tmpFileName = Application::instance()->getProject()->generateUniqueTmpFilePath("dat");
            imputed_ss.setPath( tmpFileName );
            imputed_ss.writeToFS();

//            //save its metadata file
//            imputed_ss->updateMetaDataFile();

//            //causes an update to the child objects in the project tree
//            imputed_ss->setInfoFromMetadataFile();

            //crate an FTM make auxiliary object.
            FaciesTransitionMatrixMaker<DataFile> ftmMaker( &imputed_ss, m_atVariable->getAttributeGEOEASgivenIndex()-1 );

            //if data file is a point or segment set...
            if( ! imputed_ss.isRegular() ){
                // Index == -1 means to not group by (treat entire data set as a single sequence).
                if( m_atVariableGroupBy )
                    ftmMaker.setGroupByColumn( m_atVariableGroupBy->getAttributeGEOEASgivenIndex()-1 );
                else
                    ftmMaker.setGroupByColumn( -1 );
            }

            // Compute FTM from alternating facies in data from -Z to +Z.
            FaciesTransitionMatrix ftmOfRealization = ftmMaker.makeSimple( DataSetOrderForFaciesString::FROM_BOTTOM_TO_TOP, false );

            // If the FTM of the realization has illegal transitions...
            if( m_enforceFTM->hasInexistentTransitions( ftmOfRealization, m_enforceThreshold ) ){
                //...warn user
                Application::instance()->logWarn("Realization have forbidden transitions.  Simulating again...");
                //...clear realization data
                imputedDataRealization.clear();
                //...simulate again
                --iReal;
            }

            // delete the temporary file
            imputed_ss.deleteFromFS();

        } //discard realizations with forbidden transitions (if user wants it)

    } //for each realization

    return true;
}

void MCMCDataImputation::setNumberOfRealizations(uint numberOfRealizations)
{
    m_imputedData = std::vector< std::vector< std::vector<double> > >( numberOfRealizations );
}

bool MCMCDataImputation::isOKtoRun()
{
    if( ! m_atVariable ){
        m_lastError = "Categorical variable not provided.";
        return false;
    }

    if( ! m_dataSet ){
        m_lastError = "Data set to be imputed not provided.";
        return false;
    } else if( ! m_dataSet->hasNoDataValue() ) {
        m_lastError = "Data set does not have a No-Data value configured.";
        return false;
    }

    if( ! m_FTM ){
        m_lastError = "Facies Transition Matrix not provided.";
        return false;
    } else if( ! m_FTM->isUsable() ) {
        m_lastError = "Facies Transition Matrix is not usable.  It likely contains a facies name not present the associated categorical definition.";
        return false;
    }

    {
        m_FTM->readFromFS();

        CategoryDefinition* cdOfVariable = m_dataSet->getCategoryDefinition( m_atVariable );
        if( ! cdOfVariable ){
            m_lastError = "Category definition of input variable not found (nullptr).";
            return false;
        }
        CategoryDefinition* cdOfFTM = m_FTM->getAssociatedCategoryDefinition();
        if( ! cdOfFTM ){
            m_lastError = "Category definition of FTM not found (nullptr).";
            return false;
        }
        if( cdOfFTM != cdOfVariable ){
            m_lastError = "Category definition of input variable must be the same object as that of the FTM.";
            return false;
        }
        if( ! m_FTM->isMainDiagonalZeroed() ){
            m_lastError = "FTM must have a zeroed main diagonal (no self-transitions) to be compatible with Embedded Markov Chains.";
            return false;
        }
    }

    CategoryDefinition* cdOfPrimData = m_dataSet->getCategoryDefinition( m_atVariable );
    {
        cdOfPrimData->loadQuintuplets();
        int nDistributions = m_distributions.size();
        int nCategories = cdOfPrimData->getCategoryCount();
        if( nDistributions != nCategories ){
            m_lastError = " Number of distributions ( " +
                    QString::number(nDistributions) + " ) differs from the number of categories ( " +
                    QString::number(nCategories) + " ).";
            return false;
        }
        for( const std::pair<int, UnivariateDistribution*>& facies_ud : m_distributions ){
            if( ! facies_ud.second ){
                m_lastError = "Passed null pointer to the collection of distributions.";
                return false;
            }
        }
    }

    if( m_pdfForImputationWithPreviousUnavailable ) {
        CategoryDefinition* cdOfPDF = m_pdfForImputationWithPreviousUnavailable->getCategoryDefinition();
        if( cdOfPDF ){
            if( cdOfPDF != cdOfPrimData ){
                m_lastError = "If a PDF is provides, its associated category definition must be the same object as "
                              "the input variable's.";
                return false;
            }
        }
    }

    if( m_sequenceDirection != SequenceDirection::FROM_MINUS_Z_TO_PLUS_Z ) {
        m_lastError = "Unrecognized sequence direction.  Must be one of the constants in the SequenceDirection enum.";
        return false;
    }

    if( m_enforceFTM ){
        if( ! m_enforceFTM->isUsable() ) {
            m_lastError = "Facies Transition Matrix to limit transitions is not usable.  It likely contains a facies name not present the associated categorical definition.";
            return false;
        }
        {
            m_enforceFTM->readFromFS();

            CategoryDefinition* cdOfVariable = m_dataSet->getCategoryDefinition( m_atVariable ); //this is verified previously
            CategoryDefinition* cdOfFTM = m_enforceFTM->getAssociatedCategoryDefinition();
            if( ! cdOfFTM ){
                m_lastError = "Category definition of FTM to enforce transitions not found (nullptr).";
                return false;
            }
            if( cdOfFTM != cdOfVariable ){
                m_lastError = "Category definition of input variable must be the same object as that of the FTM used to enforce transitions.";
                return false;
            }
        }
    }

    return true;
}
