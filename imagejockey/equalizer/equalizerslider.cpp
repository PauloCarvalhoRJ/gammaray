#include "equalizerslider.h"
#include "ui_equalizerslider.h"

#include "util.h"

#include <qwt_slider.h>

EqualizerSlider::EqualizerSlider(double centralFrequency, bool showScale, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EqualizerSlider),
    m_centralFrequency( centralFrequency )
{
    ui->setupUi(this);

    //add the slider
    m_slider = new QwtSlider( Qt::Vertical );
    m_slider->setUpperBound( 100.0d );
    m_slider->setLowerBound( -100.0d );
    if( ! showScale )
        m_slider->setScalePosition( QwtSlider::ScalePosition::NoScale );
    ui->frmSliderPlace->layout()->addWidget( m_slider );

    //display the current center frequency
    ui->lblCenterFrequency->setText( Util::humanReadable( m_centralFrequency ) );
}

EqualizerSlider::~EqualizerSlider()
{
    delete ui;
}
double EqualizerSlider::centralFrequency() const
{
    return m_centralFrequency;
}

void EqualizerSlider::setCentralFrequency(double centralFrequency)
{
    m_centralFrequency = centralFrequency;
    ui->lblCenterFrequency->setText( Util::humanReadable( m_centralFrequency ) );
}

