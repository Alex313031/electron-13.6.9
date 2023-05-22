// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_TRAY_H_
#define SHELL_BROWSER_API_ELECTRON_API_TRAY_H_

#include <memory>
#include <string>
#include <vector>

#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/browser/javascript_environment.h"
#include "shell/browser/ui/tray_icon.h"
#include "shell/browser/ui/tray_icon_observer.h"
#include "shell/common/gin_converters/guid_converter.h"
#include "shell/common/gin_helper/cleaned_up_at_exit.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/error_thrower.h"

namespace gfx {
class Image;
}

namespace gin_helper {
class Dictionary;
}

namespace electron {

namespace api {

class Menu;
class NativeImage;

class Tray : public gin::Wrappable<Tray>,
             public gin_helper::EventEmitterMixin<Tray>,
             public gin_helper::Constructible<Tray>,
             public gin_helper::CleanedUpAtExit,
             public TrayIconObserver {
 public:
  // gin_helper::Constructible
  static gin::Handle<Tray> New(gin_helper::ErrorThrower thrower,
                               v8::Local<v8::Value> image,
                               base::Optional<UUID> guid,
                               gin::Arguments* args);
  static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
      v8::Isolate*,
      v8::Local<v8::ObjectTemplate>);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;

 private:
  Tray(v8::Isolate* isolate,
       v8::Local<v8::Value> image,
       base::Optional<UUID> guid);
  ~Tray() override;

  // TrayIconObserver:
  void OnClicked(const gfx::Rect& bounds,
                 const gfx::Point& location,
                 int modifiers) override;
  void OnDoubleClicked(const gfx::Rect& bounds, int modifiers) override;
  void OnRightClicked(const gfx::Rect& bounds, int modifiers) override;
  void OnBalloonShow() override;
  void OnBalloonClicked() override;
  void OnBalloonClosed() override;
  void OnDrop() override;
  void OnDropFiles(const std::vector<std::string>& files) override;
  void OnDropText(const std::string& text) override;
  void OnDragEntered() override;
  void OnDragExited() override;
  void OnDragEnded() override;
  void OnMouseUp(const gfx::Point& location, int modifiers) override;
  void OnMouseDown(const gfx::Point& location, int modifiers) override;
  void OnMouseEntered(const gfx::Point& location, int modifiers) override;
  void OnMouseExited(const gfx::Point& location, int modifiers) override;
  void OnMouseMoved(const gfx::Point& location, int modifiers) override;

  // JS API:
  void Destroy();
  bool IsDestroyed();
  void SetImage(v8::Isolate* isolate, v8::Local<v8::Value> image);
  void SetPressedImage(v8::Isolate* isolate, v8::Local<v8::Value> image);
  void SetToolTip(const std::string& tool_tip);
  void SetTitle(const std::string& title,
                const base::Optional<gin_helper::Dictionary>& options,
                gin::Arguments* args);
  std::string GetTitle();
  void SetIgnoreDoubleClickEvents(bool ignore);
  bool GetIgnoreDoubleClickEvents();
  void DisplayBalloon(gin_helper::ErrorThrower thrower,
                      const gin_helper::Dictionary& options);
  void RemoveBalloon();
  void Focus();
  void PopUpContextMenu(gin::Arguments* args);
  void CloseContextMenu();
  void SetContextMenu(gin_helper::ErrorThrower thrower,
                      v8::Local<v8::Value> arg);
  gfx::Rect GetBounds();

  bool CheckAlive();

  v8::Global<v8::Value> menu_;
  std::unique_ptr<TrayIcon> tray_icon_;

  DISALLOW_COPY_AND_ASSIGN(Tray);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_TRAY_H_
