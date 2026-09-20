#include "slope.h"
#include "lve_model.h"
#include "lve_terrain.h"
#include <fstream>
#include <stdexcept>
#include <array>
#include <cstring>

namespace slope {

    Slope::Slope(lve::LveDevice& device, const std::string& computeShaderPath)
        : device(device), computeShaderPath(computeShaderPath) {
    }

    Slope::~Slope() { clean(); }

    std::vector<char> Slope::readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open slope compute shader file: " + filename);
        }
        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    VkShaderModule Slope::createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule module;
        if (vkCreateShaderModule(device.getDevice(), &createInfo, nullptr, &module) != VK_SUCCESS) {
            throw std::runtime_error("failed to create slope shader module!");
        }
        return module;
    }

    void Slope::createDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        info.bindingCount = static_cast<uint32_t>(bindings.size());
        info.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device.getDevice(), &info, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create slope descriptor set layout!");
        }
    }

    void Slope::createDescriptorPool(uint32_t maxSets) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize.descriptorCount = maxSets * 2;

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.poolSizeCount = 1;
        info.pPoolSizes = &poolSize;
        info.maxSets = maxSets;

        if (vkCreateDescriptorPool(device.getDevice(), &info, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create slope descriptor pool!");
        }
    }

    void Slope::createPipelineLayout() {
        VkPushConstantRange range{};
        range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        range.offset = 0;
        range.size = sizeof(PushConstantData);

        VkPipelineLayoutCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        info.setLayoutCount = 1;
        info.pSetLayouts = &descriptorSetLayout;
        info.pushConstantRangeCount = 1;
        info.pPushConstantRanges = &range;

        if (vkCreatePipelineLayout(device.getDevice(), &info, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create slope pipeline layout!");
        }
    }

    void Slope::createComputePipeline() {
        auto code = readFile(computeShaderPath);
        computeShaderModule = createShaderModule(code);

        VkPipelineShaderStageCreateInfo stage{};
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage.module = computeShaderModule;
        stage.pName = "main";

        VkComputePipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        info.stage = stage;
        info.layout = pipelineLayout;

        if (vkCreateComputePipelines(device.getDevice(), VK_NULL_HANDLE, 1, &info, nullptr, &computePipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create slope compute pipeline!");
        }
    }

    void Slope::createSlopeBuffers(uint32_t count) {
        VkDeviceSize size = sizeof(float) * slopeResolution * slopeResolution;

        slopeBuffers.resize(count);
        slopeBuffersMemory.resize(count);
        slopeBuffersMapped.resize(count);

        for (uint32_t i = 0; i < count; i++) {
            device.createBuffer(
                size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                slopeBuffers[i],
                slopeBuffersMemory[i]
            );
            vkMapMemory(device.getDevice(), slopeBuffersMemory[i], 0, size, 0, &slopeBuffersMapped[i]);
            std::memset(slopeBuffersMapped[i], 0, static_cast<size_t>(size));
        }
    }

    void Slope::updateDescriptorSets() {
        // 这个函数目前只更新 slope buffer 到 binding 1。
        // binding 0 的高度 buffer 是外部传入的，等 recordComputeCommands 时再更新。
        // 这里先不写。
    }

    void Slope::init(uint32_t slopeResolution,
        uint32_t mapVertexCount,
        float step) {
        this->slopeResolution = slopeResolution;
        this->mapVertexCount = mapVertexCount;
        this->step = step;

        createDescriptorSetLayout();
        createDescriptorPool(1); // 单 frame；多帧改成 maxFramesInFlight
        createPipelineLayout();
        createComputePipeline();

        descriptorSets.resize(1);
        std::vector<VkDescriptorSetLayout> layouts(1, descriptorSetLayout);
        VkDescriptorSetAllocateInfo alloc{};
        alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc.descriptorPool = descriptorPool;
        alloc.descriptorSetCount = 1;
        alloc.pSetLayouts = layouts.data();
        if (vkAllocateDescriptorSets(device.getDevice(), &alloc, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate slope descriptor sets!");
        }

        createSlopeBuffers(2);
    }

    void Slope::recordComputeCommands(VkCommandBuffer cmdBuffer,
        uint32_t frameIndex,
        VkBuffer heightBuffer,
        int mapVertexCount) {
        // 更新 binding 0（高度）和 binding 1（坡度）
        VkDescriptorBufferInfo heightInfo{};
        heightInfo.buffer = heightBuffer;
        heightInfo.offset = 0;
        heightInfo.range = VK_WHOLE_SIZE;

        VkDeviceSize slopeSize = sizeof(float) * slopeResolution * slopeResolution;
        VkDescriptorBufferInfo slopeInfo{};
        slopeInfo.buffer = slopeBuffers[frameIndex];
        slopeInfo.offset = 0;
        slopeInfo.range = slopeSize;

        std::array<VkWriteDescriptorSet, 2> writes{};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = descriptorSets[frameIndex];
        writes[0].dstBinding = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &heightInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptorSets[frameIndex];
        writes[1].dstBinding = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[1].descriptorCount = 1;
        writes[1].pBufferInfo = &slopeInfo;

        vkUpdateDescriptorSets(device.getDevice(),
            static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

        // 绑定管线
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);

        PushConstantData pc{};
        pc.mapVertexCount = mapVertexCount;
        pc.slopeRes = static_cast<int>(slopeResolution);
        pc.step = step;
        pc.padding = 0.0f;

        vkCmdPushConstants(cmdBuffer, pipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstantData), &pc);

        // 8x8 局部大小，按坡度图分辨率派发
        const uint32_t gx = (slopeResolution + 7) / 8;
        const uint32_t gy = (slopeResolution + 7) / 8;
        vkCmdDispatch(cmdBuffer, gx, gy, 1);
    }

    void Slope::clean() {
        VkDevice dev = device.getDevice();

        for (size_t i = 0; i < slopeBuffers.size(); i++) {
            vkUnmapMemory(dev, slopeBuffersMemory[i]);
            vkDestroyBuffer(dev, slopeBuffers[i], nullptr);
            vkFreeMemory(dev, slopeBuffersMemory[i], nullptr);
        }
        slopeBuffers.clear();
        slopeBuffersMemory.clear();
        slopeBuffersMapped.clear();

        if (computePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(dev, computePipeline, nullptr);
            computePipeline = VK_NULL_HANDLE;
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(dev, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(dev, descriptorSetLayout, nullptr);
            descriptorSetLayout = VK_NULL_HANDLE;
        }
        if (computeShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(dev, computeShaderModule, nullptr);
            computeShaderModule = VK_NULL_HANDLE;
        }
        descriptorSets.clear();
    }
    void Slope::runSlopeSync(lve::LveDevice& device,
        uint32_t frameIndex,
        VkBuffer heightBuffer,
        int mapVertexCount) {
        VkCommandBuffer cmd;
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = device.getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(device.getDevice(), &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        recordComputeCommands(cmd, frameIndex, heightBuffer, mapVertexCount);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        VkFence fence;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &fence);

        vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, fence);
        vkWaitForFences(device.getDevice(), 1, &fence, VK_TRUE, UINT64_MAX);
        vkDestroyFence(device.getDevice(), fence, nullptr);
        vkFreeCommandBuffers(device.getDevice(), device.getCommandPool(), 1, &cmd);
    }
}