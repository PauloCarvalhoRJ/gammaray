#include "transiogramdialog.h"
#include "ui_transiogramdialog.h"

#include <QChart>
#include <QLineSeries>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QGraphicsLayout>
#include <QValueAxis>
#include <QChartView>
#include <QInputDialog>
#include <QClipboard>
#include <QScrollBar>

#include "domain/datafile.h"
#include "domain/application.h"
#include "domain/segmentset.h"
#include "domain/project.h"
#include "domain/attribute.h"
#include "domain/categorydefinition.h"
#include "domain/auxiliary/faciestransitionmatrixmaker.h"
#include "domain/auxiliary/thicknesscalculator.h"
#include "domain/verticaltransiogrammodel.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/transiogramchartview.h"
#include "dialogs/dynamicfaciesrelationshipdiagramdialog.h"

TransiogramDialog::TransiogramDialog(VerticalTransiogramModel *vtm,
                                     QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TransiogramDialog),
    m_vSizePerTransiogram( 100 ),
    m_hSizePerTransiogram( 200 ),
    m_vtm( vtm )
{
    using namespace QtCharts;

    ui->setupUi(this);

    setWindowTitle("Transiogram Dialog");

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setAcceptDrops(true);

    //if client code passed a non-null VerticalTransiogramModel, we
    //must display the charts with the model curves
    if( m_vtm ){
        m_vtm->readFromFS();
        makeChartsForModelReview();
    }
}

TransiogramDialog::~TransiogramDialog()
{
    delete ui;
}

void TransiogramDialog::dragEnterEvent(QDragEnterEvent *e)
{
    e->acceptProposedAction();
}

void TransiogramDialog::dragMoveEvent(QDragMoveEvent *e)
{
    Q_UNUSED( e )
}

void TransiogramDialog::dropEvent(QDropEvent *e)
{
    //user may be dropping data files to add to the project
    if (e->mimeData()->hasUrls()) {
        foreach (const QUrl &url, e->mimeData()->urls()) {
            QString fileName = url.toLocalFile();
            Application::instance()->addDataFile( fileName );
        }
        return;
    }

    //otherwise, they are project objects
    //inform user that an object was dropped
    QString object_locator = e->mimeData()->text();
    Application::instance()->logInfo("TransiogramDialog::dropEvent(): Dropped object locator: " + object_locator);
    ProjectComponent* object = Application::instance()->getProject()->findObject( object_locator );
    if( ! object ){
        Application::instance()->logError("TransiogramDialog::dropEvent(): object not found. Drop operation failed.");
    } else {
        Application::instance()->logInfo("TransiogramDialog::dropEvent(): Found object: " + object->getName());
        if( object->isAttribute() ){
            Attribute* attributeAspect = dynamic_cast<Attribute*>( object );
            if( attributeAspect->isCategorical() ){
                tryToAddAttribute( attributeAspect );
            }else
                Application::instance()->logError("TransiogramDialog::dropEvent(): attribute is not categorical.");
        } else
            Application::instance()->logError("TransiogramDialog::dropEvent(): object is not an attribute.");
    }
}

void TransiogramDialog::tryToAddAttribute(Attribute *attribute)
{
    CategoryDefinition* CDofFirst = nullptr;
    if( ! m_categoricalAttributes.empty() ){
        //get info from the first attribute added
        DataFile* parentOfFirst = dynamic_cast<DataFile*>( m_categoricalAttributes.front()->getContainingFile() );
        CDofFirst = parentOfFirst->getCategoryDefinition( m_categoricalAttributes.front() );

        //get info from the attribute to be added
        DataFile* myParent = dynamic_cast<DataFile*>( attribute->getContainingFile() );
        CategoryDefinition* myCD = myParent->getCategoryDefinition( attribute );

        //the category defininion must be the same
        if( CDofFirst != myCD ){
            Application::instance()->logError( "TransiogramDialog::tryToAddAttribute(): all attributes must be associated to the same categorical definition.", true );
            return;
        }

        //it can't be added twice
        for( Attribute* at : m_categoricalAttributes ){
            if( at == attribute ){
                Application::instance()->logError( "TransiogramDialog::tryToAddAttribute(): the attribute has already been added.", true );
                return;
            }
        }
    }

    m_categoricalAttributes.push_back( attribute );
    ui->lblAttributesCount->setText( QString::number( m_categoricalAttributes.size() ) + " attribute(s)");
}

void TransiogramDialog::clearCharts()
{
    for( QWidget* cv : m_transiogramChartViews )
        delete cv;
    m_transiogramChartViews.clear();
    for( QWidget* cv : m_sumChartViews )
        delete cv;
    m_sumChartViews.clear();
}

void TransiogramDialog::makeChartsForModelReview()
{
    using namespace QtCharts;

    //get pointer to the grid layout (assumes it is a grid layout)
    QGridLayout* gridLayout = static_cast<QGridLayout*>( ui->frmTransiograms->layout() );

    //place a frame in the top left corner of the grid
    QFrame* topLeftCorner = new QFrame();
    gridLayout->addWidget( topLeftCorner, 0, 0 );

    //lay out the column headers.
    for( int j = 0; j < m_vtm->getRowOrColCount(); ++j ){
        QLabel* faciesLabel = new QLabel();
        faciesLabel->setAlignment( Qt::AlignCenter );
        QColor color = m_vtm->getColorOfCategory( j );
        faciesLabel->setStyleSheet( "QLabel {background-color: rgb(" +
                                                                   QString::number(color.red()) + "," +
                                                                   QString::number(color.green()) + "," +
                                                                   QString::number(color.blue()) +"); }");
        QString fontColor = "black";
        if( Util::isDark( color ) )
            fontColor = "white";
        faciesLabel->setText( "<font color=\"" + fontColor + "\"><b>" + m_vtm->getNameOfCategory( j ) + "</b></font>");
        gridLayout->addWidget( faciesLabel, 0, j+1 );
    }

    //put a last label corresponding to the sum of transiograms
    {
        QLabel* faciesLabel = new QLabel();
        faciesLabel->setAlignment( Qt::AlignCenter );
        QColor color = Qt::black;
        faciesLabel->setStyleSheet( "QLabel {background-color: rgb(" +
                                                                   QString::number(color.red()) + "," +
                                                                   QString::number(color.green()) + "," +
                                                                   QString::number(color.blue()) +"); }");
        faciesLabel->setText( "<font color=\"white\"><b>BALANCE</b></font>");
        gridLayout->addWidget( faciesLabel, 0, m_vtm->getRowOrColCount()+1 );
    }

    // for each row (facies in the rows of the FTMs (one per h)
    for( int iHeadFacies = 0; iHeadFacies < m_vtm->getRowOrColCount(); ++iHeadFacies ){

        //-------------------------------lay out the line headers.------------------------
        QLabel* faciesLabel = new QLabel();
        faciesLabel->setAlignment( Qt::AlignCenter );
        QColor color = m_vtm->getColorOfCategory( iHeadFacies );
        faciesLabel->setStyleSheet( "QLabel {background-color: rgb(" +
                                                                   QString::number(color.red()) + "," +
                                                                   QString::number(color.green()) + "," +
                                                                   QString::number(color.blue()) +"); color: rgb(0.5,0.5,0.5); }");
        QString fontColor = "black";
        if( Util::isDark( color ) )
            fontColor = "white";
        faciesLabel->setText( "<font color=\"" + fontColor + "\"><b>" + m_vtm->getNameOfCategory( iHeadFacies ) + "</b></font>");
        gridLayout->addWidget( faciesLabel, iHeadFacies+1, 0 );
        //-------------------------------------------------------------------------

        //loop to create a row of charts (for each column)
        for( int jTailFacies = 0; jTailFacies < m_vtm->getRowOrColCount(); ++jTailFacies ){

            int tailFaciesCode = m_vtm->getCodeOfCategory( jTailFacies );

            //set the final display range as 3x the longest range amongst the transiogram curves of the model
            double hFinal = m_vtm->getLongestRange() * 3.0;

            //create the chart's axes
            QValueAxis *axisY = new QValueAxis();
            axisY->setRange( 0.0, 1.0 );
            axisY->applyNiceNumbers();
            axisY->setLabelFormat(m_formatForLabelsInYAxis);
            QValueAxis *axisX = new QValueAxis();
            axisX->setRange( 0.0, hFinal );
            axisX->applyNiceNumbers();
            axisX->setLabelFormat(m_formatForLabelsInXAxis);

            //create a chart object to contain all the data series in the same chart area
            QChart *chart = new QChart();
            {
                chart->legend()->hide();
                //chart->addSeries( seriesExperimentalTransiogram );
                //chart->addSeries( seriesSill );
                //chart->createDefaultAxes();
                //chart->axisX()->setRange( hInitial, hFinal );
                //chart->axisY()->setRange( -1.0, 0.0 );

                //putting all series in the same scale
                //chart->setAxisX( axisX, seriesExperimentalTransiogram );
                //chart->setAxisY( axisY, seriesExperimentalTransiogram );
                //chart->setAxisX( axisX, seriesSill );
                //chart->setAxisY( axisY, seriesSill );

                //more space for the curves
                chart->layout()->setContentsMargins(2, 2, 2, 2);
                chart->setMargins(QMargins(2, 2, 2, 2));
            }

            //create a chart widget to render the chart object
            TransiogramType transiogramType = TransiogramType::CROSS_TRANSIOGRAM;
            if( iHeadFacies == jTailFacies )
                transiogramType = TransiogramType::AUTO_TRANSIOGRAM;
            TransiogramChartView *transiogramChartView = makeNewTransiogramChartView( chart,
                                                                                      transiogramType,
                                                                                      hFinal,
                                                                                      axisX,
                                                                                      axisY,
                                                                                      m_vtm->getNameOfCategory( iHeadFacies ),
                                                                                      m_vtm->getNameOfCategory( jTailFacies ),
                                                                                      m_vtm->getRange( iHeadFacies, jTailFacies ),
                                                                                      m_vtm->getSill( iHeadFacies, jTailFacies )
                                                                                    );

            //lay out chart widget in the grid layout so it is in accordance to the pair of facies
            //as set in the FTM.
            gridLayout->setRowStretch( iHeadFacies+1, 1 );
            gridLayout->setColumnStretch( jTailFacies+1, 1 );
            gridLayout->addWidget( transiogramChartView, iHeadFacies+1, jTailFacies+1 );
            m_transiogramChartViews.push_back( transiogramChartView );
        }//loop to create a row of charts (for each column)

        //append an additional chart so the user can check whether the transiogram models
        //sum to 1.0 at all h's.  Recall the transiograms model probabilities.  This means
        //that, for Markovian transiograms, they must sum up to 1.0 along a row.
        {
            //create a chart object to contain all the data series in the same chart area
            QChart *chart = new QChart();
            {
                chart->legend()->hide();
                //more space for the curves
                chart->layout()->setContentsMargins(2, 2, 2, 2);
                chart->setMargins(QMargins(2, 2, 2, 2));
            }
            QtCharts::QChartView *sumChartView = new QtCharts::QChartView( chart );
            gridLayout->setRowStretch( iHeadFacies+1, 1 );
            gridLayout->setColumnStretch( m_vtm->getRowOrColCount()+1, 1 );
            gridLayout->addWidget( sumChartView, iHeadFacies+1, m_vtm->getRowOrColCount()+1 );
            m_sumChartViews.push_back( sumChartView );
        }

    }// for each row (facies in the rows of the FTMs (one per h)

    ui->scrollAreaWidgetContents->setFixedSize( m_hSizePerTransiogram * m_vtm->getRowOrColCount() ,
                                                m_vSizePerTransiogram * m_vtm->getRowOrColCount() );

}

TransiogramChartView *TransiogramDialog::makeNewTransiogramChartView( QtCharts::QChart *chart,
                                                                      TransiogramType type,
                                                                      double hMax,
                                                                      QtCharts::QValueAxis *axisX,
                                                                      QtCharts::QValueAxis *axisY,
                                                                      QString headFaciesName,
                                                                      QString tailFaciesName,
                                                                      double initialRange,
                                                                      double initialSill )
{
    TransiogramChartView* transiogramChartView = new TransiogramChartView( chart,
                                     type,
                                     hMax,
                                     axisX,
                                     axisY,
                                     headFaciesName,
                                     tailFaciesName
                                     );

    transiogramChartView->setRenderHint(QPainter::Antialiasing);
    transiogramChartView->setMinimumHeight( 100 );
    transiogramChartView->setSizePolicy( QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred );
    transiogramChartView->setModelParameters( initialRange, initialSill );

    //to be notified if transiogram model updates.
    connect( transiogramChartView, SIGNAL(modelWasUpdated()), this, SLOT(onTransiogramModelUpdated()) );

    return transiogramChartView;
}

void TransiogramDialog::performCalculation()
{
    using namespace QtCharts;

    //guard against misconfiguration
    if( m_categoricalAttributes.empty() ){
        Application::instance()->logError( "TransiogramDialog::performCalculation(): no categorical attributes to work with." );
        return;
    }

    clearCharts();

    //----------------------------------------------GET USER SETTINGS-----------------------------------
    double hInitial = ui->dblSpinHIni->value();
    double hFinal = ui->dblSpinHFin->value();
    int nSteps = ui->spinNSteps->value();
    double toleranceCoefficient = ui->dblSpinTolCoeff->value();

    //----------------------------------------------COMPUTE FTMs FOR ALL h's-----------------------------------

//    //get pointer to the category definition of the first variable (assumed the same for all variables).
    DataFile* parentOfFirst = dynamic_cast<DataFile*>( m_categoricalAttributes.front()->getContainingFile() );
    CategoryDefinition* CDofFirst = parentOfFirst->getCategoryDefinition( m_categoricalAttributes.front() );
    CDofFirst->readFromFS();

    std::vector<hFTM> hFTMs = Util::computeFaciesTransitionMatrices( m_categoricalAttributes, hInitial, hFinal, nSteps, toleranceCoefficient );

    //get a reference to one of the FTM (assumes the FTM for each h referes to the same facies after compression).
    FaciesTransitionMatrix& firstFTM = hFTMs.front().second;

    //----------------------------------------------COMPRESS FTMs---------------------------------------------------

    Application::instance()->logInfo("Compressing FTMs...");
    QApplication::processEvents();

    Util::compressFaciesTransitionMatrices( hFTMs );

    //----------------------------------------------MAKE THE TRANSIOGRAMS CHARTS -----------------------------------

    Application::instance()->logInfo("Making transiograms...");
    QApplication::processEvents();

    //get pointer to the grid layout (assumes it is a grid layout)
    QGridLayout* gridLayout = static_cast<QGridLayout*>( ui->frmTransiograms->layout() );

    //place a frame in the top left corner of the grid
    QFrame* topLeftCorner = new QFrame();
    gridLayout->addWidget( topLeftCorner, 0, 0 );

    //lay out the column headers.
    for( int j = 0; j < firstFTM.getColumnCount(); ++j ){
        QLabel* faciesLabel = new QLabel();
        faciesLabel->setAlignment( Qt::AlignCenter );
        QColor color = firstFTM.getColorOfCategoryInColumnHeader( j );
        faciesLabel->setStyleSheet( "QLabel {background-color: rgb(" +
                                                                   QString::number(color.red()) + "," +
                                                                   QString::number(color.green()) + "," +
                                                                   QString::number(color.blue()) +"); }");
        QString fontColor = "black";
        if( Util::isDark( color ) )
            fontColor = "white";
        faciesLabel->setText( "<font color=\"" + fontColor + "\"><b>" + firstFTM.getColumnHeader( j ) + "</b></font>");
        gridLayout->addWidget( faciesLabel, 0, j+1 );
    }

    //put a last label corresponding to the sum of transiograms
    {
        QLabel* faciesLabel = new QLabel();
        faciesLabel->setAlignment( Qt::AlignCenter );
        QColor color = Qt::black;
        faciesLabel->setStyleSheet( "QLabel {background-color: rgb(" +
                                                                   QString::number(color.red()) + "," +
                                                                   QString::number(color.green()) + "," +
                                                                   QString::number(color.blue()) +"); }");
        faciesLabel->setText( "<font color=\"white\"><b>BALANCE</b></font>");
        gridLayout->addWidget( faciesLabel, 0, firstFTM.getColumnCount()+1 );
    }

    // for each row (facies in the rows of the FTMs (one per h)
    for( int iHeadFacies = 0; iHeadFacies < firstFTM.getRowCount(); ++iHeadFacies ){

        //-------------------------------lay out the line headers.------------------------
        QLabel* faciesLabel = new QLabel();
        faciesLabel->setAlignment( Qt::AlignCenter );
        QColor color = firstFTM.getColorOfCategoryInRowHeader( iHeadFacies );
        faciesLabel->setStyleSheet( "QLabel {background-color: rgb(" +
                                                                   QString::number(color.red()) + "," +
                                                                   QString::number(color.green()) + "," +
                                                                   QString::number(color.blue()) +"); color: rgb(0.5,0.5,0.5); }");
        QString fontColor = "black";
        if( Util::isDark( color ) )
            fontColor = "white";
        faciesLabel->setText( "<font color=\"" + fontColor + "\"><b>" + firstFTM.getRowHeader( iHeadFacies ) + "</b></font>");
        gridLayout->addWidget( faciesLabel, iHeadFacies+1, 0 );
        //-------------------------------------------------------------------------

        //compute the mean thickness for the "from" facies (facies in the row of the FTM)
        double meanThicknessForFromFacies = std::numeric_limits<double>::quiet_NaN();
        {
            //for each file (each categorical attribute)
            for( Attribute* at : m_categoricalAttributes ){
                DataFile* dataFile = dynamic_cast<DataFile*>( at->getContainingFile() );
                //dataFile->readFromFS(); //assumes the file has been loaded before in this routine.
                //if the data file is a segment set
                if( dataFile->getFileType() == "SEGMENTSET" ){
                    //make an auxiliary object to perform thickness-related calculations.
                    ThicknessCalculator<SegmentSet> thicknessCalculator( dynamic_cast<SegmentSet*>(dataFile),
                                                                         at->getAttributeGEOEASgivenIndex()-1 );
                    //get the category definition object
                    //assumes all FTMs for all h's refers to the same category definition
                    CategoryDefinition* cd = firstFTM.getAssociatedCategoryDefinition();
                    if( cd ){
                        //get the mean thickness occupied by the facies
                        meanThicknessForFromFacies = thicknessCalculator.getMeanThicknessForSingleValue(
                                                     cd->getCategoryCodeByName( firstFTM.getRowHeader( iHeadFacies ) ) );
                    } else {
                        Application::instance()->logError( "TransiogramDialog::performCalculation(): null category definition. "
                                                           "Mean thickness will be innacurate or invalid.");
                    }
                } else {
                    Application::instance()->logError("TransiogramDialog::performCalculation(): Data files of type " +
                                                       dataFile->getFileType()+ " not currently supported for thickness calculation. "
                                                      "Mean thickness will be innacurate or invalid.");
                }
            }
        }

        //loop to create a row of charts (for each column)
        for( int jTailFacies = 0; jTailFacies < firstFTM.getColumnCount(); ++jTailFacies ){

            //according to  Li, W (2007) "Transiograms for Characterizing Spatial Variability of Soil Classes"
            //the sill of the transiogram is, ideally, the proportion of the tail facies for the cross-transiograms
            //(and for the auto-transiograms in effect)
            int tailFaciesCode = CDofFirst->getCategoryCodeByName( (*(hFTMs.begin())).second.getColumnHeader( jTailFacies ) );

            //for each file (each categorical attribute)
            //compute the tail facies proportion, then
            //compute a proportion from all files
            double globalProportionOfTailFacies = 0.0;
            int globalCount = 0;
            for( Attribute* at : m_categoricalAttributes ){
                //get the data file
                DataFile* dataFile = dynamic_cast<DataFile*>( at->getContainingFile() );
                //get the proportion for the facies code in one data file
                double proportion = dataFile->getProportion( at->getAttributeGEOEASgivenIndex()-1, tailFaciesCode, tailFaciesCode );
                //make the global proportion a weighted mean of the proportion for each file
                globalProportionOfTailFacies += dataFile->getDataLineCount() * proportion;
                globalCount += dataFile->getDataLineCount();
            }
            globalProportionOfTailFacies /= globalCount;

            //create a data series (line in the chart) for the experimental transiogram
            QLineSeries *seriesExperimentalTransiogram = new QLineSeries();
            {
                //add the first point, the transriogram value at h = 0.
                //for auto-transiograms, its value is 1.0
                //for cross-transiograms, its value is 0.0
                // see "Transiograms for Characterizing Spatial Variability of Soil Classes", - Li, W. (2007)
                if( iHeadFacies == jTailFacies )
                    seriesExperimentalTransiogram->append( 0.0, 1.0 );
                else
                    seriesExperimentalTransiogram->append( 0.0, 0.0 );

                //for each separation h
                for( hFTM& hftm : hFTMs ){
                    double probability = hftm.second.getUpwardTransitionProbability( iHeadFacies, jTailFacies );
                    if( std::isfinite( probability ) && probability > 0.0)
                        seriesExperimentalTransiogram->append( hftm.first, probability );
                }
                QPen pen( QRgb(0xFF0000) );
                pen.setWidth( 3 );
                seriesExperimentalTransiogram->setPen( pen );
            }

            //create a data series (line in the chart) to represent graphically the sill a priori of the transiogram
            QLineSeries *seriesSill = new QLineSeries();
            {
                //make a single straight line to mark the sill in the graph
                seriesSill->append(                      0.0, globalProportionOfTailFacies );
                seriesSill->append( (*(hFTMs.end()-1)).first, globalProportionOfTailFacies );
                QPen pen( QRgb(0x008F00) );
                pen.setWidth( 1 );
                pen.setStyle( Qt::DashLine );
                seriesSill->setPen( pen );
            }

            //create the chart's axes
            QValueAxis *axisY = new QValueAxis();
            axisY->setRange( 0.0, 1.0 );
            axisY->applyNiceNumbers();
            axisY->setLabelFormat(m_formatForLabelsInYAxis);
            QValueAxis *axisX = new QValueAxis();
            axisX->setRange( hInitial, hFinal );
            axisX->applyNiceNumbers();
            axisX->setLabelFormat(m_formatForLabelsInXAxis);

            //create a chart object to contain all the data series in the same chart area
            QChart *chart = new QChart();
            {
                chart->legend()->hide();
                chart->addSeries( seriesExperimentalTransiogram );
                chart->addSeries( seriesSill );
                //chart->createDefaultAxes();
                //chart->axisX()->setRange( hInitial, hFinal );
                //chart->axisY()->setRange( -1.0, 0.0 );

                //putting all series in the same scale
                chart->setAxisX( axisX, seriesExperimentalTransiogram );
                chart->setAxisY( axisY, seriesExperimentalTransiogram );
                chart->setAxisX( axisX, seriesSill );
                chart->setAxisY( axisY, seriesSill );

                //more space for the curves
                chart->layout()->setContentsMargins(2, 2, 2, 2);
                chart->setMargins(QMargins(2, 2, 2, 2));
            }

            //create a chart widget to render the chart object
            TransiogramType transiogramType = TransiogramType::CROSS_TRANSIOGRAM;
            if( iHeadFacies == jTailFacies )
                transiogramType = TransiogramType::AUTO_TRANSIOGRAM;
            TransiogramChartView *transiogramChartView = makeNewTransiogramChartView( chart,
                                                                                      transiogramType,
                                                                                      hFinal,
                                                                                      axisX,
                                                                                      axisY,
                                                                                      firstFTM.getRowHeader( iHeadFacies ),
                                                                                      firstFTM.getColumnHeader( jTailFacies ),
                                                                                      (hFinal-hInitial) * 0.33,
                                                                                      globalProportionOfTailFacies
                                                                                    );

            //lay out chart widget in the grid layout so it is in accordance to the pair of facies
            //as set in the FTM.
            gridLayout->setRowStretch( iHeadFacies+1, 1 );
            gridLayout->setColumnStretch( jTailFacies+1, 1 );
            gridLayout->addWidget( transiogramChartView, iHeadFacies+1, jTailFacies+1 );
            m_transiogramChartViews.push_back( transiogramChartView );
        }//loop to create a row of charts (for each column)

        //append an additional chart so the user can check whether the transiogram models
        //sum to 1.0 at all h's.  Recall the transiograms model probabilities.  This means
        //that, for Markovian transiograms, they must sum up to 1.0 along a row.
        {
            //create a chart object to contain all the data series in the same chart area
            QChart *chart = new QChart();
            {
                chart->legend()->hide();
                //more space for the curves
                chart->layout()->setContentsMargins(2, 2, 2, 2);
                chart->setMargins(QMargins(2, 2, 2, 2));
            }
            QtCharts::QChartView *sumChartView = new QtCharts::QChartView( chart );
            gridLayout->setRowStretch( iHeadFacies+1, 1 );
            gridLayout->setColumnStretch( firstFTM.getColumnCount()+1, 1 );
            gridLayout->addWidget( sumChartView, iHeadFacies+1, firstFTM.getColumnCount()+1 );
            m_sumChartViews.push_back( sumChartView );
        }

    }// for each row (facies in the rows of the FTMs (one per h)

    ui->scrollAreaWidgetContents->setFixedSize( m_hSizePerTransiogram * firstFTM.getColumnCount() ,
                                                m_vSizePerTransiogram * firstFTM.getColumnCount() );

    Application::instance()->logInfo("Transiography completed.");
    QApplication::processEvents();
}

void TransiogramDialog::onSave()
{
    // get the category definition object and do some sanity checks
    CategoryDefinition* CDofFirst = nullptr;
    if( ! m_categoricalAttributes.empty() ){
        //get info from the first attribute added
        DataFile* parentOfFirst = dynamic_cast<DataFile*>( m_categoricalAttributes.front()->getContainingFile() );
        CDofFirst = parentOfFirst->getCategoryDefinition( m_categoricalAttributes.front() );
    } else {
        Application::instance()->logInfo( "TransiogramDialog::onSave(): no attributes. Assuming user is editing an existing model.", false );
        if( m_vtm ){
            CDofFirst = m_vtm->getCategoryDefinition();
        } else {
            Application::instance()->logError( "TransiogramDialog::onSave(): no attributes and no transiogram model to review. Aborted.", true );
            return;
        }
    }
    if ( ! CDofFirst ){
        Application::instance()->logError( "TransiogramDialog::onSave(): null category definition. Aborted.", true );
        return;
    }

    //determine whether we want to create a new transiogram model
    bool isNew = false;
    if( ! m_vtm )
        isNew = true;

    bool ok = true;
    QString new_transiogram_model_name;
    if( isNew ){
        //propose a name for the new file
        QString proposed_name = CDofFirst->getName() + "_Vertical_Transiogram_Model";
        //open file rename dialog
        new_transiogram_model_name = QInputDialog::getText(this, "Name the new file",
                                             "New vertical transiogram model file name:", QLineEdit::Normal,
                                                           proposed_name, &ok);
    }

    //create and populate the new object
    if (ok && ( !new_transiogram_model_name.isEmpty() || !isNew )){

        if( isNew )
            //create the domain object
            m_vtm = new VerticalTransiogramModel( Application::instance()->getProject()->getPath() + "/" + new_transiogram_model_name,
                                                                 CDofFirst->getName() );
        else
            //reset the existing one
            m_vtm->clearParameters();

        //get the transiogram parameters from the transiogram chart widgets.
        for( QWidget* w : m_transiogramChartViews ){
            TransiogramChartView* tcvAspect = static_cast< TransiogramChartView* >( w );
            m_vtm->addParameters( tcvAspect->getHeadFaciesName(), tcvAspect->getTailFaciesName(),
                                  { tcvAspect->getTransiogramStructureType(), tcvAspect->getRange(), tcvAspect->getSill() } );
        }

        //save it to file
        m_vtm->writeToFS();

        if( isNew ){
            //adds to the project tree
            Application::instance()->getProject()->addVerticalTransiogramModel( m_vtm );
            Application::instance()->refreshProjectTree();
        }
    }
}

void TransiogramDialog::onTransiogramModelUpdated()
{
    using namespace QtCharts;

    //get user setting
    double hInitial = ui->dblSpinHIni->value();
    double hFinal = ui->dblSpinHFin->value();

    //compute the number of columns of the matrix of transiogram chart views
    //by counting how many times one facies name appear as head facies
    int numberOfColumns = 0;
    {
        QString oneFaciesName;
        for( QWidget* w : m_transiogramChartViews ){
            TransiogramChartView* tcvAspect = static_cast< TransiogramChartView* >( w );
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

        //create a data series for the sum of transiogram model curve values
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
                TransiogramChartView* tcvAspect = static_cast< TransiogramChartView* >( m_transiogramChartViews[iTransiogramChart] );

                //get the transiogram model's data series
                QLineSeries *seriesTransiogramSeries = tcvAspect->getSeriesTransiogramModel();

                //traverse the transiogram model curve values
                int iPoint = 0;
                for( QPointF& point : seriesTransiogramSeries->pointsVector() ){
                    //append or add-up values
                    if( iTransiogramChart == iSumChart * numberOfColumns )
                        seriesSum->append( point.x(), point.y() );
                    else {
                        if( iPoint < seriesSum->count() ){
                            seriesSum->replace( iPoint, seriesSum->at(iPoint).x(), seriesSum->at(iPoint).y() + point.y() );
                        } else
                            Application::instance()->logError("TransiogramDialog::onTransiogramModelUpdated(): attempt to replace non-existent point. A graph is likely to be truncated.");
                    }
                    ++iPoint;
                }
            }
        }

        Application::instance()->logErrorOn();

        //create a data series (line in the chart) to represent graphically the 1.0 level for
        //the transiograms sum charts between the h's configured by the user
        QLineSeries *seriesOnes = new QLineSeries();
        {
            //make a single straight line to mark the sill in the graph
            seriesOnes->append( hInitial, 1.0 );
            seriesOnes->append( hFinal,   1.0 );
            QPen pen( QRgb(0x008F00) );
            pen.setWidth( 1 );
            pen.setStyle( Qt::DashLine );
            seriesOnes->setPen( pen );
        }

        //-------create the sum chart's axes once-----------------
        QValueAxis *axisY = dynamic_cast<QValueAxis *>( chart->axisY() );
        if( ! axisY )
            axisY = new QValueAxis();
        axisY->setRange( 0.0, 2.0 );
        axisY->applyNiceNumbers();
        axisY->setLabelFormat(m_formatForLabelsInYAxis);
        QValueAxis *axisX = dynamic_cast<QValueAxis *>( chart->axisX() );
        if( ! axisX )
            axisX = new QValueAxis();
        axisX->setRange( hInitial, hFinal );
        axisX->applyNiceNumbers();
        axisX->setLabelFormat(m_formatForLabelsInXAxis);
        //-------------------------------------------------

        chart->removeAllSeries();

        chart->addSeries( seriesSum );
        chart->addSeries( seriesOnes );

//            //putting all series in the same scale
        chart->setAxisX( axisX, seriesSum );
        chart->setAxisY( axisY, seriesSum );
        chart->setAxisX( axisX, seriesOnes );
        chart->setAxisY( axisY, seriesOnes );
    }
}

void TransiogramDialog::onDynamicFRD()
{
    double hInitial = ui->dblSpinHIni->value();
    double hFinal = ui->dblSpinHFin->value();
    int nSteps = ui->spinNSteps->value();
    double toleranceCoefficient = ui->dblSpinTolCoeff->value();

    DynamicFaciesRelationshipDiagramDialog *dfrdd = new DynamicFaciesRelationshipDiagramDialog(
                m_categoricalAttributes,
                hInitial,
                hFinal,
                nSteps,
                toleranceCoefficient,
                this );
    dfrdd->show();
}

void TransiogramDialog::onCaptureExperimentalTransiography()
{
//    //turn off model visibility
//    for( QWidget* w : m_transiogramChartViews ){
//        TransiogramChartView* tcvAspect = static_cast< TransiogramChartView* >( w );
//        tcvAspect->setModelVisible( false );
//    }

    //capture the transiogram widgets image
    QRect rectangle = ui->frmTransiograms->frameRect();
    QPixmap pixmap(rectangle.size());
    ui->frmTransiograms->render(&pixmap, QPoint(), QRegion(rectangle));

    //copy the image to clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setPixmap( pixmap,  QClipboard::Clipboard );
    QMessageBox::information(this, "Plot snapshot", "The transiograms image has been copied to the clipboard. You can paste it to any application.");

//    //turn back on model visibility
//    for( QWidget* w : m_transiogramChartViews ){
//        TransiogramChartView* tcvAspect = static_cast< TransiogramChartView* >( w );
//        tcvAspect->setModelVisible( true );
    //    }
}

void TransiogramDialog::onZoomIn()
{

    m_hSizePerTransiogram *= 1.25;
    m_vSizePerTransiogram *= 1.25;
    ui->scrollAreaWidgetContents->setFixedSize( m_hSizePerTransiogram * m_sumChartViews.size() ,
                                                m_vSizePerTransiogram * m_sumChartViews.size() );
}

void TransiogramDialog::onZoomOut()
{
    m_hSizePerTransiogram /= 1.25;
    m_vSizePerTransiogram /= 1.25;
    ui->scrollAreaWidgetContents->setFixedSize( m_hSizePerTransiogram * m_sumChartViews.size() ,
                                                m_vSizePerTransiogram * m_sumChartViews.size() );
}

void TransiogramDialog::onCaptureExperimentalTransiographyWithoutOffscreen()
{
    //capture the transiogram widgets image
    QRect rectangle = ui->scrollAreaTransiograms->frameRect();

    //scrolls to the bottom of plot for convenience
    int y = ui->scrollAreaTransiograms->verticalScrollBar()->value();
    int x = ui->scrollAreaTransiograms->horizontalScrollBar()->value();
    rectangle.moveTo( x, y );

    QPixmap pixmap(rectangle.size());
    ui->frmTransiograms->render(&pixmap, QPoint(), QRegion(rectangle));

    //copy the image to clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setPixmap( pixmap,  QClipboard::Clipboard );
    QMessageBox::information(this, "Plot snapshot", "The transiograms image (only visible ones) has been copied to the clipboard. You can paste it to any application.");
}

void TransiogramDialog::onResetAttributesList()
{
    m_categoricalAttributes.clear();
    ui->lblAttributesCount->setText("0 attribute(s)");
}
