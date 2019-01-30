#include "entropycyclicityanalysisdialog.h"
#include "ui_entropycyclicityanalysisdialog.h"

#include "domain/faciestransitionmatrix.h"
#include "viewer3d/view3dcolortables.h"
#include "util.h"

#include <QMessageBox>
#include <QStringBuilder>
#include <vtkLookupTable.h>

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
    //////////////////////////////// render a color table ///////////////////
    reportHTML = reportHTML % "<table><tr><td>0</td>";
    double max = m_faciesTransitionMatrix->getValueMax();
    for( int i = 1; i <= 32; ++i ){
        double e = i / 32.0 * max ;
        reportHTML = reportHTML % "<td style='border: 0px; padding 0px;' bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW, 0.0, max ) % "'>&nbsp;</td>";
    }
    reportHTML = reportHTML % "<td>" % QString::number( max ) % " (count)</td></tr></table>";
    /////////////////////////////////////////////////////////////////
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
                double e = m_faciesTransitionMatrix->getValue( i, j );
                reportHTML = reportHTML % "<td bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW, 0.0, max ) % "'>";
                reportHTML = reportHTML % QString::number( e );
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
    //////////////////////////////// render a color table ///////////////////
    reportHTML = reportHTML % "<table><tr><td>0.0</td>";
    for( int i = 1; i <= 32; ++i ){
        double e = i / 32.0 ;
        reportHTML = reportHTML % "<td style='border: 0px; padding 0px;' bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW ) % "'>&nbsp;</td>";
    }
    reportHTML = reportHTML % "<td>1.0 (probabilities)</td></tr></table>";
    /////////////////////////////////////////////////////////////////
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
                double e = m_faciesTransitionMatrix->getUpwardTransitionProbability( i, j );
                reportHTML = reportHTML % "<td bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW ) % "'>";
                reportHTML = reportHTML % QString::number( e );
                reportHTML = reportHTML % "</td>";
            }
            reportHTML = reportHTML % "</tr>";
        }
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "<H2>c) Downward transition probability matrix (Q):</H2>";
    //////////////////////////////// render a color table ///////////////////
    reportHTML = reportHTML % "<table><tr><td>0.0</td>";
    for( int i = 1; i <= 32; ++i ){
        double e = i / 32.0 ;
        reportHTML = reportHTML % "<td style='border: 0px; padding 0px;' bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW ) % "'>&nbsp;</td>";
    }
    reportHTML = reportHTML % "<td>1.0 (probabilities)</td></tr></table>";
    /////////////////////////////////////////////////////////////////
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
                double e = m_faciesTransitionMatrix->getDownwardTransitionProbability( j, i );
                reportHTML = reportHTML % "<td bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW ) % "'>";
                reportHTML = reportHTML % QString::number( e );
                reportHTML = reportHTML % "</td>";
            }
            reportHTML = reportHTML % "</tr>";
        }
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "<H2>d) Entropies matrix:</H2>";
    //////////////////////////////// render a color table ///////////////////
    reportHTML = reportHTML % "<table><tr><td>0.0</td>";
    for( int i = 1; i <= 32; ++i ){
        double e = i / 32.0 ;
        reportHTML = reportHTML % "<td style='border: 0px; padding 0px;' bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW ) % "'>&nbsp;</td>";
    }
    reportHTML = reportHTML % "<td>1.0 (unitized entropy values)</td></tr></table>";
    /////////////////////////////////////////////////////////////////
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

            double e = m_faciesTransitionMatrix->getPostDepositionalEntropy( i, true );
            reportHTML = reportHTML % "<td bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW ) % "'>";
            reportHTML = reportHTML % QString::number( e );
            reportHTML = reportHTML % "</td>";

            e = m_faciesTransitionMatrix->getPreDepositionalEntropy( i, true );
            reportHTML = reportHTML % "<td bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW ) % "'>";
            reportHTML = reportHTML % QString::number( e );
            reportHTML = reportHTML % "</td>";

            reportHTML = reportHTML % "</tr>";
        }
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "<H2>e) Independent trail matrix (R):</H2>";
    //////////////////////////////// render a color table ///////////////////
    reportHTML = reportHTML % "<table><tr><td>0.0</td>";
    for( int i = 1; i <= 32; ++i ){
        double e = i / 32.0 ;
        reportHTML = reportHTML % "<td style='border: 0px; padding 0px;' bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW ) % "'>&nbsp;</td>";
    }
    reportHTML = reportHTML % "<td>1.0 (probabilities)</td></tr></table>";
    /////////////////////////////////////////////////////////////////
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
                double e = m_faciesTransitionMatrix->getIndependentTrail( i, j );
                reportHTML = reportHTML % "<td bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW ) % "'>";
                reportHTML = reportHTML % QString::number( e );
                reportHTML = reportHTML % "</td>";
            }
            reportHTML = reportHTML % "</tr>";
        }
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "<H2>f) Difference matrix (D):</H2>";
    double maxAbs = m_faciesTransitionMatrix->getMaxAbsDifference();
    //////////////////////////////// render a color table ///////////////////
    reportHTML = reportHTML % "<table><tr><td>" % QString::number(-maxAbs) % "</td>";
    for( int i = 1; i <= 32; ++i ){
        double e = i / 32.0 ;
        reportHTML = reportHTML % "<td style='border: 0px; padding 0px;' bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::SEISMIC ) % "'>&nbsp;</td>";
    }
    reportHTML = reportHTML % "<td>" % QString::number(maxAbs) % " (diff. probabilities)</td></tr></table>";
    /////////////////////////////////////////////////////////////////
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
                double e = m_faciesTransitionMatrix->getDifference( i, j );
                reportHTML = reportHTML % "<td bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::SEISMIC, -maxAbs, maxAbs ) % "'>";
                reportHTML = reportHTML % QString::number( e );
                reportHTML = reportHTML % "</td>";
            }
            reportHTML = reportHTML % "</tr>";
        }
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "<H2>g) Expected frequency matrix (E):</H2>";
    max = m_faciesTransitionMatrix->getMaxExpectedFrequency();
    //////////////////////////////// render a color table ///////////////////
    reportHTML = reportHTML % "<table><tr><td>" % QString::number(0.0) % "</td>";
    for( int i = 1; i <= 32; ++i ){
        double e = i / 32.0 ;
        reportHTML = reportHTML % "<td style='border: 0px; padding 0px;' bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW ) % "'>&nbsp;</td>";
    }
    reportHTML = reportHTML % "<td>" % QString::number(max) % " (frequency)</td></tr></table>";
    /////////////////////////////////////////////////////////////////
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
                double e = m_faciesTransitionMatrix->getExpectedFrequency( i, j );
                reportHTML = reportHTML % "<td bgcolor='" % Util::getHTMLColorFromValue( e, ColorTable::RAINBOW, 0.0, max ) % "'>";
                reportHTML = reportHTML % QString::number( e );
                reportHTML = reportHTML % "</td>";
            }
            reportHTML = reportHTML % "</tr>";
        }
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "<H2>h) Test of significance (E):</H2>";
    reportHTML = reportHTML % "<table class=\"countMatrix\">";
    {
        int rank = m_faciesTransitionMatrix->getRank();
        int n = m_faciesTransitionMatrix->getColumnCount();
        int degreesOfFreedom = n*n - (n+n);
        //column headers
        reportHTML = reportHTML % "<tr>";
        reportHTML = reportHTML % "<td><strong>Test of Equation</strong></td>";
        reportHTML = reportHTML % "<td><strong>Computed value of</strong></td>";
        reportHTML = reportHTML % "<td><strong>Limiting value at 0.5% significance level</strong></td>";
        reportHTML = reportHTML % "<td><strong>Degrees of freedom</strong></td>";
        reportHTML = reportHTML % "</tr>";
        reportHTML = reportHTML % "<tr>";
        reportHTML = reportHTML % "<td><center>Billingslay</center></td>";
        reportHTML = reportHTML % "<td><center>" % QString::number( m_faciesTransitionMatrix->getChiSquared() ) % "</center></td>";
        reportHTML = reportHTML % "<td><center>" % QString::number( Util::chiSquaredAreaToTheRight( 0.005, degreesOfFreedom, 0.001 ) ) % "</center></td>";
        reportHTML = reportHTML % "<td><center>" % QString::number( degreesOfFreedom ) % "</center></td>";
        reportHTML = reportHTML % "</tr>";
    }
    reportHTML = reportHTML % "</table><BR/><BR/>";


    reportHTML = reportHTML % "</body></html>";

    ui->txtEdCalcResult->setHtml( reportHTML );
}
