#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

#include "lve_device.h"

namespace spectrum {

	struct SpectrumPushConstant {
		uint32_t resolution;
		uint32_t oceanRange;
	};

	class Spectrum {
	public:
		Spectrum(
			lve::LveDevice& device,
			const std::string& shaderPath,
			uint32_t resolution,
			uint32_t oceanRange
		);

		~Spectrum();
		void generateInitialSpectrum();//生成初始频谱图
		Spectrum(const Spectrum&) = delete;
		Spectrum& operator=(const Spectrum&) = delete;
		void clean();
		
		VkImage getImage() const { return spectrumImage; }
		VkImageView getImageView() const { return spectrumImageView; }
		VkSampler getSampler() const { return spectrumSampler; }

	private:
		lve::LveDevice& lveDevice;
		SpectrumPushConstant pushConstant{};

		VkFormat spectrumFormat = VK_FORMAT_R32G32_SFLOAT;

		VkImage spectrumImage = VK_NULL_HANDLE;
		VkDeviceMemory spectrumImageMemory = VK_NULL_HANDLE;
		VkImageView spectrumImageView = VK_NULL_HANDLE;
		VkSampler spectrumSampler = VK_NULL_HANDLE;

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline computePipeline = VK_NULL_HANDLE;

		std::vector<char> readFile(const std::string& filename);
		VkShaderModule createShaderModule(
			VkDevice device,
			const std::vector<char>& code
		);

		void createImage();
		void createSampler();
		void transitionImageToGeneral();
		void recordInitialSpectrum(VkCommandBuffer commandBuffer);
		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDescriptorSet();
		void createPipelineLayout();
		void createComputePipeline(const std::string& shaderPath);

		
	};

}