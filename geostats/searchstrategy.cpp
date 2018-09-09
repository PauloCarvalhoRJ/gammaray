#include "searchstrategy.h"
#include <utility>

SearchStrategy::SearchStrategy(SearchNeighborhoodPtr nb,
                               uint nb_samples,
                               double minDistanceBetweenSamples,
                               uint minNumberOfSamples ) :
    m_searchNB( nb ),
	m_nb_samples( nb_samples ),
    m_minDistanceBetweenSamples( minDistanceBetweenSamples ),
    m_minNumberOfSamples( minNumberOfSamples ) // 0 == not used
{
}
