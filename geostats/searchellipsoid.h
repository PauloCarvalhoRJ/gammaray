#ifndef SEARCHELLIPSOID_H
#define SEARCHELLIPSOID_H

#include "searchneighborhood.h"

class SearchEllipsoid : public SearchNeighborhood
{
public:
	SearchEllipsoid(double hMax, double hMin, double hVert);
	double m_hMax, m_hMin, m_hVert;
};

#endif // SEARCHELLIPSOID_H
