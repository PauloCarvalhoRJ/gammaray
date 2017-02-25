#include "categorypdf.h"
#include "util.h"

CategoryPDF::CategoryPDF(QString path) : IntDoublePairs ( path )
{
}

QIcon CategoryPDF::getIcon()
{
    if( Util::getDisplayResolutionClass() == DisplayResolution::NORMAL_DPI)
        return QIcon(":icons/catpdf16");
    else
        return QIcon(":icons32/catpdf32");
}

void CategoryPDF::save(QTextStream *txt_stream)
{
    (*txt_stream) << this->getFileType() << ":" << this->getFileName() << '\n';
}
