// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_SKIA_UTIL_H_
#define SHELL_COMMON_SKIA_UTIL_H_

#include <string>

#include "ui/gfx/image/image_skia.h"

namespace electron {

namespace util {

bool PopulateImageSkiaRepsFromPath(gfx::ImageSkia* image,
                                   const base::FilePath& path);

bool AddImageSkiaRepFromBuffer(gfx::ImageSkia* image,
                               const unsigned char* data,
                               size_t size,
                               int width,
                               int height,
                               double scale_factor);

bool AddImageSkiaRepFromJPEG(gfx::ImageSkia* image,
                             const unsigned char* data,
                             size_t size,
                             double scale_factor);

bool AddImageSkiaRepFromPNG(gfx::ImageSkia* image,
                            const unsigned char* data,
                            size_t size,
                            double scale_factor);

#if defined(OS_WIN)
bool ReadImageSkiaFromICO(gfx::ImageSkia* image, HICON icon);
#endif

}  // namespace util

}  // namespace electron

#endif  // SHELL_COMMON_SKIA_UTIL_H_
