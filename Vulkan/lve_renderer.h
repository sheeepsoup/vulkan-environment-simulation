#pragma once
#include<vulkan/vulkan.h>
#include<vector>
#include<iostream>
#include"lve_swapChain.h"
#include"lve_device.h"
#include"lve_model.h"
#include"lve_uniform.h"
#include"lve_camera.h"
#include"lve_compute.h"
#include"lve_terrain.h"
#include"evolution.h"
#include"displacement.h"
#include"oceanCascade.h"
#include"ifft.h"
#include"ocean.h"
#include "shadow.h"
namespace lve {
	class LveRenderer {
	public:
		void createCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount);
		void createSignalSemaphore(VkDevice device, uint32_t swapChainImageCount);
		/*renderPrarams里面的参数单纯是表示各种微调shader的数值[imgui里] x是bias
		* 
		* 
		* 
		*/
		void run(VkDevice device, LveSwapChain& swapChain, VkQueue graphicsQueue, VkQueue presentQueue, uint32_t& currentFrame,
			VkRenderPass& renderPass, LveModel& model, const std::vector<VkDescriptorSet> descriptorSets, VkPipelineLayout pipelineLayout, LveUniform& uniform,
			const glm::mat4 modelMatirx, const glm::mat4 view, const glm::mat4 proj, LveCompute& compute, glm::vec3 cameraPos, std::vector<uint32_t>& indices,
			LveTerrain& terrain, float renderDistance, shadow::Shadow& shadow, const glm::mat4& lightViewProj, glm::vec4 renderParams,
			float time, ocean::Ocean& oceanObj, float oceanHeight, float horizontal_displacement_intensity, ocean_cascade::OceanCascade& bigOceanCascade,
			ocean_cascade::OceanCascade& middleOceanCascade, ocean_cascade::OceanCascade& smallOceanCascade, ocean_cascade::OceanCascade& tinyOceanCascade);
		uint32_t getImageIndex() { return imageIndex; };
		uint32_t getMaxFramesInFlight() { return MAX_FRAMES_IN_FLIGHT; };
		void clean(VkDevice device);
		VkPipeline& getGraphicsPipeline() { return graphicsPipeline; };
	private:
		std::vector<VkCommandBuffer> commandBuffers;//命令缓冲区[每帧的]
		std::vector<VkSemaphore> imageAvailableSemaphores; //信号量[每帧的]图像准备渲染的信号量
		std::vector<VkSemaphore> renderFinishedSemaphores; //信号量[每帧的]渲染完成的信号量
		std::vector<VkFence> inFlightFences;//栅栏[每帧的]
		const int MAX_FRAMES_IN_FLIGHT = 2;//同时处理多少帧

		uint32_t imageIndex;//当前交换链图像索引
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;//一个类,用来保存管线
		//命令缓冲区的记录
		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkRenderPass renderPass,
			VkFramebuffer framebuffer, VkExtent2D extent, LveModel& model, VkPipelineLayout pipelineLayout, uint32_t currentFrame,
			const std::vector<VkDescriptorSet> descriptorSets, LveCompute& compute, std::vector<uint32_t>& indices, LveTerrain& terrain,
			float renderDistance, glm::vec3 cameraPos, shadow::Shadow& shadow, const glm::mat4& lightViewProj,
			float time, ocean::Ocean& oceanObj, float oceanHeight, float horizontal_displacement_intensity, ocean_cascade::OceanCascade& bigOceanCascade,
			ocean_cascade::OceanCascade& middleOceanCascade, ocean_cascade::OceanCascade& smallOceanCascade, ocean_cascade::OceanCascade& tinyOceanCascade);
	};

}