#ifndef FKESTIMATIONRUNNER_H
#define FKESTIMATIONRUNNER_H

#include <QObject>

class Attribute;
class GridCell;
class FKEstimation;
class VariogramModel;

/** This is an auxiliary class used in FKEstimation::run() to enable the progress dialog.
 * The processing takes place in a separate thread, so the progress bar updates.
 */
class FKEstimationRunner : public QObject
{

    Q_OBJECT

public:
    explicit FKEstimationRunner(FKEstimation* fkEstimation, QObject *parent = 0);
	virtual ~FKEstimationRunner();

    bool isFinished(){ return m_finished; }

    std::vector<double> getFactor(){ return m_factor; }

    std::vector<double> getMeans(){ return m_means; }

	std::vector<uint> getNSamples(){ return m_nSamples; }

signals:
    void progress(int);
    void setLabel(QString);

public slots:
    void doRun( );

private:
    bool m_finished;
    FKEstimation* m_fkEstimation;
    std::vector<double> m_factor;
    std::vector<double> m_means;
	std::vector<uint> m_nSamples;
	VariogramModel* m_singleStructVModel;

	/** Perform factorial kriging in a single cell in the output grid according to the formulation at
	 * https://pubs.geoscienceworld.org/geophysics/article/82/2/G35/520853/data-analysis-of-potential-field-methods-using
	 * Data analysis of potential field methods using geostatistics - Shamsipour et al, 2017
	 *
	 * @param estimationCell Object containing info about the cell such as parent grid, indexes, etc.
	 * @param estimatedMean The value of the estimated mean computed during FK estimation.
	 * @param nSamples The number of samples used to inform the estimation.
	 * @param nFailed Its value is increased by the number of kriging operations that failed (resulted in
	 *                NaN or inifinity).
	 */
	double fk(GridCell &estimationCell, double& estimatedMean, uint& nSamples, int& nFailed );
};

#endif // FKESTIMATIONRUNNER_H
