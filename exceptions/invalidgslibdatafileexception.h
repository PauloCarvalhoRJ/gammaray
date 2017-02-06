#ifndef INVALIDGSLIBDATAFILEEXCEPTION_H
#define INVALIDGSLIBDATAFILEEXCEPTION_H

#include <QException>

/**
 * @brief The InvalidGSLibDataFileException class is an exception thrown when there is something
 * wrong with a GSLib data file, most likley a misformatted file.
 */
class InvalidGSLibDataFileException : public QException
{
public:
    explicit InvalidGSLibDataFileException(){}
    void raise() const { throw *this; }
    InvalidGSLibDataFileException *clone() const { return new InvalidGSLibDataFileException(*this); }
};

#endif // INVALIDGSLIBDATAFILEEXCEPTION_H
