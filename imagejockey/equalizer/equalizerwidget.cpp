#include "equalizerwidget.h"
#include "ui_equalizerwidget.h"

#include "equalizerslider.h"

#include <qwt_wheel.h>

#include <QPainter>

EqualizerWidget::EqualizerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EqualizerWidget),
    m_frequencyWindowBegin(0.0d),
    m_frequencyWindowEnd(1000.0d)
{
    ui->setupUi(this);

    setStyleSheet("EqualizerWidget{border-radius: 10px; border: 2px inset gray;}");

    //the combo box of available numbers of central widgets
    ui->cmbNCenterFrequencies->addItem( QString::number(1) );
    ui->cmbNCenterFrequencies->addItem( QString::number(2) );
    ui->cmbNCenterFrequencies->addItem( QString::number(4) );
    ui->cmbNCenterFrequencies->addItem( QString::number(8) );
    ui->cmbNCenterFrequencies->addItem( QString::number(16) );
    ui->cmbNCenterFrequencies->addItem( QString::number(32) );
    ui->cmbNCenterFrequencies->setCurrentIndex(3); //set defult to 8 sliders
    setNumberOfCentralFrequencies(3);

    //the frequency window adjustment wheels
    m_maxFreqWheel = new QwtWheel();
    ui->frmWheelFMaxPlace->layout()->addWidget( m_maxFreqWheel );
    connect( m_maxFreqWheel, SIGNAL(valueChanged(double)), this, SLOT(setFrequencyWindowEnd(double)));
    m_minFreqWheel = new QwtWheel();
    ui->frmWheelFMinPlace->layout()->addWidget( m_minFreqWheel );
    connect( m_minFreqWheel, SIGNAL(valueChanged(double)), this, SLOT(setFrequencyWindowBegin(double)));
}

EqualizerWidget::~EqualizerWidget()
{
    delete ui;
}
double EqualizerWidget::getFrequencyWindowBegin() const
{
    return m_frequencyWindowBegin;
}

void EqualizerWidget::setFrequencyWindowBegin(double value)
{
    m_frequencyWindowBegin = value;
    updateSlidersCentralFrequencies();
    emit frequencyWindowUpdated( m_frequencyWindowBegin, m_frequencyWindowEnd );
}

double EqualizerWidget::getFrequencyWindowEnd() const
{
    return m_frequencyWindowEnd;
}

void EqualizerWidget::paintEvent(QPaintEvent */*pe*/)
{
    QStyleOption o;
    o.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive( QStyle::PE_Widget, &o, &p, this );
}

void EqualizerWidget::setFrequencyLimits(double minFreqLim, double maxFreqLim)
{
    m_maxFrequencyLimit = maxFreqLim;
    m_minFrequencyLimit = minFreqLim;
    m_maxFreqWheel->setMinimum( minFreqLim );
    m_maxFreqWheel->setMaximum( maxFreqLim );
    m_maxFreqWheel->setValue( maxFreqLim );
    m_maxFreqWheel->setSingleStep( (maxFreqLim-minFreqLim) / 360.0 ); //divide tha range per wheel degree
    m_minFreqWheel->setMinimum( minFreqLim );
    m_minFreqWheel->setMaximum( maxFreqLim );
    m_minFreqWheel->setValue( minFreqLim );
    m_minFreqWheel->setSingleStep( (maxFreqLim-minFreqLim) / 360.0 ); //divide tha range per wheel degree

    m_frequencyWindowEnd = maxFreqLim;
    m_frequencyWindowBegin = minFreqLim;
    updateSlidersCentralFrequencies();
}

void EqualizerWidget::setFrequencyWindowEnd(double value)
{
    m_frequencyWindowEnd = value;
    updateSlidersCentralFrequencies();
    emit frequencyWindowUpdated( m_frequencyWindowBegin, m_frequencyWindowEnd );
}

void EqualizerWidget::makeAdjustment(double centralFrequency, double dB)
{
    emit equalizerAdjusted( centralFrequency, dB );
}

void EqualizerWidget::setNumberOfCentralFrequencies( int /*indexInCombobox*/ )
{
    //get the selected number of central frequencies
    int nfreq = ui->cmbNCenterFrequencies->currentText().toInt();

    //if the number didn't actually change, do nothing.
    if( nfreq == (int)m_sliders.size() )
        return;

    //remove the current sliders from view (just deleting them works assuming their QWidget parents are set)
    std::vector<EqualizerSlider*>::iterator it = m_sliders.begin();
    for(; it != m_sliders.end(); ++it){
        delete *it;
    }

    //clears the list
    m_sliders.clear();

    //create the necessary sliders
    bool showScale = true;
    for( int i = 0; i < nfreq; ++i){
        EqualizerSlider* slider = new EqualizerSlider(1000.0, showScale);
        ui->frmSliders->layout()->addWidget( slider );
        m_sliders.push_back( slider );
        showScale = false; //its only necessary to show the scale for the first slider.
        //connect the sliders to this widget's slot to be notified of equalizer changes
        connect( slider, SIGNAL(adjustmentMade(double,double)), this, SLOT(makeAdjustment(double,double)) );
    }

    //set the sliders' central frequencies
    updateSlidersCentralFrequencies();
}

void EqualizerWidget::updateSlidersCentralFrequencies()
{
    //get the number of slides
    int nfreq = m_sliders.size();

    //set the central frequencies
    double delta = m_frequencyWindowEnd - m_frequencyWindowBegin;
    double step = delta / nfreq;
    for( int i = 0; i < nfreq; ++i){
        double centralFrequency = m_frequencyWindowBegin + step/2.0d + step * i;
        EqualizerSlider* slider = m_sliders[i];
        slider->setCentralFrequency( centralFrequency );
    }
}


