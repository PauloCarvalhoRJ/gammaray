#ifndef PSWIDGET_H
#define PSWIDGET_H

#include <QWidget>

namespace Ui {
class PSWidget;
}

class QLabelWithCrossHairs;

/**
 * @brief The PSWidget class is a widget to view the Postscript files, such as the ones produced by
 * GSLib plot programs.
 */
class PSWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PSWidget(QWidget *parent = 0);
    ~PSWidget();

    /**
     * @brief displayPS Loads and displays the given PostScript file.
     */
    void displayPS(const QString path_to_ps_file);

public slots:
    void onIncreasePlotResolution();
    void onDecreasePlotResolution();
    void onTakeSnapshot();
    void onSetPlotRes80dpi();
    void onSetPlotRes150dpi();
    void onSetPlotRes300dpi();
    void onSetPlotRes600dpi();
    void onChangeParameters();
    void onSavePlot();
    void onShowHideCrossHairs();

signals:
    void userWantsToChangeParameters();
    void userWantsToSavePlot();

private:
    Ui::PSWidget *ui;
    int _current_plot_dpi;
    QString _path_to_ps_file;
    QLabelWithCrossHairs* _lblImage;
};

#endif // PSWIDGET_H
