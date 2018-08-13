#include "fkestimationrunner.h"
#include "fkestimation.h"
#include "domain/cartesiangrid.h"
#include "gridcell.h"

FKEstimationRunner::FKEstimationRunner(FKEstimation *fkEstimation, QObject *parent) :
    QObject(parent),
    m_finished( false ),
    m_fkEstimation( fkEstimation )
{
}

void FKEstimationRunner::doRun()
{
    CartesianGrid* estimationGrid = m_fkEstimation->getEstimationGrid();

    //get the grid dimensions
    uint nI = estimationGrid->getNX();
    uint nJ = estimationGrid->getNY();
    uint nK = estimationGrid->getNZ();

    //prepare the vector with the results (to not overwrite the original data)
    m_results.clear();
    m_results.reserve( nI * nJ * nK );

    //for all grid cells
    int nKriging = 0;
    int nIllConditioned = 0;
    int nFailed = 0;
    for( uint k = 0; k <nK; ++k)
        for( uint j = 0; j <nJ; ++j){
            emit setLabel("Running FK:\n" +
                          QString::number(nKriging) + " kriging operations (" +
                          QString::number(nIllConditioned) + " ill-conditioned, " +
                          QString::number(nFailed) + " failed). " );
            emit progress( j * nI + k * nI * nJ );
            for( uint i = 0; i <nI; ++i){
				GridCell estimationCell( estimationGrid, -1, i, j, k );
				m_results.push_back( fk( estimationCell, nIllConditioned, nFailed ) );
                ++nKriging;
            }
        }

    //inform the calling thread the computation has finished.
    m_finished = true;
}

double FKEstimationRunner::fk( GridCell& estimationCell, int &nIllConditioned, int &nFailed )
{
	//collects samples from the input data set ordered by their distance with respect
	//to the estimation cell.
	std::multiset<DataCell> vSamples = m_fkEstimation->getSamples( estimationCell );

	return -1.0;
}
