#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

#include "lve_device.h"
#include "evolution.h"

namespace ifft {

	struct IFFTPushConstant {
		uint32_t resolution;
		uint32_t stage;
		uint32_t axis;      // 0 = 横向，1 = 纵向
		uint32_t padding;
	};

	class IFFT {
	public:
		IFFT(
			lve::LveDevice& lveDevice,
			const evolution::Evolution& evolution,
			const std::string& shaderPath,
			uint32_t resolution);

		~IFFT();

		IFFT(const IFFT&) = delete;
		IFFT& operator=(const IFFT&) = delete;

		// 在 Evolution 后、绘制海洋前调用
		void recordIFFTCommands(VkCommandBuffer commandBuffer);

		// 所有横向 + 纵向 IFFT 完成后的最终结果
		VkImage getHeightMapImage() const { return pongImage; }
		VkImageView getHeightMapImageView() const { return pongImageView; }
		VkFormat getFormat() const { return imageFormat; }

	private:
		void createWorkingImages();
		void initializeImageLayout(VkImage image);

		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDescriptorSets(VkImageView htSpectrumImageView);
		void createComputePipeline(const std::string& shaderPath);

		void barrierForNextIFFTStage(
			VkCommandBuffer commandBuffer,
			VkImage writtenImage);

		uint32_t getStageCount() const;

		static std::vector<char> readFile(const std::string& filePath);
		VkShaderModule createShaderModule(const std::vector<char>& code);

		lve::LveDevice& lveDevice;
		IFFTPushConstant pushConstant{};

		VkFormat imageFormat = VK_FORMAT_R32G32_SFLOAT;

		// 两张中间图交替读写
		VkImage pingImage = VK_NULL_HANDLE;
		VkDeviceMemory pingImageMemory = VK_NULL_HANDLE;
		VkImageView pingImageView = VK_NULL_HANDLE;

		VkImage pongImage = VK_NULL_HANDLE;
		VkDeviceMemory pongImageMemory = VK_NULL_HANDLE;
		VkImageView pongImageView = VK_NULL_HANDLE;

		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

		// 0: htSpectrum -> ping
		// 1: ping -> pong
		// 2: pong -> ping
		VkDescriptorSet descriptorSets[3]{
			VK_NULL_HANDLE,
			VK_NULL_HANDLE,
			VK_NULL_HANDLE
		};

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline computePipeline = VK_NULL_HANDLE;
	};

}