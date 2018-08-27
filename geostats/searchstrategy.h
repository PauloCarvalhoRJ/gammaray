#ifndef SEARCHSTRATEGY_H
#define SEARCHSTRATEGY_H

#include "searchneighborhood.h"
#include <qglobal.h>
#include <memory>

/** This class models the sample search parameters common to geostatistics methods. */
class SearchStrategy
{
public:
    SearchStrategy( SearchNeighborhoodPtr nb, uint nb_samples );

    SearchNeighborhoodPtr m_searchNB;
	uint m_nb_samples;
};

typedef std::shared_ptr<SearchStrategy> SearchStrategyPtr;

#endif // SEARCHSTRATEGY_H
