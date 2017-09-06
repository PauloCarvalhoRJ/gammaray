#ifndef EQUALIZERSLIDER_H
#define EQUALIZERSLIDER_H

#include <QWidget>

namespace Ui {
class EqualizerSlider;
}

class QwtSlider;

class EqualizerSlider : public QWidget
{
    Q_OBJECT

public:
    explicit EqualizerSlider(double centralFrequency, bool showScale = false, QWidget *parent = 0);
    ~EqualizerSlider();

    double centralFrequency() const;
    void setCentralFrequency(double centralFrequency);

Q_SIGNALS:
    /** Negative dB variation means attentuation.  Positive variations mean amplification. */
    void adjustmentMade( double centralFrequency, double delta_dB );

private Q_SLOTS:
    /** User slided the equalizer knob. */
    void setValue( double value );

private:
    Ui::EqualizerSlider *ui;

    QwtSlider* m_slider;
    double m_centralFrequency;
    double m_previous_dB;
};

#endif // EQUALIZERSLIDER_H
