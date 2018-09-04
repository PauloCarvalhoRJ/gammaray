#include "fkestimation.h"
#include "searchstrategy.h"
#include "domain/datafile.h"
#include "domain/cartesiangrid.h"
#include "domain/pointset.h"
#include "domain/attribute.h"
#include "domain/application.h"
#include "fkestimationrunner.h"
#include "datacell.h"
#include "gridcell.h"
#include "pointsetcell.h"
#include "spatialindex/spatialindexpoints.h"

#include <QCoreApplication>
#include <QProgressDialog>
#include <QThread>
#include <iostream>

FKEstimation::FKEstimation() :
    m_searchStrategy( nullptr ),
    m_variogramModel( nullptr ),
    m_meanSK( 0.0 ),
    m_ktype( KrigingType::OK ),
    m_at_input( nullptr ),
	m_cg_estimation( nullptr ),
	m_spatialIndexPoints( new SpatialIndexPoints() ),
    m_inputDataFile( nullptr ),
    m_factorNumber( 0 ) //0 == nugget effect.
{
}

FKEstimation::~FKEstimation()
{
	delete m_spatialIndexPoints;
}

void FKEstimation::setSearchStrategy(SearchStrategyPtr searchStrategy)
{
    m_searchStrategy = searchStrategy;
}

void FKEstimation::setVariogramModel(VariogramModel *variogramModel)
{
    m_variogramModel = variogramModel;
	//Take the opportunity to update the variogam sill value.
	m_variogramSill = variogramModel->getSill();
}

void FKEstimation::setMeanForSimpleKriging(double meanSK)
{
    m_meanSK = meanSK;
}

void FKEstimation::setKrigingType(KrigingType ktype)
{
    m_ktype = ktype;
}

void FKEstimation::setInputVariable(Attribute *at_input)
{
    m_at_input = at_input;
	//Update the pointer to the data file;
	m_inputDataFile = static_cast<DataFile*>( m_at_input->getContainingFile() );
	//Build a spatial index according to the type of the data file.
	if( m_inputDataFile->isRegular() ){
		Application::instance()->logError( "FKEstimation::setInputVariable(): SPATIAL INDEX NOT IMPLEMENTED FOR REGULAR DATA SETS." );
	} else {
		PointSet* ps = static_cast<PointSet*>( m_inputDataFile );
		m_spatialIndexPoints->fill( ps, 0.000001 );
		Application::instance()->logInfo( "Spatial index created for " + m_inputDataFile->getName() + " point set." );
	}
}

void FKEstimation::setEstimationGrid(CartesianGrid *cg_estimation)
{
    m_cg_estimation = cg_estimation;
}

void FKEstimation::setFactorNumber(int factorNumber)
{
    m_factorNumber = factorNumber;
}

DataCellPtrMultiset FKEstimation::getSamples(const GridCell & estimationCell )
{
	DataCellPtrMultiset result;
	if( m_searchStrategy && m_at_input ){

		if( m_inputDataFile->isRegular() ){
			Application::instance()->logError( "FKEstimation::getSamples(): NOT IMPLEMENTED FOR REGULAR DATA SETS." );
		} else { //TODO: this currently assumes the irregular data is a PointSet object.
            QList<uint> samplesIndexes = m_spatialIndexPoints->getNearestWithin( estimationCell, m_searchStrategy->m_nb_samples, *(m_searchStrategy->m_searchNB) );
            QList<uint>::iterator it = samplesIndexes.begin();
			for( ; it != samplesIndexes.end(); ++it ){
				DataCellPtr p(new PointSetCell( static_cast<PointSet*>( m_inputDataFile ), m_at_input->getAttributeGEOEASgivenIndex()-1, *it ));
				p->computeCartesianDistance( estimationCell );
				result.insert( p );
			}
		}
	} else {
		Application::instance()->logError( "FKEstimation::getSamples(): sample search failed.  Search strategy and/or input data not set." );
	}
	return result;
}

std::vector<double> FKEstimation::run( )
{
    if( ! m_variogramModel ){
        Application::instance()->logError("FKEstimation::run(): variogram model not specified. Aborted.", true);
        return std::vector<double>();
    } else {
        m_variogramModel->readFromFS();
    }

    //Get the data file containing the input variable.
    DataFile *input_datafile = static_cast<DataFile*>( m_at_input->getContainingFile());

    if( ! input_datafile->hasNoDataValue() ){
        Application::instance()->logError("FKEstimation::run(): No-data-value not set for the input dataset. Aborted.", true);
        return std::vector<double>();
    } else {
        bool ok;
        m_NDV_of_input = input_datafile->getNoDataValue().toDouble( &ok );
        if( ! ok ){
            Application::instance()->logError("FKEstimation::run(): No-data-value setting of the input dataset is not a valid number. Aborted.", true);
            return std::vector<double>();
        }
    }

    if( ! m_cg_estimation->hasNoDataValue() ){
        Application::instance()->logError("FKEstimation::run(): No-data-value not set for the estimation grid. Aborted.", true);
        return std::vector<double>();
    } else {
        bool ok;
        m_NDV_of_output = m_cg_estimation->getNoDataValue().toDouble( &ok );
        if( ! ok ){
            Application::instance()->logError("FKEstimation::run(): No-data-value setting of the output grid is not a valid number. Aborted.", true);
            return std::vector<double>();
        }
    }

    //loads data previously to prevent clash with the progress dialog of both data
    //loading and estimation running.
    input_datafile->loadData();
    m_cg_estimation->loadData();

    //get the estimation grid dimensions
    uint nI = m_cg_estimation->getNX();
    uint nJ = m_cg_estimation->getNY();
    uint nK = m_cg_estimation->getNZ();

    Application::instance()->logInfo("Factorial Kriging started...");

    //suspend message reporting as it tends to slow things down.
    Application::instance()->logWarningOff();
    Application::instance()->logErrorOff();

    //estimation takes place in another thread, so we can show and update a progress bar
    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Running FK...");
    progressDialog.setMinimum( 0 );
    progressDialog.setValue( 0 );
    progressDialog.setMaximum( nI * nJ * nK );
    QThread* thread = new QThread();
    FKEstimationRunner* runner = new FKEstimationRunner( this );
    runner->moveToThread(thread);
    runner->connect(thread, SIGNAL(finished()), runner, SLOT(deleteLater()));
    runner->connect(thread, SIGNAL(started()), runner, SLOT(doRun()));
    runner->connect(runner, SIGNAL(progress(int)), &progressDialog, SLOT(setValue(int)));
    runner->connect(runner, SIGNAL(setLabel(QString)), &progressDialog, SLOT(setLabelText(QString)));
    thread->start();
    /////////////////////////////////

    //wait for the kriging to finish
    //not very beautiful, but simple and effective
    while( ! runner->isFinished() ){
        thread->wait( 200 ); //reduces cpu usage, refreshes at each 200 milliseconds
        QCoreApplication::processEvents(); //let Qt repaint widgets
    }

    //flushes any messages that have been generated for logging.
    Application::instance()->logWarningOn();
    Application::instance()->logErrorOn();

    //get the factor wanted by the user.
    std::vector<double> results;
	if( m_factorNumber == -1 )
        results = runner->getMeans();
    else
        results = runner->getFactor();

    //discard the worker object.
    delete runner;

    Application::instance()->logInfo("Factorial Kriging completed.");

    return results;
}
