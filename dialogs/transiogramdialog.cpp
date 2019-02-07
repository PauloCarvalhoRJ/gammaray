#include "transiogramdialog.h"
#include "ui_transiogramdialog.h"

#include "domain/faciestransitionmatrix.h"

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QMessageBox>

TransiogramDialog::TransiogramDialog(FaciesTransitionMatrix *ftm, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TransiogramDialog),
    m_faciesTransitionMatrix( ftm )
{
    ui->setupUi(this);

    setWindowTitle("Transiogram Dialog");

    //deletes dialog from memory upon user closing it
    this->setAttribute(Qt::WA_DeleteOnClose);

    m_faciesTransitionMatrix->readFromFS();

    if( m_faciesTransitionMatrix->isUsable() ){
        performCalculation();
    } else {
        QMessageBox::critical( this, "Error", "The matrix is not usable.  You need to associate it to a category definition or"
                                              " some facies names were not found in the associated category definition.\n"
                                              " Please refer to the program manual for forther details. ");
    }
}

TransiogramDialog::~TransiogramDialog()
{
    delete ui;
}

void TransiogramDialog::performCalculation()
{
    using namespace QtCharts;

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
