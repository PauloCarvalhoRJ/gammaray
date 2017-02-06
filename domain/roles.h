#ifndef ROLES_H
#define ROLES_H

#include <QString>
#include <QIcon>
#include <QtGlobal> //For the uint type used throughout the rest of the code.

/** This namespace contains the role definitions.  Roles can be given by the user to certain objects in the project.
    Roles are not part of GSLib formats and thus are not necessary for the programs to work, but they leverage
    GammaRay automation and allows the software to minimize possibly visual confusion. */
namespace Roles {

/*! Roles given to uni- or bi- distribution file columns. */
enum class DistributionColumnRole : uint {
    UNDEFINED = 0, /*!< Column role not set. */
    VALUE,         /*!< Column that corresponds to variable values in linear scale. */
    LOGVALUE,      /*!< Column that corresponds to variable values in logarithmic scale. */
    PVALUE         /*!< Column that corresponds to probability/frequency p.d.f. */
};

QString getRoleText( DistributionColumnRole role );

QIcon getRoleIcon( DistributionColumnRole role );

} //namespace Roles



#endif // ROLES_H
