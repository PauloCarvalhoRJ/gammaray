#ifndef INVALIDMETHODEXCEPTION_H
#define INVALIDMETHODEXCEPTION_H

#include <QException>

/**
 * @brief The InvalidMethodException class is an exception thrown when calling a method that
 * has been implemented only because a superclass has made it abstract (pure virtual).
 */
class InvalidMethodException : public QException
{
public:
    explicit InvalidMethodException(){}
    void raise() const { throw *this; }
    InvalidMethodException *clone() const { return new InvalidMethodException(*this); }
};

#endif // INVALIDMETHODEXCEPTION_H
