#ifndef INTERSECTIONFINDER_H
#define INTERSECTIONFINDER_H

#include "geometry/vector3d.h"
#include <vector>
#include <vtkSmartPointer.h>

class CartesianGrid;
class SegmentSet;
class vtkOBBTree;

/** This class contains utilities to find intersections between spatial objects. */
class IntersectionFinder
{
public:
    IntersectionFinder();

    /** Initializes this intersection finder with a surface (a 2D Cartesian grid with Z as variable) to test
     * intersections against.
     * @variableIndex The non-GEOEAS (1st == 0) index of the variable containing the Z values.
     */
    void initWithSurface( CartesianGrid *cg, int variableIndexOfZ );

    /** Returns the itersections between the passed SegmentSet and the object used to initialize this finder.
     * May return an empty collection in case of no intersection.
     * NOTE: This method only considers the segments.  Gaps in the trajectory are not considered.  Hence, if
     * the input geometry passes through gaps in the segment set, for example, this method will return an empty list.
     */
    std::vector< Vector3D > getIntersections( const SegmentSet* segmentSet );

    /** Does the same as getIntersections( const SegmentSet* ) but connects possible gaps
     * in the segment set for the purpose of finding intersections.
     */
    std::vector< Vector3D > getIntersectionsConnectingGaps( const SegmentSet* segmentSet );

private:
    vtkSmartPointer<vtkOBBTree> m_tree;
};

#endif // INTERSECTIONFINDER_H
