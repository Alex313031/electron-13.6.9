// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_UI_GTK_UTIL_H_
#define SHELL_BROWSER_UI_GTK_UTIL_H_

#include <gtk/gtk.h>

class SkBitmap;

namespace gtk_util {

/* These are `const char*` rather than the project-preferred `const char[]`
   because they must fit the type of an external dependency */
extern const char* const kCancelLabel;
extern const char* const kNoLabel;
extern const char* const kOkLabel;
extern const char* const kOpenLabel;
extern const char* const kSaveLabel;
extern const char* const kYesLabel;

// Convert and copy a SkBitmap to a GdkPixbuf. NOTE: this uses BGRAToRGBA, so
// it is an expensive operation.  The returned GdkPixbuf will have a refcount of
// 1, and the caller is responsible for unrefing it when done.
GdkPixbuf* GdkPixbufFromSkBitmap(const SkBitmap& bitmap);

}  // namespace gtk_util

#endif  // SHELL_BROWSER_UI_GTK_UTIL_H_
