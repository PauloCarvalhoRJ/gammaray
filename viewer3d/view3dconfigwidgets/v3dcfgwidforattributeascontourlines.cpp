#include "v3dcfgwidforattributeascontourlines.h"
#include "ui_v3dcfgwidforattributeascontourlines.h"

#include "domain/application.h"

#include <QColorDialog>

#include <vtkActor.h>
#include <vtkContourFilter.h>
#include <vtkProperty.h>

V3DCfgWidForAttributeAsContourLines::V3DCfgWidForAttributeAsContourLines(
        CartesianGrid */*cartesianGrid*/,
        Attribute */*attribute*/,
        View3DViewData viewObjects,
        QWidget *parent) :
    View3DConfigWidget(parent),
    ui(new Ui::V3DCfgWidForAttributeAsContourLines),
    _viewObjects( viewObjects ),
    m_contourLinesColor( Qt::GlobalColor::white )
{
    ui->setupUi(this);

    /////initialize widgets with whatever values were used to configure the vtkActor and vtkCountourFilter.

    vtkSmartPointer<vtkContourFilter> contourFilter = _viewObjects.contourFilter;
    vtkActor* actor = dynamic_cast<vtkActor*>( _viewObjects.actor.Get() );
    if( ! actor ){
        Application::instance()->logError("V3DCfgWidForAttributeAsContourLines::V3DCfgWidForAttributeAsContourLines(): "
                                          " the visual representation of the 2D grid is not a vtkActor. "
                                          " Check the View3DBuilders::buildForAttribute2DContourLines()"
                                          " method for how it is being made.");
        return;
    }

    //init number of countour lines
    ui->spinNumberOfLines->blockSignals( true );
    int nLines = contourFilter->GetNumberOfContours();
    ui->spinNumberOfLines->setValue( nLines );
    ui->spinNumberOfLines->blockSignals( false );

    //init minimum value
    ui->spinContourMin->blockSignals( true );
    double min = contourFilter->GetValue( 0 );
    ui->spinContourMin->setValue( min );
    ui->spinContourMin->blockSignals( false );

    //init maximum value
    ui->spinContourMax->blockSignals( true );
    double max = contourFilter->GetValue( nLines-1 );
    ui->spinContourMax->setValue( max );
    ui->spinContourMax->blockSignals( false );

    //init contour lines color
    double r, g, b;
    actor->GetProperty()->GetColor( r, g, b );
    m_contourLinesColor.setRgbF( r, g, b );

    //init contour lines width
    ui->spinContourLinesWidth->blockSignals( true );
    int lineWidth = actor->GetProperty()->GetLineWidth();
    ui->spinContourLinesWidth->setValue( lineWidth );
    ui->spinContourLinesWidth->blockSignals( false );

    ////////////////////////////////////////////////////////////////////////

    updateGUI();
}

V3DCfgWidForAttributeAsContourLines::~V3DCfgWidForAttributeAsContourLines()
{
    delete ui;
}

void V3DCfgWidForAttributeAsContourLines::updateGUI()
{
    // Paint the label with the text color chosen by the user in a dialog.
    QString values = QString("%1, %2, %3").arg( m_contourLinesColor.red()).
                                           arg( m_contourLinesColor.green()).
                                           arg( m_contourLinesColor.blue());
    ui->lblColor->setStyleSheet("QLabel { background-color: rgb("+values+"); }");
}

void V3DCfgWidForAttributeAsContourLines::onUserMadeChanges()
{
    vtkSmartPointer<vtkContourFilter> contourFilter = _viewObjects.contourFilter;
    vtkActor* actor = dynamic_cast<vtkActor*>( _viewObjects.actor.Get() );
    if( ! actor ){
        Application::instance()->logError("V3DCfgWidForAttributeAsContourLines::onUserMadeChanges(): "
                                          " the visual representation of the 2D grid is not a vtkActor. "
                                          " Check the View3DBuilders::buildForAttribute2DContourLines()"
                                          " method for how it is being made.");
        return;
    }

    //set minimum value for contour computation
    double min = ui->spinContourMin->value();

    //set maximum value for contour computation
    double max = ui->spinContourMax->value();

    //set number of lines for contour computation
    int nLines = ui->spinNumberOfLines->value();

    //update the contour filter
    contourFilter->GenerateValues( nLines, min, max); // (numContours, rangeStart, rangeEnd)
    contourFilter->Update();

    //set the color of the contour lines
    actor->GetProperty()->SetColor( m_contourLinesColor.redF(),
                                    m_contourLinesColor.greenF(),
                                    m_contourLinesColor.blueF() );

    //set the width of the contour lines
    actor->GetProperty()->SetLineWidth( ui->spinContourLinesWidth->value() );

    //notify client code of 3D scene updates
    emit changed();
}

void V3DCfgWidForAttributeAsContourLines::onColorChoose()
{
    QColorDialog colorD;

    int result = colorD.exec(); //opens the dialog modally

    if( result == QDialog::Accepted ){
        m_contourLinesColor = colorD.selectedColor();
        updateGUI();
        onUserMadeChanges();
    }
}
