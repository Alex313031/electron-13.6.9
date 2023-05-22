// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_VIEWS_SUBMENU_BUTTON_H_
#define SHELL_BROWSER_UI_VIEWS_SUBMENU_BUTTON_H_

#include <memory>

#include "ui/accessibility/ax_node_data.h"
#include "ui/views/animation/ink_drop_highlight.h"
#include "ui/views/controls/button/menu_button.h"

namespace electron {

// Special button that used by menu bar to show submenus.
class SubmenuButton : public views::MenuButton {
 public:
  SubmenuButton(PressedCallback callback,
                const std::u16string& title,
                const SkColor& background_color);
  ~SubmenuButton() override;

  void SetAcceleratorVisibility(bool visible);
  void SetUnderlineColor(SkColor color);

  char16_t accelerator() const { return accelerator_; }

  void GetAccessibleNodeData(ui::AXNodeData* node_data) override;

  // views::MenuButton:
  void PaintButtonContents(gfx::Canvas* canvas) override;

  // views::InkDropHostView:
  std::unique_ptr<views::InkDropRipple> CreateInkDropRipple() const override;
  std::unique_ptr<views::InkDrop> CreateInkDrop() override;

 private:
  bool GetUnderlinePosition(const std::u16string& text,
                            char16_t* accelerator,
                            int* start,
                            int* end) const;
  void GetCharacterPosition(const std::u16string& text,
                            int index,
                            int* pos) const;

  char16_t accelerator_ = 0;

  bool show_underline_ = false;

  int underline_start_ = 0;
  int underline_end_ = 0;
  int text_width_ = 0;
  int text_height_ = 0;
  SkColor underline_color_ = SK_ColorBLACK;
  SkColor background_color_;

  DISALLOW_COPY_AND_ASSIGN(SubmenuButton);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_VIEWS_SUBMENU_BUTTON_H_
