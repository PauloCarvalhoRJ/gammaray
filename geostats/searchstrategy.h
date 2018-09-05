#ifndef SEARCHSTRATEGY_H
#define SEARCHSTRATEGY_H

#include "searchneighborhood.h"
#include <qglobal.h>
#include <memory>

/** This class models the sample search parameters common to geostatistics methods. */
class SearchStrategy
{
public:
	/** @param minDistanceBetweenSamples Set zero to not impose a minimum distance between samples. */
	SearchStrategy( SearchNeighborhoodPtr nb, uint nb_samples, double minDistanceBetweenSamples );

    SearchNeighborhoodPtr m_searchNB;
	uint m_nb_samples;
	double m_minDistanceBetweenSamples;
};

typedef std::shared_ptr<SearchStrategy> SearchStrategyPtr;

#endif // SEARCHSTRATEGY_H
