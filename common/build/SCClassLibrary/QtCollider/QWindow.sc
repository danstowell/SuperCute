QWindow : QView {
  classvar <allWindows, <>initAction;

  var <drawHook, <background;

  //dummy for compatibility
  var <currentSheet;

  *initClass {
    ShutDown.add( { QWindow.closeAll } );
  }

  *new { arg name,
         bounds = Rect( 128, 64, 400, 400 ),
         resizable = true,
         border = true,
         server,
         scroll = false;

    bounds.top = QWindow.screenBounds.height - bounds.top - bounds.height;

    //NOTE we omit server, which is only for compatibility with SwingOSC
    ^super.newCustom( [name, bounds.asRect, resizable, border, scroll] );
  }

  *screenBounds {
    ^this.prScreenBounds( Rect.new );
  }

  *closeAll {
    allWindows.copy.do { |win| win.close };
  }

  asView {
    ^this;
  }

  view {
    ^this;
  }

  bounds_ { arg aRect;
    aRect.top = QWindow.screenBounds.height - aRect.top - aRect.height;
    super.bounds_( aRect );
  }

  background_ { arg aColor;
    background = aColor;
    this.setProperty( \background, aColor, true );
  }

  drawHook_ { arg aFunction;
    drawHook = aFunction;
    this.setProperty( \paint, true );
    this.refresh;
  }

  refresh {
    if( drawHook.notNil ) {
      this.invokeMethod( \rebuildPen );
    };
    super.refresh;
  }
  // ---------------------- private ------------------------------------

  *prScreenBounds { arg return;
    _QWindow_ScreenBounds
    ^this.primitiveFailed
  }

  *prAddWindow { arg window;
    allWindows = allWindows.add( window );
  }

  *prRemoveWindow { arg window;
    allWindows.remove( window );
  }

  prDraw {
    drawHook.value( this );
  }
}
