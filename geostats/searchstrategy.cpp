#include "searchstrategy.h"
#include <utility>

SearchStrategy::SearchStrategy(SearchNeighborhood && nb , uint nb_samples) :
	m_searchNB( std::move(nb) ),
	m_nb_samples( nb_samples )
{
}
