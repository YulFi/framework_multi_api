#include "Texture.h"
#include "Renderer.h"
#include "../Logger.h"
#include <stdexcept>

namespace VK
{

Texture::Texture(VkDevice device, VkPhysicalDevice physicalDevice, Renderer* renderer)
    : m_renderer(renderer)
    , m_device(device)
    , m_physicalDevice(physicalDevice)
    , m_image(VK_NULL_HANDLE)
    , m_imageMemory(VK_NULL_HANDLE)
    , m_imageView(VK_NULL_HANDLE)
    , m_sampler(VK_NULL_HANDLE)
    , m_descriptorSet(VK_NULL_HANDLE)
    , m_width(0)
    , m_height(0)
    , m_format(TextureFormat::RGBA)
    , m_vkFormat(VK_FORMAT_R8G8B8A8_UNORM)
    , m_minFilter(TextureFilter::Linear)
    , m_magFilter(TextureFilter::Linear)
    , m_wrapS(TextureWrap::Repeat)
    , m_wrapT(TextureWrap::Repeat)
{
    LOG_DEBUG("[Vulkan] Texture created");
}

Texture::~Texture()
{
    cleanup();
}

Texture::Texture(Texture&& other) noexcept
    : m_renderer(other.m_renderer)
    , m_device(other.m_device)
    , m_physicalDevice(other.m_physicalDevice)
    , m_image(other.m_image)
    , m_imageMemory(other.m_imageMemory)
    , m_imageView(other.m_imageView)
    , m_sampler(other.m_sampler)
    , m_width(other.m_width)
    , m_height(other.m_height)
    , m_format(other.m_format)
    , m_vkFormat(other.m_vkFormat)
    , m_minFilter(other.m_minFilter)
    , m_magFilter(other.m_magFilter)
    , m_wrapS(other.m_wrapS)
    , m_wrapT(other.m_wrapT)
{
    other.m_device = VK_NULL_HANDLE;
    other.m_image = VK_NULL_HANDLE;
    other.m_imageMemory = VK_NULL_HANDLE;
    other.m_imageView = VK_NULL_HANDLE;
    other.m_sampler = VK_NULL_HANDLE;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        cleanup();

        m_renderer = other.m_renderer;
        m_device = other.m_device;
        m_physicalDevice = other.m_physicalDevice;
        m_image = other.m_image;
        m_imageMemory = other.m_imageMemory;
        m_imageView = other.m_imageView;
        m_sampler = other.m_sampler;
        m_width = other.m_width;
        m_height = other.m_height;
        m_format = other.m_format;
        m_vkFormat = other.m_vkFormat;
        m_minFilter = other.m_minFilter;
        m_magFilter = other.m_magFilter;
        m_wrapS = other.m_wrapS;
        m_wrapT = other.m_wrapT;

        other.m_device = VK_NULL_HANDLE;
        other.m_image = VK_NULL_HANDLE;
        other.m_imageMemory = VK_NULL_HANDLE;
        other.m_imageView = VK_NULL_HANDLE;
        other.m_sampler = VK_NULL_HANDLE;
    }
    return *this;
}

void Texture::bind(uint32_t slot)
{
    // In Vulkan, textures are bound via descriptor sets
    // Tell the renderer about this texture so it can bind the descriptor set
    if (m_renderer)
    {
        m_renderer->setCurrentTexture(this);
        //LOG_DEBUG("[Vulkan] Texture set as current (slot: {})", slot);
    }
}

void Texture::unbind()
{
    // No-op for Vulkan
}

void Texture::setData(const void* data, uint32_t width, uint32_t height, TextureFormat format)
{
    if (!data)
    {
        LOG_ERROR("[Vulkan] Cannot set texture data - data is null");
        return;
    }

    // Clean up existing resources if any
    cleanup();

    m_width = width;
    m_height = height;
    m_format = format;
    m_vkFormat = convertTextureFormat(format);

    // Calculate data size
    uint32_t bytesPerPixel = 4; // Default to RGBA
    switch (format)
    {
        case TextureFormat::RGB:   bytesPerPixel = 3; break;
        case TextureFormat::RGBA:  bytesPerPixel = 4; break;
        case TextureFormat::Red:   bytesPerPixel = 1; break;
        case TextureFormat::RG:    bytesPerPixel = 2; break;
        case TextureFormat::Depth: bytesPerPixel = 4; break;
    }
    VkDeviceSize imageSize = width * height * bytesPerPixel;

    // Create staging buffer
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create staging buffer for texture");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, stagingBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS)
    {
        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
        throw std::runtime_error("Failed to allocate staging buffer memory");
    }

    vkBindBufferMemory(m_device, stagingBuffer, stagingBufferMemory, 0);

    // Copy data to staging buffer
    void* mappedData;
    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &mappedData);
    memcpy(mappedData, data, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_device, stagingBufferMemory);

    // Create image
    createImage(width, height, m_vkFormat,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Transition image layout and copy buffer
    transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, width, height);
    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Clean up staging resources
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);

    // Create image view and sampler
    createImageView(m_vkFormat);
    createSampler();

    // Create descriptor set
    if (m_renderer)
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_renderer->getDescriptorPool();
        allocInfo.descriptorSetCount = 1;
        VkDescriptorSetLayout layout = m_renderer->getDescriptorSetLayout();
        allocInfo.pSetLayouts = &layout;

        if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate descriptor set");
        }

        // Update descriptor set
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_imageView;
        imageInfo.sampler = m_sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
        LOG_DEBUG("[Vulkan] Descriptor set created and updated");
    }

    LOG_INFO("[Vulkan] Texture data set ({}x{}, format: {})", width, height, static_cast<int>(format));
}

void Texture::updateData(const void* data, uint32_t xOffset, uint32_t yOffset,
                        uint32_t width, uint32_t height)
{
    LOG_WARNING("[Vulkan] Texture::updateData not fully implemented yet");
    // TODO: Implement partial texture updates using staging buffers
}

void Texture::setFilter(TextureFilter minFilter, TextureFilter magFilter)
{
    m_minFilter = minFilter;
    m_magFilter = magFilter;

    // Recreate sampler with new filter settings
    if (m_sampler != VK_NULL_HANDLE)
    {
        // Queue old sampler for deferred deletion instead of immediate destruction
        if (m_renderer)
        {
            m_renderer->deferDeleteSampler(m_sampler);
        }
        m_sampler = VK_NULL_HANDLE;
    }
    createSampler();

    // Update descriptor set if it exists
    if (m_descriptorSet != VK_NULL_HANDLE && m_renderer)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_imageView;
        imageInfo.sampler = m_sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    }
}

void Texture::setWrap(TextureWrap wrapS, TextureWrap wrapT)
{
    m_wrapS = wrapS;
    m_wrapT = wrapT;

    // Recreate sampler with new wrap settings
    if (m_sampler != VK_NULL_HANDLE)
    {
        // Queue old sampler for deferred deletion instead of immediate destruction
        if (m_renderer)
        {
            m_renderer->deferDeleteSampler(m_sampler);
        }
        m_sampler = VK_NULL_HANDLE;
    }
    createSampler();

    // Update descriptor set if it exists
    if (m_descriptorSet != VK_NULL_HANDLE && m_renderer)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_imageView;
        imageInfo.sampler = m_sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    }
}

void Texture::generateMipmaps()
{
    LOG_WARNING("[Vulkan] Texture::generateMipmaps not implemented yet");
    // TODO: Implement mipmap generation using vkCmdBlitImage
}

void Texture::createImage(uint32_t width, uint32_t height, VkFormat format,
                         VkImageTiling tiling, VkImageUsageFlags usage,
                         VkMemoryPropertyFlags properties)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_device, &imageInfo, nullptr, &m_image) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, m_image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory");
    }

    vkBindImageMemory(m_device, m_image, m_imageMemory, 0);
}

void Texture::createImageView(VkFormat format)
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture image view");
    }
}

void Texture::createSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = convertFilter(m_magFilter);
    samplerInfo.minFilter = convertFilter(m_minFilter);
    samplerInfo.addressModeU = convertWrap(m_wrapS);
    samplerInfo.addressModeV = convertWrap(m_wrapT);
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture sampler");
    }
}

void Texture::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout)
{
    if (!m_renderer)
    {
        LOG_ERROR("[Vulkan] Cannot transition image layout - no renderer");
        return;
    }

    VkCommandBuffer commandBuffer = m_renderer->beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        LOG_ERROR("[Vulkan] Unsupported layout transition");
        return;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    m_renderer->endSingleTimeCommands(commandBuffer);
    LOG_DEBUG("[Vulkan] Image layout transitioned from {} to {}", static_cast<int>(oldLayout), static_cast<int>(newLayout));
}

void Texture::copyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height)
{
    if (!m_renderer)
    {
        LOG_ERROR("[Vulkan] Cannot copy buffer to image - no renderer");
        return;
    }

    VkCommandBuffer commandBuffer = m_renderer->beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    m_renderer->endSingleTimeCommands(commandBuffer);
    LOG_DEBUG("[Vulkan] Buffer copied to image ({}x{})", width, height);
}

uint32_t Texture::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

VkFormat Texture::convertTextureFormat(TextureFormat format) const
{
    switch (format)
    {
        case TextureFormat::RGB:   return VK_FORMAT_R8G8B8_UNORM;
        case TextureFormat::RGBA:  return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::Red:   return VK_FORMAT_R8_UNORM;
        case TextureFormat::RG:    return VK_FORMAT_R8G8_UNORM;
        case TextureFormat::Depth: return VK_FORMAT_D32_SFLOAT;
        default:                   return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

VkFilter Texture::convertFilter(TextureFilter filter) const
{
    switch (filter)
    {
        case TextureFilter::Nearest: return VK_FILTER_NEAREST;
        case TextureFilter::Linear:  return VK_FILTER_LINEAR;
        default:                     return VK_FILTER_LINEAR;
    }
}

VkSamplerAddressMode Texture::convertWrap(TextureWrap wrap) const
{
    switch (wrap)
    {
        case TextureWrap::Repeat:         return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case TextureWrap::ClampToEdge:    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case TextureWrap::ClampToBorder:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case TextureWrap::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default:                          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
}

void Texture::cleanup()
{
    if (m_device != VK_NULL_HANDLE)
    {
        // Wait for device to be idle before destroying resources
        // This ensures no commands are still using this texture
        vkDeviceWaitIdle(m_device);

        // Note: Descriptor sets are owned by the descriptor pool and will be
        // freed when the pool is destroyed. We just need to ensure the device
        // is idle before destroying the resources they reference.
        m_descriptorSet = VK_NULL_HANDLE;

        if (m_sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(m_device, m_sampler, nullptr);
            m_sampler = VK_NULL_HANDLE;
        }
        if (m_imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_device, m_imageView, nullptr);
            m_imageView = VK_NULL_HANDLE;
        }
        if (m_image != VK_NULL_HANDLE)
        {
            vkDestroyImage(m_device, m_image, nullptr);
            m_image = VK_NULL_HANDLE;
        }
        if (m_imageMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_device, m_imageMemory, nullptr);
            m_imageMemory = VK_NULL_HANDLE;
        }
    }
    LOG_DEBUG("[Vulkan] Texture cleaned up");
}

} // namespace VK
