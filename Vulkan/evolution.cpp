#include "evolution.h"

#include <fstream>
#include <stdexcept>
#include <vector>

namespace evolution {

	Evolution::Evolution(
		lve::LveDevice& lveDevice,
		const spectrum::Spectrum& initialSpectrum,
		const std::string& shaderPath,
		uint32_t resolution,
		uint32_t oceanRange)
		: lveDevice{ lveDevice } {

		pushConstant.resolution = resolution;
		pushConstant.oceanRange = oceanRange;
		pushConstant.time = 0.0f;
		pushConstant.padding = 0.0f;

		createHtSpectrumImage();
		initializeHtSpectrumLayout();
		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSet(initialSpectrum);
		createComputePipeline(shaderPath);
	}

	Evolution::~Evolution() {
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

		if (htSpectrumImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device, htSpectrumImageView, nullptr);
		}

		if (htSpectrumImage != VK_NULL_HANDLE) {
			vkDestroyImage(device, htSpectrumImage, nullptr);
		}

		if (htSpectrumImageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device, htSpectrumImageMemory, nullptr);
		}
	}

	void Evolution::createHtSpectrumImage() {
		lveDevice.createImage(
			pushConstant.resolution,
			pushConstant.resolution,
			spectrumFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			htSpectrumImage,
			htSpectrumImageMemory);

		htSpectrumImageView = lveDevice.createImageView(
			htSpectrumImage,
			spectrumFormat,
			VK_IMAGE_ASPECT_COLOR_BIT);
	}

	void Evolution::initializeHtSpectrumLayout() {
		VkCommandBuffer commandBuffer;

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = lveDevice.getCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(
			lveDevice.getDevice(),
			&allocInfo,
			&commandBuffer) != VK_SUCCESS) {

			throw std::runtime_error("failed to allocate evolution layout command buffer!");
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = htSpectrumImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

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

		vkCreateFence(lveDevice.getDevice(), &fenceInfo, nullptr, &fence);
		vkQueueSubmit(lveDevice.getGraphicsQueue(), 1, &submitInfo, fence);
		vkWaitForFences(lveDevice.getDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

		vkDestroyFence(lveDevice.getDevice(), fence, nullptr);
		vkFreeCommandBuffers(
			lveDevice.getDevice(),
			lveDevice.getCommandPool(),
			1,
			&commandBuffer);
	}

	void Evolution::createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding h0Binding{};
		h0Binding.binding = 0;
		h0Binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		h0Binding.descriptorCount = 1;
		h0Binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding htBinding{};
		htBinding.binding = 1;
		htBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		htBinding.descriptorCount = 1;
		htBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutBinding bindings[] = {
			h0Binding,
			htBinding
		};

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 2;
		layoutInfo.pBindings = bindings;

		if (vkCreateDescriptorSetLayout(
			lveDevice.getDevice(),
			&layoutInfo,
			nullptr,
			&descriptorSetLayout) != VK_SUCCESS) {

			throw std::runtime_error("failed to create evolution descriptor set layout!");
		}
	}

	void Evolution::createDescriptorPool() {
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSize.descriptorCount = 2;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = 1;

		if (vkCreateDescriptorPool(
			lveDevice.getDevice(),
			&poolInfo,
			nullptr,
			&descriptorPool) != VK_SUCCESS) {

			throw std::runtime_error("failed to create evolution descriptor pool!");
		}
	}

	void Evolution::createDescriptorSet(const spectrum::Spectrum& initialSpectrum) {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(
			lveDevice.getDevice(),
			&allocInfo,
			&descriptorSet) != VK_SUCCESS) {

			throw std::runtime_error("failed to allocate evolution descriptor set!");
		}

		VkDescriptorImageInfo h0ImageInfo{};
		h0ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		h0ImageInfo.imageView = initialSpectrum.getImageView();

		VkDescriptorImageInfo htImageInfo{};
		htImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		htImageInfo.imageView = htSpectrumImageView;

		VkWriteDescriptorSet writes[2]{};

		writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[0].dstSet = descriptorSet;
		writes[0].dstBinding = 0;
		writes[0].descriptorCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[0].pImageInfo = &h0ImageInfo;

		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = descriptorSet;
		writes[1].dstBinding = 1;
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		writes[1].pImageInfo = &htImageInfo;

		vkUpdateDescriptorSets(
			lveDevice.getDevice(),
			2,
			writes,
			0,
			nullptr);
	}

	void Evolution::createComputePipeline(const std::string& shaderPath) {
		std::vector<char> shaderCode = readFile(shaderPath);
		VkShaderModule shaderModule = createShaderModule(shaderCode);

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(EvolutionPushConstant);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(
			lveDevice.getDevice(),
			&pipelineLayoutInfo,
			nullptr,
			&pipelineLayout) != VK_SUCCESS) {

			throw std::runtime_error("failed to create evolution pipeline layout!");
		}

		VkPipelineShaderStageCreateInfo shaderStageInfo{};
		shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageInfo.module = shaderModule;
		shaderStageInfo.pName = "main";

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage = shaderStageInfo;
		pipelineInfo.layout = pipelineLayout;

		if (vkCreateComputePipelines(
			lveDevice.getDevice(),
			VK_NULL_HANDLE,
			1,
			&pipelineInfo,
			nullptr,
			&computePipeline) != VK_SUCCESS) {

			vkDestroyShaderModule(lveDevice.getDevice(), shaderModule, nullptr);
			throw std::runtime_error("failed to create evolution compute pipeline!");
		}

		vkDestroyShaderModule(lveDevice.getDevice(), shaderModule, nullptr);
	}

	void Evolution::recordEvolutionCommands(
		VkCommandBuffer commandBuffer,
		float time) {

		pushConstant.time = time;

		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			computePipeline);

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			pipelineLayout,
			0,
			1,
			&descriptorSet,
			0,
			nullptr);

		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_COMPUTE_BIT,
			0,
			sizeof(EvolutionPushConstant),
			&pushConstant);

		uint32_t groupCount =
			(pushConstant.resolution + 15) / 16;

		vkCmdDispatch(commandBuffer, groupCount, groupCount, 1);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = htSpectrumImage;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}

	std::vector<char> Evolution::readFile(const std::string& filePath) {
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open evolution shader: " + filePath);
		}

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	VkShaderModule Evolution::createShaderModule(
		const std::vector<char>& code) {

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode =
			reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;

		if (vkCreateShaderModule(
			lveDevice.getDevice(),
			&createInfo,
			nullptr,
			&shaderModule) != VK_SUCCESS) {

			throw std::runtime_error("failed to create evolution shader module!");
		}

		return shaderModule;
	}

}