#ifndef EQUALIZERWIDGET_H
#define EQUALIZERWIDGET_H

#include <QWidget>

namespace Ui {
class EqualizerWidget;
}

class EqualizerSlider;
class QwtWheel;

/**
 * @brief The EqualizerWidget class encapsulates the entire graphic equalizer used to edit
 * spectra in the Image Jockey feature.
 */
class EqualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EqualizerWidget(QWidget *parent = 0);
    ~EqualizerWidget();

    double getFrequencyWindowBegin() const;
    double getFrequencyWindowEnd() const;

    /** This is necessary to enable this QWidget subclass to be usable in stylesheets. */
    void paintEvent(QPaintEvent *pe);

Q_SIGNALS:
    void frequencyWindowUpdated( double begin, double end );

public Q_SLOTS:
    /** Sets the absolute limits the selected frequency limits can vary within. */
    void setFrequencyLimits( double minFreqLim, double maxFreqLim );

private Q_SLOTS:
    /** The parameter is the index in the combo box, not the number of frequencies itself. */
    void setNumberOfCentralFrequencies(int indexInCombobox );
    void setFrequencyWindowBegin(double value);
    void setFrequencyWindowEnd(double value);

private:
    /** Re-sets the central frequencies of the sliders.  This is normally called after the
     * frequency window is changed. */
    void updateSlidersCentralFrequencies();

    Ui::EqualizerWidget *ui;

    /** The frequency limits the frequency window can vary within. */
    double m_minFrequencyLimit;
    double m_maxFrequencyLimit;

    /** The currently set frequency window. */
    double m_frequencyWindowBegin;
    double m_frequencyWindowEnd;

    /** List of equalizer sliders. */
    std::vector<EqualizerSlider*> m_sliders;

    /** Widget to control the start of frquency window. */
    QwtWheel* m_minFreqWheel;

    /** Widget to controls the end of frequency window. */
    QwtWheel* m_maxFreqWheel;
};

#endif // EQUALIZERWIDGET_H
