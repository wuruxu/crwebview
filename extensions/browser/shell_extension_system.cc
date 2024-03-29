// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/crwebview/extensions/browser/shell_extension_system.h"

#include <memory>
#include <string>

#include "apps/launcher.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "components/services/app_service/public/mojom/types.mojom-shared.h"
#include "components/value_store/value_store_factory_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/app_runtime/app_runtime_api.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/null_app_sorting.h"
#include "extensions/browser/quota_service.h"
#include "extensions/browser/service_worker_manager.h"
#include "extensions/browser/user_script_manager.h"
#include "extensions/common/constants.h"
#include "extensions/browser/state_store.h"
#include "extensions/common/file_util.h"
#include "android_webview/crwebview/extensions/browser/shell_extension_loader.h"

using content::BrowserContext;
namespace extensions {

ShellExtensionSystem::ShellExtensionSystem(BrowserContext* browser_context)
    : browser_context_(browser_context),
      store_factory_(
          new value_store::ValueStoreFactoryImpl(browser_context->GetPath())) {
  LOG(INFO) << "ShellExtensionSystem ShellExtensionSystem " << browser_context->GetPath().value();
  state_store_ = std::make_unique<StateStore>(
      browser_context, store_factory_, StateStore::BackendType::STATE, true);

  rules_store_ = std::make_unique<StateStore>(
      browser_context, store_factory_, StateStore::BackendType::RULES, false);

  dynamic_user_scripts_store_ = std::make_unique<StateStore>(
      browser_context, store_factory_, StateStore::BackendType::SCRIPTS, false);
}

ShellExtensionSystem::~ShellExtensionSystem() = default;

const Extension* ShellExtensionSystem::LoadExtension(
    const base::FilePath& extension_dir) {
  const Extension* ext = extension_loader_->LoadExtension(extension_dir);
  return ext;
}

const Extension* ShellExtensionSystem::LoadApp(const base::FilePath& app_dir) {
  return LoadExtension(app_dir);
}

void ShellExtensionSystem::FinishInitialization() {
  // Inform the rest of the extensions system to start.
  ready_.Signal();
}

void ShellExtensionSystem::LaunchApp(const ExtensionId& extension_id) {
  // Send the onLaunched event.
  DCHECK(ExtensionRegistry::Get(browser_context_)
             ->enabled_extensions()
             .Contains(extension_id));
  const Extension* extension = ExtensionRegistry::Get(browser_context_)
                                   ->enabled_extensions()
                                   .GetByID(extension_id);
  apps::LaunchPlatformApp(browser_context_, extension,
                          AppLaunchSource::kSourceUntracked);
}

void ShellExtensionSystem::ReloadExtension(const ExtensionId& extension_id) {
  extension_loader_->ReloadExtension(extension_id);
}

void ShellExtensionSystem::Shutdown() {
  extension_loader_.reset();
}

void ShellExtensionSystem::InitForRegularProfile(bool extensions_enabled) {
  service_worker_manager_ =
      std::make_unique<ServiceWorkerManager>(browser_context_);
  quota_service_ = std::make_unique<QuotaService>();
  app_sorting_ = std::make_unique<NullAppSorting>();
  extension_loader_ = std::make_unique<ShellExtensionLoader>(browser_context_);
  user_script_manager_ = std::make_unique<UserScriptManager>(browser_context_);


}

ExtensionService* ShellExtensionSystem::extension_service() {
  LOG(INFO) << "ExtensionService extension_service return nullptr";
  return nullptr;
}

ManagementPolicy* ShellExtensionSystem::management_policy() {
  LOG(INFO) << "ExtensionService management_policy return nullptr";
  return nullptr;
}

ServiceWorkerManager* ShellExtensionSystem::service_worker_manager() {
  return service_worker_manager_.get();
}

UserScriptManager* ShellExtensionSystem::user_script_manager() {
  return user_script_manager_.get();
}

StateStore* ShellExtensionSystem::state_store() {
  return state_store_.get();
}

StateStore* ShellExtensionSystem::rules_store() {
  return rules_store_.get();
}

StateStore* ShellExtensionSystem::dynamic_user_scripts_store() {
  return dynamic_user_scripts_store_.get();
}

scoped_refptr<value_store::ValueStoreFactory>
ShellExtensionSystem::store_factory() {
  return store_factory_;
}

InfoMap* ShellExtensionSystem::info_map() {
  if (!info_map_.get())
    info_map_ = new InfoMap;
  return info_map_.get();
}

QuotaService* ShellExtensionSystem::quota_service() {
  return quota_service_.get();
}

AppSorting* ShellExtensionSystem::app_sorting() {
  return app_sorting_.get();
}

void ShellExtensionSystem::RegisterExtensionWithRequestContexts(
    const Extension* extension,
    base::OnceClosure callback) {
  content::GetIOThreadTaskRunner({})->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&InfoMap::AddExtension, info_map(),
                     base::RetainedRef(extension), base::Time::Now(), false,
                     false),
      std::move(callback));
  //ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context_);
  //std::unique_ptr<ExtensionSet> all = registry->GenerateInstalledExtensionsSet();
  //for(const scoped_refptr<const extensions::Extension> ext : *all) {
  //  LOG(INFO) << "ShellExtensionSystem RegisterExtensionWithRequestContexts InstalledExtensions " << ext->id();
  //}
  //registry->AddEnabled(extension);
}

void ShellExtensionSystem::UnregisterExtensionWithRequestContexts(
    const std::string& extension_id) {}

const base::OneShotEvent& ShellExtensionSystem::ready() const {
  return ready_;
}

bool ShellExtensionSystem::is_ready() const {
  return ready_.is_signaled();
}

ContentVerifier* ShellExtensionSystem::content_verifier() {
  LOG(INFO) << "ExtensionService content_verifier return nullptr";
  return nullptr;
}

std::unique_ptr<ExtensionSet> ShellExtensionSystem::GetDependentExtensions(
    const Extension* extension) {
  return std::make_unique<ExtensionSet>();
}

void ShellExtensionSystem::InstallUpdate(
    const std::string& extension_id,
    const std::string& public_key,
    const base::FilePath& temp_dir,
    bool install_immediately,
    InstallUpdateCallback install_update_callback) {
  NOTREACHED();
  base::DeletePathRecursively(temp_dir);
}

void ShellExtensionSystem::PerformActionBasedOnOmahaAttributes(
    const std::string& extension_id,
    const base::Value& attributes) {
  NOTREACHED();
}

bool ShellExtensionSystem::FinishDelayedInstallationIfReady(
    const std::string& extension_id,
    bool install_immediately) {
  NOTREACHED();
  return false;
}

void ShellExtensionSystem::OnExtensionRegisteredWithRequestContexts(
    scoped_refptr<Extension> extension) {
  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context_);
  registry->AddReady(extension);
  registry->TriggerOnReady(extension.get());
}

}  // namespace extensions
