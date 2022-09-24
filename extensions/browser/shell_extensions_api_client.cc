// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/crwebview/extensions/browser/shell_extensions_api_client.h"

#include <utility>

#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/api/messaging/messaging_delegate.h"
//#include "extensions/shell/browser/api/feedback_private/shell_feedback_private_delegate.h"
//#include "extensions/shell/browser/delegates/shell_kiosk_delegate.h"
//#include "android_webview/crwebview/extensions/browser/shell_app_view_guest_delegate.h"
//#include "extensions/browser/api/system_display/display_info_provider.h"
#include "android_webview/crwebview/extensions/browser/shell_extension_web_contents_observer.h"
//#include "android_webview/crwebview/extensions/browser/shell_virtual_keyboard_delegate.h"
//#include "android_webview/crwebview/extensions/browser/shell_web_view_guest_delegate.h"

namespace extensions {

ShellExtensionsAPIClient::ShellExtensionsAPIClient() {}

ShellExtensionsAPIClient::~ShellExtensionsAPIClient() {}

void ShellExtensionsAPIClient::AttachWebContentsHelpers(
    content::WebContents* web_contents) const {
  LOG(INFO) << "ShellExtensionsAPIClient AttachWebContentsHelpers call CreateForWebContents";
  ShellExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

AppViewGuestDelegate* ShellExtensionsAPIClient::CreateAppViewGuestDelegate()
    const {
  LOG(INFO) << "AppViewGuestDelegate CreateAppViewGuestDelegate";
  return nullptr; //new ShellAppViewGuestDelegate();
}

WebViewGuestDelegate* ShellExtensionsAPIClient::CreateWebViewGuestDelegate(
    WebViewGuest* web_view_guest) const {
  LOG(INFO) << "AppViewGuestDelegate CreateWebViewGuestDelegate";
  return nullptr; //new ShellWebViewGuestDelegate();
}

std::unique_ptr<VirtualKeyboardDelegate>
ShellExtensionsAPIClient::CreateVirtualKeyboardDelegate(
    content::BrowserContext* browser_context) const {
  //return std::make_unique<ShellVirtualKeyboardDelegate>();
  return nullptr;
}

std::unique_ptr<DisplayInfoProvider>
ShellExtensionsAPIClient::CreateDisplayInfoProvider() const {
  //return std::make_unique<DisplayInfoProvider>();
  return nullptr;
}

MessagingDelegate* ShellExtensionsAPIClient::GetMessagingDelegate() {
  // The default implementation does nothing, which is fine.
  if (!messaging_delegate_)
    messaging_delegate_ = std::make_unique<MessagingDelegate>();
  return messaging_delegate_.get();
}

FeedbackPrivateDelegate*
ShellExtensionsAPIClient::GetFeedbackPrivateDelegate() {
  return nullptr;
}

}  // namespace extensions
