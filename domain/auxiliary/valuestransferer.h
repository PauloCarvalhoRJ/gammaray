#ifndef VALUESTRANSFERER_H
#define VALUESTRANSFERER_H

#include <QString>

class DataFile;
class Attribute;
class GeoGrid;

/**
 * This class is used to perform a transfer of values between data sets.  The transfer is done in spatial domain (XYZ).
 * Support, accuracy and methods for this operation is heavily dependant on the implementations
 * in the several derived classes.
 */
class ValuesTransferer
{
public:
    ValuesTransferer( const QString newAttributeName,
                      DataFile* dfDestination,
                      const Attribute* atOrigin );

    /** Performs the transfer. Returns false if the transfer fails for any reason. */
    bool transfer();

private:
    QString m_newAttributeName;
    DataFile* m_dfDestination;
    const Attribute* m_atOrigin;

    bool transferFromCGtoGG();
    bool transferFromCGtoPS();
    bool transferFromCGtoCG();
    bool transferFromCGtoSection();
    bool transferFromPStoCG();
};

#endif // VALUESTRANSFERER_H
