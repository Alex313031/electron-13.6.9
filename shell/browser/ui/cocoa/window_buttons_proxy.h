// Copyright (c) 2021 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_PROXY_H_
#define SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_PROXY_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#include "base/optional.h"
#include "ui/gfx/geometry/point.h"

@class WindowButtonsProxy;

// A helper view that floats above the window buttons.
@interface ButtonsAreaHoverView : NSView {
 @private
  WindowButtonsProxy* proxy_;
}
- (id)initWithProxy:(WindowButtonsProxy*)proxy;
@end

// Manipulating the window buttons.
@interface WindowButtonsProxy : NSObject {
 @private
  NSWindow* window_;

  // Current left-top margin of buttons.
  gfx::Point margin_;
  // The default left-top margin.
  gfx::Point default_margin_;

  // Track mouse moves above window buttons.
  BOOL show_on_hover_;
  BOOL mouse_inside_;
  base::scoped_nsobject<NSTrackingArea> tracking_area_;
  base::scoped_nsobject<ButtonsAreaHoverView> hover_view_;
}

- (id)initWithWindow:(NSWindow*)window;

- (void)setVisible:(BOOL)visible;
- (BOOL)isVisible;

// Only show window buttons when mouse moves above them.
- (void)setShowOnHover:(BOOL)yes;

// Set left-top margin of the window buttons..
- (void)setMargin:(const base::Optional<gfx::Point>&)margin;

// Return the bounds of all 3 buttons, with margin on all sides.
- (NSRect)getButtonsContainerBounds;

- (void)redraw;
- (void)updateTrackingAreas;

- (gfx::Point)getMargin;
- (NSRect)getButtonsBounds;
@end

#endif  // SHELL_BROWSER_UI_COCOA_WINDOW_BUTTONS_PROXY_H_
