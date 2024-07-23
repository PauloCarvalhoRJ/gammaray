// Copied from https://stackoverflow.com/a/10251861/2153955

#include <algorithm>
#include <vector>
#include <boost/numeric/ublas/storage.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>

namespace ublas = boost::numeric::ublas;

template <typename T, typename F = ublas::row_major>
ublas::matrix<T, F> makeMatrix(std::size_t m, std::size_t n, const std::vector<T>& v)
{
    if (m * n != v.size()) {
        ; // Handle this case
    }
    ublas::unbounded_array<T> storage(m * n);
    std::copy(v.begin(), v.end(), storage.begin());
    return ublas::matrix<T>(m, n, storage);
}

int main() {
    ;
    std::vector<double> vec{ 1, 2, 3, 4, 5, 6 };
    ublas::matrix<double> mm = makeMatrix(3, 2, vec);
    std::cout << mm << std::endl;
}