#include "ocean.h"

#include <array>
#include <fstream>
#include <stdexcept>

namespace ocean {

	Ocean::Ocean(
		lve::LveDevice& lveDevice,
		const ifft::IFFT& ifftObj,
		VkRenderPass renderPass,
		VkDescriptorSetLayout globalDescriptorSetLayout,
		const std::string& vertexShaderPath,
		const std::string& fragmentShaderPath,
		uint32_t meshResolution,
		float oceanRange)
		: lveDevice{ lveDevice },
		ifftObj{ ifftObj },
		meshResolution{ meshResolution },
		oceanRange{ oceanRange } {

		if (meshResolution < 2) {
			throw std::runtime_error(
				"ocean mesh resolution must be at least 2!");
		}

		if (oceanRange <= 0.0f) {
			throw std::runtime_error(
				"ocean range must be greater than zero!");
		}

		createMesh();
		createHeightMapSampler();

		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSet();

		createGraphicsPipeline(
			renderPass,
			globalDescriptorSetLayout,
			vertexShaderPath,
			fragmentShaderPath);
	}

	Ocean::~Ocean() {
		model.clean(lveDevice.getDevice());

		if (graphicsPipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(
				lveDevice.getDevice(),
				graphicsPipeline,
				nullptr);
		}

		if (pipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(
				lveDevice.getDevice(),
				pipelineLayout,
				nullptr);
		}

		if (descriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(
				lveDevice.getDevice(),
				descriptorPool,
				nullptr);
		}

		if (descriptorSetLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(
				lveDevice.getDevice(),
				descriptorSetLayout,
				nullptr);
		}

		if (heightMapSampler != VK_NULL_HANDLE) {
			vkDestroySampler(
				lveDevice.getDevice(),
				heightMapSampler,
				nullptr);
		}
	}

	void Ocean::createMesh() {
		vertices.resize(
			static_cast<size_t>(meshResolution) *
			static_cast<size_t>(meshResolution));

		indices.reserve(
			static_cast<size_t>(meshResolution - 1) *
			static_cast<size_t>(meshResolution - 1) *
			6);

		const float halfRange = oceanRange * 0.5f;

		const float step =
			oceanRange /
			static_cast<float>(meshResolution - 1);

		for (uint32_t y = 0; y < meshResolution; y++) {
			for (uint32_t x = 0; x < meshResolution; x++) {
				const float worldX =
					-halfRange +
					static_cast<float>(x) * step;

				const float worldY =
					-halfRange +
					static_cast<float>(y) * step;

				const size_t index =
					static_cast<size_t>(y) *
					meshResolution +
					x;

				vertices[index] = {
					glm::vec3(worldX, worldY, 0.0f),
					glm::vec3(1.0f),
					glm::vec3(0.0f, 0.0f, 1.0f),
					0.0f
				};
			}
		}

		for (uint32_t y = 0; y < meshResolution - 1; y++) {
			for (uint32_t x = 0; x < meshResolution - 1; x++) {
				const uint32_t topLeft =
					y * meshResolution + x;

				const uint32_t topRight =
					topLeft + 1;

				const uint32_t bottomLeft =
					topLeft + meshResolution;

				const uint32_t bottomRight =
					bottomLeft + 1;

				indices.push_back(topLeft);
				indices.push_back(topRight);
				indices.push_back(bottomLeft);

				indices.push_back(topRight);
				indices.push_back(bottomRight);
				indices.push_back(bottomLeft);
			}
		}

		model.createVertexBufferWithStaging(
			lveDevice,
			vertices);

		model.createIndexBufferWithStaging(
			lveDevice,
			indices);
	}

	void Ocean::createHeightMapSampler() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType =
			VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU =
			VK_SAMPLER_ADDRESS_MODE_REPEAT;

		samplerInfo.addressModeV =
			VK_SAMPLER_ADDRESS_MODE_REPEAT;

		samplerInfo.addressModeW =
			VK_SAMPLER_ADDRESS_MODE_REPEAT;

		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;

		samplerInfo.borderColor =
			VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;

		samplerInfo.mipmapMode =
			VK_SAMPLER_MIPMAP_MODE_LINEAR;

		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(
			lveDevice.getDevice(),
			&samplerInfo,
			nullptr,
			&heightMapSampler) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create ocean height-map sampler!");
		}
	}

	void Ocean::createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding heightMapBinding{};

		heightMapBinding.binding = 0;
		heightMapBinding.descriptorType =
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		heightMapBinding.descriptorCount = 1;

		heightMapBinding.stageFlags =
			VK_SHADER_STAGE_VERTEX_BIT |
			VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType =
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &heightMapBinding;

		if (vkCreateDescriptorSetLayout(
			lveDevice.getDevice(),
			&layoutInfo,
			nullptr,
			&descriptorSetLayout) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create ocean descriptor set layout!");
		}
	}

	void Ocean::createDescriptorPool() {
		VkDescriptorPoolSize poolSize{};
		poolSize.type =
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		poolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType =
			VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = 1;

		if (vkCreateDescriptorPool(
			lveDevice.getDevice(),
			&poolInfo,
			nullptr,
			&descriptorPool) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create ocean descriptor pool!");
		}
	}

	void Ocean::createDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType =
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(
			lveDevice.getDevice(),
			&allocInfo,
			&descriptorSet) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to allocate ocean descriptor set!");
		}

		VkDescriptorImageInfo heightMapInfo{};
		heightMapInfo.imageLayout =
			VK_IMAGE_LAYOUT_GENERAL;

		heightMapInfo.imageView =
			ifftObj.getHeightMapImageView();

		heightMapInfo.sampler =
			heightMapSampler;

		VkWriteDescriptorSet write{};
		write.sType =
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		write.dstSet = descriptorSet;
		write.dstBinding = 0;

		write.descriptorType =
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		write.descriptorCount = 1;
		write.pImageInfo = &heightMapInfo;

		vkUpdateDescriptorSets(
			lveDevice.getDevice(),
			1,
			&write,
			0,
			nullptr);
	}

	void Ocean::createGraphicsPipeline(
		VkRenderPass renderPass,
		VkDescriptorSetLayout globalDescriptorSetLayout,
		const std::string& vertexShaderPath,
		const std::string& fragmentShaderPath) {

		const std::vector<char> vertCode =
			readFile(vertexShaderPath);

		const std::vector<char> fragCode =
			readFile(fragmentShaderPath);

		VkShaderModule vertShader =
			createShaderModule(vertCode);

		VkShaderModule fragShader =
			createShaderModule(fragCode);

		VkPipelineShaderStageCreateInfo shaderStages[2]{};

		shaderStages[0].sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		shaderStages[0].stage =
			VK_SHADER_STAGE_VERTEX_BIT;

		shaderStages[0].module = vertShader;
		shaderStages[0].pName = "main";

		shaderStages[1].sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

		shaderStages[1].stage =
			VK_SHADER_STAGE_FRAGMENT_BIT;

		shaderStages[1].module = fragShader;
		shaderStages[1].pName = "main";

		const VkVertexInputBindingDescription bindingDescription =
			lve::LveModel::Vertex::getBindingDescription();

		const auto attributeDescriptions =
			lve::LveModel::Vertex::
			getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInput{};
		vertexInput.sType =
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		vertexInput.vertexBindingDescriptionCount = 1;
		vertexInput.pVertexBindingDescriptions =
			&bindingDescription;

		vertexInput.vertexAttributeDescriptionCount =
			static_cast<uint32_t>(
				attributeDescriptions.size());

		vertexInput.pVertexAttributeDescriptions =
			attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType =
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

		inputAssembly.topology =
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType =
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType =
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

		rasterizer.frontFace =
			VK_FRONT_FACE_COUNTER_CLOCKWISE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType =
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

		multisampling.rasterizationSamples =
			VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType =
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;

		depthStencil.depthCompareOp =
			VK_COMPARE_OP_LESS;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;

		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType =
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments =
			&colorBlendAttachment;

		const std::array<VkDynamicState, 2> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType =
			VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

		dynamicState.dynamicStateCount =
			static_cast<uint32_t>(
				dynamicStates.size());

		dynamicState.pDynamicStates =
			dynamicStates.data();

		VkDescriptorSetLayout setLayouts[2] = {
			globalDescriptorSetLayout, // set = 0
			descriptorSetLayout        // set = 1
		};

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags =
			VK_SHADER_STAGE_VERTEX_BIT;

		pushConstantRange.offset = 0;
		pushConstantRange.size =
			sizeof(OceanPushConstant);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		pipelineLayoutInfo.setLayoutCount = 2;
		pipelineLayoutInfo.pSetLayouts = setLayouts;

		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges =
			&pushConstantRange;

		if (vkCreatePipelineLayout(
			lveDevice.getDevice(),
			&pipelineLayoutInfo,
			nullptr,
			&pipelineLayout) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create ocean pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType =
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInput;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;

		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;

		if (vkCreateGraphicsPipelines(
			lveDevice.getDevice(),
			VK_NULL_HANDLE,
			1,
			&pipelineInfo,
			nullptr,
			&graphicsPipeline) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create ocean graphics pipeline!");
		}

		vkDestroyShaderModule(
			lveDevice.getDevice(),
			vertShader,
			nullptr);

		vkDestroyShaderModule(
			lveDevice.getDevice(),
			fragShader,
			nullptr);
	}

	void Ocean::recordHeightMapReadyForGraphics(
		VkCommandBuffer commandBuffer) const {

		VkImageMemoryBarrier barrier{};
		barrier.sType =
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

		barrier.srcQueueFamilyIndex =
			VK_QUEUE_FAMILY_IGNORED;

		barrier.dstQueueFamilyIndex =
			VK_QUEUE_FAMILY_IGNORED;

		barrier.image =
			ifftObj.getHeightMapImage();

		barrier.subresourceRange.aspectMask =
			VK_IMAGE_ASPECT_COLOR_BIT;

		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		barrier.srcAccessMask =
			VK_ACCESS_SHADER_WRITE_BIT;

		barrier.dstAccessMask =
			VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}

	void Ocean::draw(
		VkCommandBuffer commandBuffer,
		VkDescriptorSet globalDescriptorSet,
		float heightScale) {

		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicsPipeline);

		// set = 0，全局 UBO 和阴影
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&globalDescriptorSet,
			0,
			nullptr);

		// set = 1，Ocean 的 IFFT 高度图
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			1,
			1,
			&descriptorSet,
			0,
			nullptr);

		OceanPushConstant pushConstant{};
		pushConstant.oceanRange = oceanRange;
		pushConstant.heightScale = heightScale;

		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(OceanPushConstant),
			&pushConstant);

		model.bindVertex(commandBuffer);
		model.bindIndexBuffer(commandBuffer);

		vkCmdDrawIndexed(
			commandBuffer,
			static_cast<uint32_t>(indices.size()),
			1,
			0,
			0,
			0);
	}

	std::vector<char> Ocean::readFile(
		const std::string& filePath) {

		std::ifstream file(
			filePath,
			std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error(
				"failed to open ocean shader: " +
				filePath);
		}

		const size_t fileSize =
			static_cast<size_t>(file.tellg());

		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);

		return buffer;
	}

	VkShaderModule Ocean::createShaderModule(
		const std::vector<char>& code) const {

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType =
			VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		createInfo.codeSize = code.size();
		createInfo.pCode =
			reinterpret_cast<const uint32_t*>(
				code.data());

		VkShaderModule shaderModule{};

		if (vkCreateShaderModule(
			lveDevice.getDevice(),
			&createInfo,
			nullptr,
			&shaderModule) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create ocean shader module!");
		}

		return shaderModule;
	}

}