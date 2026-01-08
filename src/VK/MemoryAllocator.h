#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace VK
{
    // Simple memory pool to reduce allocation count and fragmentation
    // This is a lightweight alternative to VMA for basic pooling needs

    struct MemoryBlock
    {
        VkDeviceMemory memory;
        VkDeviceSize size;
        VkDeviceSize used;
        uint32_t memoryTypeIndex;
    };

    struct Allocation
    {
        VkDeviceMemory memory;
        VkDeviceSize offset;
        VkDeviceSize size;
        MemoryBlock* owningBlock;
    };

    class MemoryAllocator
    {
    public:
        MemoryAllocator(VkDevice device, VkPhysicalDevice physicalDevice);
        ~MemoryAllocator();

        // Allocate memory for a buffer
        Allocation allocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags properties);

        // Allocate memory for an image
        Allocation allocateImageMemory(VkImage image, VkMemoryPropertyFlags properties);

        // Free an allocation
        void free(const Allocation& allocation);

        // Cleanup all memory blocks
        void cleanup();

    private:
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        MemoryBlock* findOrCreateBlock(uint32_t memoryTypeIndex, VkDeviceSize size, VkDeviceSize alignment);

        VkDevice m_device;
        VkPhysicalDevice m_physicalDevice;
        std::vector<MemoryBlock> m_memoryBlocks;

        // Default block size: 256MB (can be tuned based on usage)
        static constexpr VkDeviceSize DEFAULT_BLOCK_SIZE = 256 * 1024 * 1024;
    };

} // namespace VK
