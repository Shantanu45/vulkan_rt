#include "VulkanSwapchain.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vulkan_rt::render::vulkan
{
namespace
{
struct SwapchainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

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

SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
  SwapchainSupportDetails details;

  throw_if_failed(
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities),
    "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

  std::uint32_t format_count = 0;
  throw_if_failed(
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr),
    "vkGetPhysicalDeviceSurfaceFormatsKHR");

  details.formats.resize(format_count);
  if(format_count > 0)
  {
    throw_if_failed(
      vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, details.formats.data()),
      "vkGetPhysicalDeviceSurfaceFormatsKHR");
  }

  std::uint32_t present_mode_count = 0;
  throw_if_failed(
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr),
    "vkGetPhysicalDeviceSurfacePresentModesKHR");

  details.present_modes.resize(present_mode_count);
  if(present_mode_count > 0)
  {
    throw_if_failed(
      vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, details.present_modes.data()),
      "vkGetPhysicalDeviceSurfacePresentModesKHR");
  }

  return details;
}

VkSurfaceFormatKHR choose_surface_format(std::span<const VkSurfaceFormatKHR> formats)
{
  if(formats.empty())
  {
    throw std::runtime_error("Vulkan surface reported no supported formats.");
  }

  const auto preferred =
    std::find_if(formats.begin(), formats.end(), [](const VkSurfaceFormatKHR &format) {
      return format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });

  return preferred != formats.end() ? *preferred : formats.front();
}

VkPresentModeKHR choose_present_mode(std::span<const VkPresentModeKHR> present_modes)
{
  if(present_modes.empty())
  {
    throw std::runtime_error("Vulkan surface reported no supported present modes.");
  }

  const auto mailbox =
    std::find(present_modes.begin(), present_modes.end(), VK_PRESENT_MODE_MAILBOX_KHR);
  if(mailbox != present_modes.end())
  {
    return VK_PRESENT_MODE_MAILBOX_KHR;
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR &capabilities, SwapchainExtent requested_extent)
{
  if(capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
  {
    return capabilities.currentExtent;
  }

  VkExtent2D extent{
    .width = requested_extent.width,
    .height = requested_extent.height,
  };

  extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
  extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
  return extent;
}
}

VulkanSwapchain::VulkanSwapchain(
  const VulkanContext &context, const VulkanDevice &device, SwapchainExtent requested_extent)
{
  create(context, device, requested_extent);
}

VulkanSwapchain::~VulkanSwapchain()
{
  destroy();
}

VulkanSwapchain::VulkanSwapchain(VulkanSwapchain &&other) noexcept
  : device_(other.device_)
  , swapchain_(other.swapchain_)
  , image_format_(other.image_format_)
  , extent_(other.extent_)
  , images_(std::move(other.images_))
  , image_views_(std::move(other.image_views_))
{
  other.device_ = VK_NULL_HANDLE;
  other.swapchain_ = VK_NULL_HANDLE;
  other.image_format_ = VK_FORMAT_UNDEFINED;
  other.extent_ = {};
}

VulkanSwapchain &VulkanSwapchain::operator=(VulkanSwapchain &&other) noexcept
{
  if(this != &other)
  {
    destroy();

    device_ = other.device_;
    swapchain_ = other.swapchain_;
    image_format_ = other.image_format_;
    extent_ = other.extent_;
    images_ = std::move(other.images_);
    image_views_ = std::move(other.image_views_);

    other.device_ = VK_NULL_HANDLE;
    other.swapchain_ = VK_NULL_HANDLE;
    other.image_format_ = VK_FORMAT_UNDEFINED;
    other.extent_ = {};
  }

  return *this;
}

VkSwapchainKHR VulkanSwapchain::swapchain() const
{
  return swapchain_;
}

VkFormat VulkanSwapchain::image_format() const
{
  return image_format_;
}

VkExtent2D VulkanSwapchain::extent() const
{
  return extent_;
}

std::span<const VkImage> VulkanSwapchain::images() const
{
  return std::span<const VkImage>{images_};
}

std::span<const VkImageView> VulkanSwapchain::image_views() const
{
  return std::span<const VkImageView>{image_views_};
}

void VulkanSwapchain::create(
  const VulkanContext &context, const VulkanDevice &device, SwapchainExtent requested_extent)
{
  device_ = device.device();

  const auto support = query_swapchain_support(device.physical_device(), context.surface());
  const auto surface_format = choose_surface_format(support.formats);
  const auto present_mode = choose_present_mode(support.present_modes);
  extent_ = choose_extent(support.capabilities, requested_extent);
  image_format_ = surface_format.format;

  std::uint32_t image_count = support.capabilities.minImageCount + 1;
  if(support.capabilities.maxImageCount > 0)
  {
    image_count = std::min(image_count, support.capabilities.maxImageCount);
  }

  const auto &queue_families = device.queue_families();
  const std::array queue_family_indices{
    queue_families.graphics.value(),
    queue_families.present.value(),
  };

  VkSwapchainCreateInfoKHR create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = context.surface();
  create_info.minImageCount = image_count;
  create_info.imageFormat = surface_format.format;
  create_info.imageColorSpace = surface_format.colorSpace;
  create_info.imageExtent = extent_;
  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  create_info.preTransform = support.capabilities.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = present_mode;
  create_info.clipped = VK_TRUE;
  create_info.oldSwapchain = VK_NULL_HANDLE;

  if(queue_families.graphics != queue_families.present)
  {
    create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    create_info.queueFamilyIndexCount = static_cast<std::uint32_t>(queue_family_indices.size());
    create_info.pQueueFamilyIndices = queue_family_indices.data();
  }
  else
  {
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  throw_if_failed(vkCreateSwapchainKHR(device_, &create_info, nullptr, &swapchain_), "vkCreateSwapchainKHR");

  std::uint32_t actual_image_count = 0;
  throw_if_failed(vkGetSwapchainImagesKHR(device_, swapchain_, &actual_image_count, nullptr), "vkGetSwapchainImagesKHR");

  images_.resize(actual_image_count);
  throw_if_failed(
    vkGetSwapchainImagesKHR(device_, swapchain_, &actual_image_count, images_.data()), "vkGetSwapchainImagesKHR");

  image_views_.reserve(images_.size());
  for(const auto image : images_)
  {
    VkImageViewCreateInfo image_view_info{};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = image_format_;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;

    VkImageView image_view = VK_NULL_HANDLE;
    throw_if_failed(vkCreateImageView(device_, &image_view_info, nullptr, &image_view), "vkCreateImageView");
    image_views_.push_back(image_view);
  }
}

void VulkanSwapchain::destroy()
{
  if(device_ == VK_NULL_HANDLE)
  {
    return;
  }

  for(const auto image_view : image_views_)
  {
    vkDestroyImageView(device_, image_view, nullptr);
  }
  image_views_.clear();
  images_.clear();

  if(swapchain_ != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }

  image_format_ = VK_FORMAT_UNDEFINED;
  extent_ = {};
  device_ = VK_NULL_HANDLE;
}
}
