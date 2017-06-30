#include "gridcell.h"

GridCell::GridCell() :
    _dataIndex(0), _i(0), _j(0), _k(0)
{

}

GridCell::GridCell(int dataIndex, int i, int j, int k) :
    _dataIndex(dataIndex), _i(i), _j(j), _k(k)
{
}
