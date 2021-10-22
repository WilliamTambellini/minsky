/*
  @copyright Steve Keen 2021
  @author Janak Porwal
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

#ifndef WINDOW_INFORMATION_H
#define WINDOW_INFORMATION_H

#include <cairoSurfaceImage.h>
#if defined(CAIRO_HAS_WIN32_SURFACE) && !defined(__CYGWIN__)
#define USE_WIN32_SURFACE
#include <windows.h>
#include <wingdi.h>
#elif defined(CAIRO_HAS_XLIB_SURFACE) && !defined(MAC_OSX_TK)
#define USE_X11
#include <cairo/cairo-xlib.h>
#include <X11/Xlib.h>
#endif

#include <thread>
#include <atomic>

namespace minsky
{
  class WindowInformation
  {
    bool isRendering=false;
#ifdef USE_WIN32_SURFACE
    HWND parentWindowId, childWindowId;
    HBITMAP hbmMem; // backing buffer pixmap
    HANDLE hOld;    // 
#elif defined(MAC_OSX_TK)
#elif defined(USE_X11)
    Window parentWindowId;
    Window childWindowId, bufferWindowId;
    
    Display*	display; // Weak reference, returned by system
    GC graphicsContext;
    XWindowAttributes wAttr;

    struct EventThread: std::thread
    {
      WindowInformation& winfo;
      std::atomic<bool> running{true};
      void run();
      EventThread(WindowInformation& w): thread([this]{run();}), winfo(w) {}
      ~EventThread() {running=false; join();}
    };

    std::unique_ptr<EventThread> eventThread;
#endif
    ecolab::cairo::SurfacePtr bufferSurface;

  public: 
#ifdef USE_WIN32_SURFACE
    HDC hdcMem; // backing buffer bitmap device context
#endif
    int childWidth;
    int childHeight;
    int offsetLeft;
    int offsetTop;
      
    bool getRenderingFlag();
    void setRenderingFlag(bool value);
    void copyBufferToMain();
      
  public:
    ~WindowInformation();
    WindowInformation(uint64_t parentWin, int left, int top, int cWidth, int cHeight);
    
    const ecolab::cairo::SurfacePtr& getBufferSurface();
    void blit(int x, int y, int width, int height);

    
    
    WindowInformation(const WindowInformation&)=delete;
    void operator=(const WindowInformation&)=delete;
  };
} // namespace minsky

#endif
