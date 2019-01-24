#include "entropycyclicityanalysisdialog.h"
#include "ui_entropycyclicityanalysisdialog.h"

#include "domain/faciestransitionmatrix.h"

#include <QMessageBox>
#include <QStringBuilder>

EntropyCyclicityAnalysisDialog::EntropyCyclicityAnalysisDialog(FaciesTransitionMatrix* ftm, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EntropyCyclicityAnalysisDialog),
    m_faciesTransitionMatrix( ftm )
{
    ui->setupUi(this);

    setWindowTitle("Entropy Cyclicity Analysis");

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

EntropyCyclicityAnalysisDialog::~EntropyCyclicityAnalysisDialog()
{
    delete ui;
}

void EntropyCyclicityAnalysisDialog::performCalculation()
{
    QString reportHTML = "<html><head>\n"
                    "<style type=\"text/css\">\n"
                    "table.countMatrix {\n"
                    "  font-family: \"Trebuchet MS\", Arial, Helvetica, sans-serif;\n"
                    "   border-collapse: collapse;\n"
                    "   border-width: 1px;\n"
                    "   border-color: gray;\n"
                    "   border-style: solid;\n"
                    "   width: 100%;margin-top: 0px;margin-bottom: 0px;color: black;\n"
                    " }\n"
                    "table.countMatrix td {\n"
                    "   padding: 8px;\n"
                    " }\n"
                    "</style>\n"
                    "</head><body>";



    //The % operator is a faster string opertation with the internal QStringBuilder class.

    reportHTML = reportHTML % "<H2>a) Transition count matrix (F):</H2>";
    reportHTML = reportHTML % "<table class=\"countMatrix\">";
    {
        //column headers
        reportHTML = reportHTML % "<tr>";
        reportHTML = reportHTML % "<td>&nbsp;</td>";
        for( int j = 0; j < m_faciesTransitionMatrix->getColumnCount(); ++j ){
            reportHTML = reportHTML % "<td bgcolor='" % m_faciesTransitionMatrix->getColorOfCategoryInColumnHeader( j ).name(QColor::HexRgb) % "'>";
            reportHTML = reportHTML % m_faciesTransitionMatrix->getColumnHeader( j );
            reportHTML = reportHTML % "</td>";
        }
        reportHTML = reportHTML % "<td>S<sub>Ri</sub></td>";
        reportHTML = reportHTML % "<td>T - S<sub>Ri</sub></td>";
        reportHTML = reportHTML % "</tr>";
        //row headers and values
        for( int i = 0; i < m_faciesTransitionMatrix->getRowCount(); ++i ){
            reportHTML = reportHTML % "<tr>";
            reportHTML = reportHTML % "<td bgcolor='" % m_faciesTransitionMatrix->getColorOfCategoryInRowHeader( i ).name(QColor::HexRgb) % "'>"
                                                      % m_faciesTransitionMatrix->getRowHeader(i) % "</td>";
            for( int j = 0; j < m_faciesTransitionMatrix->getColumnCount(); ++j ){
                reportHTML = reportHTML % "<td>";
                reportHTML = reportHTML % QString::number( m_faciesTransitionMatrix->getValue( i, j ) );
                reportHTML = reportHTML % "</td>";
            }
            reportHTML = reportHTML % "<td>" % QString::number( m_faciesTransitionMatrix->getSumOfRow( i ) ) % "</td>";
            reportHTML = reportHTML % "<td>" % QString::number( m_faciesTransitionMatrix->getTotalMinusSumOfRow( i ) ) % "</td>";
            reportHTML = reportHTML % "</tr>";
        }
        //last row with sums per column
        reportHTML = reportHTML % "<tr>";
        reportHTML = reportHTML % "<td>S<sub>Cj</sub></td>";
        for( int j = 0; j < m_faciesTransitionMatrix->getColumnCount(); ++j ){
            reportHTML = reportHTML % "<td>";
            reportHTML = reportHTML % QString::number( m_faciesTransitionMatrix->getSumOfColumn( j ) );
            reportHTML = reportHTML % "</td>";
        }
        reportHTML = reportHTML % "<td colspan='2'>Total = " % QString::number( m_faciesTransitionMatrix->getTotal() ) % "</td>";
        reportHTML = reportHTML % "</tr>";
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "<H2>b) Upward transition probability matrix (P):</H2>";
    reportHTML = reportHTML % "<table class=\"countMatrix\">";
    {
        //column headers
        reportHTML = reportHTML % "<tr>";
        reportHTML = reportHTML % "<td>&nbsp;</td>";
        for( int j = 0; j < m_faciesTransitionMatrix->getColumnCount(); ++j ){
            reportHTML = reportHTML % "<td bgcolor='" % m_faciesTransitionMatrix->getColorOfCategoryInColumnHeader( j ).name(QColor::HexRgb) % "'>";
            reportHTML = reportHTML % m_faciesTransitionMatrix->getColumnHeader( j );
            reportHTML = reportHTML % "</td>";
        }
        reportHTML = reportHTML % "</tr>";
        //row headers and values
        for( int i = 0; i < m_faciesTransitionMatrix->getRowCount(); ++i ){
            reportHTML = reportHTML % "<tr>";
            reportHTML = reportHTML % "<td bgcolor='" % m_faciesTransitionMatrix->getColorOfCategoryInRowHeader( i ).name(QColor::HexRgb) % "'>"
                                                      % m_faciesTransitionMatrix->getRowHeader(i) % "</td>";
            for( int j = 0; j < m_faciesTransitionMatrix->getColumnCount(); ++j ){
                reportHTML = reportHTML % "<td>";
                reportHTML = reportHTML % QString::number( m_faciesTransitionMatrix->getUpwardTransitionProbability( i, j ) );
                reportHTML = reportHTML % "</td>";
            }
            reportHTML = reportHTML % "</tr>";
        }
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "<H2>c) Downward transition probability matrix (Q):</H2>";
    reportHTML = reportHTML % "<table class=\"countMatrix\">";
    {
        //column headers
        reportHTML = reportHTML % "<tr>";
        reportHTML = reportHTML % "<td>&nbsp;</td>";
        for( int j = 0; j < m_faciesTransitionMatrix->getColumnCount(); ++j ){
            reportHTML = reportHTML % "<td bgcolor='" % m_faciesTransitionMatrix->getColorOfCategoryInColumnHeader( j ).name(QColor::HexRgb) % "'>";
            reportHTML = reportHTML % m_faciesTransitionMatrix->getColumnHeader( j );
            reportHTML = reportHTML % "</td>";
        }
        reportHTML = reportHTML % "</tr>";
        //row headers and values
        for( int i = 0; i < m_faciesTransitionMatrix->getRowCount(); ++i ){
            reportHTML = reportHTML % "<tr>";
            reportHTML = reportHTML % "<td bgcolor='" % m_faciesTransitionMatrix->getColorOfCategoryInRowHeader( i ).name(QColor::HexRgb) % "'>"
                                                      % m_faciesTransitionMatrix->getRowHeader(i) % "</td>";
            for( int j = 0; j < m_faciesTransitionMatrix->getColumnCount(); ++j ){
                reportHTML = reportHTML % "<td>";
                reportHTML = reportHTML % QString::number( m_faciesTransitionMatrix->getDownwardTransitionProbability( j, i ) );
                reportHTML = reportHTML % "</td>";
            }
            reportHTML = reportHTML % "</tr>";
        }
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "<H2>d) Entropies matrix:</H2>";
    reportHTML = reportHTML % "<table class=\"countMatrix\">";
    {
        //column headers
        reportHTML = reportHTML % "<tr>";
        reportHTML = reportHTML % "<td>&nbsp;</td>";
        reportHTML = reportHTML % "<td>E<sub>Post</sub></td>";
        reportHTML = reportHTML % "<td>E<sub>Pre</sub></td>";
        reportHTML = reportHTML % "<td>E<sub>nPost</sub></td>";
        reportHTML = reportHTML % "<td>E<sub>nPre</sub></td>";
        reportHTML = reportHTML % "</tr>";
        //row headers and values
        for( int i = 0; i < m_faciesTransitionMatrix->getRowCount(); ++i ){
            reportHTML = reportHTML % "<tr>";
            reportHTML = reportHTML % "<td bgcolor='" % m_faciesTransitionMatrix->getColorOfCategoryInRowHeader( i ).name(QColor::HexRgb) % "'>"
                                                      % m_faciesTransitionMatrix->getRowHeader(i) % "</td>";
            reportHTML = reportHTML % "<td>";
            reportHTML = reportHTML % QString::number( m_faciesTransitionMatrix->getPostDepositionalEntropy( i, false ) );
            reportHTML = reportHTML % "</td>";
            reportHTML = reportHTML % "<td>";
            reportHTML = reportHTML % QString::number( m_faciesTransitionMatrix->getPreDepositionalEntropy( i, false ) );
            reportHTML = reportHTML % "</td>";
            reportHTML = reportHTML % "<td>";
            reportHTML = reportHTML % QString::number( m_faciesTransitionMatrix->getPostDepositionalEntropy( i, true ) );
            reportHTML = reportHTML % "</td>";
            reportHTML = reportHTML % "<td>";
            reportHTML = reportHTML % QString::number( m_faciesTransitionMatrix->getPreDepositionalEntropy( i, true ) );
            reportHTML = reportHTML % "</td>";
            reportHTML = reportHTML % "</tr>";
        }
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "</body></html>";

    ui->txtEdCalcResult->setHtml( reportHTML );
}
