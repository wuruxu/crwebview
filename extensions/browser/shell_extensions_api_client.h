// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_SHELL_EXTENSIONS_API_CLIENT_H_
#define EXTENSIONS_SHELL_BROWSER_SHELL_EXTENSIONS_API_CLIENT_H_

#include <memory>

#include "extensions/browser/api/extensions_api_client.h"

#include "build/build_config.h"
#include "build/chromeos_buildflags.h"

namespace extensions {

class MessagingDelegate;
class VirtualKeyboardDelegate;

class ShellExtensionsAPIClient : public ExtensionsAPIClient {
 public:
  ShellExtensionsAPIClient();
  ShellExtensionsAPIClient(const ShellExtensionsAPIClient&) = delete;
  ShellExtensionsAPIClient& operator=(const ShellExtensionsAPIClient&) = delete;
  ~ShellExtensionsAPIClient() override;

  // ExtensionsAPIClient implementation.
  void AttachWebContentsHelpers(content::WebContents* web_contents) const
      override;
  AppViewGuestDelegate* CreateAppViewGuestDelegate() const override;
  WebViewGuestDelegate* CreateWebViewGuestDelegate(
      WebViewGuest* web_view_guest) const override;
  std::unique_ptr<VirtualKeyboardDelegate> CreateVirtualKeyboardDelegate(
      content::BrowserContext* browser_context) const override;
  std::unique_ptr<DisplayInfoProvider> CreateDisplayInfoProvider()
      const override;

  MessagingDelegate* GetMessagingDelegate() override;
  FeedbackPrivateDelegate* GetFeedbackPrivateDelegate() override;

 private:

  std::unique_ptr<MessagingDelegate> messaging_delegate_;
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_SHELL_EXTENSIONS_API_CLIENT_H_
