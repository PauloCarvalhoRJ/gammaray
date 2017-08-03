#ifndef IMAGEJOCKEYDIALOG_H
#define IMAGEJOCKEYDIALOG_H

#include <QDialog>

class CartesianGridSelector;
class VariableSelector;
class ImageJockeyGridPlot;
class Attribute;
class QwtWheel;
class GRCompass;
class Spectrogram1DParameters;

namespace Ui {
class ImageJockeyDialog;
}

/**
 * @brief The Image Jockey user interface.  The Image Jockey allows one to perform filtering in frequancy domain
 * of grid data like a DJ does to enhance frequencies via an equalizer.
 */
class ImageJockeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageJockeyDialog(QWidget *parent = 0);
    ~ImageJockeyDialog();

private:
    Ui::ImageJockeyDialog *ui;

    /** Selector of the grid with the data in frequency domain (Fourier image). */
    CartesianGridSelector* m_cgSelector;

    /** Variable with the real part of the Fourier transform. */
    VariableSelector* m_atSelector;

    /** Widget that displays the grid. */
    ImageJockeyGridPlot* m_gridPlot;

    /** Controls the color scale dB max. value. */
    QwtWheel* m_wheelColorMax;

    /** Controls the color scale dB min. value. */
    QwtWheel* m_wheelColorMin;

    /** Controls the reference dB value (0dB). */
    QwtWheel* m_wheelColorDecibelReference;

    /** Controls the azimuth of the band to collect 1D spectrum data. */
    GRCompass* m_azimuthCompass;

    /** Controls the angle variation around the azimuth to collect 1D spectrum data. */
    QwtWheel* m_azimthTolControl;

    /** Controls the width of the band to collect 1D spectrum data. */
    QwtWheel* m_bandwidthControl;

    /** Controls the distance from the spectrogram center to collect 1D spectrum data. */
    QwtWheel* m_radiusControl;

    /** The set of parameters used to calculate a 1D spectrogram from a band on a 2D spectrogram. */
    Spectrogram1DParameters* m_spectrogram1Dparams;

private Q_SLOTS:
    void onUpdateGridPlot( Attribute *at );

};

#endif // IMAGEJOCKEYDIALOG_H
