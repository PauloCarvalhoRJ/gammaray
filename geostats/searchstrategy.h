#ifndef SEARCHSTRATEGY_H
#define SEARCHSTRATEGY_H

#include "searchneighborhood.h"

class SearchNeighborhood;

/** This class models the sample search parameters common to geostatistics methods. */
class SearchStrategy
{
public:
	SearchStrategy( SearchNeighborhood&& nb, uint nb_samples );
	SearchNeighborhood&& m_searchNB;
	uint m_nb_samples;
};

#endif // SEARCHSTRATEGY_H
