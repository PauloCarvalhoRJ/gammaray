#include "transiogrambanddialog.h"

#include <QInputDialog>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include "widgets/transiogrambandchartview.h"
#include "domain/attribute.h"
#include "domain/application.h"
#include "domain/categorydefinition.h"
#include "domain/project.h"

TransiogramBandDialog::TransiogramBandDialog(VerticalTransiogramModel *vtm,
                                             VerticalTransiogramModel *vtm2,
                                             QWidget *parent) :
    TransiogramDialog( vtm, parent ),
    m_vtm2( vtm2 )
{
    setWindowTitle("Transiogram Band Dialog");

    //Since base class' virtual functions are used when called from its constructor,
    //we need to re-build any single VTM transiogram chart widgets that TransiogramDialog's constructor
    //may have built so we have two-VTM chart widgets as required
    //for a transiogram band dialog.

    //if client code passed two non-null VerticalTransiogramModels, it means that the user
    //wants to review a transiogram band (assuming they're compatible).  Then, we
    //must display the charts with both models curves
    if( m_vtm && m_vtm2 ){
        m_vtm2->readFromFS(); //assuming m_vtm's data is already loaded by base class' constructor.
        clearCharts(); //destroy and single-transiogram chart widgets that the base class' constructor may have created.
        makeChartsForModelReview(); //creates two-transiogram chart widgets now.
    }

}

TransiogramChartView *TransiogramBandDialog::makeNewTransiogramChartView( QtCharts::QChart *chart,
                                                                          TransiogramType type,
                                                                          double hMax,
                                                                          QtCharts::QValueAxis *axisX,
                                                                          QtCharts::QValueAxis *axisY,
                                                                          QString headFaciesName,
                                                                          QString tailFaciesName,
                                                                          double initialRange,
                                                                          double initialSill)
{
    TransiogramBandChartView* transiogramChartView = new TransiogramBandChartView( chart,
                                                                                   type,
                                                                                   hMax,
                                                                                   axisX,
                                                                                   axisY,
                                                                                   headFaciesName,
                                                                                   tailFaciesName
                                                                                   );

    transiogramChartView->setRenderHint( QPainter::Antialiasing );
    transiogramChartView->setMinimumHeight( 100 );
    transiogramChartView->setSizePolicy( QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred );
    transiogramChartView->setModelParameters( initialRange, initialSill );
    transiogramChartView->setModel2Parameters( initialRange, initialSill );

    // If there is a 2nd variogram model, retrieve its parameters and reset the curve in the chart widget for the 2nd VTM.
    if( m_vtm2 ){
        int headFaciesVTM2MatrixIndex = m_vtm2->getCategoryMatrixIndex( headFaciesName );
        int tailFaciesVTM2MatrixIndex = m_vtm2->getCategoryMatrixIndex( tailFaciesName );
        double sill2  = m_vtm2->getSill ( headFaciesVTM2MatrixIndex, tailFaciesVTM2MatrixIndex );
        double range2 = m_vtm2->getRange( headFaciesVTM2MatrixIndex, tailFaciesVTM2MatrixIndex );
        transiogramChartView->setModel2Parameters( range2, sill2 );
    }

    //to be notified if the two transiogram models update.
    //onTransiogramModel2Updated() calls base class' onTransiogramModelUpdated(), so we need only to
    //connect both signals to the onTransiogramModel2Updated() slot.
    connect( transiogramChartView, SIGNAL(modelWasUpdated()),  this, SLOT(onTransiogramModel2Updated()) );
    connect( transiogramChartView, SIGNAL(model2WasUpdated()), this, SLOT(onTransiogramModel2Updated()) );

    return transiogramChartView;
}

void TransiogramBandDialog::onSave()
{
    //Saving the 1st transiogram model is already done in superclass' code.
    TransiogramDialog::onSave();

    // get the category definition object and do some sanity checks
    CategoryDefinition* CDofFirst = nullptr;
    if( ! m_categoricalAttributes.empty() ){
        //get info from the first attribute added
        DataFile* parentOfFirst = dynamic_cast<DataFile*>( m_categoricalAttributes.front()->getContainingFile() );
        CDofFirst = parentOfFirst->getCategoryDefinition( m_categoricalAttributes.front() );
    } else {
        Application::instance()->logInfo( "TransiogramBandDialog::onSave(): no attributes. Assuming user is editing an existing 2nd model.", false );
        if( m_vtm2 ){
            CDofFirst = m_vtm2->getCategoryDefinition();
        } else {
            Application::instance()->logError( "TransiogramBandDialog::onSave(): no attributes and no 2nd transiogram model to review. Aborted.", true );
            return;
        }
    }
    if ( ! CDofFirst ){
        Application::instance()->logError( "TransiogramBandDialog::onSave(): null category definition. Aborted.", true );
        return;
    }

    //determine whether we want to create a new transiogram model
    bool isNew = false;
    if( ! m_vtm2 )
        isNew = true;

    bool ok = true;
    QString new_transiogram_model_name;
    if( isNew ){
        //propose a name for the new file
        QString proposed_name = CDofFirst->getName() + "_Vertical_Transiogram_Model_Two";
        //open file rename dialog
        new_transiogram_model_name = QInputDialog::getText(this, "Name the new file",
                                             "New second vertical transiogram model file name:", QLineEdit::Normal,
                                                           proposed_name, &ok);
    }

    //create and populate the new object
    if (ok && ( !new_transiogram_model_name.isEmpty() || !isNew )){

        if( isNew )
            //create the domain object
            m_vtm2 = new VerticalTransiogramModel( Application::instance()->getProject()->getPath() + "/" + new_transiogram_model_name,
                                                                 CDofFirst->getName() );
        else
            //reset the existing one
            m_vtm2->clearParameters();

        //get the transiogram parameters from the transiogram chart widgets.
        for( QWidget* w : m_transiogramChartViews ){
            TransiogramBandChartView* tcvAspect = static_cast< TransiogramBandChartView* >( w );
            m_vtm2->addParameters(  tcvAspect->getHeadFaciesName(),
                                    tcvAspect->getTailFaciesName(),
                                    { tcvAspect->getTransiogram2StructureType(),
                                      tcvAspect->getRange2(),
                                      tcvAspect->getSill2()
                                    } );
        }

        //save it to file
        m_vtm2->writeToFS();

        if( isNew ){
            //adds the second transiogram model to the project tree
            Application::instance()->getProject()->addVerticalTransiogramModel( m_vtm2 );
            Application::instance()->refreshProjectTree();
        }
    }
}

void TransiogramBandDialog::onTransiogramModel2Updated()
{
    using namespace QtCharts;

    //do the stuff for the 1st transiograms.
    onTransiogramModelUpdated();

    //compute the number of columns of the matrix of transiogram chart views
    //by counting how many times one facies name appear as head facies
    int numberOfColumns = 0;
    {
        QString oneFaciesName;
        for( QWidget* w : m_transiogramChartViews ){
            TransiogramBandChartView* tcvAspect = static_cast< TransiogramBandChartView* >( w );
            if( oneFaciesName.isEmpty() ){
                oneFaciesName = tcvAspect->getHeadFaciesName();
                ++numberOfColumns;
            }else{
                if( tcvAspect->getHeadFaciesName() == oneFaciesName )
                    ++numberOfColumns;
            }
        }
    }

    //for each sum chart
    for( int iSumChart = 0; iSumChart < m_sumChartViews.size(); ++iSumChart ){

        QChartView *sumChartView = dynamic_cast<QChartView *>( m_sumChartViews[iSumChart] );

        QChart *chart = sumChartView->chart();

        Application::instance()->logErrorOff();

        //create a data series for the sum of 2nd transiogram model curve values
        QLineSeries *seriesSum = new QLineSeries();
        {
            QPen pen( QRgb(0x008F8F) );
            pen.setWidth( 3 );
            seriesSum->setPen( pen );

            //for each transiogram chart of the sum chart's row
            for( int iTransiogramChart = iSumChart * numberOfColumns;
                     iTransiogramChart < ( iSumChart + 1 ) * numberOfColumns;
                     ++iTransiogramChart
               ){
                //get the transiogram chart
                TransiogramBandChartView* tcvAspect = static_cast< TransiogramBandChartView* >(
                            m_transiogramChartViews[iTransiogramChart] );

                //get the transiogram model's data series
                QLineSeries *seriesTransiogram2Series = tcvAspect->getSeriesTransiogramModel2();

                //traverse the transiogram model curve values
                int iPoint = 0;
                for( QPointF& point : seriesTransiogram2Series->pointsVector() ){
                    //append or add-up values
                    if( iTransiogramChart == iSumChart * numberOfColumns )
                        seriesSum->append( point.x(), point.y() );
                    else {
                        if( iPoint < seriesSum->count() ){
                            seriesSum->replace( iPoint, seriesSum->at(iPoint).x(), seriesSum->at(iPoint).y() + point.y() );
                        } else
                            Application::instance()->logError("TransiogramBandDialog::onTransiogramModelUpdated(): "
                                                              "attempt to replace non-existent point. A graph is "
                                                              "likely to be truncated.");
                    }
                    ++iPoint;
                }
            }
        }

        Application::instance()->logErrorOn();

        //Assuming the sum chart's axes were created in TransiogramDialog::onTransiogramModelUpdated()
        QValueAxis *axisY = dynamic_cast<QValueAxis *>( chart->axisY() );
        QValueAxis *axisX = dynamic_cast<QValueAxis *>( chart->axisX() );

        //assuming the other data series (curves) were added in
        //TransiogramDialog::onTransiogramModelUpdated(), so only adding
        //the summation curve for the 2nd transiograms
        chart->addSeries( seriesSum );

        //putting all series in the same scale
        chart->setAxisX( axisX, seriesSum );
        chart->setAxisY( axisY, seriesSum );
    }
}
