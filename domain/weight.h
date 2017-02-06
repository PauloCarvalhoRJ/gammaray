#include "attribute.h"

#ifndef WEIGHT_H
#define WEIGHT_H

/** The Weight class represents a declustering weight, a special type of Attribute
 * that, as the name implies, is a weight of another Attribute.
 * Declustering weights tipically exist only as variables (Attribute) in point set files,
 * since they do not make sense in regular data (CartesianGrid).
 */
class Weight : public Attribute
{
public:
    /** The constructor is the same as Attribute's and also requires a parent
     * Attribute the Weight refers to.
     */
    Weight( QString name, int index_in_file, Attribute* parentAttribute );


    // ProjectComponent interface
public:
    QIcon getIcon();
};

#endif // WEIGHT_H
