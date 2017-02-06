#include "gslibparvarweight.h"

GSLibParVarWeight::GSLibParVarWeight(const QString name, const QString label, const QString description) :
    GSLibParType(name, label, description)
{
}

GSLibParVarWeight::GSLibParVarWeight(uint var_index, uint wgt_index) :
    GSLibParType("", "", ""),
    _var_index(var_index),
    _wgt_index(wgt_index)
{
}
