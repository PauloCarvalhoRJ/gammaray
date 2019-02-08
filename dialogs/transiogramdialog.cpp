#include "transiogramdialog.h"
#include "ui_transiogramdialog.h"

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QMimeData>

#include "domain/datafile.h"
#include "domain/application.h"
#include "domain/auxiliary/faciestransitionmatrixmaker.h"
#include "domain/segmentset.h"
#include "domain/project.h"
#include "domain/attribute.h"

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
    CategoryDefinition* theCD = nullptr;
    if( ! m_categoricalAttributes.empty() ){
        DataFile* parent = dynamic_cast<DataFile*>( m_categoricalAttributes.front()->getContainingFile() );
        theCD = parent->getCategoryDefinition( m_categoricalAttributes.front() );

        DataFile* myParent = dynamic_cast<DataFile*>( attribute->getContainingFile() );
        CategoryDefinition* myCD = myParent->getCategoryDefinition( attribute );

        if( theCD != myCD ){
            Application::instance()->logError( "TransiogramDialog::tryToAddAttribute(): all attributes must be associated to the same categorical definition.", true );
            return;
        }
    }

    m_categoricalAttributes.push_back( attribute );
    ui->lblAttributesCount->setText( QString::number( m_categoricalAttributes.size() ) + " attribute(s)");
}

void TransiogramDialog::performCalculation()
{
    using namespace QtCharts;
    double hInitial = ui->dblSpinHIni->value();
    double hFinal = ui->dblSpinHFin->value();
    int nSteps = ui->spinNSteps->value();
    double toleranceCoefficient = ui->dblSpinTolCoeff->value();

//    FaciesTransitionMatrix ftm("");
//    if( m_dataFile->getFileType() == "SEGMENTSET" ){
//        FaciesTransitionMatrixMaker<SegmentSet> ftmMaker( dynamic_cast<SegmentSet*>(m_dataFile), m_variableIndex );
//        ftmMaker.makeAlongTrajectory( h, tolerance );
//    } else {
//        Application::instance()->logError("TransiogramDialog::performCalculation(): Data files of type " +
//                                          m_dataFile->getFileType()+ " not currently supported.", true);
//    }

    for( int j = 0; j < 10; ++j )
        for( int i = 0; i < 10; ++i ){

             QLineSeries *series = new QLineSeries();
             series->append(0, 6);
             series->append(2, 4);
             series->append(3, 8);
             series->append(7, 4);
             series->append(10, 5);
             *series << QPointF(11, 1) << QPointF(13, 3) << QPointF(17, 6) << QPointF(18, 3) << QPointF(20, 2);

             QChart *chart = new QChart();
             chart->legend()->hide();
             chart->addSeries(series);
             chart->createDefaultAxes();
             chart->setTitle("toto");

             QChartView *chartView = new QChartView(chart);
             chartView->setRenderHint(QPainter::Antialiasing);

        }
}

void TransiogramDialog::onResetAttributesList()
{
    m_categoricalAttributes.clear();
    ui->lblAttributesCount->setText("0 attribute(s)");
}
