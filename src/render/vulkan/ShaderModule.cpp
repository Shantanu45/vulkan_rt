#include "ShaderModule.hpp"

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace vulkan_rt::render::vulkan
{
namespace
{
std::string vulkan_result_message(const char *operation, VkResult result)
{
  return std::string{operation} + " failed with VkResult " + std::to_string(static_cast<int>(result));
}

void throw_if_failed(VkResult result, const char *operation)
{
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error(vulkan_result_message(operation, result));
  }
}
}

ShaderModule::ShaderModule(const VulkanDevice &device, const std::filesystem::path &spirv_path)
{
  create(device, spirv_path);
}

ShaderModule::~ShaderModule()
{
  destroy();
}

ShaderModule::ShaderModule(ShaderModule &&other) noexcept
  : device_(other.device_)
  , module_(other.module_)
  , path_(std::move(other.path_))
{
  other.device_ = VK_NULL_HANDLE;
  other.module_ = VK_NULL_HANDLE;
}

ShaderModule &ShaderModule::operator=(ShaderModule &&other) noexcept
{
  if(this != &other)
  {
    destroy();

    device_ = other.device_;
    module_ = other.module_;
    path_ = std::move(other.path_);

    other.device_ = VK_NULL_HANDLE;
    other.module_ = VK_NULL_HANDLE;
  }

  return *this;
}

VkShaderModule ShaderModule::module() const
{
  return module_;
}

const std::filesystem::path &ShaderModule::path() const
{
  return path_;
}

std::vector<char> ShaderModule::read_spirv_file(const std::filesystem::path &spirv_path)
{
  if(!std::filesystem::exists(spirv_path))
  {
    throw std::runtime_error("SPIR-V shader file does not exist: " + spirv_path.string());
  }

  const auto file_size = std::filesystem::file_size(spirv_path);
  if(file_size == 0)
  {
    throw std::runtime_error("SPIR-V shader file is empty: " + spirv_path.string());
  }

  if(file_size % sizeof(std::uint32_t) != 0)
  {
    throw std::runtime_error("SPIR-V shader file size is not 32-bit aligned: " + spirv_path.string());
  }

  std::vector<char> bytes(static_cast<std::size_t>(file_size));
  std::ifstream file{spirv_path, std::ios::binary};
  if(!file)
  {
    throw std::runtime_error("Failed to open SPIR-V shader file: " + spirv_path.string());
  }

  file.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  if(!file)
  {
    throw std::runtime_error("Failed to read SPIR-V shader file: " + spirv_path.string());
  }

  return bytes;
}

void ShaderModule::create(const VulkanDevice &device, const std::filesystem::path &spirv_path)
{
  const auto bytes = read_spirv_file(spirv_path);

  VkShaderModuleCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = bytes.size();
  create_info.pCode = reinterpret_cast<const std::uint32_t *>(bytes.data());

  throw_if_failed(vkCreateShaderModule(device.device(), &create_info, nullptr, &module_), "vkCreateShaderModule");

  device_ = device.device();
  path_ = spirv_path;
}

void ShaderModule::destroy()
{
  if(module_ != VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(device_, module_, nullptr);
    module_ = VK_NULL_HANDLE;
  }

  device_ = VK_NULL_HANDLE;
  path_.clear();
}
}
