#include "meshloader.h"

#include <QFileInfo>
#include <QTextStream>

#include "util.h"
#include "../application.h"


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
	QTextStream in(&_file);
	long bytesReadSofar = 0;
	QStringList valuesAsString;

	bool isInVertexesSection = false;

	bool isInCellDefsSection = false;

	for (int i = 0; !in.atEnd(); ++i)
	{
	   //read file line by line
	   QString line = in.readLine();

	   //Clear the list with the tokenized data values as string
	   valuesAsString.clear();

	   //updates the progress
	   bytesReadSofar += line.size() + 1; //account for line break char (in Windows may have 2, though.)

	   if( ! ( i % 100 ) ){ //update progress for each 100 lines to not impact performance much
		   // allows tracking progress of a file up to about 400GB
		   emit progress( (int)(bytesReadSofar / 100) );
	   }

	   if( i == 0 || i == 1 ){} //first and second lines are ignored
	   else { //parse lines containing data (must be within the target interval)

		   if( !isInVertexesSection && !isInCellDefsSection &&
			   line.startsWith( "VERTEX LOCATIONS", Qt::CaseInsensitive ) ){
			   isInVertexesSection = true;
			   continue; //move on to the next file line
		   }

		   if( isInVertexesSection && !isInCellDefsSection &&
			   line.startsWith( "CELL VERTEX INDEXES", Qt::CaseInsensitive ) ){
			   isInVertexesSection = false;
			   isInCellDefsSection = true;
			   continue; //move on to the next file line
		   }

		   //read line from the GSLib data file
		   Util::fastSplit( line, valuesAsString );

		   if( isInVertexesSection ){
			   if( valuesAsString.size() != 3 ){
				   Application::instance()->logError( QString("MeshLoader::doLoad(): vertex coordinate count different from 3 in line ").append(QString::number(i)) );
				   m_vertexes.push_back( VertexRecordPtr( new VertexRecord{ 0.0, 0.0, 0.0 } ) );
			   } else {
				   bool ok[] = {true, true, true};
				   double x = valuesAsString[0].toDouble( &ok[0] );
				   double y = valuesAsString[1].toDouble( &ok[1] );
				   double z = valuesAsString[2].toDouble( &ok[2] );
				   if( !ok[0] || !ok[1] || !ok[2] )
					   Application::instance()->logError( QString("MeshLoader::doLoad(): error in vertex section of mesh file (line ").
														  append(QString::number(i)).append("): could not convert a value to double.") );
				   m_vertexes.push_back( VertexRecordPtr( new VertexRecord{ x, y, z } ) ) ;
			   }
		   } else if( isInCellDefsSection ) {
			   if( valuesAsString.size() != 8 ){
				   Application::instance()->logError( QString("MeshLoader::doLoad(): vertex id count in cell definition different from 8 in line ").append(QString::number(i)) );
				   m_cellDefs.push_back( CellDefRecordPtr( new CellDefRecord{ {0, 0, 0, 0, 0, 0, 0, 0} } ) );
			   } else {
				   bool ok[] = {true, true, true, true, true, true, true, true};
				   int vId[8];
				   vId[0] = valuesAsString[0].toInt( &ok[0] );
				   vId[1] = valuesAsString[1].toInt( &ok[1] );
				   vId[2] = valuesAsString[2].toInt( &ok[2] );
				   vId[3] = valuesAsString[3].toInt( &ok[3] );
				   vId[4] = valuesAsString[4].toInt( &ok[4] );
				   vId[5] = valuesAsString[5].toInt( &ok[5] );
				   vId[6] = valuesAsString[6].toInt( &ok[6] );
				   vId[7] = valuesAsString[7].toInt( &ok[7] );
				   if( !ok[0] || !ok[1] || !ok[2] || !ok[3] || !ok[4] || !ok[5] || !ok[6] || !ok[7] )
					   Application::instance()->logError( QString("MeshLoader::doLoad(): error in cell definition section of mesh file (line ").
														  append(QString::number(i)).append("): could not convert a value to unsigned integer.") );
				   m_cellDefs.push_back( CellDefRecordPtr( new CellDefRecord{ { vId[0], vId[1], vId[2], vId[3], vId[4], vId[5], vId[6], vId[7] } } ) );
			   }
		   }

		   ++_data_line_count;
	   }
	}

	_finished = true;
}
