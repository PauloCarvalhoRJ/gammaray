#include "meshloader.h"



MeshLoader::MeshLoader(QFile & file, std::vector<VertexRecordPtr> & vertexes,
					   std::vector<CellDefRecordPtr> & cellDefs,
					   uint & data_line_count, QObject * parent) :
	QObject(parent),
	_file(file),
	m_vertexes( vertexes ),
	m_cellDefs( cellDefs ),
	_data_line_count(data_line_count),
	_finished(false)
{

}

void MeshLoader::doLoad()
{

}
