// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_WEB_CONTENTS_PREFERENCES_H_
#define SHELL_BROWSER_WEB_CONTENTS_PREFERENCES_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "content/public/browser/web_contents_user_data.h"

namespace base {
class CommandLine;
}

namespace content {
struct WebPreferences;
}

namespace gin_helper {
class Dictionary;
}

namespace electron {

// Stores and applies the preferences of WebContents.
class WebContentsPreferences
    : public content::WebContentsUserData<WebContentsPreferences> {
 public:
  // Get self from WebContents.
  static WebContentsPreferences* From(content::WebContents* web_contents);

  WebContentsPreferences(content::WebContents* web_contents,
                         const gin_helper::Dictionary& web_preferences);
  ~WebContentsPreferences() override;

  // Set WebPreferences defaults onto the JS object.
  void SetDefaults();

  // A simple way to know whether a Boolean property is enabled.
  bool IsEnabled(base::StringPiece name, bool default_value = false) const;

  // $.extend(|web_preferences|, |new_web_preferences|).
  void Merge(const base::DictionaryValue& new_web_preferences);

  // Append command paramters according to preferences.
  void AppendCommandLineSwitches(base::CommandLine* command_line,
                                 bool is_subframe);

  // Modify the WebPreferences according to preferences.
  void OverrideWebkitPrefs(blink::web_pref::WebPreferences* prefs);

  // Clear the current WebPreferences.
  void Clear();

  // Return true if the particular preference value exists.
  bool GetPreference(base::StringPiece name, std::string* value) const;

  // Returns the preload script path.
  bool GetPreloadPath(base::FilePath* path) const;

  // Returns the web preferences.
  base::Value* preference() { return &preference_; }
  base::Value* last_preference() { return &last_preference_; }

 private:
  friend class content::WebContentsUserData<WebContentsPreferences>;
  friend class ElectronBrowserClient;

  // Get WebContents according to process ID.
  static content::WebContents* GetWebContentsFromProcessID(int process_id);

  // Checks if the key is not defined
  bool IsUndefined(base::StringPiece key);

  // Set preference value to given bool if user did not provide value
  bool SetDefaultBoolIfUndefined(base::StringPiece key, bool val);

  // Set preference value to given bool
  void SetBool(base::StringPiece key, bool value);

  static std::vector<WebContentsPreferences*> instances_;

  content::WebContents* web_contents_;

  base::Value preference_ = base::Value(base::Value::Type::DICTIONARY);
  base::Value last_preference_ = base::Value(base::Value::Type::DICTIONARY);

  WEB_CONTENTS_USER_DATA_KEY_DECL();

  DISALLOW_COPY_AND_ASSIGN(WebContentsPreferences);
};

}  // namespace electron

#endif  // SHELL_BROWSER_WEB_CONTENTS_PREFERENCES_H_
