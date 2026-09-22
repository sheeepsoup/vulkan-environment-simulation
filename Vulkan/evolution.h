#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>

#include "lve_device.h"
#include "spectrum.h"

namespace evolution {

	struct EvolutionPushConstant {
		uint32_t resolution;
		uint32_t oceanRange;
		float time;
		float padding;
	};

	class Evolution {
	public:
		Evolution(
			lve::LveDevice& lveDevice,
			const spectrum::Spectrum& initialSpectrum,
			const std::string& shaderPath,
			uint32_t resolution,
			uint32_t oceanRange);

		~Evolution();

		Evolution(const Evolution&) = delete;
		Evolution& operator=(const Evolution&) = delete;
		VkImageView getHtSpectrumView() const {
			return htSpectrumImageView;
		}
		// 每帧录制；必须在 IFFT 和海洋绘制之前调用
		void recordEvolutionCommands(VkCommandBuffer commandBuffer, float time);

		VkImage getHtSpectrumImage() const { return htSpectrumImage; }
		VkImageView getHtSpectrumImageView() const { return htSpectrumImageView; }
		VkFormat getFormat() const { return spectrumFormat; }

	private:
		void createHtSpectrumImage();
		void initializeHtSpectrumLayout();
		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDescriptorSet(const spectrum::Spectrum& initialSpectrum);
		void createComputePipeline(const std::string& shaderPath);

		static std::vector<char> readFile(const std::string& filePath);
		VkShaderModule createShaderModule(const std::vector<char>& code);

		lve::LveDevice& lveDevice;

		EvolutionPushConstant pushConstant{};

		VkFormat spectrumFormat = VK_FORMAT_R32G32_SFLOAT;

		VkImage htSpectrumImage = VK_NULL_HANDLE;
		VkDeviceMemory htSpectrumImageMemory = VK_NULL_HANDLE;
		VkImageView htSpectrumImageView = VK_NULL_HANDLE;

		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline computePipeline = VK_NULL_HANDLE;
	};

}