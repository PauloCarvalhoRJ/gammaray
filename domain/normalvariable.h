#include "attribute.h"

#ifndef NORMALVARIABLE_H
#define NORMALVARIABLE_H

/**
 * NormalVariable represents an attribute that is a normal score transform from another Attribute.
 */
class NormalVariable : public Attribute
{
public:
    /** The constructor is the same as Attribute's and also requires a parent
     * Attribute that was normal score transformed into this Attribute.
     */
    NormalVariable( QString name, int index_in_file, Attribute* parentAttribute );

    // ProjectComponent interface
public:
    QIcon getIcon();
};

#endif // NORMALVARIABLE_H
