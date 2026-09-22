#include "ifft.h"

#include <fstream>
#include <stdexcept>

namespace ifft {

	IFFT::IFFT(
		lve::LveDevice& lveDevice,
		const evolution::Evolution& evolution,
		const std::string& shaderPath,
		uint32_t resolution)
		: lveDevice{ lveDevice } {

		if (resolution == 0 || (resolution & (resolution - 1)) != 0) {
			throw std::runtime_error(
				"IFFT resolution must be a power of two!");
		}

		pushConstant.resolution = resolution;
		pushConstant.stage = 0;
		pushConstant.axis = 0;
		pushConstant.padding = 0;

		createWorkingImages();
		initializeImageLayout(pingImage);
		initializeImageLayout(pongImage);

		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSets(evolution.getHtSpectrumImageView());
		createComputePipeline(shaderPath);
	}

	IFFT::~IFFT() {
		VkDevice device = lveDevice.getDevice();

		if (computePipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(device, computePipeline, nullptr);
		}

		if (pipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		}

		if (descriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		}

		if (descriptorSetLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		}

		if (pingImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device, pingImageView, nullptr);
		}

		if (pingImage != VK_NULL_HANDLE) {
			vkDestroyImage(device, pingImage, nullptr);
		}

		if (pingImageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device, pingImageMemory, nullptr);
		}

		if (pongImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device, pongImageView, nullptr);
		}

		if (pongImage != VK_NULL_HANDLE) {
			vkDestroyImage(device, pongImage, nullptr);
		}

		if (pongImageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device, pongImageMemory, nullptr);
		}
	}

	void IFFT::createWorkingImages() {
		const VkImageUsageFlags usage =
			VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT;

		lveDevice.createImage(
			pushConstant.resolution,
			pushConstant.resolution,
			imageFormat,
			VK_IMAGE_TILING_OPTIMAL,
			usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			pingImage,
			pingImageMemory);

		pingImageView = lveDevice.createImageView(
			pingImage,
			imageFormat,
			VK_IMAGE_ASPECT_COLOR_BIT);

		lveDevice.createImage(
			pushConstant.resolution,
			pushConstant.resolution,
			imageFormat,
			VK_IMAGE_TILING_OPTIMAL,
			usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			pongImage,
			pongImageMemory);

		pongImageView = lveDevice.createImageView(
			pongImage,
			imageFormat,
			VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void IFFT::initializeImageLayout(VkImage image) {
		VkCommandBuffer commandBuffer;

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType =
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = lveDevice.getCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(
			lveDevice.getDevice(),
			&allocInfo,
			&commandBuffer) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to allocate IFFT layout command buffer!");
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType =
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags =
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkImageMemoryBarrier barrier{};
		barrier.sType =
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex =
			VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex =
			VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask =
			VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask =
			VK_ACCESS_SHADER_WRITE_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkFence fence;
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		vkCreateFence(
			lveDevice.getDevice(),
			&fenceInfo,
			nullptr,
			&fence);

		vkQueueSubmit(
			lveDevice.getGraphicsQueue(),
			1,
			&submitInfo,
			fence);

		vkWaitForFences(
			lveDevice.getDevice(),
			1,
			&fence,
			VK_TRUE,
			UINT64_MAX);

		vkDestroyFence(lveDevice.getDevice(), fence, nullptr);

		vkFreeCommandBuffers(
			lveDevice.getDevice(),
			lveDevice.getCommandPool(),
			1,
			&commandBuffer);
	}

	void IFFT::createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding inputBinding{};
		inputBinding.binding = 0;
		inputBinding.descriptorType =
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		inputBinding.descriptorCount = 1;
		inputBinding.stageFlags =
			VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding outputBinding{};
		outputBinding.binding = 1;
		outputBinding.descriptorType =
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		outputBinding.descriptorCount = 1;
		outputBinding.stageFlags =
			VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding bindings[] = {
			inputBinding,
			outputBinding
		};

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType =
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 2;
		layoutInfo.pBindings = bindings;

		if (vkCreateDescriptorSetLayout(
			lveDevice.getDevice(),
			&layoutInfo,
			nullptr,
			&descriptorSetLayout) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create IFFT descriptor set layout!");
		}
	}

	void IFFT::createDescriptorPool() {
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSize.descriptorCount = 6;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType =
			VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = 3;

		if (vkCreateDescriptorPool(
			lveDevice.getDevice(),
			&poolInfo,
			nullptr,
			&descriptorPool) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create IFFT descriptor pool!");
		}
	}

	void IFFT::createDescriptorSets(
		VkImageView htSpectrumImageView) {

		VkDescriptorSetLayout layouts[] = {
			descriptorSetLayout,
			descriptorSetLayout,
			descriptorSetLayout
		};

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType =
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 3;
		allocInfo.pSetLayouts = layouts;

		if (vkAllocateDescriptorSets(
			lveDevice.getDevice(),
			&allocInfo,
			descriptorSets) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to allocate IFFT descriptor sets!");
		}

		VkImageView inputViews[3] = {
			htSpectrumImageView,
			pingImageView,
			pongImageView
		};

		VkImageView outputViews[3] = {
			pingImageView,
			pongImageView,
			pingImageView
		};

		for (uint32_t i = 0; i < 3; i++) {
			VkDescriptorImageInfo inputInfo{};
			inputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			inputInfo.imageView = inputViews[i];

			VkDescriptorImageInfo outputInfo{};
			outputInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			outputInfo.imageView = outputViews[i];

			VkWriteDescriptorSet writes[2]{};

			writes[0].sType =
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = descriptorSets[i];
			writes[0].dstBinding = 0;
			writes[0].descriptorCount = 1;
			writes[0].descriptorType =
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writes[0].pImageInfo = &inputInfo;

			writes[1].sType =
				VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet = descriptorSets[i];
			writes[1].dstBinding = 1;
			writes[1].descriptorCount = 1;
			writes[1].descriptorType =
				VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writes[1].pImageInfo = &outputInfo;

			vkUpdateDescriptorSets(
				lveDevice.getDevice(),
				2,
				writes,
				0,
				nullptr);
		}
	}

	void IFFT::createComputePipeline(
		const std::string& shaderPath) {

		std::vector<char> shaderCode = readFile(shaderPath);
		VkShaderModule shaderModule =
			createShaderModule(shaderCode);

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags =
			VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size =
			sizeof(IFFTPushConstant);

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
			lveDevice.getDevice(),
			&pipelineLayoutInfo,
			nullptr,
			&pipelineLayout) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create IFFT pipeline layout!");
		}

		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = "main";

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType =
			VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = shaderStageInfo;
		pipelineInfo.layout = pipelineLayout;

		if (vkCreateComputePipelines(
			lveDevice.getDevice(),
			VK_NULL_HANDLE,
			1,
			&pipelineInfo,
			nullptr,
			&computePipeline) != VK_SUCCESS) {

			vkDestroyShaderModule(
				lveDevice.getDevice(),
				shaderModule,
				nullptr);

			throw std::runtime_error(
				"failed to create IFFT compute pipeline!");
		}

		vkDestroyShaderModule(
			lveDevice.getDevice(),
			shaderModule,
			nullptr);
	}

	uint32_t IFFT::getStageCount() const {
		uint32_t stageCount = 0;
		uint32_t value = pushConstant.resolution;

		while (value > 1) {
			value >>= 1;
			stageCount++;
		}

		return stageCount;
	}

	void IFFT::barrierForNextIFFTStage(
		VkCommandBuffer commandBuffer,
		VkImage writtenImage) {

		VkImageMemoryBarrier barrier{};
		barrier.sType =
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex =
			VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex =
			VK_QUEUE_FAMILY_IGNORED;
		barrier.image = writtenImage;
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
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}

	void IFFT::recordIFFTCommands(
		VkCommandBuffer commandBuffer) {

		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			computePipeline);

		const uint32_t stageCount = getStageCount();
		const uint32_t groupCount =
			(pushConstant.resolution + 15) / 16;

		bool isFirstPass = true;
		bool sourceIsPing = false;

		for (uint32_t axis = 0; axis < 2; axis++) {
			for (uint32_t stage = 0;
				stage < stageCount;
				stage++) {

				pushConstant.axis = axis;
				pushConstant.stage = stage;

				VkDescriptorSet activeSet;
				VkImage writtenImage;

				if (isFirstPass) {
					// htSpectrum -> ping
					activeSet = descriptorSets[0];
					writtenImage = pingImage;
					sourceIsPing = true;
					isFirstPass = false;
				}
				else if (sourceIsPing) {
					// ping -> pong
					activeSet = descriptorSets[1];
					writtenImage = pongImage;
					sourceIsPing = false;
				}
				else {
					// pong -> ping
					activeSet = descriptorSets[2];
					writtenImage = pingImage;
					sourceIsPing = true;
				}

				vkCmdBindDescriptorSets(
					commandBuffer,
					VK_PIPELINE_BIND_POINT_COMPUTE,
					pipelineLayout,
					0,
					1,
					&activeSet,
					0,
					nullptr);

				vkCmdPushConstants(
					commandBuffer,
					pipelineLayout,
					VK_SHADER_STAGE_COMPUTE_BIT,
					0,
					sizeof(IFFTPushConstant),
					&pushConstant);

				vkCmdDispatch(
					commandBuffer,
					groupCount,
					groupCount,
					1);

				barrierForNextIFFTStage(
					commandBuffer,
					writtenImage);
			}
		}
	}

	std::vector<char> IFFT::readFile(
		const std::string& filePath) {

		std::ifstream file(
			filePath,
			std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error(
				"failed to open IFFT shader: " + filePath);
		}

		size_t fileSize =
			static_cast<size_t>(file.tellg());

		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	VkShaderModule IFFT::createShaderModule(
		const std::vector<char>& code) {

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType =
			VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode =
			reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;

		if (vkCreateShaderModule(
			lveDevice.getDevice(),
			&createInfo,
			nullptr,
			&shaderModule) != VK_SUCCESS) {

			throw std::runtime_error(
				"failed to create IFFT shader module!");
		}

		return shaderModule;
	}

}