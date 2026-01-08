#include "MemoryAllocator.h"
#include "../Logger.h"
#include <algorithm>
#include <stdexcept>

namespace VK
{

MemoryAllocator::MemoryAllocator(VkDevice device, VkPhysicalDevice physicalDevice)
    : m_device(device)
    , m_physicalDevice(physicalDevice)
{
    LOG_INFO("[Vulkan] Memory allocator initialized");
}

MemoryAllocator::~MemoryAllocator()
{
    cleanup();
}

Allocation MemoryAllocator::allocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags properties)
{
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

    uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    // For small allocations, try to use pooled memory
    // For large allocations (>16MB), allocate separately
    if (memRequirements.size < 16 * 1024 * 1024)
    {
        MemoryBlock* block = findOrCreateBlock(memoryTypeIndex, memRequirements.size, memRequirements.alignment);

        if (block)
        {
            // Align the offset
            VkDeviceSize alignedOffset = (block->used + memRequirements.alignment - 1) & ~(memRequirements.alignment - 1);

            if (alignedOffset + memRequirements.size <= block->size)
            {
                Allocation allocation;
                allocation.memory = block->memory;
                allocation.offset = alignedOffset;
                allocation.size = memRequirements.size;
                allocation.owningBlock = block;

                block->used = alignedOffset + memRequirements.size;

                LOG_DEBUG("[Vulkan] Allocated {} bytes from pool at offset {}", memRequirements.size, alignedOffset);
                return allocation;
            }
        }
    }

    // Fallback: allocate dedicated memory for large resources or if pooling fails
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory memory;
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    Allocation allocation;
    allocation.memory = memory;
    allocation.offset = 0;
    allocation.size = memRequirements.size;
    allocation.owningBlock = nullptr; // Dedicated allocation

    LOG_DEBUG("[Vulkan] Dedicated allocation of {} bytes", memRequirements.size);
    return allocation;
}

Allocation MemoryAllocator::allocateImageMemory(VkImage image, VkMemoryPropertyFlags properties)
{
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device, image, &memRequirements);

    uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    // Images typically have stricter alignment requirements,
    // so we use dedicated allocations for simplicity
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory memory;
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory");
    }

    Allocation allocation;
    allocation.memory = memory;
    allocation.offset = 0;
    allocation.size = memRequirements.size;
    allocation.owningBlock = nullptr; // Dedicated allocation

    LOG_DEBUG("[Vulkan] Image dedicated allocation of {} bytes", memRequirements.size);
    return allocation;
}

void MemoryAllocator::free(const Allocation& allocation)
{
    if (allocation.owningBlock)
    {
        // Pooled allocation - just mark the space as potentially reusable
        // For a full implementation, you'd track free regions and defragment
        LOG_DEBUG("[Vulkan] Freed pooled allocation of {} bytes", allocation.size);
    }
    else
    {
        // Dedicated allocation - free it immediately
        vkFreeMemory(m_device, allocation.memory, nullptr);
        LOG_DEBUG("[Vulkan] Freed dedicated allocation of {} bytes", allocation.size);
    }
}

void MemoryAllocator::cleanup()
{
    // Free all memory blocks
    for (auto& block : m_memoryBlocks)
    {
        if (block.memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_device, block.memory, nullptr);
        }
    }
    m_memoryBlocks.clear();

    LOG_INFO("[Vulkan] Memory allocator cleaned up");
}

uint32_t MemoryAllocator::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

MemoryBlock* MemoryAllocator::findOrCreateBlock(uint32_t memoryTypeIndex, VkDeviceSize size, VkDeviceSize alignment)
{
    // Try to find an existing block with enough space
    for (auto& block : m_memoryBlocks)
    {
        if (block.memoryTypeIndex == memoryTypeIndex)
        {
            VkDeviceSize alignedOffset = (block.used + alignment - 1) & ~(alignment - 1);
            if (alignedOffset + size <= block.size)
            {
                return &block;
            }
        }
    }

    // Create a new block
    VkDeviceSize blockSize = std::max(DEFAULT_BLOCK_SIZE, size * 2);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = blockSize;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory memory;
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
    {
        LOG_WARNING("[Vulkan] Failed to create memory pool block");
        return nullptr;
    }

    MemoryBlock block;
    block.memory = memory;
    block.size = blockSize;
    block.used = 0;
    block.memoryTypeIndex = memoryTypeIndex;

    m_memoryBlocks.push_back(block);

    LOG_INFO("[Vulkan] Created new memory pool block: {} MB", blockSize / (1024 * 1024));
    return &m_memoryBlocks.back();
}

} // namespace VK
