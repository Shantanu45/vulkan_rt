#include "VulkanImage.hpp"

#include <stdexcept>
#include <string>

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

VulkanImage::VulkanImage(
  const VulkanDevice &device, const VulkanAllocator &allocator, const ImageCreateInfo &create_info)
{
  create(device, allocator, create_info);
}

VulkanImage::~VulkanImage()
{
  destroy();
}

VulkanImage::VulkanImage(VulkanImage &&other) noexcept
  : allocator_(other.allocator_)
  , device_(other.device_)
  , image_(other.image_)
  , image_view_(other.image_view_)
  , allocation_(other.allocation_)
  , format_(other.format_)
  , extent_(other.extent_)
{
  other.allocator_ = VK_NULL_HANDLE;
  other.device_ = VK_NULL_HANDLE;
  other.image_ = VK_NULL_HANDLE;
  other.image_view_ = VK_NULL_HANDLE;
  other.allocation_ = VK_NULL_HANDLE;
  other.format_ = VK_FORMAT_UNDEFINED;
  other.extent_ = {};
}

VulkanImage &VulkanImage::operator=(VulkanImage &&other) noexcept
{
  if(this != &other)
  {
    destroy();

    allocator_ = other.allocator_;
    device_ = other.device_;
    image_ = other.image_;
    image_view_ = other.image_view_;
    allocation_ = other.allocation_;
    format_ = other.format_;
    extent_ = other.extent_;

    other.allocator_ = VK_NULL_HANDLE;
    other.device_ = VK_NULL_HANDLE;
    other.image_ = VK_NULL_HANDLE;
    other.image_view_ = VK_NULL_HANDLE;
    other.allocation_ = VK_NULL_HANDLE;
    other.format_ = VK_FORMAT_UNDEFINED;
    other.extent_ = {};
  }

  return *this;
}

VkImage VulkanImage::image() const
{
  return image_;
}

VkImageView VulkanImage::image_view() const
{
  return image_view_;
}

VkFormat VulkanImage::format() const
{
  return format_;
}

VkExtent2D VulkanImage::extent() const
{
  return extent_;
}

void VulkanImage::create(const VulkanDevice &device, const VulkanAllocator &allocator, const ImageCreateInfo &create_info)
{
  if(create_info.width == 0 || create_info.height == 0)
  {
    throw std::runtime_error("Cannot create zero-sized Vulkan image.");
  }

  if(create_info.usage == 0)
  {
    throw std::runtime_error("Cannot create Vulkan image without usage flags.");
  }

  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = create_info.format;
  image_info.extent = VkExtent3D{.width = create_info.width, .height = create_info.height, .depth = 1};
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = create_info.usage;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VmaAllocationCreateInfo allocation_info{};
  allocation_info.usage = create_info.memory_usage;

  throw_if_failed(
    vmaCreateImage(allocator.allocator(), &image_info, &allocation_info, &image_, &allocation_, nullptr),
    "vmaCreateImage");

  VkImageViewCreateInfo view_info{};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image_;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = create_info.format;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  throw_if_failed(vkCreateImageView(device.device(), &view_info, nullptr, &image_view_), "vkCreateImageView");

  allocator_ = allocator.allocator();
  device_ = device.device();
  format_ = create_info.format;
  extent_ = VkExtent2D{.width = create_info.width, .height = create_info.height};
}

void VulkanImage::destroy()
{
  if(image_view_ != VK_NULL_HANDLE)
  {
    vkDestroyImageView(device_, image_view_, nullptr);
    image_view_ = VK_NULL_HANDLE;
  }

  if(image_ != VK_NULL_HANDLE)
  {
    vmaDestroyImage(allocator_, image_, allocation_);
    image_ = VK_NULL_HANDLE;
    allocation_ = VK_NULL_HANDLE;
  }

  allocator_ = VK_NULL_HANDLE;
  device_ = VK_NULL_HANDLE;
  format_ = VK_FORMAT_UNDEFINED;
  extent_ = {};
}
}
