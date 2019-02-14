#include "transiogramdialog.h"
#include "ui_transiogramdialog.h"

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QGraphicsLayout>

#include "domain/datafile.h"
#include "domain/application.h"
#include "domain/auxiliary/faciestransitionmatrixmaker.h"
#include "domain/segmentset.h"
#include "domain/project.h"
#include "domain/attribute.h"
#include "domain/categorydefinition.h"

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

void TransiogramDialog::performCalculation()
{
    using namespace QtCharts;

    //guard against misconfiguration
    if( m_categoricalAttributes.empty() ){
        Application::instance()->logError( "TransiogramDialog::performCalculation(): no categorical attributes to work with." );
        return;
    }

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

    //the column headers.
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

    for( int i = 0; i < firstFTM.getRowCount(); ++i ){

        //the line headers.
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

        //loop to create a row of charts
        for( int j = 0; j < firstFTM.getColumnCount(); ++j ){

             QLineSeries *series = new QLineSeries();

             //for each separation h
             for( hFTM& hftm : hFTMs ){
                 double rate = hftm.second.getTransitionRate( i, j, hftm.first, true );
                 if( std::isfinite( rate ))
                    series->append( hftm.first, rate );
             }

             QChart *chart = new QChart();
             chart->legend()->hide();
             chart->addSeries(series);
             chart->createDefaultAxes();

             //more space for the curves
             chart->layout()->setContentsMargins(2, 2, 2, 2);
             chart->setMargins(QMargins(2, 2, 2, 2));

             QChartView *chartView = new QChartView(chart);
             chartView->setRenderHint(QPainter::Antialiasing);

             chartView->setSizePolicy( QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred );
             gridLayout->setRowStretch( i+1, 1 );
             gridLayout->setColumnStretch( j+1, 1 );
             gridLayout->addWidget( chartView, i+1, j+1 );
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
