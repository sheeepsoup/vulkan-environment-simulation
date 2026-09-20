#include"shadow.h"
#include"lve_model.h"
#include"lve_terrain.h"
#include <array>
#include <stdexcept>
#include <fstream>
namespace shadow {
	Shadow::Shadow(lve::LveDevice& device, const std::string& shadowShaderPath)
		: device(device), shadowShaderPath(shadowShaderPath) {
	}

	Shadow::~Shadow() { clean(); }

	void Shadow::createPipelineLayout() {
		VkPushConstantRange range{};
		range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;   // lightViewProj 只在 vert 用
		range.offset = 0;
		range.size = sizeof(PushConstantData);

		VkPipelineLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount = 0;
		info.pSetLayouts = nullptr;
		info.pushConstantRangeCount = 1;
		info.pPushConstantRanges = &range;

		if (vkCreatePipelineLayout(device.getDevice(), &info, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shadow pipeline layout!");
		}
	}

	void Shadow::updatePushConstant(VkCommandBuffer cmd, const glm::mat4& lightViewProj) {
		vkCmdPushConstants(
			cmd,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(PushConstantData),
			&lightViewProj
		);
	}
    void Shadow::init(uint32_t shadowMapSize) {
        this->shadowMapSize = shadowMapSize;

        createDepthResources();   // image + view
        createRenderPass();       // 依赖 depthFormat
        createFramebuffer();      // 依赖 imageView + renderPass
        createSampler();
        createPipelineLayout();
        createShadowPipeline();   // 依赖 shadowRenderPass
    }
	std::vector<char> Shadow::readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("failed to open shadow shader file: " + filename);
		}
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}
	VkShaderModule Shadow::createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shadow shader module!");
		}
		return shaderModule;
	}
	
    void Shadow::createShadowPipeline() {
        auto code = readFile(shadowShaderPath);
        shadowShaderModule = createShaderModule(code);

        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = shadowShaderModule;
        vertStage.pName = "main";

        // 顶点输入（和主渲染用同一个 Vertex 布局）
        auto bindingDesc = lve::LveModel::Vertex::getBindingDescription();
        auto attributeDescs = lve::LveModel::Vertex::getAttributeDescriptions();

        VkVertexInputAttributeDescription posAttr{};
        posAttr.binding = 0;
        posAttr.location = 0;
        posAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
        posAttr.offset = offsetof(lve::LveModel::Vertex, pos);

        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &bindingDesc;
        vertexInput.vertexAttributeDescriptionCount = 1;      // ★ 只 1 个
        vertexInput.pVertexAttributeDescriptions = &posAttr; // ★ 只有 pos

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_TRUE;          // 阴影 Pass 关键，防 acne
        rasterizer.depthBiasConstantFactor = 1.25f;
        rasterizer.depthBiasSlopeFactor = 1.75f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        // 阴影 Pass 没有颜色附件，所以 ColorBlend 的 attachmentCount = 0
        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 0;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 1;                 // 只有 vertex
        pipelineInfo.pStages = &vertStage;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = shadowRenderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device.getDevice(), VK_NULL_HANDLE, 1,
            &pipelineInfo, nullptr, &shadowPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow graphics pipeline!");
        }
    }
    void Shadow::createDepthResources() {
        shadowDepthFormat = device.findDepthFormat();

        device.createImage(
            shadowMapSize, shadowMapSize,
            shadowDepthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            shadowImage,
            shadowImageMemory
        );

        shadowImageView = device.createImageView(
            shadowImage,
            shadowDepthFormat,
            VK_IMAGE_ASPECT_DEPTH_BIT
        );
    }
    void Shadow::createRenderPass() {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = shadowDepthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthRef{};
        depthRef.attachment = 0;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;   // 没有颜色附件
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dependencies[2]{};

        // 上一帧主渲染读完阴影图，才能重新写
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // 本次阴影图写完，主渲染才能采样它
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = 1;
        info.pAttachments = &depthAttachment;
        info.subpassCount = 1;
        info.pSubpasses = &subpass;
        info.dependencyCount = 2;
        info.pDependencies = dependencies;

        if (vkCreateRenderPass(device.getDevice(), &info, nullptr, &shadowRenderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow render pass!");
        }
    }
    void Shadow::createFramebuffer() {
        VkFramebufferCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = shadowRenderPass;
        info.attachmentCount = 1;
        info.pAttachments = &shadowImageView;
        info.width = shadowMapSize;
        info.height = shadowMapSize;
        info.layers = 1;

        if (vkCreateFramebuffer(device.getDevice(), &info, nullptr, &shadowFramebuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow framebuffer!");
        }
    }
    void Shadow::createSampler() {
        VkSamplerCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        info.magFilter = VK_FILTER_LINEAR;
        info.minFilter = VK_FILTER_LINEAR;
        info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        info.compareEnable = VK_TRUE;
        info.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        if (vkCreateSampler(device.getDevice(), &info, nullptr, &shadowSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shadow sampler!");
        }
    }
    void Shadow::clean() {
        VkDevice dev = device.getDevice();

        if (shadowPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(dev, shadowPipeline, nullptr);
            shadowPipeline = VK_NULL_HANDLE;
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(dev, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }
        if (shadowShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(dev, shadowShaderModule, nullptr);
            shadowShaderModule = VK_NULL_HANDLE;
        }
        if (shadowFramebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(dev, shadowFramebuffer, nullptr);
            shadowFramebuffer = VK_NULL_HANDLE;
        }
        if (shadowRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(dev, shadowRenderPass, nullptr);
            shadowRenderPass = VK_NULL_HANDLE;
        }
        if (shadowSampler != VK_NULL_HANDLE) {
            vkDestroySampler(dev, shadowSampler, nullptr);
            shadowSampler = VK_NULL_HANDLE;
        }
        if (shadowImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(dev, shadowImageView, nullptr);
            shadowImageView = VK_NULL_HANDLE;
        }
        if (shadowImage != VK_NULL_HANDLE) {
            vkDestroyImage(dev, shadowImage, nullptr);
            shadowImage = VK_NULL_HANDLE;
        }
        if (shadowImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(dev, shadowImageMemory, nullptr);
            shadowImageMemory = VK_NULL_HANDLE;
        }
    }
    void Shadow::recordShadowPass(VkCommandBuffer cmd,
        const glm::mat4& lightViewProj,
        lve::LveModel& model,
        lve::LveTerrain& terrain) {
        VkRenderPassBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begin.renderPass = shadowRenderPass;
        begin.framebuffer = shadowFramebuffer;
        begin.renderArea.offset = { 0, 0 };
        begin.renderArea.extent = { shadowMapSize, shadowMapSize };

        VkClearValue clear{};
        clear.depthStencil = { 1.0f, 0 };
        begin.clearValueCount = 1;
        begin.pClearValues = &clear;

        vkCmdBeginRenderPass(cmd, &begin, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport vp{};
        vp.width = static_cast<float>(shadowMapSize);
        vp.height = static_cast<float>(shadowMapSize);
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &vp);

        VkRect2D sc{};
        sc.extent = { shadowMapSize, shadowMapSize };
        vkCmdSetScissor(cmd, 0, 1, &sc);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);
        updatePushConstant(cmd, lightViewProj);

        model.bindVertex(cmd);
        model.bindIndexBuffer(cmd);
        terrain.drawAllChunks(cmd);//这里没做距离剔除啥的,可能会慢,后面记得改进

        vkCmdEndRenderPass(cmd);
    }
}