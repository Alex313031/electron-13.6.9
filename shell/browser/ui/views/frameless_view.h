// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_VIEWS_FRAMELESS_VIEW_H_
#define SHELL_BROWSER_UI_VIEWS_FRAMELESS_VIEW_H_

#include "ui/views/window/non_client_view.h"

namespace views {
class Widget;
}

namespace electron {

class NativeWindowViews;

class FramelessView : public views::NonClientFrameView {
 public:
  static const char kViewClassName[];
  FramelessView();
  ~FramelessView() override;

  virtual void Init(NativeWindowViews* window, views::Widget* frame);

  // Returns whether the |point| is on frameless window's resizing border.
  int ResizingBorderHitTest(const gfx::Point& point);

 protected:
  // views::NonClientFrameView:
  gfx::Rect GetBoundsForClientView() const override;
  gfx::Rect GetWindowBoundsForClientBounds(
      const gfx::Rect& client_bounds) const override;
  int NonClientHitTest(const gfx::Point& point) override;
  void GetWindowMask(const gfx::Size& size, SkPath* window_mask) override;
  void ResetWindowControls() override;
  void UpdateWindowIcon() override;
  void UpdateWindowTitle() override;
  void SizeConstraintsChanged() override;

  // Overridden from View:
  gfx::Size CalculatePreferredSize() const override;
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  const char* GetClassName() const override;

  // Not owned.
  NativeWindowViews* window_ = nullptr;
  views::Widget* frame_ = nullptr;

  friend class NativeWindowsViews;

 private:
  DISALLOW_COPY_AND_ASSIGN(FramelessView);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_VIEWS_FRAMELESS_VIEW_H_
