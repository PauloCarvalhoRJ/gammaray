#include "geogrid.h"

GeoGrid::GeoGrid()
{
}

double GeoGrid::getDataSpatialLocation(uint line, CartesianCoord whichCoord)
{

}

bool GeoGrid::canHaveMetaData()
{

}

QString GeoGrid::getFileType()
{

}

void GeoGrid::updateMetaDataFile()
{

}

QIcon GeoGrid::getIcon()
{

}

void GeoGrid::save(QTextStream * txt_stream)
{

}

View3DViewData GeoGrid::build3DViewObjects(View3DWidget * widget3D)
{

}

void GeoGrid::getSpatialAndTopologicalCoordinates(int iRecord, double & x, double & y, double & z, int & i, int & j, int & k)
{

}

double GeoGrid::getNeighborValue(int iRecord, int iVar, int dI, int dJ, int dK)
{

}
