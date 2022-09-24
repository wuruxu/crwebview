// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/crwebview/extensions/browser/shell_extension_host_delegate.h"

#include "base/notreached.h"
#include "content/public/browser/web_contents_delegate.h"
#include "extensions/browser/media_capture_util.h"
#include "android_webview/crwebview/extensions/browser/shell_extension_web_contents_observer.h"

namespace extensions {

ShellExtensionHostDelegate::ShellExtensionHostDelegate() = default;
ShellExtensionHostDelegate::~ShellExtensionHostDelegate() = default;

void ShellExtensionHostDelegate::OnExtensionHostCreated(
    content::WebContents* web_contents) {
  LOG(INFO) << "ShellExtensionHostDelegate OnExtensionHostCreated ";
  ShellExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

void ShellExtensionHostDelegate::OnMainFrameCreatedForBackgroundPage(
    ExtensionHost* host) {}

content::JavaScriptDialogManager*
ShellExtensionHostDelegate::GetJavaScriptDialogManager() {
  // TODO(jamescook): Create a JavaScriptDialogManager or reuse the one from
  // content_shell.
  LOG(INFO) << "TODO: ShellExtensionHostDelegate GetJavaScriptDialogManager";
  return NULL;
}

void ShellExtensionHostDelegate::CreateTab(
    std::unique_ptr<content::WebContents> web_contents,
    const std::string& extension_id,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture) {
  // TODO(jamescook): Should app_shell support opening popup windows?
  LOG(INFO) << "TODO: ShellExtensionHostDelegate CreateTab for " << extension_id;
}

void ShellExtensionHostDelegate::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback,
    const Extension* extension) {
  // Allow access to the microphone and/or camera.
  media_capture_util::GrantMediaStreamRequest(web_contents, request,
                                              std::move(callback), extension);
}

bool ShellExtensionHostDelegate::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    blink::mojom::MediaStreamType type,
    const Extension* extension) {
  media_capture_util::VerifyMediaAccessPermission(type, extension);
  return true;
}

content::PictureInPictureResult
ShellExtensionHostDelegate::EnterPictureInPicture(
    content::WebContents* web_contents) {
  LOG(INFO) << "ShellExtensionHostDelegate EnterPictureInPicture ";
  return content::PictureInPictureResult::kNotSupported;
}

void ShellExtensionHostDelegate::ExitPictureInPicture() {
  LOG(INFO) << "ShellExtensionHostDelegate ExitPictureInPicture ";
}

}  // namespace extensions
