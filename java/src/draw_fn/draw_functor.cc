// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/glue/java/src/draw_glue/draw_functor.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "android_webview/public/browser/draw_fn.h"
#include "android_webview/glue/java/src/draw_glue/allocator.h"
#include "base/android/jni_array.h"
#include "base/logging.h"
#include "base/memory/raw_ptr.h"
#include "base/native_library.h"
#include "base/threading/thread_restrictions.h"
#include "gpu/vulkan/init/skia_vk_memory_allocator_impl.h"
#include "gpu/vulkan/init/vulkan_factory.h"
#include "gpu/vulkan/vulkan_device_queue.h"
#include "gpu/vulkan/vulkan_function_pointers.h"
#include "gpu/vulkan/vulkan_implementation.h"
#include "gpu/vulkan/vulkan_instance.h"
#include "gpu/vulkan/vulkan_surface.h"
#include "gpu/vulkan/vulkan_swap_chain.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkDrawable.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkSurfaceProps.h"
#include "third_party/skia/include/gpu/MutableTextureState.h"
#include "third_party/skia/include/gpu/ganesh/GrBackendSemaphore.h"
#include "third_party/skia/include/gpu/ganesh/GrBackendSurface.h"
#include "third_party/skia/include/gpu/ganesh/GrDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/GrTypes.h"
#include "third_party/skia/include/gpu/ganesh/SkSurfaceGanesh.h"
#include "third_party/skia/include/gpu/ganesh/vk/GrBackendDrawableInfo.h"
#include "third_party/skia/include/gpu/ganesh/vk/GrVkBackendSemaphore.h"
#include "third_party/skia/include/gpu/ganesh/vk/GrVkBackendSurface.h"
#include "third_party/skia/include/gpu/ganesh/vk/GrVkDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/vk/GrVkTypes.h"
#include "third_party/skia/include/gpu/vk/VulkanBackendContext.h"
#include "third_party/skia/include/gpu/vk/VulkanExtensions.h"
#include "third_party/skia/include/gpu/vk/VulkanMutableTextureState.h"
#include "third_party/skia/include/gpu/vk/VulkanTypes.h"
#include "ui/gfx/color_space.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "android_webview/draw_fn_impl_jni_headers/MyDrawFunctor_jni.h"

namespace draw_fn {

static std::unique_ptr<gpu::VulkanDeviceQueue> device_queue_;
static std::unique_ptr<gpu::VulkanImplementation> vulkan_implementation_;

namespace {

template <typename T>
void SetColorSpace(T* params) {
  // Hard coded value for sRGB.
  params->transfer_function_g = 2.4f;
  params->transfer_function_a = 0.947867f;
  params->transfer_function_b = 0.0521327f;
  params->transfer_function_c = 0.0773994f;
  params->transfer_function_d = 0.0404499f;
  params->transfer_function_e = 0.f;
  params->transfer_function_f = 0.f;
  params->color_space_toXYZD50[0] = 0.436028f;
  params->color_space_toXYZD50[1] = 0.385101f;
  params->color_space_toXYZD50[2] = 0.143091f;
  params->color_space_toXYZD50[3] = 0.222479f;
  params->color_space_toXYZD50[4] = 0.716897f;
  params->color_space_toXYZD50[5] = 0.0606241f;
  params->color_space_toXYZD50[6] = 0.0139264f;
  params->color_space_toXYZD50[7] = 0.0970921f;
  params->color_space_toXYZD50[8] = 0.714191;
}

class VkFunctorDrawHandler : public SkDrawable::GpuDrawHandler {
 public:
  VkFunctorDrawHandler(OverlaysManager* overlays_manager,
                       int functor,
                       int scroll_x,
                       int scroll_y,
                       const SkMatrix& matrix,
                       const SkIRect& clip_bounds,
                       const SkImageInfo& image_info)
      : overlays_manager_(overlays_manager),
        functor_(functor),
        scroll_x_(scroll_x),
        scroll_y_(scroll_y),
        matrix_(matrix),
        clip_bounds_(clip_bounds),
        image_info_(image_info) {}

  ~VkFunctorDrawHandler() override {
    AwDrawFn_PostDrawVkParams params{kAwDrawFnVersion};
    FunctorData& data = Allocator::Get()->get(functor_);
    data.functor_callbacks->post_draw_vk(functor_, data.data, &params);
  }

  void draw(const GrBackendDrawableInfo& backend_drawable_info) override {
    GrVkDrawableInfo vulkan_info;
    CHECK(backend_drawable_info.getVkDrawableInfo(&vulkan_info));
    AwDrawFn_DrawVkParams params{kAwDrawFnVersion};
    params.width = image_info_.width();
    params.height = image_info_.height();

    SkM44 mat4(matrix_);
    mat4.postTranslate(-scroll_x_, -scroll_y_);
    mat4.getColMajor(&params.transform[0]);

    params.secondary_command_buffer = vulkan_info.fSecondaryCommandBuffer;
    params.color_attachment_index = vulkan_info.fColorAttachmentIndex;
    params.compatible_render_pass = vulkan_info.fCompatibleRenderPass;
    params.format = vulkan_info.fFormat;

    params.clip_left = clip_bounds_.fLeft;
    params.clip_top = clip_bounds_.fTop;
    params.clip_right = clip_bounds_.fRight;
    params.clip_bottom = clip_bounds_.fBottom;

    SetColorSpace(&params);

    FunctorData& data = Allocator::Get()->get(functor_);
    OverlaysManager::ScopedDraw scoped_draw(*overlays_manager_, data, params);
    data.functor_callbacks->draw_vk(functor_, data.data, &params);
  }

 private:
  raw_ptr<OverlaysManager> overlays_manager_;
  int functor_;
  int scroll_x_;
  int scroll_y_;
  const SkMatrix matrix_;
  const SkIRect clip_bounds_;
  const SkImageInfo image_info_;
};

class FunctorDrawable : public SkDrawable {
 public:
  FunctorDrawable(OverlaysManager* overlays_manager,
                  int functor,
                  int scroll_x,
                  int scroll_y,
                  int width,
                  int height)
      : overlays_manager_(overlays_manager),
        functor_(functor),
        scroll_x_(scroll_x),
        scroll_y_(scroll_y),
        width_(width),
        height_(height) {}

 protected:
  SkRect onGetBounds() override { return SkRect::MakeWH(width_, height_); }
  void onDraw(SkCanvas*) override { NOTREACHED(); }

  std::unique_ptr<GpuDrawHandler> onSnapGpuDrawHandler(
      GrBackendApi backend_api,
      const SkMatrix& matrix,
      const SkIRect& clip_bounds,
      const SkImageInfo& image_info) override {
    CHECK_EQ(backend_api, GrBackendApi::kVulkan);
    LOG(INFO) << "FunctorDrawable VkFunctorDrawHandler created for functor " << functor_;
    return std::make_unique<VkFunctorDrawHandler>(overlays_manager_, functor_,
                                                  scroll_x_, scroll_y_, matrix,
                                                  clip_bounds, image_info);
  }

 private:
  raw_ptr<OverlaysManager> overlays_manager_;
  int functor_;
  int scroll_x_;
  int scroll_y_;
  int width_;
  int height_;
};

class MyDrawFunctorVulkan : public MyDrawFunctor {
 public:
  MyDrawFunctorVulkan();
  ~MyDrawFunctorVulkan() override;

  void ResizeSurface(JNIEnv* env, int width, int height) override;
  base::android::ScopedJavaLocalRef<jintArray> Draw(
      JNIEnv* env,
      int width,
      int height,
      int scroll_x,
      int scroll_y,
      jboolean readback_quadrants) override;
  void DoCreateContext(JNIEnv* env, int width, int height) override;
  void DestroyContext() override;
  void CurrentFunctorChanged() override;

 private:
  void MaybeCallFunctorInitVk();

  std::unique_ptr<gpu::VulkanSurface> vulkan_surface_;
  sk_sp<GrDirectContext> gr_context_;
  std::vector<sk_sp<SkSurface>> sk_surfaces_;
};

MyDrawFunctorVulkan::MyDrawFunctorVulkan() {}

MyDrawFunctorVulkan::~MyDrawFunctorVulkan() {
  DestroyContext();
}

void MyDrawFunctorVulkan::ResizeSurface(JNIEnv* env, int width, int height) {
  DCHECK(vulkan_surface_);
  CHECK(vulkan_surface_->Reshape(
      gfx::Size(width, height), gfx::OverlayTransform::OVERLAY_TRANSFORM_NONE));
  LOG(INFO) << "crWebView ResizeSurface STEP000 sk_surfaces_ size " << sk_surfaces_.size();
  sk_surfaces_.clear();
  LOG(INFO) << "crWebView ResizeSurface STEP001 sk_surfaces_ size " << sk_surfaces_.size();
  sk_surfaces_.resize(vulkan_surface_->swap_chain()->num_images());
  LOG(INFO) << "crWebView ResizeSurface vulkan_surface_->swap_chain()->num_images " << vulkan_surface_->swap_chain()->num_images() << " sk_surfaces_ size " << sk_surfaces_.size();
}

base::android::ScopedJavaLocalRef<jintArray> MyDrawFunctorVulkan::Draw(
    JNIEnv* env,
    int width,
    int height,
    int scroll_x,
    int scroll_y,
    jboolean readback_quadrants) {
  int results[] = {0, 0, 0, 0};
  if (!current_functor_) {
    LOG(ERROR) << "Draw failed no functor:" << current_functor_;
    return readback_quadrants ? base::android::ToJavaIntArray(env, results)
                              : nullptr;
  }

  {
    gpu::VulkanSwapChain::ScopedWrite scoped_write(
        vulkan_surface_->swap_chain());
    CHECK(scoped_write.success());

    auto& sk_surface = sk_surfaces_[scoped_write.image_index()];
    if (!sk_surface) {
      SkSurfaceProps surface_props(0, kRGB_H_SkPixelGeometry);
      const auto surface_format = vulkan_surface_->surface_format().format;
      DCHECK(surface_format == VK_FORMAT_B8G8R8A8_UNORM ||
             surface_format == VK_FORMAT_R8G8B8A8_UNORM);
      GrVkImageInfo vk_image_info;
      vk_image_info.fImage = scoped_write.image();
      vk_image_info.fImageTiling = VK_IMAGE_TILING_OPTIMAL;
      vk_image_info.fImageLayout = scoped_write.image_layout();
      vk_image_info.fFormat = surface_format;
      vk_image_info.fImageUsageFlags = scoped_write.image_usage();
      vk_image_info.fSampleCount = 1;
      vk_image_info.fLevelCount = 1;
      vk_image_info.fCurrentQueueFamily = VK_QUEUE_FAMILY_IGNORED;
      vk_image_info.fProtected = GrProtected::kNo;
      const auto& vk_image_size = vulkan_surface_->image_size();
      auto render_target = GrBackendRenderTargets::MakeVk(
          vk_image_size.width(), vk_image_size.height(), vk_image_info);

      auto sk_color_type = surface_format == VK_FORMAT_B8G8R8A8_UNORM
                               ? kBGRA_8888_SkColorType
                               : kRGBA_8888_SkColorType;
      sk_surface = SkSurfaces::WrapBackendRenderTarget(
          gr_context_.get(), render_target, kTopLeft_GrSurfaceOrigin,
          sk_color_type, gfx::ColorSpace::CreateSRGB().ToSkColorSpace(),
          &surface_props);
    } else {
      auto backend = SkSurfaces::GetBackendRenderTarget(
          sk_surface.get(), SkSurfaces::BackendHandleAccess::kFlushRead);
      GrBackendRenderTargets::SetVkImageLayout(&backend,
                                               scoped_write.image_layout());
    }

    {
      VkSemaphore vk_semaphore = scoped_write.begin_semaphore();
      DCHECK(vk_semaphore != VK_NULL_HANDLE);
      GrBackendSemaphore begin_semaphore =
          GrBackendSemaphores::MakeVk(vk_semaphore);
      bool result = sk_surface->wait(1, &begin_semaphore,
                                     /*deleteSemaphoresAfterWait=*/false);
      CHECK(result);
    }

    auto functor_drawable = sk_make_sp<FunctorDrawable>(
        &overlays_manager_, current_functor_, scroll_x, scroll_y,
        vulkan_surface_->image_size().width(),
        vulkan_surface_->image_size().height());
    sk_surface->getCanvas()->drawDrawable(functor_drawable.get());

    if (readback_quadrants) {
      int quarter_width = width / 4;
      int quarter_height = height / 4;
      SkBitmap bitmap;
      bitmap.allocN32Pixels(1, 1);

      CHECK(sk_surface->readPixels(bitmap, quarter_width, quarter_height));
      results[0] = (int)bitmap.getColor(0, 0);
      CHECK(sk_surface->readPixels(bitmap, quarter_width * 3, quarter_height));
      results[1] = (int)bitmap.getColor(0, 0);
      CHECK(sk_surface->readPixels(bitmap, quarter_width, quarter_height * 3));
      results[2] = (int)bitmap.getColor(0, 0);
      CHECK(sk_surface->readPixels(bitmap, quarter_width * 3,
                                   quarter_height * 3));
      results[3] = (int)bitmap.getColor(0, 0);
    }

    {
      GrBackendSemaphore end_semaphore =
          GrBackendSemaphores::MakeVk(scoped_write.end_semaphore());
      GrFlushInfo flush_info = {
          .fNumSemaphores = 1,
          .fSignalSemaphores = &end_semaphore,
      };
      uint32_t queue_index = device_queue_->GetVulkanQueueIndex();
      skgpu::MutableTextureState state =
          skgpu::MutableTextureStates::MakeVulkan(
              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, queue_index);
      GrSemaphoresSubmitted submitted =
          gr_context_->flush(sk_surface.get(), flush_info, &state);
      CHECK_EQ(GrSemaphoresSubmitted::kYes, submitted);
    }
    gr_context_->submit(GrSyncCpu::kNo);
  } 

  vulkan_surface_->SwapBuffers(base::DoNothingAs<void(const gfx::PresentationFeedback&)>());
  return readback_quadrants ? base::android::ToJavaIntArray(env, results) : nullptr;
}
void MyDrawFunctorVulkan::DoCreateContext(JNIEnv* env, int width, int height) {

  vulkan_surface_ = vulkan_implementation_->CreateViewSurface(
      native_window_.a_native_window());
  CHECK(vulkan_surface_);
  CHECK(vulkan_surface_->Initialize(device_queue_.get(),
                                    gpu::VulkanSurface::FORMAT_RGBA_32));
  ResizeSurface(env, width, height);
  LOG(INFO) << "crWebView DoCreate vulkan_surface_ " << vulkan_surface_;

  skgpu::VulkanBackendContext backend_context;
  backend_context.fInstance = device_queue_->GetVulkanInstance();
  backend_context.fPhysicalDevice = device_queue_->GetVulkanPhysicalDevice();
  backend_context.fDevice = device_queue_->GetVulkanDevice();
  backend_context.fQueue = device_queue_->GetVulkanQueue();
  backend_context.fGraphicsQueueIndex = device_queue_->GetVulkanQueueIndex();
  backend_context.fMaxAPIVersion = vulkan_implementation_->GetVulkanInstance()
                                       ->vulkan_info()
                                       .used_api_version;
  backend_context.fMemoryAllocator = gpu::CreateSkiaVulkanMemoryAllocator(device_queue_.get());

  skgpu::VulkanGetProc get_proc = [](const char* proc_name, VkInstance instance,
                                     VkDevice device) {
    if (device) {
      return vkGetDeviceProcAddr(device, proc_name);
    }
    return vkGetInstanceProcAddr(instance, proc_name);
  };

  const auto& instance_extensions = vulkan_implementation_->GetVulkanInstance()
                                        ->vulkan_info()
                                        .enabled_instance_extensions;

  std::vector<const char*> device_extensions;
  device_extensions.reserve(device_queue_->enabled_extensions().size());
  for (const auto& extension : device_queue_->enabled_extensions())
    device_extensions.push_back(extension.data());
  skgpu::VulkanExtensions vk_extensions;
  vk_extensions.init(get_proc,
                     vulkan_implementation_->GetVulkanInstance()->vk_instance(),
                     device_queue_->GetVulkanPhysicalDevice(),
                     (uint32_t)instance_extensions.size(), instance_extensions.data(),
                     (uint32_t)device_extensions.size(), device_extensions.data());
  backend_context.fVkExtensions = &vk_extensions;
  backend_context.fDeviceFeatures2 =
      &device_queue_->enabled_device_features_2();
  backend_context.fGetProc = get_proc;
  backend_context.fProtectedContext = GrProtected::kNo;

  //GrContextOptions options;
  //gr_context_ = GrDirectContexts::MakeVulkan(backend_context, options);
  gr_context_ = GrDirectContexts::MakeVulkan(backend_context);

  MaybeCallFunctorInitVk();
}

void MyDrawFunctorVulkan::DestroyContext() {
  if (java_surface_.IsEmpty()) {
    return;
  }

  if (current_functor_) {
    FunctorData& data = Allocator::Get()->get(current_functor_);
    data.functor_callbacks->on_context_destroyed(data.functor, data.data);
    current_functor_ = 0;
  }

  sk_surfaces_.clear();
  vulkan_surface_->Finish();
  vulkan_surface_->Destroy();
  vulkan_surface_.reset();

  native_window_ = nullptr;
  java_surface_ = nullptr;
}

void MyDrawFunctorVulkan::CurrentFunctorChanged() {
  MaybeCallFunctorInitVk();
}

void MyDrawFunctorVulkan::MaybeCallFunctorInitVk() {
  if (!vulkan_surface_ || !current_functor_)
    return;

  AwDrawFn_InitVkParams params{kAwDrawFnVersion};
  params.instance = device_queue_->GetVulkanInstance();
  params.physical_device = device_queue_->GetVulkanPhysicalDevice();
  params.device = device_queue_->GetVulkanDevice();
  params.queue = device_queue_->GetVulkanQueue();
  params.graphics_queue_index = device_queue_->GetVulkanQueueIndex();
  const gpu::VulkanInfo& vulkan_info =
      vulkan_implementation_->GetVulkanInstance()->vulkan_info();
  params.api_version = vulkan_info.used_api_version;
  params.enabled_instance_extension_names =
      vulkan_info.enabled_instance_extensions.data();
  params.enabled_instance_extension_names_length =
      (uint32_t)vulkan_info.enabled_instance_extensions.size();

  std::vector<const char*> enabled_device_extension_names;
  for (const auto& extension_string_piece :
       device_queue_->enabled_extensions()) {
    enabled_device_extension_names.push_back(extension_string_piece.data());
  }
  params.enabled_device_extension_names = enabled_device_extension_names.data();
  params.enabled_device_extension_names_length =
      (uint32_t)enabled_device_extension_names.size();
  VkPhysicalDeviceFeatures2 device_features_2 =
      device_queue_->enabled_device_features_2();
  params.device_features_2 = &device_features_2;

  FunctorData& data = Allocator::Get()->get(current_functor_);
  data.functor_callbacks->init_vk(data.functor, data.data, &params);
}

}  // namespace


MyDrawFunctor::MyDrawFunctor() = default;

MyDrawFunctor::~MyDrawFunctor() = default;

void MyDrawFunctor::SetSurface(JNIEnv* env,
                                const base::android::JavaRef<jobject>& surface,
                                int width,
                                int height) {
  LOG(INFO) << "crWebView MyDrawFunctor SetSurface called width " << width << " height " << height;
  if (!java_surface_.IsEmpty()) {
    LOG(INFO) << "crWebView MyDrawFunctor SetSurface SETP 001";
    DestroyContext();
  }
  if (!surface.is_null()) {
    LOG(INFO) << "crWebView MyDrawFunctor SetSurface SETP 002";
    CreateContext(env, surface, width, height);
  }
}

void MyDrawFunctor::SetOverlaysSurface(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& surface) {
  //overlays_manager_.SetSurface(current_functor_, env, surface);
}

void MyDrawFunctor::Sync(JNIEnv* env, int functor, bool apply_force_dark) {
  bool functor_changing = current_functor_ != functor;
  if (current_functor_ && functor_changing) {
    FunctorData& data = Allocator::Get()->get(current_functor_);
    overlays_manager_.RemoveOverlays(data);
    data.functor_callbacks->on_context_destroyed(data.functor, data.data);
    Allocator::Get()->MarkReleasedByManager(current_functor_);
  }
  current_functor_ = functor;

  FunctorData& data = Allocator::Get()->get(current_functor_);
  AwDrawFn_OnSyncParams params{kAwDrawFnVersion, apply_force_dark};
  data.functor_callbacks->on_sync(current_functor_, data.data, &params);

  if (functor_changing)
    CurrentFunctorChanged();
}

void MyDrawFunctor::CreateContext(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& surface,
    int width,
    int height) {
  LOG(INFO) << "crWebView MyDrawFunctor CreateContext called";
  java_surface_ = gl::ScopedJavaSurface(surface, /*auto_release=*/false);
  if (!java_surface_.IsValid()) {
    return;
  }

  LOG(INFO) << "*crWebView MyDrawFunctor CreateContext called STEP 001 " << this ;
  native_window_ = gl::ScopedANativeWindow(java_surface_);
  LOG(INFO) << "*crWebView MyDrawFunctor CreateContext called STEP 002 native_window_ ";
  CHECK(native_window_);

  DoCreateContext(env, width, height);
}

static jlong JNI_MyDrawFunctor_GetDrawFnFunctionTable(JNIEnv* env,
                                                       jboolean use_vulkan) {
  vulkan_implementation_ = gpu::CreateVulkanImplementation(
      /*use_swiftshader=*/false,
      /*allow_protected_memory*/ false);
  CHECK(vulkan_implementation_);
  CHECK(
      vulkan_implementation_->InitializeVulkanInstance(/*using_surface=*/true));
  uint32_t flags = gpu::VulkanDeviceQueue::GRAPHICS_QUEUE_FLAG |
                   gpu::VulkanDeviceQueue::PRESENTATION_SUPPORT_QUEUE_FLAG;
  device_queue_ =
      gpu::CreateVulkanDeviceQueue(vulkan_implementation_.get(), flags,
                                   /*gpu_info=*/nullptr, 0, true);
  CHECK(device_queue_);

  draw_fn::SetDrawFnUseVulkan(use_vulkan);
  return reinterpret_cast<intptr_t>(draw_fn::GetDrawFnFunctionTable());
}

static jlong JNI_MyDrawFunctor_Init(JNIEnv* env, jboolean use_vulkan) {
  MyDrawFunctor* manager = new draw_fn::MyDrawFunctorVulkan;
  return reinterpret_cast<intptr_t>(manager);
}

}  // namespace draw_fn
