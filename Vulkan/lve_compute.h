#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "lve_device.h"

namespace lve {

    class LveCompute {
    public:
        uint32_t EROSON_EXTENT = 2000000;//侵蚀n次
        int a = 0;
        LveCompute(LveDevice& device, const std::string& computeShaderPath);
        ~LveCompute();

        // 禁止拷贝
        LveCompute(const LveCompute&) = delete;
        LveCompute& operator=(const LveCompute&) = delete;

        // 初始化：传入帧数量和数据大小
        void init(uint32_t maxFramesInFlight, VkDeviceSize bufferSize);

        // 更新数据（CPU -> GPU）
        void updateStorageBuffer(uint32_t frameIndex, void* data, VkDeviceSize size);

        // 记录计算命令
        void recordComputeCommands(//记录
            VkCommandBuffer cmdBuffer,
            uint32_t frameIndex,
            int width,
            uint32_t batchDropletCount,
            uint32_t baseDropletId,
            float slopeStep,
            int slopeRes
        );

        // 清理
        void clean();

        // 获取资源
        const std::vector<VkDescriptorSet>& getDescriptorSets() const { return descriptorSets; }
        VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
        void* getMappedData(uint32_t frameIndex) const { return storageBuffersMapped[frameIndex]; }
        VkBuffer getStorageBuffer(uint32_t frameIndex) const {
            return storageBuffers[frameIndex];
        }
      
        void* getFlowMappedData(uint32_t frameIndex) const {
            return flowBuffersMapped[frameIndex];
        }
        void* getErosionMappedData(uint32_t frameIndex) const {
            return erosionBuffersMapped[frameIndex];
        }
		void* getResearchMappedData(uint32_t frameIndex) const {
			return researchBuffersMapped[frameIndex];
		}
        struct ResearchStats {//研究测试的对应数据
            uint32_t totalCandidates;
            uint32_t erosionSkipped;
            uint32_t flatSlopeStopped;
        };
        void runErosionSync(LveDevice& device, uint32_t bufferIndex, int mapVertexCount, std::vector<int32_t>& heightData,
            std::vector<uint32_t>& flowData, std::vector<uint32_t>& erosionData, ResearchStats& researchData, VkDeviceSize bufferSize, float slopeStep, int slopeRes);

        void setSlopeBuffers(const std::vector<VkBuffer>& buffers) {
            externalSlopeBuffers = buffers;
            updateDescriptorSets();
        }
    private:
        LveDevice& device;
        std::string computeShaderPath;
        VkDeviceSize bufferSize = 0;
        std::vector<VkBuffer> externalSlopeBuffers;//存有坡度的缓冲区


        // Vulkan 对象
        VkShaderModule computeShaderModule = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline computePipeline = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;

        // 存储缓冲（计算数据）
        std::vector<VkBuffer> storageBuffers;
        std::vector<VkDeviceMemory> storageBuffersMemory;
        std::vector<void*> storageBuffersMapped;

        std::vector<VkBuffer> flowBuffers;
        std::vector<VkDeviceMemory> flowBuffersMemory;
        std::vector<void*> flowBuffersMapped;

        std::vector<VkBuffer> erosionBuffers;
        std::vector<VkDeviceMemory> erosionBuffersMemory;
        std::vector<void*> erosionBuffersMapped;

        std::vector<VkBuffer> researchBuffers;
		std::vector<VkDeviceMemory> researchBuffersMemory;
		std::vector<void*> researchBuffersMapped;
        struct PushConstantData {//分批上传的数据
            int width;          // 高度图边长（顶点数）
            int waterDorpNum;
            uint32_t baseDropletId;
            int slopeRes;       // ★ 坡度图分辨率（比如 512）
            float slopeStep;    // ★ 高度图顶点间距（世界单位）
        };
      
        // 内部辅助函数
        std::vector<char> readFile(const std::string& filename);
        VkShaderModule createShaderModule(const std::vector<char>& code);
        void createDescriptorSetLayout();
        void createDescriptorPool(uint32_t maxSets);
        void createPipelineLayout();
        void createComputePipeline();
        void createStorageBuffers(uint32_t count, VkDeviceSize size);
        void updateDescriptorSets();  // ★ 这里之前漏了，现在补上
 
    };

} // namespace lve