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

void UiLayer::draw(const UiStats &stats)
{
  last_stats_ = stats;
  if(!initialized_)
  {
    return;
  }

  ImGui::SetNextWindowPos(ImVec2{12.0F, 12.0F}, ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowBgAlpha(0.80F);
  if(ImGui::Begin("Viewport"))
  {
    ImGui::Text("FPS %.1f", stats.fps);
    ImGui::Text("Frame %.0f ms", stats.frame_time_ms);
    ImGui::Text("Frame index %llu", static_cast<unsigned long long>(stats.frame_index));
    ImGui::Text("Extent %dx%d", stats.framebuffer_extent.width, stats.framebuffer_extent.height);
  }
  ImGui::End();
}

void UiLayer::end_frame()
{
  if(initialized_)
  {
    ImGui::Render();
  }
}

const UiStats &UiLayer::last_stats() const { return last_stats_; }
}// namespace vulkan_rt::app
