#ifndef EXTERNALPROGRAMEXCEPTION_H
#define EXTERNALPROGRAMEXCEPTION_H

#include <QException>

/**
 * @brief The ExternalProgramException class is an exception thrown when calling an external program
 * such as a GSLib program or the GhostScript parser.
 */
class ExternalProgramException : public QException
{
public:
    explicit ExternalProgramException(int error_code){ code = error_code; }
    void raise() const { throw *this; }
    ExternalProgramException *clone() const { return new ExternalProgramException(*this); }
    int code;
};


#endif // EXTERNALPROGRAMEXCEPTION_H
