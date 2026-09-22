#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include "ifft.h"
#include "lve_device.h"
#include "lve_model.h"

namespace ocean {

	struct OceanPushConstant {
		float oceanRange;
		float heightScale;
		float padding0;
		float padding1;
	};

	class Ocean {
	public:
		Ocean(
			lve::LveDevice& lveDevice,
			const ifft::IFFT& ifftObj,
			VkRenderPass renderPass,
			VkDescriptorSetLayout globalDescriptorSetLayout,
			const std::string& vertexShaderPath,
			const std::string& fragmentShaderPath,
			uint32_t meshResolution,
			float oceanRange);

		~Ocean();

		Ocean(const Ocean&) = delete;
		Ocean& operator=(const Ocean&) = delete;

		// IFFT 后、RenderPass 前调用
		void recordHeightMapReadyForGraphics(
			VkCommandBuffer commandBuffer) const;

		// 在 RenderPass 内调用
		// globalDescriptorSet 就是 uniform.getDescriptorSets()[currentFrame]
		void draw(
			VkCommandBuffer commandBuffer,
			VkDescriptorSet globalDescriptorSet,
			float heightScale);

		float getOceanRange() const {
			return oceanRange;
		}

	private:
		void createMesh();

		void createHeightMapSampler();

		// Ocean 专用 set = 1, binding = 0
		void createDescriptorSetLayout();
		void createDescriptorPool();
		void createDescriptorSet();

		void createGraphicsPipeline(
			VkRenderPass renderPass,
			VkDescriptorSetLayout globalDescriptorSetLayout,
			const std::string& vertexShaderPath,
			const std::string& fragmentShaderPath);

		static std::vector<char> readFile(
			const std::string& filePath);

		VkShaderModule createShaderModule(
			const std::vector<char>& code) const;

		lve::LveDevice& lveDevice;
		const ifft::IFFT& ifftObj;

		lve::LveModel model;

		std::vector<lve::LveModel::Vertex> vertices;
		std::vector<uint32_t> indices;

		VkSampler heightMapSampler = VK_NULL_HANDLE;

		VkDescriptorSetLayout descriptorSetLayout =
			VK_NULL_HANDLE;

		VkDescriptorPool descriptorPool =
			VK_NULL_HANDLE;

		VkDescriptorSet descriptorSet =
			VK_NULL_HANDLE;

		VkPipelineLayout pipelineLayout =
			VK_NULL_HANDLE;

		VkPipeline graphicsPipeline =
			VK_NULL_HANDLE;

		uint32_t meshResolution = 0;
		float oceanRange = 0.0f;
	};

}