// This was generated with asking Microsoft's Copilot: "boost spatial index example".

#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <vector>
#include <iostream>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

int main() {
    typedef bg::model::point<float, 2, bg::cs::cartesian> Point;
    typedef bg::model::box<Point> Box;
    typedef std::pair<Box, unsigned> Value;

    // Create some sample data
    std::vector<Value> values = {
        {Box(Point(0, 0), Point(1, 1)), 1},
        {Box(Point(1, 1), Point(2, 2)), 2},
        {Box(Point(2, 2), Point(3, 3)), 3}
    };

    // Create the R-tree
    bgi::rtree<Value, bgi::quadratic<16>> rtree(values.begin(), values.end());

    // Query the R-tree
    Box query_box(Point(0.5, 0.5), Point(2.5, 2.5));
    std::vector<Value> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

    // Output the results
    for (const auto& value : result_s) {
        std::cout << "Box's max corner's X: " << value.first.max_corner().get<0>() << ", Value: " << value.second << std::endl;
    }

    return 0;
}
