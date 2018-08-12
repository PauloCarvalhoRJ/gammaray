#ifndef FKESTIMATIONRUNNER_H
#define FKESTIMATIONRUNNER_H

#include <QObject>

class Attribute;
class GridCell;
class FKEstimation;

/** This is an auxiliary class used in FKEstimation::run() to enable the progress dialog.
 * The processing takes place in a separate thread, so the progress bar updates.
 */
class FKEstimationRunner : public QObject
{

    Q_OBJECT

public:
    explicit FKEstimationRunner(FKEstimation* fkEstimation, QObject *parent = 0);

    bool isFinished(){ return m_finished; }

    std::vector<double> getResults(){ return m_results; }

signals:
    void progress(int);
    void setLabel(QString);

public slots:
    void doRun( );

private:
    bool m_finished;
    FKEstimation* m_fkEstimation;
    std::vector<double> m_results;

    /** Perform factorial kriging in a single cell in the output grid.
     * @param Object containing info about the cell such as parent grid, indexes, etc.
     * @param nIllConditioned its value is increased by the number of ill-conditioned kriging matrices encountered.
     * @param nFailed its value is increased by the number of kriging operations that failed (resulted in NaN or inifinity).
     */
    double fk(GridCell &cell, int& nIllConditioned, int & nFailed );

};

#endif // FKESTIMATIONRUNNER_H
