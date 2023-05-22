// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_SERIAL_ELECTRON_SERIAL_DELEGATE_H_
#define SHELL_BROWSER_SERIAL_ELECTRON_SERIAL_DELEGATE_H_

#include <memory>
#include <unordered_map>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/serial_delegate.h"
#include "shell/browser/serial/serial_chooser_controller.h"

namespace electron {

class SerialChooserController;

class ElectronSerialDelegate : public content::SerialDelegate {
 public:
  ElectronSerialDelegate();
  ~ElectronSerialDelegate() override;

  std::unique_ptr<content::SerialChooser> RunChooser(
      content::RenderFrameHost* frame,
      std::vector<blink::mojom::SerialPortFilterPtr> filters,
      content::SerialChooser::Callback callback) override;
  bool CanRequestPortPermission(content::RenderFrameHost* frame) override;
  bool HasPortPermission(content::RenderFrameHost* frame,
                         const device::mojom::SerialPortInfo& port) override;
  device::mojom::SerialPortManager* GetPortManager(
      content::RenderFrameHost* frame) override;
  void AddObserver(content::RenderFrameHost* frame,
                   Observer* observer) override;
  void RemoveObserver(content::RenderFrameHost* frame,
                      Observer* observer) override;

  void DeleteControllerForFrame(content::RenderFrameHost* render_frame_host);

 private:
  SerialChooserController* ControllerForFrame(
      content::RenderFrameHost* render_frame_host);
  SerialChooserController* AddControllerForFrame(
      content::RenderFrameHost* render_frame_host,
      std::vector<blink::mojom::SerialPortFilterPtr> filters,
      content::SerialChooser::Callback callback);

  std::unordered_map<content::RenderFrameHost*,
                     std::unique_ptr<SerialChooserController>>
      controller_map_;

  base::WeakPtrFactory<ElectronSerialDelegate> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ElectronSerialDelegate);
};

}  // namespace electron

#endif  // SHELL_BROWSER_SERIAL_ELECTRON_SERIAL_DELEGATE_H_
