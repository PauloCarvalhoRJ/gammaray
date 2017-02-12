#include "categorydefinition.h"

CategoryDefinition::CategoryDefinition(QString path) : IntIntQStringTriplets ( path )
{
}

void CategoryDefinition::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}
