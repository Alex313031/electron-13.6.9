// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_GUEST_VIEW_CONTAINER_H_
#define SHELL_RENDERER_GUEST_VIEW_CONTAINER_H_

#include "base/callback.h"
#include "content/public/renderer/render_frame.h"

namespace gfx {
class Size;
}

namespace electron {

class GuestViewContainer {
 public:
  typedef base::Callback<void(const gfx::Size&)> ResizeCallback;

  explicit GuestViewContainer(content::RenderFrame* render_frame);
  virtual ~GuestViewContainer();

  static GuestViewContainer* FromID(int element_instance_id);

  void RegisterElementResizeCallback(const ResizeCallback& callback);
  void SetElementInstanceID(int element_instance_id);
  void DidResizeElement(const gfx::Size& new_size);

 private:
  int element_instance_id_;

  ResizeCallback element_resize_callback_;

  base::WeakPtrFactory<GuestViewContainer> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(GuestViewContainer);
};

}  // namespace electron

#endif  // SHELL_RENDERER_GUEST_VIEW_CONTAINER_H_
