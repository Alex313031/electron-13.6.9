// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef SHELL_BROWSER_UI_DEVTOOLS_UI_H_
#define SHELL_BROWSER_UI_DEVTOOLS_UI_H_

#include "base/compiler_specific.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_ui_controller.h"

namespace electron {

class BrowserContext;

class DevToolsUI : public content::WebUIController {
 public:
  explicit DevToolsUI(content::BrowserContext* browser_context,
                      content::WebUI* web_ui);

 private:
  DISALLOW_COPY_AND_ASSIGN(DevToolsUI);
};

}  // namespace electron

#endif  // SHELL_BROWSER_UI_DEVTOOLS_UI_H_
