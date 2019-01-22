#include "faciestransitionmatrix.h"

FaciesTransitionMatrix::FaciesTransitionMatrix(QString path,
                                               CategoryDefinition* associatedCategoryDefinition) :
    File( path ),
    m_associatedCategoryDefinition( associatedCategoryDefinition )
{

}

QIcon FaciesTransitionMatrix::getIcon()
{
    return QIcon(":icons32/thrcdf32");
}

QString FaciesTransitionMatrix::getTypeName()
{
    return "FACIESTRANSITIONMATRIX";
}

void FaciesTransitionMatrix::save(QTextStream *txt_stream)
{
}

bool FaciesTransitionMatrix::canHaveMetaData()
{
    return true;
}

QString FaciesTransitionMatrix::getFileType()
{
    return getTypeName();
}

void FaciesTransitionMatrix::updateMetaDataFile()
{
}

void FaciesTransitionMatrix::writeToFS()
{
    throw InvalidMethodException();
}

void FaciesTransitionMatrix::readFromFS()
{
}

void FaciesTransitionMatrix::clearLoadedContents()
{
}

bool FaciesTransitionMatrix::isDataFile()
{
    return false;
}

bool FaciesTransitionMatrix::isDistribution()
{
    return false;
}
