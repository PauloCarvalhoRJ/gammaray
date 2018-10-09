#ifndef MESHLOADER_H
#define MESHLOADER_H

#include <QObject>
#include <QFile>
#include "../geogrid.h"

/** This is an auxiliary class used in GeoGrid::loadMesh() to enable the progress dialog.
 * The file is read in a separate thread, so the progress bar updates.
 */
class MeshLoader : public QObject
{

	Q_OBJECT

public:
	explicit MeshLoader(QFile &file,
						std::vector< VertexRecordPtr > &vertexes,
						std::vector< CellDefRecordPtr > &cellDefs,
						uint &data_line_count,
						QObject *parent = 0);

	bool isFinished(){ return _finished; }

public slots:
	void doLoad( );
signals:
	void progress(int);

private:
	QFile &_file;
	std::vector< VertexRecordPtr > &m_vertexes;
	std::vector< CellDefRecordPtr > &m_cellDefs;
	uint &_data_line_count;
	bool _finished;
};

#endif // MESHLOADER_H
