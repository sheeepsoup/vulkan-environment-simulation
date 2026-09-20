#include "lve_compute.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <array>

namespace lve {

    LveCompute::LveCompute(LveDevice& device, const std::string& computeShaderPath)
        : device(device), computeShaderPath(computeShaderPath) {
    }

    LveCompute::~LveCompute() {
        clean();
    }

    // 读取文件
    std::vector<char> LveCompute::readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open compute shader file: " + filename);
        }
        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    // 创建着色器模块
    VkShaderModule LveCompute::createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute shader module!");
        }
        return shaderModule;
    }

    // 描述符集布局（绑定存储缓冲）
    void LveCompute::createDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 5> bindings = {};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		bindings[3].binding = 3;
		bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[3].descriptorCount = 1;
		bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[4].binding = 4;
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute descriptor set layout!");
        }
    }

    // 描述符池
    void LveCompute::createDescriptorPool(uint32_t maxSets) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize.descriptorCount = maxSets * 5;//由于流量也来了所以*3

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = maxSets;

        if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute descriptor pool!");
        }
    }

    // 管线布局
    void LveCompute::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstantData);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &descriptorSetLayout;
        layoutInfo.pushConstantRangeCount = 1; //上传
        layoutInfo.pPushConstantRanges = &pushConstantRange;


        if (vkCreatePipelineLayout(device.getDevice(), &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }
    }

    // 创建计算管线
    void LveCompute::createComputePipeline() {
        auto computeCode = readFile(computeShaderPath);
        computeShaderModule = createShaderModule(computeCode);

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = computeShaderModule;
        stageInfo.pName = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = stageInfo;
        pipelineInfo.layout = pipelineLayout;

        if (vkCreateComputePipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline!");
        }
    }

    // ★ 新增：创建存储缓冲区（对应你问的 computeBuffer）
    void LveCompute::createStorageBuffers(uint32_t count, VkDeviceSize size) {
        bufferSize = size;
        storageBuffers.resize(count);
        storageBuffersMemory.resize(count);
        storageBuffersMapped.resize(count);
        flowBuffers.resize(count);
        flowBuffersMemory.resize(count);
        flowBuffersMapped.resize(count);
        erosionBuffers.resize(count);
        erosionBuffersMemory.resize(count);
        erosionBuffersMapped.resize(count);
        researchBuffers.resize(count);
		researchBuffersMemory.resize(count);
		researchBuffersMapped.resize(count);

        for (uint32_t i = 0; i < count; i++) {
            // 使用你现成的 device.createBuffer
            device.createBuffer(
                size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // ★ 重点是 Storage Buffer
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                storageBuffers[i],
                storageBuffersMemory[i]
            );
            //流量
            device.createBuffer(
                size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                flowBuffers[i],
                flowBuffersMemory[i]
            );

            //创建侵蚀量内容
            device.createBuffer(
                size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                erosionBuffers[i],
                erosionBuffersMemory[i]
            );
            //研究数据
            device.createBuffer(
                sizeof(ResearchStats),
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                researchBuffers[i],
                researchBuffersMemory[i]
            );
            vkMapMemory(device.getDevice(), storageBuffersMemory[i], 0, size, 0, &storageBuffersMapped[i]);
            vkMapMemory(device.getDevice(),flowBuffersMemory[i],0,size,0,&flowBuffersMapped[i]);
            vkMapMemory(device.getDevice(),erosionBuffersMemory[i],0,size,0,&erosionBuffersMapped[i]);
            vkMapMemory(device.getDevice(),researchBuffersMemory[i],0, sizeof(ResearchStats),0,&researchBuffersMapped[i]);
            // 原始显存内容不是自动为0
            std::memset(flowBuffersMapped[i], 0, static_cast<size_t>(size));
            std::memset(erosionBuffersMapped[i], 0, static_cast<size_t>(size));
            std::memset(researchBuffersMapped[i], 0, sizeof(ResearchStats));
        }
    }

    // ★ 新增：更新描述符集（把刚才创建的 Buffer 绑定到描述符集）
    void LveCompute::updateDescriptorSets() {
        for (uint32_t i = 0; i < descriptorSets.size(); i++) {
            VkDescriptorBufferInfo bufferInfos[5]{};

            bufferInfos[0].buffer = storageBuffers[i];
            bufferInfos[0].offset = 0;
            bufferInfos[0].range = bufferSize;

            bufferInfos[1].buffer = flowBuffers[i];
            bufferInfos[1].offset = 0;
            bufferInfos[1].range = bufferSize;

            bufferInfos[2].buffer = erosionBuffers[i];
            bufferInfos[2].offset = 0;
            bufferInfos[2].range = bufferSize;

            bufferInfos[3].buffer = researchBuffers[i];
            bufferInfos[3].offset = 0;
            bufferInfos[3].range = sizeof(ResearchStats);

            bufferInfos[4].buffer = externalSlopeBuffers[i];    // ← 从 Slope 传入
            bufferInfos[4].offset = 0;
            bufferInfos[4].range = VK_WHOLE_SIZE;      // 或 slopeSize

            VkWriteDescriptorSet writes[5]{}; 

            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = descriptorSets[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[0].descriptorCount = 1;
            writes[0].pBufferInfo = &bufferInfos[0];

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = descriptorSets[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &bufferInfos[1];
  
            writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[2].dstSet = descriptorSets[i];
            writes[2].dstBinding = 2;
            writes[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[2].descriptorCount = 1;
            writes[2].pBufferInfo = &bufferInfos[2];

			writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[3].dstSet = descriptorSets[i];
			writes[3].dstBinding = 3;
            writes[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[3].descriptorCount = 1;
            writes[3].pBufferInfo = &bufferInfos[3];


            writes[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[4].dstSet = descriptorSets[i];
            writes[4].dstBinding = 4;
            writes[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[4].descriptorCount = 1;
            writes[4].pBufferInfo = &bufferInfos[4];

            vkUpdateDescriptorSets(
                device.getDevice(),
                5,
                writes,
                0,
                nullptr
            );
        }
    }

    // 初始化（整合所有步骤）
    void LveCompute::init(uint32_t maxFramesInFlight, VkDeviceSize bufferSize) {
        createDescriptorSetLayout();
        createDescriptorPool(maxFramesInFlight);
        createPipelineLayout();
        createComputePipeline();

        // 分配描述符集
        descriptorSets.resize(maxFramesInFlight);
        std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = maxFramesInFlight;
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate compute descriptor sets!");
        }

        // ★ 创建存储缓冲并绑定到描述符集
        createStorageBuffers(maxFramesInFlight, bufferSize);
        updateDescriptorSets();
    }

    // 更新数据（从 CPU 传到 GPU）
    void LveCompute::updateStorageBuffer(uint32_t frameIndex, void* data, VkDeviceSize size) {
        if (frameIndex >= storageBuffersMapped.size()) return;
        memcpy(storageBuffersMapped[frameIndex], data, size);
    }

    // 记录计算命令
    void LveCompute::recordComputeCommands(VkCommandBuffer cmdBuffer,uint32_t frameIndex,int width,uint32_t batchDropletCount,uint32_t baseDropletId,
        float slopeStep, int slopeRes) {
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(
            cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            pipelineLayout, 0, 1, &descriptorSets[frameIndex],
            0, nullptr
        );

        PushConstantData pushData{};
        pushData.width = width;
        pushData.waterDorpNum = static_cast<int>(batchDropletCount);
        pushData.baseDropletId = baseDropletId;
		pushData.slopeStep = slopeStep;
		pushData.slopeRes = slopeRes;

        vkCmdPushConstants(
            cmdBuffer, pipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0, sizeof(PushConstantData), &pushData
        );

        const uint32_t groupSize = 64;
        const uint32_t groupCount =
            (batchDropletCount + groupSize - 1) / groupSize;

        vkCmdDispatch(cmdBuffer, groupCount, 1, 1);
    }

    // 清理
    void LveCompute::clean() {
        VkDevice vkDevice = device.getDevice();

        for (size_t i = 0; i < storageBuffers.size(); i++) {
            vkUnmapMemory(vkDevice, storageBuffersMemory[i]); // 解除映射
            vkDestroyBuffer(vkDevice, storageBuffers[i], nullptr);
            vkFreeMemory(vkDevice, storageBuffersMemory[i], nullptr);
        }
        storageBuffers.clear();
        storageBuffersMemory.clear();
        storageBuffersMapped.clear();

        if (computePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(vkDevice, computePipeline, nullptr);
            computePipeline = VK_NULL_HANDLE;
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(vkDevice, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(vkDevice, descriptorSetLayout, nullptr);
            descriptorSetLayout = VK_NULL_HANDLE;
        }
        if (computeShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(vkDevice, computeShaderModule, nullptr);
            computeShaderModule = VK_NULL_HANDLE;
        }
        for (size_t i = 0; i < flowBuffers.size(); i++) {
            vkUnmapMemory(vkDevice, flowBuffersMemory[i]);
            vkDestroyBuffer(vkDevice, flowBuffers[i], nullptr);
            vkFreeMemory(vkDevice, flowBuffersMemory[i], nullptr);

        }
        for (size_t i = 0; i < erosionBuffers.size(); i++) {
            vkUnmapMemory(vkDevice, erosionBuffersMemory[i]);
            vkDestroyBuffer(vkDevice, erosionBuffers[i], nullptr);
            vkFreeMemory(vkDevice, erosionBuffersMemory[i], nullptr);
        }
        for (size_t i = 0; i < researchBuffers.size(); i++) {
            vkUnmapMemory(vkDevice, researchBuffersMemory[i]);
            vkDestroyBuffer(vkDevice, researchBuffers[i], nullptr);
            vkFreeMemory(vkDevice, researchBuffersMemory[i], nullptr);
        }

        researchBuffers.clear();
        researchBuffersMemory.clear();
        researchBuffersMapped.clear();
        erosionBuffers.clear();
        erosionBuffersMemory.clear();
        erosionBuffersMapped.clear();
        flowBuffers.clear();
        flowBuffersMemory.clear();
        flowBuffersMapped.clear();
    }

    void LveCompute::runErosionSync(LveDevice& device, uint32_t bufferIndex, int mapVertexCount, std::vector<int32_t>& heightData,
        std::vector<uint32_t>& flowData,std::vector<uint32_t>& erosionData,ResearchStats& researchData, VkDeviceSize bufferSize, float slopeStep, int slopeRes) {
        //情空
        std::memset(flowBuffersMapped[bufferIndex],0,static_cast<size_t>(bufferSize));
        std::memset(erosionBuffersMapped[bufferIndex],0,static_cast<size_t>(bufferSize));
        std::memset(researchBuffersMapped[bufferIndex],0,sizeof(ResearchStats));
        //计算地形
        updateStorageBuffer(bufferIndex, heightData.data(), bufferSize);
        //提交一次计算
        VkCommandBuffer computeCmdBuf;
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = device.getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(device.getDevice(), &allocInfo, &computeCmdBuf);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(computeCmdBuf, &beginInfo);

        //分批次提交,用于记录地貌
        constexpr uint32_t batchCount = 4;
         uint32_t batchDropletCount = EROSON_EXTENT / batchCount;

        for (uint32_t batch = 0; batch < batchCount; batch++) {
            recordComputeCommands(
                computeCmdBuf,
                bufferIndex,
                mapVertexCount,
                batchDropletCount,
                batch * batchDropletCount,
                slopeStep,
                slopeRes
            );

            if (batch + 1 < batchCount) {
                VkMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                barrier.dstAccessMask =
                    VK_ACCESS_SHADER_READ_BIT |
                    VK_ACCESS_SHADER_WRITE_BIT;

                vkCmdPipelineBarrier(
                    computeCmdBuf,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    0,
                    1, &barrier,
                    0, nullptr,
                    0, nullptr
                );
            }
        }
        vkEndCommandBuffer(computeCmdBuf);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeCmdBuf;

        VkFence computeFence;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &computeFence);

        vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, computeFence);
        vkWaitForFences(device.getDevice(), 1, &computeFence, VK_TRUE, UINT64_MAX);
        vkDestroyFence(device.getDevice(), computeFence, nullptr);
        vkFreeCommandBuffers(device.getDevice(), device.getCommandPool(), 1, &computeCmdBuf);
        //计算完毕拷回来
        memcpy(heightData.data(), getMappedData(bufferIndex), bufferSize);
        memcpy(flowData.data(),getFlowMappedData(bufferIndex), sizeof(uint32_t) * flowData.size());
        memcpy(erosionData.data(),getErosionMappedData(bufferIndex), sizeof(uint32_t) * erosionData.size());
        memcpy(&researchData,getResearchMappedData(bufferIndex),sizeof(ResearchStats));
    };
} // namespace lve