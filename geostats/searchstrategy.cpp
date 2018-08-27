#include "searchstrategy.h"
#include <utility>

SearchStrategy::SearchStrategy(SearchNeighborhoodPtr nb , uint nb_samples) :
    m_searchNB( nb ),
	m_nb_samples( nb_samples )
{
}

