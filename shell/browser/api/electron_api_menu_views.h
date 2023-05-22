// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_MENU_VIEWS_H_
#define SHELL_BROWSER_API_ELECTRON_API_MENU_VIEWS_H_

#include <map>
#include <memory>

#include "base/memory/weak_ptr.h"
#include "shell/browser/api/electron_api_menu.h"
#include "ui/display/screen.h"
#include "ui/views/controls/menu/menu_runner.h"

namespace electron {

namespace api {

class MenuViews : public Menu {
 public:
  explicit MenuViews(gin::Arguments* args);
  ~MenuViews() override;

 protected:
  void PopupAt(BaseWindow* window,
               int x,
               int y,
               int positioning_item,
               base::OnceClosure callback) override;
  void ClosePopupAt(int32_t window_id) override;

 private:
  void OnClosed(int32_t window_id, base::OnceClosure callback);

  // window ID -> open context menu
  std::map<int32_t, std::unique_ptr<views::MenuRunner>> menu_runners_;

  base::WeakPtrFactory<MenuViews> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(MenuViews);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_MENU_VIEWS_H_
