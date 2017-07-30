#ifndef IMAGEJOCKEYDIALOG_H
#define IMAGEJOCKEYDIALOG_H

#include <QDialog>

class CartesianGridSelector;
class VariableSelector;
class ImageJockeyGridPlot;
class Attribute;

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

private Q_SLOTS:
    void onUpdateGridPlot( Attribute *at );
};

#endif // IMAGEJOCKEYDIALOG_H
