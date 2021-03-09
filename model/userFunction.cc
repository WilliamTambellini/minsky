/*
  @copyright Steve Keen 2020
  @author Russell Standish
  This file is part of Minsky.

  Minsky is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Minsky is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Minsky.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "userFunction.h"
#include <assert.h>           // for assert
#include <ctype.h>            // for isalnum, isalpha
#include <stddef.h>           // for size_t
#include <cmath>              // for isfinite, isinf, isnan, nan
#include <exprtk/exprtk.hpp>  // for expression_node<>::node_type, expressio...
#include <map>                // for operator!=, _Rb_tree_iterator, _Rb_tree...
#include <regex>              // for regex_match, match_results<>::_Base_type
#include <set>                // for set
#include "classdesc.h"        // for Exclude
#include "minsky.h"           // for minsky, Minsky
#include "variableValue.h"    // for VariableValues, VariableValue, Variable...

#include "minsky_epilogue.h"

namespace minsky
{
  int UserFunction::nextId=0;
  
  namespace {
    // resolve overloads
    inline double isfinite(double x) {return std::isfinite(x);}
    inline double isinf(double x) {return std::isinf(x);}
    inline double isnan(double x) {return std::isnan(x);}

    void addTimeVariables(exprtk::symbol_table<double>& table)
    {
      // Vensim names for these variables.
      // TODO replace these by xmile names, and add user function to provide aliases for Vensim, and add the ability to resolve var names to argumentless functions
      table.add_variable("time",minsky().t);
      table.add_variable("timeStep",minsky().stepMax);
      table.add_variable("initialTime",minsky().t0);
      table.add_variable("finalTime",minsky().tmax);

      table.add_function("isfinite",isfinite);
      table.add_function("isinf",isinf);
      table.add_function("isnan",isnan);
    }
  
    exprtk::parser<double> parser;

    struct ExprTkCallableFunction: public exprtk::ivararg_function<double>
    {
      std::weak_ptr<CallableFunction> f;
      ExprTkCallableFunction(const std::weak_ptr<CallableFunction>& f): f(f) {}
      double operator()(const std::vector<double>& x) {
        if (auto sf=f.lock())
          return (*sf)(x);
        return nan("");
      }
    };
  }

  struct UserFunction::Impl
  {
    exprtk::symbol_table<double> symbols;
    exprtk::expression<double> compiledExpression;
    std::vector<ExprTkCallableFunction> functions;
  };
  

  template <> void Operation<OperationType::userFunction>::iconDraw(cairo_t*) const
  {assert(false);}

  UserFunction::UserFunction(const string& name, const string& expression): impl(make_shared<Impl>()), argNames{"x","y"}, expression(expression)  {
    UserFunction::description(name);
  }

  vector<string> UserFunction::symbolNames() const
  {
    std::set<std::string> symbolNames;

    string word;
    bool inWord=false, inString=false, quoted=false;
    for (auto c: expression)
      {
      switch (c)
        {
        case '\'': if (!quoted) inString=!inString; break;
        case '\\': quoted=true; break;
        default: quoted=false; break; // I'm assuming that \" embeds a quote, but may not be true
        }
                                                     
      if (!inWord && !inString)
        inWord=isalpha(c);

      if (inWord)
        {
          if (isalnum(c) || c=='_' || c=='.')
            word+=c;
          else
            {
              // trailing '.' not allowed
              if (word.back()=='.') word.erase(word.end()-1);
              symbolNames.insert(word);
              word.clear();
              inWord=false;
            }
        }
      }
    if (!word.empty()) // we ended on an identifier
      symbolNames.insert(word);
    return {symbolNames.begin(), symbolNames.end()};
  }

  void UserFunction::compile()
  {
    impl->compiledExpression=exprtk::expression<double>();

    // build symbol table
    impl->symbols.clear();
    impl->functions.clear();
    addTimeVariables(impl->symbols);
    for (auto& i: symbolNames())
      {
        auto scopedName=VariableValue::valueIdFromScope(group.lock(),i);
        auto v=minsky().variableValues.find(scopedName);
        if (v!=minsky().variableValues.end())
          {
            impl->symbols.add_variable(i, (*v->second)[0]);
            continue;
          }
        auto f=minsky().userFunctions.find(scopedName);
        if (f!=minsky().userFunctions.end())
          {
            impl->functions.emplace_back(f->second);
            impl->symbols.add_function(i,impl->functions.back());
          }
      }

    // add arguments
    argVals.resize(argNames.size());
    for (size_t i=0; i<argNames.size(); ++i)
      impl->symbols.add_variable(argNames[i], argVals[i]);
    impl->compiledExpression.register_symbol_table(impl->symbols);
    
    if (!parser.compile(expression, impl->compiledExpression))
      {
        string errorInfo;
        for (size_t i=0; i<parser.error_count(); ++i)
          errorInfo+=parser.get_error(i).diagnostic+'\n';
        throw_error("Invalid function expression:\n"+errorInfo);
      }
  }
  
  double UserFunction::evaluate(double in1, double in2)
  {
    if (argVals.size()>0) argVals[0]=in1;
    if (argVals.size()>1) argVals[1]=in2;
    for (size_t i=2; i<argVals.size(); ++i) argVals[i]=0;
    return impl->compiledExpression.value();
  }

  double UserFunction::operator()(const std::vector<double>& p)
  {
    size_t i=0;
    for (; i<p.size() && i<argVals.size(); ++i) argVals[i]=p[i];
    for (; i<argVals.size(); ++i) argVals[i]=0;
    return impl->compiledExpression.value();
  }

  string UserFunction::description(const string& nm)
  {
    NamedOp::description(nm);
    static regex extractArgList(R"([^(]*\(([^)]*)\))");
    smatch match;
    string argList;
    if (regex_match(nm,match,extractArgList))
      argList=match[1];

    argNames.clear();
    auto end=argList.find(',');
    decltype(end) begin=0;
    for (; end!=string::npos; begin=end+1, end=argList.find(',',begin))
      argNames.push_back(argList.substr(begin,end-begin));
    argNames.push_back(argList.substr(begin));
    return nm;
  }

  string UserFunction::name() const
  {
    static regex extractName(R"(([^(]*).*)");
    smatch match;
    auto d=description();
    regex_match(d, match, extractName);
    assert (match.size()>1);
    return match[1];
  }    


  
}

