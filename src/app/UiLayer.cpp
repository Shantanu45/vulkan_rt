#include "app/UiLayer.hpp"

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

namespace vulkan_rt::app {
UiStats make_ui_stats(const engine::FrameStats &frame_stats, Extent framebuffer_extent)
{
  UiStats stats;
  stats.frame_time_ms = frame_stats.frame_time_ms;
  stats.fps = frame_stats.fps;
  stats.frame_index = frame_stats.frame_index;
  stats.framebuffer_extent = framebuffer_extent;
  return stats;
}

UiLayer::UiLayer() = default;

UiLayer::~UiLayer()
{
  if(initialized_)
  {
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
  }
}

void UiLayer::initialize(SDL_Window *window)
{
  if(initialized_)
  {
    return;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplSDL3_InitForVulkan(window);
  initialized_ = true;
}

void UiLayer::handle_event(const SDL_Event &event)
{
  if(initialized_)
  {
    ImGui_ImplSDL3_ProcessEvent(&event);
  }
}

void UiLayer::begin_frame()
{
  if(!initialized_)
  {
    return;
  }

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

UiActions UiLayer::draw(const UiStats &stats, render::RendererSettings &settings)
{
  last_stats_ = stats;
  UiActions actions;
  if(!initialized_)
  {
    return actions;
  }

  ImGui::SetNextWindowPos(ImVec2{12.0F, 12.0F}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.80F);
  if(ImGui::Begin("Viewport"))
  {
    ImGui::Text("FPS %.1f", stats.fps);
    ImGui::Text("Frame %.0f ms", stats.frame_time_ms);
    ImGui::Text("Frame index %llu", static_cast<unsigned long long>(stats.frame_index));
    ImGui::Text("Extent %dx%d", stats.framebuffer_extent.width, stats.framebuffer_extent.height);
    ImGui::Separator();

    int max_bounces = static_cast<int>(settings.max_bounces);
    if(ImGui::SliderInt("Max bounces", &max_bounces, 1, 8))
    {
      settings.max_bounces = static_cast<std::uint32_t>(max_bounces);
      actions.renderer_settings_changed = true;
    }

    if(ImGui::Checkbox("Direct lighting", &settings.direct_lighting_enabled))
    {
      actions.renderer_settings_changed = true;
    }

    if(ImGui::Checkbox("Jitter", &settings.jitter_enabled))
    {
      actions.renderer_settings_changed = true;
    }

    if(ImGui::SliderFloat("Exposure", &settings.exposure, 0.05F, 5.0F, "%.2f"))
    {
      actions.renderer_settings_changed = true;
    }

    if(ImGui::Button("Reset accumulation"))
    {
      actions.reset_accumulation_requested = true;
    }
  }
  ImGui::End();
  return actions;
}

void UiLayer::end_frame()
{
  if(initialized_)
  {
    ImGui::Render();
  }
}

const UiStats &UiLayer::last_stats() const { return last_stats_; }

bool UiLayer::wants_keyboard() const
{
  return initialized_ && ImGui::GetIO().WantCaptureKeyboard;
}

bool UiLayer::wants_mouse() const
{
  return initialized_ && ImGui::GetIO().WantCaptureMouse;
}
}// namespace vulkan_rt::app
