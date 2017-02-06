#ifndef SCRIPTING_H
#define SCRIPTING_H

//FIXME: migrate to a 64-bit tool set since exprtk.hpp is too large resulting in
//       "too many section" excpetion with MinGW 32-bit.  Also it is necessary
//       to enable the -Wa,-mbig-obj for MinGW or /bigobj for MSVC.
//       Another option is to use another scripting engine such as Python, Lua or muParser.
/**
 * @brief The Scripting class encapsulates the scripting engine (currently trying the ExprTK header library).
 */
class Scripting
{
public:
    Scripting();
    void trig_function();
};

#endif // SCRIPTING_H
