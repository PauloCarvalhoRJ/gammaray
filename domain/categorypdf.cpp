#include "categorypdf.h"

CategoryPDF::CategoryPDF(QString path) : IntDoublePairs ( path )
{
}

void CategoryPDF::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}
