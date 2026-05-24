#pragma once

#include <volk.h>

#include "VulkanDevice.hpp"

#include <filesystem>
#include <vector>

namespace vulkan_rt::render::vulkan
{
class ShaderModule
{
public:
  ShaderModule(const VulkanDevice &device, const std::filesystem::path &spirv_path);
  ~ShaderModule();

  [[nodiscard]] VkShaderModule module() const;
  [[nodiscard]] const std::filesystem::path &path() const;

  ShaderModule(const ShaderModule &) = delete;
  ShaderModule &operator=(const ShaderModule &) = delete;

  ShaderModule(ShaderModule &&other) noexcept;
  ShaderModule &operator=(ShaderModule &&other) noexcept;

private:
  static std::vector<char> read_spirv_file(const std::filesystem::path &spirv_path);

  void create(const VulkanDevice &device, const std::filesystem::path &spirv_path);
  void destroy();

  VkDevice device_ = VK_NULL_HANDLE;
  VkShaderModule module_ = VK_NULL_HANDLE;
  std::filesystem::path path_;
};
}
