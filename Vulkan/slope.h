#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "lve_device.h"

namespace lve {
    class LveModel;
    class LveTerrain;
}

namespace slope {

    class Slope {
    public:
        Slope(lve::LveDevice& device, const std::string& computeShaderPath);
        ~Slope();

        // slopeResolution: 坡度图边长
        // mapVertexCount: 高度图边长（顶点数）
        // step:           高度图顶点间距
        void init(uint32_t slopeResolution,
            uint32_t mapVertexCount,
            float step);

        // 记录坡度计算命令（高度 buffer 从外部传入）
        void recordComputeCommands(VkCommandBuffer cmdBuffer,
            uint32_t frameIndex,
            VkBuffer heightBuffer,
            int mapVertexCount);

        void clean();
        void* getSlopeBufferMapped(uint32_t index = 0) const {
            return slopeBuffersMapped[index];
        }
        // GPU 侧坡度 buffer 的访问接口（给下游 compute 用）
        std::vector<VkBuffer> getSlopeBuffer() const {  
            return slopeBuffers;
        }
        uint32_t getSlopeResolution() const { return slopeResolution; }
        float getStep() const { return step; }
        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
        void runSlopeSync(lve::LveDevice& device,
            uint32_t frameIndex,
            VkBuffer heightBuffer,
			int mapVertexCount);//同步计算坡度

    private:
        lve::LveDevice& device;
        std::string computeShaderPath;

        uint32_t slopeResolution = 512;
        uint32_t mapVertexCount = 0;
        float step = 1.0f;

        VkShaderModule computeShaderModule = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline computePipeline = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;

        std::vector<VkBuffer> slopeBuffers;
        std::vector<VkDeviceMemory> slopeBuffersMemory;
        std::vector<void*> slopeBuffersMapped;

        struct PushConstantData {
            int mapVertexCount;
            int slopeRes;
            float step;
            float padding;
        };

        std::vector<char> readFile(const std::string& filename);
        VkShaderModule createShaderModule(const std::vector<char>& code);
        void createDescriptorSetLayout();
        void createDescriptorPool(uint32_t maxSets);
        void createPipelineLayout();
        void createComputePipeline();

        void createSlopeBuffers(uint32_t count);
        void updateDescriptorSets();
    };

}