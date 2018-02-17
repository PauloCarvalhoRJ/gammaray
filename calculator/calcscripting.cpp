#include "calcscripting.h"
#include <exprtk.hpp>

CalcScripting::CalcScripting()
{
}

void CalcScripting::trig_function()
{
   typedef exprtk::symbol_table<double> symbol_table_t;
   typedef exprtk::expression<double>     expression_t;
   typedef exprtk::parser<double>             parser_t;

   std::string expression_string = "clamp(-1.0,sin(2 * pi * x) + cos(x / 2 * pi),+1.0)";

   double x;

   symbol_table_t symbol_table;
   symbol_table.add_variable("x",x);
   symbol_table.add_constants();

   expression_t expression;
   expression.register_symbol_table(symbol_table);

   parser_t parser;
   parser.compile(expression_string,expression);

   for (x = double(-5); x <= double(+5); x += double(0.1))
   {
      double y = expression.value();
      printf("%19.15f\t%19.15f\n",x,y);
   }
}
