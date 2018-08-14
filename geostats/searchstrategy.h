#ifndef SEARCHSTRATEGY_H
#define SEARCHSTRATEGY_H

#include "searchneighborhood.h"
#include <qglobal.h>

/** This class models the sample search parameters common to geostatistics methods. */
class SearchStrategy
{
public:
	SearchStrategy( SearchNeighborhood&& nb, uint nb_samples );

	const SearchNeighborhood& m_searchNB;
	uint m_nb_samples;
};

#endif // SEARCHSTRATEGY_H
