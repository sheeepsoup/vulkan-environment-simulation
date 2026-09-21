#include "spectrum.h"

#include <fstream>
#include <stdexcept>

namespace spectrum {

	Spectrum::Spectrum(
		lve::LveDevice& device,
		const std::string& shaderPath,
		uint32_t resolution,
		uint32_t oceanRange
	) : lveDevice(device) {
		pushConstant.resolution = resolution;
		pushConstant.oceanRange = oceanRange;

		createImage();
		transitionImageToGeneral();
		createSampler();

		createDescriptorSetLayout();
		createDescriptorPool();
		createDescriptorSet();

		createPipelineLayout();
		createComputePipeline(shaderPath);
	}

	Spectrum::~Spectrum() {
		clean();
	}

	void Spectrum::createImage() {
		lveDevice.createImage(
			pushConstant.resolution,
			pushConstant.resolution,
			spectrumFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_STORAGE_BIT |
			VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			spectrumImage,
			spectrumImageMemory
		);

		spectrumImageView = lveDevice.createImageView(
			spectrumImage,
			spectrumFormat,
			VK_IMAGE_ASPECT_COLOR_BIT
		);
	}

	void Spectrum::createSampler() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(
			lveDevice.getDevice(),
			&samplerInfo,
			nullptr,
			&spectrumSampler
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create spectrum sampler!"
			);
		}
	}

	void Spectrum::transitionImageToGeneral() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = lveDevice.getCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		if (vkAllocateCommandBuffers(
			lveDevice.getDevice(),
			&allocInfo,
			&commandBuffer
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to allocate spectrum transition command buffer!"
			);
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to begin spectrum transition command buffer!"
			);
		}

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = spectrumImage;
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
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to end spectrum transition command buffer!"
			);
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		if (vkQueueSubmit(
			lveDevice.getGraphicsQueue(),
			1,
			&submitInfo,
			VK_NULL_HANDLE
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to submit spectrum layout transition!"
			);
		}

		vkQueueWaitIdle(lveDevice.getGraphicsQueue());

		vkFreeCommandBuffers(
			lveDevice.getDevice(),
			lveDevice.getCommandPool(),
			1,
			&commandBuffer
		);
	}

	void Spectrum::createDescriptorSetLayout() {
		VkDescriptorSetLayoutBinding spectrumBinding{};
		spectrumBinding.binding = 0;
		spectrumBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		spectrumBinding.descriptorCount = 1;
		spectrumBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType =
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &spectrumBinding;

		if (vkCreateDescriptorSetLayout(
			lveDevice.getDevice(),
			&layoutInfo,
			nullptr,
			&descriptorSetLayout
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create spectrum descriptor set layout!"
			);
		}
	}

	void Spectrum::createDescriptorPool() {
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSize.descriptorCount = 1;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = 1;

		if (vkCreateDescriptorPool(
			lveDevice.getDevice(),
			&poolInfo,
			nullptr,
			&descriptorPool
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create spectrum descriptor pool!"
			);
		}
	}

	void Spectrum::createDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType =
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &descriptorSetLayout;

		if (vkAllocateDescriptorSets(
			lveDevice.getDevice(),
			&allocInfo,
			&descriptorSet
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to allocate spectrum descriptor set!"
			);
		}

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = spectrumImageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = descriptorSet;
		write.dstBinding = 0;
		write.dstArrayElement = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		write.descriptorCount = 1;
		write.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(
			lveDevice.getDevice(),
			1,
			&write,
			0,
			nullptr
		);
	}

	void Spectrum::createPipelineLayout() {
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SpectrumPushConstant);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType =
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

		if (vkCreatePipelineLayout(
			lveDevice.getDevice(),
			&pipelineLayoutInfo,
			nullptr,
			&pipelineLayout
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create spectrum pipeline layout!"
			);
		}
	}

	void Spectrum::createComputePipeline(
		const std::string& shaderPath
	) {
		shaderModule = createShaderModule(
			lveDevice.getDevice(),
			readFile(shaderPath)
		);

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
			&computePipeline
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create spectrum compute pipeline!"
			);
		}
	}

	void Spectrum::recordInitialSpectrum(
		VkCommandBuffer commandBuffer
	) {
		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			computePipeline
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
			sizeof(SpectrumPushConstant),
			&pushConstant
		);

		vkCmdDispatch(
			commandBuffer,
			(pushConstant.resolution + 15) / 16,
			(pushConstant.resolution + 15) / 16,
			1
		);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = spectrumImage;
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
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
			VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);
	}

	std::vector<char> Spectrum::readFile(
		const std::string& filename
	) {
		std::ifstream file(
			filename,
			std::ios::ate | std::ios::binary
		);

		if (!file.is_open()) {
			throw std::runtime_error(
				"failed to open spectrum shader: " + filename
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

	VkShaderModule Spectrum::createShaderModule(
		VkDevice device,
		const std::vector<char>& code
	) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType =
			VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode =
			reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule module = VK_NULL_HANDLE;

		if (vkCreateShaderModule(
			device,
			&createInfo,
			nullptr,
			&module
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create spectrum shader module!"
			);
		}

		return module;
	}

	void Spectrum::clean() {
		VkDevice device = lveDevice.getDevice();

		if (computePipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(device, computePipeline, nullptr);
			computePipeline = VK_NULL_HANDLE;
		}

		if (pipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			pipelineLayout = VK_NULL_HANDLE;
		}

		if (descriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(device, descriptorPool, nullptr);
			descriptorPool = VK_NULL_HANDLE;
		}

		if (descriptorSetLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(
				device,
				descriptorSetLayout,
				nullptr
			);
			descriptorSetLayout = VK_NULL_HANDLE;
		}

		if (shaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(device, shaderModule, nullptr);
			shaderModule = VK_NULL_HANDLE;
		}

		if (spectrumSampler != VK_NULL_HANDLE) {
			vkDestroySampler(device, spectrumSampler, nullptr);
			spectrumSampler = VK_NULL_HANDLE;
		}

		if (spectrumImageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device, spectrumImageView, nullptr);
			spectrumImageView = VK_NULL_HANDLE;
		}

		if (spectrumImage != VK_NULL_HANDLE) {
			vkDestroyImage(device, spectrumImage, nullptr);
			spectrumImage = VK_NULL_HANDLE;
		}

		if (spectrumImageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device, spectrumImageMemory, nullptr);
			spectrumImageMemory = VK_NULL_HANDLE;
		}
	}
	void Spectrum::generateInitialSpectrum() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = lveDevice.getCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		if (vkAllocateCommandBuffers(
			lveDevice.getDevice(),
			&allocInfo,
			&commandBuffer
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to allocate spectrum compute command buffer!"
			);
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			vkFreeCommandBuffers(
				lveDevice.getDevice(),
				lveDevice.getCommandPool(),
				1,
				&commandBuffer
			);

			throw std::runtime_error(
				"failed to begin spectrum compute command buffer!"
			);
		}

		recordInitialSpectrum(commandBuffer);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			vkFreeCommandBuffers(
				lveDevice.getDevice(),
				lveDevice.getCommandPool(),
				1,
				&commandBuffer
			);

			throw std::runtime_error(
				"failed to end spectrum compute command buffer!"
			);
		}

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VkFence fence = VK_NULL_HANDLE;

		if (vkCreateFence(
			lveDevice.getDevice(),
			&fenceInfo,
			nullptr,
			&fence
		) != VK_SUCCESS) {
			vkFreeCommandBuffers(
				lveDevice.getDevice(),
				lveDevice.getCommandPool(),
				1,
				&commandBuffer
			);

			throw std::runtime_error(
				"failed to create spectrum compute fence!"
			);
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		if (vkQueueSubmit(
			lveDevice.getGraphicsQueue(),
			1,
			&submitInfo,
			fence
		) != VK_SUCCESS) {
			vkDestroyFence(
				lveDevice.getDevice(),
				fence,
				nullptr
			);

			vkFreeCommandBuffers(
				lveDevice.getDevice(),
				lveDevice.getCommandPool(),
				1,
				&commandBuffer
			);

			throw std::runtime_error(
				"failed to submit initial spectrum compute command!"
			);
		}

		if (vkWaitForFences(
			lveDevice.getDevice(),
			1,
			&fence,
			VK_TRUE,
			UINT64_MAX
		) != VK_SUCCESS) {
			vkDestroyFence(
				lveDevice.getDevice(),
				fence,
				nullptr
			);

			vkFreeCommandBuffers(
				lveDevice.getDevice(),
				lveDevice.getCommandPool(),
				1,
				&commandBuffer
			);

			throw std::runtime_error(
				"failed to wait for initial spectrum generation!"
			);
		}

		vkDestroyFence(
			lveDevice.getDevice(),
			fence,
			nullptr
		);

		vkFreeCommandBuffers(
			lveDevice.getDevice(),
			lveDevice.getCommandPool(),
			1,
			&commandBuffer
		);
	}
}