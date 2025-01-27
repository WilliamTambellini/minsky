/*
  @copyright Steve Keen 2012
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
#include "variableValue.h"
#include "flowCoef.h"
#include "str.h"
#include "minsky.h"
#include "minsky_epilogue.h"
#include <iomanip>
#include <error.h>

#ifdef WIN32
// std::quoted not supported (yet) on MXE
string quoted(const std::string& x)
{
  string r="\"";
  for (auto& i: x)
    if (i=='"')
      r+=R"(\")";
    else
      r+=i;
  return r+"\"";
}
#endif

using namespace ecolab;
using namespace std;
namespace minsky
{
  std::vector<double> ValueVector::stockVars(1);
  std::vector<double> ValueVector::flowVars(1);

  bool VariableValue::idxInRange() const
  {return m_type==undefined || idx()+size()<=
      (isFlowVar()?ValueVector::flowVars.size(): ValueVector::stockVars.size());}
    

  
  double& VariableValue::operator[](size_t i)
  {
    assert(i<size() && idxInRange());
    return *(&valRef()+i);
  }

  VariableValue& VariableValue::operator=(minsky::TensorVal const& x)
  {
    index(x.index());
    hypercube(x.hypercube());
    assert(idxInRange());
    memcpy(&valRef(), x.begin(), x.size()*sizeof(x[0]));
    return *this;
  }

  VariableValue& VariableValue::operator=(const ITensor& x)
  {
    index(x.index());
    hypercube(x.hypercube());
    assert(idxInRange());
    for (size_t i=0; i<x.size(); ++i)
      (*this)[i]=x[i];
    return *this;
  }
 
  VariableValue& VariableValue::allocValue()                                        
  {    
    switch (m_type)
      {
      case undefined:
        m_idx=-1;
        break;
      case flow:
      case tempFlow:
      case constant:
      case parameter:
        m_idx=ValueVector::flowVars.size();
        ValueVector::flowVars.resize(ValueVector::flowVars.size()+size());
        break;
      case stock:
      case integral:
        m_idx=ValueVector::stockVars.size();
        ValueVector::stockVars.resize(ValueVector::stockVars.size()+size());
        break;
      default: break;
      }
    return *this;
  }

  const double& VariableValue::valRef() const                                     
  {
    switch (m_type)
      {
      case flow:
      case tempFlow:
      case constant:
      case parameter:
        assert(idxInRange());
         if (size_t(m_idx)<ValueVector::flowVars.size())
           return ValueVector::flowVars[m_idx];
         break;
      case stock:
      case integral:
        assert(idxInRange());
        if (size_t(m_idx)<ValueVector::stockVars.size())
          return ValueVector::stockVars[m_idx];
        break;
      default: break;
      }
    static double zero=0;
    return zero;
  }
  
  double& VariableValue::valRef()                                         
  {
    if (m_idx==-1)
      allocValue();
    switch (m_type)
      {
      case flow:
      case tempFlow:
      case constant:
      case parameter:
        assert(idxInRange());
        if (size_t(m_idx+size())<=ValueVector::flowVars.size())
          return ValueVector::flowVars[m_idx]; 
      case stock:
      case integral: 
        assert(idxInRange());
        if (size_t(m_idx+size())<=ValueVector::stockVars.size())
          return ValueVector::stockVars[m_idx]; 
        break;
      default: break;
      }
    throw error("invalid access of variable value reference: %s",name.c_str());
  }
  
  TensorVal VariableValue::initValue
  (const VariableValues& v, set<string>& visited) const
  {
    if (tensorInit.rank()>0)
      return tensorInit;
    
    FlowCoef fc(init);
    if (trimWS(fc.name).empty())
      return fc.coef;

    // special generator functions handled here
    auto p=fc.name.find('(');
    if (p!=string::npos)
      {
        string fn=fc.name.substr(0,p);
        // unpack args
        const char* x=fc.name.c_str()+p+1;
        char* e;
        vector<unsigned> dims;
        for (;;)
          {
            auto tmp=strtol(x,&e,10);
            if (tmp>0 && e>x && *e)
              {
                x=e+1;
                dims.push_back(tmp);
              }
            else
              break;
          }
        TensorVal r(dims);
        r.allocVal();

        if (fn=="iota")
          for (size_t i=0; i<r.size(); ++i)
            r[i]=i;
        else if (fn=="one")
          for (size_t i=0; i<r.size(); ++i)
            r[i]=1;
        else if (fn=="zero" || fn=="eye")
          {
            for (size_t i=0; i<r.size(); ++i)
              r[i]=0;
            if (fn=="eye")
              {
                // diagonal elements set to 1
                // find minimum dimension, and stride of diagonal elements
                size_t mind=r.size(), stride=1;
                for (auto i: dims)
                  mind=min(mind, size_t(i));
                for (size_t i=0; i<dims.size()-1; ++i)
                  stride*=(dims[i]+1);
                for (size_t i=0; i<mind; ++i)
                  r[stride*i]=1;
              }
          }
        else if (fn=="rand")
          {
            for (size_t i=0; i<r.size(); ++i)
              r[i]=double(rand())/RAND_MAX;
          }
        return r;
      }
        
    // resolve name
    auto valueId=VariableValue::valueId(m_scope.lock(), fc.name);
    if (visited.count(valueId))
      throw error("circular definition of initial value for %s",
                  fc.name.c_str());
    VariableValues::const_iterator vv=v.end();
    vv=v.find(valueId);
    if (vv==v.end())
      throw error("Unknown variable %s in initialisation of %s",fc.name.c_str(), name.c_str());

    visited.insert(valueId);
    return fc.coef*vv->second->initValue(v, visited);
  }

  void VariableValue::reset(const VariableValues& v)
  {
      if (m_idx<0) allocValue();
      // initialise variable only if its variable is not defined or it is a stock
      if (!isFlowVar() || !cminsky().definingVar(valueId()))
        {
          if (tensorInit.size())
            {
              // ensure dimensions are correct
              auto hc=tensorInit.hypercube();
              for (auto& xv: hc.xvectors)
                {
                  auto dim=cminsky().dimensions.find(xv.name);
                  if (dim!=cminsky().dimensions.end())
                    xv.dimension=dim->second;
                }
              tensorInit.hypercube(hc);
            }
          if (tensorInit.rank()>0)
            operator=(tensorInit);
          else
            operator=(initValue(v));
        }
      assert(idxInRange());
  }


  int VariableValue::scope(const std::string& name) 
  {
    smatch m;
    auto nm=utf_to_utf<char>(name);
    if (regex_search(nm, m, regex(R"((\d*)]?:.*)")))
      if (m.size()>1 && m[1].matched && !m[1].str().empty())
        {
          int r;
          sscanf(m[1].str().c_str(),"%d",&r);
          return r;
        }
      else
        return -1;
    else
      // no scope information is present
      throw error("scope requested for local variable");
  }

  GroupPtr VariableValue::scope(GroupPtr scope, const std::string& a_name)
  {
    auto name=utf_to_utf<char>(stripActive(utf_to_utf<char>(a_name)));
    if (name[0]==':' && scope)
      {
        // find maximum enclosing scope that has this same-named variable
        for (auto g=scope->group.lock(); g; g=g->group.lock())
          for (auto& i: g->items)
            if (auto* v=i->variableCast())
              {
                auto n=stripActive(v->name());
                if (n==name.substr(1)) // without ':' qualifier
                  {
                    scope=g;
                    goto break_outerloop;
                  }
              }
        scope.reset(); // global var
      break_outerloop: ;
      }
    return scope;
  }
  
  
  string VariableValue::valueIdFromScope(const GroupPtr& scope, const std::string& name)
  {
    if (name.empty() || !scope || !scope->group.lock())
      return VariableValue::valueId(-1,utf_to_utf<char>(name)); // retain previous global var id
    return VariableValue::valueId(size_t(scope.get()), utf_to_utf<char>(name));
}
  
  std::string VariableValue::uqName(const std::string& name)
  {
    string::size_type p=name.rfind(':');
    if (p==string::npos)
      return utf_to_utf<char>(name);
    return utf_to_utf<char>(name).substr(p+1);
  }
 
  string VariableValues::newName(const string& name) const
  {
    int i=1;
    string trialName;
    do
      trialName=utf_to_utf<char>(name)+to_string(i++);
    while (count(VariableValue::valueId(trialName)));
    return trialName;
  }

  void VariableValues::reset()
  {
    // reallocate all variables
    ValueVector::stockVars.clear();
    ValueVector::flowVars.clear();
    for (auto& v: *this) {
      v.second->reset_idx();  // Set idx of all flowvars and stockvars to -1 on reset. For ticket 1049		
      v.second->allocValue().reset(*this);
      assert(v.second->idxInRange());
    }
}

  bool VariableValues::validEntries() const
  {
    return all_of(begin(), end(), [](const value_type& v){return v.second->isValueId(v.first);});
//    for (auto& v: *this)
//      if (!v.second->isValueId(v.first))
//        return false;
//    return true;
  }


  EngNotation engExp(double v) 
  {
    EngNotation r;
    r.sciExp=(v!=0)? floor(log(fabs(v))/log(10)): 0;
    if (r.sciExp==3) // special case for dates
      r.engExp=0;
    else
      r.engExp=r.sciExp>=0? 3*(r.sciExp/3): 3*((r.sciExp+1)/3-1);
    return r;
  }

  string mantissa(double value, const EngNotation& e, int digits)
  {
    int width, decimal_places;
    digits=std::max(digits, 3);
    switch (e.sciExp-e.engExp)
      {
      case -3: width=digits+4; decimal_places=digits+1; break;
      case -2: width=digits+3; decimal_places=digits; break;
      case 0: case -1: width=digits+2; decimal_places=digits-1; break;
      case 1: width=digits+2; decimal_places=digits-2; break;
      case 2: case 3: width=digits+2; decimal_places=digits-3; break;
      default: return ""; // shouldn't be here...
      }
    char val[10];
    const char conv[]="%*.*f";
    sprintf(val,conv,width,decimal_places,value*pow(10,-e.engExp));
    return val;
  }
  
  string expMultiplier(int exp)
  {return exp!=0? "×10<sup>"+std::to_string(exp)+"</sup>": "";}


  void VariableValue::exportAsCSV(const string& filename, const string& comment) const
  {
    ofstream of(filename);
    if (!comment.empty())
      of<<R"(""")"<<comment<<R"(""")"<<endl;
               
    const auto& xv=hypercube().xvectors;
    ostringstream os;
    for (const auto& i: xv)
      {
        if (&i>&xv[0]) os<<",";
        os<<json(static_cast<const NamedDimension&>(i));
      }
    of<<quoted("RavelHypercube=["+os.str()+"]")<<endl;
    for (const auto& i: hypercube().xvectors)
      of<<"\""<<i.name<<"\",";
    of<<"value$\n";

    auto idxv=index();
    size_t i=0;
    for (auto d=begin(); d!=end(); ++i, ++d)
      if (isfinite(*d))
        {
          ssize_t idx=idxv.empty()? i: idxv[i];
          for (size_t j=0; j<rank(); ++j)
            {
              auto div=std::div(idx, ssize_t(hypercube().xvectors[j].size()));
              of << "\""<<str(hypercube().xvectors[j][div.rem], hypercube().xvectors[j].dimension.units) << "\",";
              idx=div.quot;
            }
          of << *d << endl;
        }
  }
}
