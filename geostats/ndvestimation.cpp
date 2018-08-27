#include "ndvestimation.h"

#include "domain/cartesiangrid.h"
#include "domain/attribute.h"
#include "domain/application.h"
#include "gridcell.h"
#include "ndvestimationrunner.h"
#include <limits>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>

NDVEstimation::NDVEstimation(Attribute *at) :
    _at(at),
    _searchMaxNumSamples(16),
    _searchNumCols(10),
    _searchNumRows(10),
    _searchNumSlices(1),
    _vmodel( nullptr ),
    _useDefaultValue( false ),
    _defaultValue( 0.0 ),
    _meanForSK( 0.0 ),
    _ndv( std::numeric_limits<double>::quiet_NaN() ),
    _ktype( KrigingType::SK )
{}

std::vector<double> NDVEstimation::run()
{
    if( ! _vmodel ){
        Application::instance()->logError("NDVEstimation::run(): variogram model not specified. Aborted.");
        return std::vector<double>();
    } else {
        _vmodel->readFromFS();
    }

    //Assumes the partent file of the selected attribut is a Cartesian grid
    CartesianGrid *cg = (CartesianGrid*)_at->getContainingFile();

    if( ! cg->hasNoDataValue() ){
        Application::instance()->logError("NDVEstimation::run(): No-data-value not set for the grid. Aborted.");
        return std::vector<double>();
    } else {
        bool ok;
        _ndv = cg->getNoDataValue().toDouble( &ok );
        if( ! ok ){
            Application::instance()->logError("NDVEstimation::run(): No-data-value setting is not a valid number. Aborted.");
            return std::vector<double>();
        }
    }

    //loads data previously to prevent clash with the progress dialog of both data
    //loading and estimation running.
    cg->loadData();

    //get the grid dimensions
    uint nI = cg->getNX();
    uint nJ = cg->getNY();
    uint nK = cg->getNZ();

    Application::instance()->logInfo("NDV Estimation started...");

    Application::instance()->logWarningOff();
    Application::instance()->logErrorOff();

    //estimation takes place in another thread, so we can show and update a progress bar
    //////////////////////////////////
    QProgressDialog progressDialog;
    progressDialog.show();
    progressDialog.setLabelText("Running estimation...");
    progressDialog.setMinimum( 0 );
    progressDialog.setValue( 0 );
    progressDialog.setMaximum( nI * nJ * nK );
    QThread* thread = new QThread();
    NDVEstimationRunner* runner = new NDVEstimationRunner( this, _at ); // Do not set a parent. The object cannot be moved if it has a parent.
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

    Application::instance()->logWarningOn();
    Application::instance()->logErrorOn();

    std::vector<double> results = runner->getResults();

    delete runner;

    Application::instance()->logInfo("NDV Estimation completed.");

    return results;
}

void NDVEstimation::setSearchParameters(int searchMaxNumSamples,
                                        int searchNumCols,
                                        int searchNumRows,
                                        int searchNumSlices)
{
    _searchMaxNumSamples=searchMaxNumSamples;
    _searchNumCols=searchNumCols;
    _searchNumRows=searchNumRows;
    _searchNumSlices=searchNumSlices;
}
int NDVEstimation::searchMaxNumSamples() const
{
    return _searchMaxNumSamples;
}

void NDVEstimation::setSearchMaxNumSamples(int searchMaxNumSamples)
{
    _searchMaxNumSamples = searchMaxNumSamples;
}
int NDVEstimation::searchNumCols() const
{
    return _searchNumCols;
}

void NDVEstimation::setSearchNumCols(int searchNumCols)
{
    _searchNumCols = searchNumCols;
}
int NDVEstimation::searchNumRows() const
{
    return _searchNumRows;
}

void NDVEstimation::setSearchNumRows(int searchNumRows)
{
    _searchNumRows = searchNumRows;
}
int NDVEstimation::searchNumSlices() const
{
    return _searchNumSlices;
}

void NDVEstimation::setSearchNumSlices(int searchNumSlices)
{
    _searchNumSlices = searchNumSlices;
}
double NDVEstimation::ndv() const
{
    return _ndv;
}

void NDVEstimation::setNdv(double ndv)
{
    _ndv = ndv;
}
VariogramModel *NDVEstimation::vmodel() const
{
    return _vmodel;
}

void NDVEstimation::setVmodel(VariogramModel *vmodel)
{
    _vmodel = vmodel;
}
double NDVEstimation::meanForSK() const
{
    return _meanForSK;
}

void NDVEstimation::setMeanForSK(double meanForSK)
{
    _meanForSK = meanForSK;
}
KrigingType NDVEstimation::ktype() const
{
    return _ktype;
}

void NDVEstimation::setKtype(const KrigingType &ktype)
{
    _ktype = ktype;
}









