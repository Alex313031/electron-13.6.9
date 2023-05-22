// Copyright (c) 2015 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.
#ifndef SHELL_BROWSER_ELECTRON_PDF_WEB_CONTENTS_HELPER_CLIENT_H_
#define SHELL_BROWSER_ELECTRON_PDF_WEB_CONTENTS_HELPER_CLIENT_H_

#include "components/pdf/browser/pdf_web_contents_helper_client.h"

namespace content {
class WebContents;
}

class ElectronPDFWebContentsHelperClient
    : public pdf::PDFWebContentsHelperClient {
 public:
  ElectronPDFWebContentsHelperClient();
  ~ElectronPDFWebContentsHelperClient() override;

 private:
  // pdf::PDFWebContentsHelperClient
  void UpdateContentRestrictions(content::WebContents* contents,
                                 int content_restrictions) override;
  void OnPDFHasUnsupportedFeature(content::WebContents* contents) override;
  void OnSaveURL(content::WebContents* contents) override;
  void SetPluginCanSave(content::WebContents* contents, bool can_save) override;
};

#endif  // SHELL_BROWSER_ELECTRON_PDF_WEB_CONTENTS_HELPER_CLIENT_H_
