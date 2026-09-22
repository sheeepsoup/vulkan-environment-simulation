#include "displacement.h"

#include <fstream>
#include <stdexcept>
#include <array>

namespace displacement {

    Displacement::Displacement(
        lve::LveDevice& device,
        const evolution::Evolution& evolutionObj,
        const std::string& computeShaderPath,
        uint32_t resolution,
        uint32_t oceanRange
    )
        : device{ device },
        evolutionObj{ evolutionObj },
        resolution{ resolution },
        oceanRange{ oceanRange } {

        createSpectrumImages();

        createDescriptorSetLayout();
        createDescriptorPool();
        createDescriptorSet();

        createComputePipeline(computeShaderPath);
    }

    Displacement::~Displacement() {
        clean();
    }

    void Displacement::createSpectrumImages() {
        createImage(
            displacementXImage,
            displacementXImageMemory,
            displacementXImageView
        );

        createImage(
            displacementYImage,
            displacementYImageMemory,
            displacementYImageView
        );
    }

    void Displacement::createImage(
        VkImage& image,
        VkDeviceMemory& imageMemory,
        VkImageView& imageView
    ) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = resolution;
        imageInfo.extent.height = resolution;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R32G32_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        imageInfo.usage =
            VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT;

        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(
            device.getDevice(),
            &imageInfo,
            nullptr,
            &image
        ) != VK_SUCCESS) {
            throw std::runtime_error("failed to create displacement spectrum image");
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetImageMemoryRequirements(
            device.getDevice(),
            image,
            &memoryRequirements
        );

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memoryRequirements.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(
            memoryRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        if (vkAllocateMemory(
            device.getDevice(),
            &allocInfo,
            nullptr,
            &imageMemory
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to allocate displacement spectrum image memory"
            );
        }

        vkBindImageMemory(
            device.getDevice(),
            image,
            imageMemory,
            0
        );

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32G32_SFLOAT;

        viewInfo.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_COLOR_BIT;

        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(
            device.getDevice(),
            &viewInfo,
            nullptr,
            &imageView
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to create displacement spectrum image view"
            );
        }

        // 和 evolution / IFFT 使用同一种 GENERAL 布局。
        device.transitionImageLayout(
            image,
            VK_FORMAT_R32G32_SFLOAT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL
        );
    }

    void Displacement::createDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 3> bindings{};

        // H(k,t)：Evolution 输出的动态频谱
        bindings[0].binding = 0;
        bindings[0].descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags =
            VK_SHADER_STAGE_COMPUTE_BIT;

        // Dx(k,t)：写入 X 位移频谱
        bindings[1].binding = 1;
        bindings[1].descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags =
            VK_SHADER_STAGE_COMPUTE_BIT;

        // Dy(k,t)：写入 Y 位移频谱
        bindings[2].binding = 2;
        bindings[2].descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags =
            VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

        layoutInfo.bindingCount =
            static_cast<uint32_t>(bindings.size());

        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(
            device.getDevice(),
            &layoutInfo,
            nullptr,
            &descriptorSetLayout
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to create displacement descriptor set layout"
            );
        }
    }

    void Displacement::createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSize.descriptorCount = 3;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(
            device.getDevice(),
            &poolInfo,
            nullptr,
            &descriptorPool
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to create displacement descriptor pool"
            );
        }
    }

    void Displacement::createDescriptorSet() {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &descriptorSetLayout;

        if (vkAllocateDescriptorSets(
            device.getDevice(),
            &allocInfo,
            &descriptorSet
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to allocate displacement descriptor set"
            );
        }

        VkDescriptorImageInfo htInfo{};
        htInfo.imageView = evolutionObj.getHtSpectrumView();
        htInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkDescriptorImageInfo dxInfo{};
        dxInfo.imageView = displacementXImageView;
        dxInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkDescriptorImageInfo dyInfo{};
        dyInfo.imageView = displacementYImageView;
        dyInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        std::array<VkWriteDescriptorSet, 3> writes{};

        for (uint32_t i = 0; i < 3; ++i) {
            writes[i].sType =
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

            writes[i].dstSet = descriptorSet;
            writes[i].dstBinding = i;
            writes[i].dstArrayElement = 0;
            writes[i].descriptorType =
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

            writes[i].descriptorCount = 1;
        }

        writes[0].pImageInfo = &htInfo;
        writes[1].pImageInfo = &dxInfo;
        writes[2].pImageInfo = &dyInfo;

        vkUpdateDescriptorSets(
            device.getDevice(),
            static_cast<uint32_t>(writes.size()),
            writes.data(),
            0,
            nullptr
        );
    }

    void Displacement::createComputePipeline(
        const std::string& computeShaderPath
    ) {
        const std::vector<char> shaderCode =
            readFile(computeShaderPath);

        VkShaderModule shaderModule =
            createShaderModule(shaderCode);

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags =
            VK_SHADER_STAGE_COMPUTE_BIT;

        pushConstantRange.offset = 0;
        pushConstantRange.size =
            sizeof(PushConstant);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType =
            VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts =
            &descriptorSetLayout;

        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges =
            &pushConstantRange;

        if (vkCreatePipelineLayout(
            device.getDevice(),
            &pipelineLayoutInfo,
            nullptr,
            &pipelineLayout
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to create displacement pipeline layout"
            );
        }

        VkPipelineShaderStageCreateInfo shaderStage{};
        shaderStage.sType =
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

        shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStage.module = shaderModule;
        shaderStage.pName = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType =
            VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

        pipelineInfo.stage = shaderStage;
        pipelineInfo.layout = pipelineLayout;

        if (vkCreateComputePipelines(
            device.getDevice(),
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &pipeline
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to create displacement compute pipeline"
            );
        }

        vkDestroyShaderModule(
            device.getDevice(),
            shaderModule,
            nullptr
        );
    }

    void Displacement::recordDisplacementCommands(
        VkCommandBuffer commandBuffer,
        float choppiness
    ) {
        PushConstant pushConstant{};
        pushConstant.resolution = resolution;
        pushConstant.oceanRange = oceanRange;
        pushConstant.choppiness = choppiness;

        vkCmdBindPipeline(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            pipeline
        );

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            pipelineLayout,
            0,
            1,
            &descriptorSet,
            0,
            nullptr
        );

        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_COMPUTE_BIT,
            0,
            sizeof(PushConstant),
            &pushConstant
        );

        const uint32_t groupCount =
            (resolution + 15) / 16;

        vkCmdDispatch(
            commandBuffer,
            groupCount,
            groupCount,
            1
        );

        // 让随后执行的两个 IFFT 可以读取 Dx / Dy。
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask =
            VK_ACCESS_SHADER_WRITE_BIT;

        barrier.dstAccessMask =
            VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            1,
            &barrier,
            0,
            nullptr,
            0,
            nullptr
        );
    }

    std::vector<char> Displacement::readFile(
        const std::string& filename
    ) {
        std::ifstream file(
            filename,
            std::ios::ate | std::ios::binary
        );

        if (!file.is_open()) {
            throw std::runtime_error(
                "failed to open file: " + filename
            );
        }

        const size_t fileSize =
            static_cast<size_t>(file.tellg());

        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    VkShaderModule Displacement::createShaderModule(
        const std::vector<char>& code
    ) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType =
            VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

        createInfo.codeSize = code.size();
        createInfo.pCode =
            reinterpret_cast<const uint32_t*>(
                code.data()
                );

        VkShaderModule shaderModule{};

        if (vkCreateShaderModule(
            device.getDevice(),
            &createInfo,
            nullptr,
            &shaderModule
        ) != VK_SUCCESS) {
            throw std::runtime_error(
                "failed to create displacement shader module"
            );
        }

        return shaderModule;
    }

    void Displacement::clean() {
        const VkDevice vkDevice = device.getDevice();

        if (displacementXImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(
                vkDevice,
                displacementXImageView,
                nullptr
            );

            displacementXImageView = VK_NULL_HANDLE;
        }

        if (displacementYImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(
                vkDevice,
                displacementYImageView,
                nullptr
            );

            displacementYImageView = VK_NULL_HANDLE;
        }

        if (displacementXImage != VK_NULL_HANDLE) {
            vkDestroyImage(
                vkDevice,
                displacementXImage,
                nullptr
            );

            displacementXImage = VK_NULL_HANDLE;
        }

        if (displacementYImage != VK_NULL_HANDLE) {
            vkDestroyImage(
                vkDevice,
                displacementYImage,
                nullptr
            );

            displacementYImage = VK_NULL_HANDLE;
        }

        if (displacementXImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(
                vkDevice,
                displacementXImageMemory,
                nullptr
            );

            displacementXImageMemory = VK_NULL_HANDLE;
        }

        if (displacementYImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(
                vkDevice,
                displacementYImageMemory,
                nullptr
            );

            displacementYImageMemory = VK_NULL_HANDLE;
        }

        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(
                vkDevice,
                pipeline,
                nullptr
            );

            pipeline = VK_NULL_HANDLE;
        }

        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(
                vkDevice,
                pipelineLayout,
                nullptr
            );

            pipelineLayout = VK_NULL_HANDLE;
        }

        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(
                vkDevice,
                descriptorPool,
                nullptr
            );

            descriptorPool = VK_NULL_HANDLE;
        }

        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(
                vkDevice,
                descriptorSetLayout,
                nullptr
            );

            descriptorSetLayout = VK_NULL_HANDLE;
        }
    }

} // namespace displacement