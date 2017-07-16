#ifndef NDVESTIMATIONRUNNER_H
#define NDVESTIMATIONRUNNER_H

#include <QObject>

class Attribute;
class GridCell;
class NDVEstimation;

/** This is an auxiliary class used in NDVEstimation::run() to enable the progress dialog.
 * The estimation takes place in a separate thread, so the progress bar updates.
 */
class NDVEstimationRunner : public QObject
{

    Q_OBJECT

public:
    explicit NDVEstimationRunner(NDVEstimation* ndvEstimation, Attribute* at, QObject *parent = 0);

    bool isFinished(){ return _finished; }

    std::vector<double> getResults(){ return _results; }

signals:
    void progress(int);
    void setLabel(QString);

public slots:
    void doRun( );

private:
    bool _finished;
    Attribute* _at;
    NDVEstimation* _ndvEstimation;
    std::vector<double> _results;

    /** Estimate, by kriging, a single cell. */
    double krige(GridCell cell , double meanSK, bool hasNDV, double NDV, double variogramSill);
};

#endif // NDVESTIMATIONRUNNER_H
