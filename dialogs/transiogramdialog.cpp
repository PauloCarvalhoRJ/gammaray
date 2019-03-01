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

#include "domain/datafile.h"
#include "domain/application.h"
#include "domain/segmentset.h"
#include "domain/project.h"
#include "domain/attribute.h"
#include "domain/categorydefinition.h"
#include "domain/auxiliary/faciestransitionmatrixmaker.h"
#include "domain/auxiliary/thicknesscalculator.h"
#include "widgets/fileselectorwidget.h"
#include "widgets/transiogramchartview.h"
#include "geostats/geostatsutils.h"

TransiogramDialog::TransiogramDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TransiogramDialog)
{
    ui->setupUi(this);

    setWindowTitle("Transiogram Dialog");

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    setAcceptDrops(true);
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
    for( QWidget* qcv : m_chartViews )
        delete qcv;
    m_chartViews.clear();
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
    double dh = ( hFinal - hInitial ) / nSteps;

    //----------------------------------------------COMPUTE FTMs FOR ALL h's-----------------------------------

    //get pointer to the category definition of the first variable (assumed the same for all variables).
    DataFile* parentOfFirst = dynamic_cast<DataFile*>( m_categoricalAttributes.front()->getContainingFile() );
    CategoryDefinition* CDofFirst = parentOfFirst->getCategoryDefinition( m_categoricalAttributes.front() );
    CDofFirst->readFromFS();

    //one FTM per h.
    typedef std::pair<double, FaciesTransitionMatrix> hFTM;
    std::vector<hFTM> hFTMs;

    //for each separation h
    for( double h = hInitial; h <= hFinal; h += dh ){
        //create a FTM for all categorical variables for each h.
        FaciesTransitionMatrix ftmAll("");
        ftmAll.setInfo( CDofFirst->getName() );
        ftmAll.initialize();
        hFTMs.push_back( { h, ftmAll } );
    }

    //for each file (each categorical attribute)
    for( Attribute* at : m_categoricalAttributes ){
        //get the data file
        DataFile* dataFile = dynamic_cast<DataFile*>( at->getContainingFile() );
        Application::instance()->logInfo("Commencing work on " + dataFile->getName() + "/" + at->getName() + "...");
        QApplication::processEvents();
        //if the data file is a segment set
        if( dataFile->getFileType() == "SEGMENTSET" ){
            //load data from file system
            dataFile->readFromFS();
            //make an auxiliary object to count facies transitions at given separations
            FaciesTransitionMatrixMaker<SegmentSet> ftmMaker( dynamic_cast<SegmentSet*>(dataFile),
                                                              at->getAttributeGEOEASgivenIndex()-1 );
            //for each separation h
            for( hFTM& hftm : hFTMs ){
                Application::instance()->logInfo("   working on h = " + QString::number( hftm.first ) + "...");
                QApplication::processEvents();
                //make a Facies Transion Matrix for a given h
                FaciesTransitionMatrix ftm = ftmMaker.makeAlongTrajectory( hftm.first, toleranceCoefficient * hftm.first );
                //add its counts to the global FTM for a given h
                hftm.second.add( ftm );
            }
        } else {
            Application::instance()->logError("TransiogramDialog::performCalculation(): Data files of type " +
                                               dataFile->getFileType()+ " not currently supported.  Transiogram calculation will be incomplete or not done at all.", true);
        }
    }

    //get a reference to one of the FTM (assumes the FTM for each h referes to the same facies after compression).
    FaciesTransitionMatrix& firstFTM = hFTMs.front().second;

    //----------------------------------------------COMPRESS FTMs---------------------------------------------------

    Application::instance()->logInfo("Compressing FTMs...");
    QApplication::processEvents();

    //for each category (compress columns)
    for( int i = 0; i < firstFTM.getColumnCount(); ++i ){
        //assume all columns are full of zeroes
        bool keepColumn = false;
        //for each separation h, query whether we have columns with only zeroes in all the FTMs.
        for( const hFTM& hftm : hFTMs ){
            const FaciesTransitionMatrix& ftm = hftm.second;
            if( ! ftm.isColumnZeroed( i ) )
                keepColumn = true;
        }
        //if a column for all h's were full of zeros
        if( !keepColumn ){
            //removes all zeroed columns for each separation h
            // so they all have the same facies
            for( hFTM& hftm : hFTMs ){
                FaciesTransitionMatrix& ftm = hftm.second;
                ftm.removeColumn( i );
            }
            --i;
        }
    }

    //for each category (compress rows)
    for( int i = 0; i < firstFTM.getRowCount(); ++i ){
        //assume all rows are full of zeroes
        bool keepRow = false;
        //for each separation h, query whether we have rows with only zeroes in all the FTMs.
        for( const hFTM& hftm : hFTMs ){
            const FaciesTransitionMatrix& ftm = hftm.second;
            if( ! ftm.isRowZeroed( i ) )
                keepRow = true;
        }
        //if a row for all h's were full of zeros
        if( !keepRow ){
            //removes all zeroed rows for each separation h
            // so they all have the same facies
            for( hFTM& hftm : hFTMs ){
                FaciesTransitionMatrix& ftm = hftm.second;
                ftm.removeRow( i );
            }
            --i;
        }
    }

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
        if( color.lightnessF() < 0.6 )
            fontColor = "white";
        faciesLabel->setText( "<font color=\"" + fontColor + "\"><b>" + firstFTM.getColumnHeader( j ) + "</b></font>");
        gridLayout->addWidget( faciesLabel, 0, j+1 );
    }

    // for each row (facies in the rows of the FTMs (one per h)
    for( int i = 0; i < firstFTM.getRowCount(); ++i ){

        //-------------------------------lay out the line headers.------------------------
        QLabel* faciesLabel = new QLabel();
        faciesLabel->setAlignment( Qt::AlignCenter );
        QColor color = firstFTM.getColorOfCategoryInRowHeader( i );
        faciesLabel->setStyleSheet( "QLabel {background-color: rgb(" +
                                                                   QString::number(color.red()) + "," +
                                                                   QString::number(color.green()) + "," +
                                                                   QString::number(color.blue()) +"); color: rgb(0.5,0.5,0.5); }");
        QString fontColor = "black";
        if( color.lightnessF() < 0.6 )
            fontColor = "white";
        faciesLabel->setText( "<font color=\"" + fontColor + "\"><b>" + firstFTM.getRowHeader( i ) + "</b></font>");
        gridLayout->addWidget( faciesLabel, i+1, 0 );
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
                                                     cd->getCategoryCodeByName( firstFTM.getRowHeader( i ) ) );
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
        for( int j = 0; j < firstFTM.getColumnCount(); ++j ){

            //according to  Li, W (2007) "Transiograms for Characterizing Spatial Variability of Soil Classes"
            //the sill of the transiogram is, ideally, the proportion of the tail facies for the cross-transiograms
            //(and for the auto-transiograms in effect)
            int tailFaciesCode = CDofFirst->getCategoryCodeByName( (*(hFTMs.begin())).second.getColumnHeader( j ) );

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
                if( i == j )
                    seriesExperimentalTransiogram->append( 0.0, 1.0 );
                else
                    seriesExperimentalTransiogram->append( 0.0, 0.0 );

                //for each separation h
                for( hFTM& hftm : hFTMs ){
                    double probability = hftm.second.getUpwardTransitionProbability( i, j );
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
            //axisY->setLabelFormat("%f1.0");
            QValueAxis *axisX = new QValueAxis();
            axisX->setRange( hInitial, hFinal );
            axisX->applyNiceNumbers();
            //axisX->setLabelFormat("%f5.1");

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
            if( i == j )
                transiogramType = TransiogramType::AUTO_TRANSIOGRAM;
            TransiogramChartView *chartView = new TransiogramChartView( chart, transiogramType, hFinal, axisX, axisY );
            {
                chartView->setRenderHint(QPainter::Antialiasing);
                chartView->setMinimumHeight( 100 );
                chartView->setSizePolicy( QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred );
                chartView->setModelParameters( 3.0, globalProportionOfTailFacies );
            }

            //lay out chart widget in the grid layout so it is in accordance to the pair of facies
            //as set in the FTM.
            gridLayout->setRowStretch( i+1, 1 );
            gridLayout->setColumnStretch( j+1, 1 );
            gridLayout->addWidget( chartView, i+1, j+1 );
            m_chartViews.push_back( chartView );
        }
    }

    Application::instance()->logInfo("Transiography completed.");
    QApplication::processEvents();
}

void TransiogramDialog::onResetAttributesList()
{
    m_categoricalAttributes.clear();
    ui->lblAttributesCount->setText("0 attribute(s)");
}
