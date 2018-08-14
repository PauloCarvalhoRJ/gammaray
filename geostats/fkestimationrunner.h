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
	 * @param estimationCell Object containing info about the cell such as parent grid, indexes, etc.
	 * @param nst Number of the target structure in the variogram model (in the m_fkEstimaion object).
	 * @param nIllConditioned Its value is increased by the number of ill-conditioned kriging matrices encountered.
	 * @param nFailed Its value is increased by the number of kriging operations that failed (resulted in NaN or inifinity).
     */
	double fk(GridCell &estimationCell, int nst, int& nIllConditioned, int & nFailed );

};

#endif // FKESTIMATIONRUNNER_H
