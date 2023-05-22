// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_
#define SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_

#include <memory>

#include "shell/browser/ui/electron_menu_model.h"
#include "shell/browser/ui/views/menu_delegate.h"
#include "shell/browser/ui/views/root_view.h"
#include "ui/views/accessible_pane_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/focus/focus_manager.h"
#include "ui/views/view.h"

namespace views {
class Button;
class MenuButton;
}  // namespace views

namespace electron {

class MenuBarColorUpdater : public views::FocusChangeListener {
 public:
  explicit MenuBarColorUpdater(MenuBar* menu_bar);
  ~MenuBarColorUpdater() override;

  void OnDidChangeFocus(views::View* focused_before,
                        views::View* focused_now) override;
  void OnWillChangeFocus(views::View* focused_before,
                         views::View* focused_now) override {}

 private:
  MenuBar* menu_bar_;
};

class MenuBar : public views::AccessiblePaneView,
                public electron::MenuDelegate::Observer {
 public:
  static const char kViewClassName[];

  explicit MenuBar(RootView* window);
  ~MenuBar() override;

  // Replaces current menu with a new one.
  void SetMenu(ElectronMenuModel* menu_model);

  // Shows underline under accelerators.
  void SetAcceleratorVisibility(bool visible);

  // Returns true if the submenu has accelerator |key|
  bool HasAccelerator(char16_t key);

  // Shows the submenu whose accelerator is |key|.
  void ActivateAccelerator(char16_t key);

  // Returns there are how many items in the root menu.
  int GetItemCount() const;

  // Get the menu under specified screen point.
  bool GetMenuButtonFromScreenPoint(const gfx::Point& point,
                                    ElectronMenuModel** menu_model,
                                    views::MenuButton** button);

  // electron::MenuDelegate::Observer:
  void OnBeforeExecuteCommand() override;
  void OnMenuClosed() override;

  // views::AccessiblePaneView:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool SetPaneFocus(views::View* initial_focus) override;
  void RemovePaneFocus() override;
  void OnThemeChanged() override;

 private:
  friend class MenuBarColorUpdater;

  // views::View:
  const char* GetClassName() const override;

  void ButtonPressed(int id, const ui::Event& event);

  void RebuildChildren();
  void UpdateViewColors();

  void RefreshColorCache();
  SkColor background_color_;
#if defined(OS_LINUX)
  SkColor enabled_color_;
  SkColor disabled_color_;
#endif

  RootView* window_ = nullptr;
  ElectronMenuModel* menu_model_ = nullptr;

  View* FindAccelChild(char16_t key);

  bool has_focus_ = true;

  std::unique_ptr<MenuBarColorUpdater> color_updater_;

  DISALLOW_COPY_AND_ASSIGN(MenuBar);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_VIEWS_MENU_BAR_H_
