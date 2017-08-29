/*
  @copyright Steve Keen 2013
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

#include "cairoItems.h"
#include "minskyTCL.h"
#include "minskyTCLObj.h"
#include "init.h"
#include <ecolab.h>
#include <ecolab_epilogue.h>
#ifdef CAIRO_HAS_XLIB_SURFACE
#include <cairo/cairo-xlib.h>
#endif
// CYGWIN has problems with WIN32_SURFACE
#define USE_WIN32_SURFACE defined(CAIRO_HAS_WIN32_SURFACE) && !defined(__CYGWIN__)
#if USE_WIN32_SURFACE
#include <cairo/cairo-win32.h>
// undocumented internal function for extracting the HDC from a Drawable
extern "C" HDC TkWinGetDrawableDC(Display*, Drawable, void*);
extern "C" HDC TkWinReleaseDrawableDC(Drawable, HDC, void*);
#endif

#if defined(MAC_OSX_TK)
#include <Carbon/Carbon.h>
#include <cairo/cairo-quartz.h>
#include "getContext.h"
#endif

#include <unistd.h>

#ifdef _WIN32
#undef Realloc
#include <windows.h>
#endif

namespace minsky
{
  namespace
  {
    Minsky* l_minsky=NULL;
  }

  Minsky& minsky()
  {
    static MinskyTCL s_minsky;
    if (l_minsky)
      return *l_minsky;
    else
      return s_minsky;
  }

  LocalMinsky::LocalMinsky(Minsky& minsky) {l_minsky=&minsky;}
  LocalMinsky::~LocalMinsky() {l_minsky=NULL;}

  cmd_data* getCommandData(const string& name)
  {
    Tcl_CmdInfo info;
    if (Tcl_GetCommandInfo(interp(),name.c_str(),&info))
      {
        if (info.isNativeObjectProc)
          return (cmd_data*)info.objClientData;
        else
          return (cmd_data*)info.clientData;
      }
    return nullptr;
  }

  int deleteTclItem(ClientData cd, Tcl_Interp *interp,
                    int argc, const char **argv)
  {
    assert( strcmp(argv[0]+strlen(argv[0])-strlen(".delete"),
                   ".delete")==0);
    std::string s(argv[0]);
    ecolab::TCL_obj_deregister(s.substr(0,s.length()-strlen(".delete")));
    delete (Item*)cd;
    return TCL_OK; 
  }

  
#if 0
  // useful structure for for figuring what commands are being called.
  struct CmdHist: public map<string,unsigned>
  {
    ~CmdHist() {
      for (auto& i: *this)
        cout << i.first<<" "<<i.second<<endl;
    }
  } cmdHist;
#endif

  // Add any additional post TCL_obj processing commands here
  void setTCL_objAttributes()
  {
    // setting this helps a lot in avoiding unnecessary callbacks, and
    // also avoiding annoying "do you want to save the model message
    // when closing Minsky
    if (auto t=getCommandData("minsky.wire.coords"))
      t->is_setterGetter=true;
    if (auto t=getCommandData("minsky.var.name"))
      t->is_setterGetter=true;
    if (auto t=getCommandData("minsky.var.init"))
      t->is_setterGetter=true;
    if (auto t=getCommandData("minsky.var.value"))
      t->is_setterGetter=true;
    if (auto t=getCommandData("minsky.integral.description"))
      t->is_setterGetter=true;
    if (auto t=getCommandData("minsky.resetEdited"))
      t->is_const=true;
    if (auto t=getCommandData("minsky.initGroupList"))
      t->is_const=true;
    if (auto t=getCommandData("minsky.godley.mouseFocus"))
      t->is_const=true;
    if (auto t=getCommandData("minsky.godley.table.setDEmode"))
      t->is_const=true;
     if (auto t=getCommandData("minsky.resetNotNeeded"))
      t->is_const=true;
 }


  tclvar TCL_obj_lib("ecolab_library",ECOLAB_LIB);
  int TCL_obj_minsky=
    (
     TCL_obj_init(minsky()),
     ::TCL_obj(minskyTCL_obj(),"minsky",static_cast<MinskyTCL&>(minsky())),
     setTCL_objAttributes(),
     1
     );

  void MinskyTCL::getValue(const std::string& valueId)
  {
    auto value=variableValues.find(valueId);
    if (value!=variableValues.end())
      TCL_obj(minskyTCL_obj(),"minsky.value",value->second);
    else
      TCL_obj_deregister("minsky.value");
  }
  
    void MinskyTCL::putClipboard(const string& s) const
    {
#ifdef MAC_OSX_TK
      int p[2];
      pipe(p);
      if (fork()==0)
        {
          dup2(p[0],0);
          close(p[1]);
          execl("/usr/bin/pbcopy","pbcopy",nullptr);
        }
      else 
        {
          close(p[0]);
          write(p[1],s.c_str(),s.length());
          close(p[1]);
        }
      int status;
      wait(&status);
#elif defined(_WIN32)
      OpenClipboard(nullptr);
      EmptyClipboard();
      HGLOBAL h=GlobalAlloc(GMEM_MOVEABLE, s.length()+1);
      LPTSTR hh=static_cast<LPTSTR>(GlobalLock(h));
      if (hh)
        {
          strcpy(hh,s.c_str());
          GlobalUnlock(h);
          if (SetClipboardData(CF_TEXT, h)==nullptr)
            GlobalFree(h);
        }
      CloseClipboard();
#else
      Tk_Window mainWin=Tk_MainWindow(interp());
      Tk_ClipboardClear(interp(), mainWin);
      Atom utf8string=Tk_InternAtom(mainWin,"UTF8_STRING");
      Tk_ClipboardAppend(interp(), mainWin, utf8string, utf8string, 
                         const_cast<char*>(s.c_str()));
#endif
    }

    string MinskyTCL::getClipboard() const
    {
#ifdef MAC_OSX_TK
      int p[2];
      pipe(p);
      string r;
      if (fork()==0)
        {
          dup2(p[1],1);
          close(p[0]);
          execl("/usr/bin/pbpaste","pbpaste",nullptr);
        }
      else 
        {
          close(p[1]);
          char c;
          while (read(p[0],&c,1)>0)
            r+=c;
          close(p[0]);
        }
      int status;
      wait(&status);
      return r;
#elif defined(_WIN32)
      string r;
      OpenClipboard(nullptr);
      if (HANDLE h=GetClipboardData(CF_TEXT))
        r=static_cast<const char*>(h);
      CloseClipboard();
      return r;
#else
      return (tclcmd()<<"clipboard get -type UTF8_STRING\n").result;
#endif
    }

  void MinskyTCL::latex(const char* filename, bool wrapLaTeXLines) 
  {
    if (cycleCheck()) throw error("cyclic network detected");
    ofstream f(filename);

    f<<"\\documentclass{article}\n";
    if (wrapLaTeXLines)
      {
        f<<"\\usepackage{breqn}\n\\begin{document}\n";
        MathDAG::SystemOfEquations(*this).latexWrapped(f);
      }
    else
      {
        f<<"\\begin{document}\n";
          MathDAG::SystemOfEquations(*this).latex(f);
      }
    f<<"\\end{document}\n";
  }

  namespace
  {
    template <class T>
    struct IconBase: public ecolab::cairo::CairoImage, public T
    {
      template <class... U> 
      IconBase(const char* imageName, U... x): T(std::forward<U>(x)...)
      {
        Tk_PhotoHandle photo = Tk_FindPhoto(interp(), imageName);
        if (photo)
          cairoSurface.reset(new cairo::TkPhotoSurface(photo));        
      }
      void draw()
      {
        initMatrix();
        cairo_translate(cairoSurface->cairo(), 0.5*cairoSurface->width(), 0.5*cairoSurface->height());
        cairo_select_font_face(cairoSurface->cairo(), "sans-serif", 
                  CAIRO_FONT_SLANT_ITALIC,CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cairoSurface->cairo(),12);
        cairo_set_line_width(cairoSurface->cairo(),1);
        T::draw(cairoSurface->cairo());
        cairoSurface->blit();
      }
    };

    struct OperationIcon
    {
      OperationIcon(const char* opName):
        op(OperationType::Type(enumKey<OperationType::Type>(opName))) 
      {}
      OperationPtr op;
      void draw(cairo_t* cairo)
      {
        RenderOperation(*op, cairo).draw();
      }
    };
  }

  void MinskyTCL::operationIcon(const char* imageName, const char* opName) const
  {
    if (string(opName)=="switch")
      IconBase<SwitchIcon>(imageName).draw();
    else
      IconBase<OperationIcon>(imageName, opName).draw();
  }

  namespace
  {
    struct TkWinPhotoSurface: public cairo::TkPhotoSurface
    {
    };
    
    // TODO refactor Canvas class to directly redraw surface whenever anything changes
    struct TkWinSurface: public ecolab::cairo::Surface
    {
      Canvas& canvas;
      Tk_ImageMaster imageMaster;
      TkWinSurface(Canvas& canvas, Tk_ImageMaster imageMaster, cairo_surface_t* surf):
        ecolab::cairo::Surface(surf), canvas(canvas),  imageMaster(imageMaster) {}
      void requestRedraw() override {
        Tk_ImageChanged(imageMaster,-1000000,-1000000,2000000,2000000,2000000,2000000);
      }
      void blit() override {cairo_surface_flush(surface());}
    };

    struct CD
    {
      Tk_Window tkWin;
      Tk_ImageMaster master;
      Canvas& canvas;
    };
    
    // Define a new image type that renders a minsky::Canvas
    int createCI(Tcl_Interp* interp, const char* name, int objc, Tcl_Obj *const objv[],
                 const Tk_ImageType* typePtr, Tk_ImageMaster master, ClientData *masterData)
    {
      try
        {
          TCL_args args(objc,objv);
          string canvas=args; // arguments should be something like -canvas minsky.canvas
          auto mb=dynamic_cast<member_entry<Canvas>*>
            ((member_entry_base*)(TCL_obj_properties()[canvas].get()));
          if (mb)
            {
              *masterData=new CD{0,master,*mb->memberptr};
              return TCL_OK;
            }
          else
            {
              Tcl_AppendResult(interp,"Not a Canvas",NULL);
              return TCL_ERROR;
            }
        }
      catch (const std::exception& e)
        {
          Tcl_AppendResult(interp,e.what(),NULL);
          return TCL_ERROR;
        }
    }

    ClientData getCI(Tk_Window win, ClientData masterData)
    {
      auto r=new CD(*(CD*)masterData);
      r->tkWin=win;
      return r;
    }
    
    void displayCI(ClientData cd, Display* display, Drawable win,
                  int imageX, int imageY, int width, int height,
                  int drawableX, int drawableY)
    {
      CD& c=*(CD*)cd;
      int depth;
      Visual *visual = Tk_GetVisual(interp(), c.tkWin, "default", &depth, NULL);
#if USE_WIN32_SURFACE
        // TkWinGetDrawableDC is an internal (ie undocumented) routine
        // for getting the DC. We need to declare something to take
        // the state parameter - two long longs should be ample here
        long long state[2];
        HDC hdc=TkWinGetDrawableDC(display, win, state);
        SaveDC(hdc);
        c.canvas.surface.reset
        (new TkWinSurface
         (c.canvas, c.master,
          cairo_win32_surface_create(hdc)));
#elif defined(MAC_OSX_TK)
        NSContext nctx(win);
        c.canvas.surface.reset
        (new TkWinSurface
         (c.canvas, c.master,
          cairo_quartz_surface_create_for_cg_context(nctx.context, Tk_Width(c.tkWin), Tk_Height(c.tkWin))));
        cairo_surface_set_device_offset(c.canvas.surface->surface(),0,Tk_Height(c.tkWin));
        cairo_surface_set_device_scale(c.canvas.surface->surface(),1,-1);
#else
        c.canvas.surface.reset
          (new TkWinSurface
           (c.canvas, c.master,
            cairo_xlib_surface_create(display, win, visual, Tk_Width(c.tkWin), Tk_Height(c.tkWin))));
        
#endif
      c.canvas.redraw();
      //cairo_surface_flush(c.canvas.surface->surface());
      // release surface prior to any context going out of scope
      c.canvas.surface->surface(nullptr);
#if USE_WIN32_SURFACE
      RestoreDC(hdc,-1);
      TkWinReleaseDrawableDC(win, hdc, state);
#endif
    }

    void freeCI(ClientData cd,Display*) {delete (CD*)cd;}
    void deleteCI(ClientData cd) {delete (CD*)cd;}
  
    Tk_ImageType canvasImage = {
      "canvasImage",
      createCI,
      getCI,
      displayCI,
      freeCI,
      deleteCI
    };

    int registerCanvasImage() {
      // ensure Tk_Init is called.
      if (!Tk_MainWindow(interp())) Tk_Init(interp());
      Tk_CreateImageType(&canvasImage);
      return 0;
    }
  
    int dum=(initVec().push_back(registerCanvasImage),0);
  }
}