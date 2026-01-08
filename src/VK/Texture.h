#pragma once

#include "../RenderAPI/ITexture.h"
#include <vulkan/vulkan.h>

namespace VK
{
    class Renderer;

    class Texture : public ITexture
    {
    public:
        Texture(VkDevice device, VkPhysicalDevice physicalDevice, Renderer* renderer = nullptr);
        ~Texture() override;

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        void bind(uint32_t slot = 0) override;
        void unbind() override;

        void setData(const void* data, uint32_t width, uint32_t height, TextureFormat format) override;
        void updateData(const void* data, uint32_t xOffset, uint32_t yOffset,
                       uint32_t width, uint32_t height) override;

        void setFilter(TextureFilter minFilter, TextureFilter magFilter) override;
        void setWrap(TextureWrap wrapS, TextureWrap wrapT) override;
        void generateMipmaps() override;

        uint32_t getWidth() const override { return m_width; }
        uint32_t getHeight() const override { return m_height; }
        TextureFormat getFormat() const override { return m_format; }

        VkImage getImage() const { return m_image; }
        VkImageView getImageView() const { return m_imageView; }
        VkSampler getSampler() const { return m_sampler; }
        VkDescriptorSet getDescriptorSet() const { return m_descriptorSet; }

    private:
        void createImage(uint32_t width, uint32_t height, VkFormat format,
                        VkImageTiling tiling, VkImageUsageFlags usage,
                        VkMemoryPropertyFlags properties);
        void createImageView(VkFormat format);
        void createSampler();
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height);

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        VkFormat convertTextureFormat(TextureFormat format) const;
        VkFilter convertFilter(TextureFilter filter) const;
        VkSamplerAddressMode convertWrap(TextureWrap wrap) const;

        void cleanup();

        Renderer* m_renderer;
        VkDevice m_device;
        VkPhysicalDevice m_physicalDevice;

        VkImage m_image;
        VkDeviceMemory m_imageMemory;
        VkImageView m_imageView;
        VkSampler m_sampler;
        VkDescriptorSet m_descriptorSet;

        uint32_t m_width;
        uint32_t m_height;
        TextureFormat m_format;
        VkFormat m_vkFormat;

        TextureFilter m_minFilter;
        TextureFilter m_magFilter;
        TextureWrap m_wrapS;
        TextureWrap m_wrapT;
    };
}
