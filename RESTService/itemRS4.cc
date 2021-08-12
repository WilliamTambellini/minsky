/*
  @copyright Steve Keen 2019
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

//#include "bookmark.h"
//#include "bookmark.rcd"
//#include "cairoSurfaceImage.h"
//#include "cairoSurfaceImage.rcd"
//#include "cairoSurfaceImage.xcd"
//#include "callableFunction.h"
//#include "callableFunction.rcd"
//#include "callableFunction.xcd"
//#include "capiRenderer.h"
//#include "capiRenderer.cd"
//#include "capiRenderer.xcd"
//#include "CSVDialog.h"
//#include "CSVDialog.rcd"
//#include "CSVParser.rcd"
//#include "dataOp.h"
//#include "dataOp.rcd"
//#include "dynamicRavelCAPI.h"
//#include "dynamicRavelCAPI.rcd"
//#include "dynamicRavelCAPI.xcd"
//#include "dataSpecSchema.h"
//#include "dataSpecSchema.rcd"
//#include "dimension.rcd"
//#include "group.h"
//#include "group.rcd"
//#include "hypercube.rcd"
//#include "hypercube.xcd"
//#include "index.rcd"
//#include "index.xcd"
//#include "intOp.h"
//#include "intOp.rcd"
//#include "item.h"
//#include "item.rcd"
//#include "lasso.h"
//#include "noteBase.rcd"
//#include "operation.h"
//#include "operation.rcd"
//#include "operationType.rcd"
//#include "plotWidget.h"
//#include "plotWidget.rcd"
//#include "plot.xcd"
//#include "plot.rcd"
//#include "polyRESTProcessBase.rcd"
//#include "polyRESTProcessBase.xcd"
//#include "port.rcd"
//#include "ravelWrap.h"
//#include "ravelWrap.rcd"
//#include "ravelWrap.xcd"
//#include "ravelState.rcd"
//#include "ravelState.xcd"
//#include "renderNativeWindow.rcd"
//#include "renderNativeWindow.xcd"
//#include "selection.h"
//#include "selection.rcd"
//#include "slider.rcd"
//#include "SVGItem.rcd"
//#include "switchIcon.h"
//#include "switchIcon.rcd"
//#include "tensorInterface.rcd"
//#include "tensorInterface.xcd"
//#include "tensorVal.rcd"
//#include "tensorVal.xcd"
//#include "variable.h"
//#include "variable.rcd"
//#include "variableType.rcd"
//#include "variableValue.rcd"
//#include "userFunction.h"
//#include "userFunction.rcd"
//#include "wire.rcd"
//#include "xvector.rcd"
//
//#include "engNotation.rcd"
//#include "engNotation.xcd"
//#include "itemRS.h"
//#include "nobble.h"
//#include "units.h"
//#include "units.rcd"
//#include "minsky_epilogue.h"
//
//#include "itemTemplateInstantiations.h"

#include "itemRS.h"
#include "item.rcd"

#include "lasso.h"
#include "lasso.rcd"
#include "noteBase.rcd"
#include "operation.h"
#include "operation.rcd"
#include "operationType.rcd"
#include "port.rcd"
#include "polyRESTProcessBase.rcd"
#include "polyRESTProcessBase.xcd"
#include "wire.h"
#include "wire.rcd"
#include "units.rcd"
#include "nobble.h"

#include "itemTemplateInstantiations.h"
#include "minsky_epilogue.h"


namespace minsky
{
//  DEFOP(userFunction)
//  DEFOP(copy)
//  DEFOP(sqrt)
//  DEFOP(exp)
//  DEFOP(ln)
//  DEFOP(sin)
//  DEFOP(cos)
//  DEFOP(tan)
//  DEFOP(asin)
//  DEFOP(acos)
//  DEFOP(atan)
//  DEFOP(sinh)
//  DEFOP(cosh)
//  DEFOP(tanh)
//  DEFOP(abs)
//  DEFOP(floor)
//  DEFOP(frac)
  DEFOP(not_)
  DEFOP(Gamma)
  DEFOP(fact)
  DEFOP(sum)
  DEFOP(product)
  DEFOP(infimum)
  DEFOP(supremum)
  DEFOP(any)
  DEFOP(all)
  DEFOP(infIndex)
  DEFOP(supIndex)
  DEFOP(runningSum)
  DEFOP(runningProduct)
  DEFOP(difference)
  DEFOP(innerProduct)
  DEFOP(outerProduct)
  DEFOP(index)
  DEFOP(gather)
  DEFOP(numOps)
}
