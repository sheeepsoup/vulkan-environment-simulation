#pragma once
#include<vulkan/vulkan.h>
#include<iostream>
#include<vector>
#include<SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>
#include<set>
#include<string>
#include"lve_windows.h"

#ifdef NDEBUG//Debug模式->自动开启验证层 Release模式->自动关闭验证层
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
const std::vector<const char*> validationLayers = {//需要用到的验证层列表
	"VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME

};//要用的逻辑设备拓展

namespace lve {
	class LveDevice
	{
	public:
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		VkCommandBuffer beginSingleTimeCommands();

		void endSingleTimeCommands(
			VkCommandBuffer commandBuffer
		);
		void createInstance();
		void pickPhysicalDevice();
		void createQueueFamiliesIndices(LveWindows& win);
		void createLogicalDevice();
		void createArrHandle();
		void createCommandPool(uint32_t queueFamilyIndex);
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
			VkDeviceMemory& bufferMemory);
		VkPhysicalDevice getPhysicalDevice() const noexcept { return physicalDevice; };
		VkDevice getDevice()const noexcept { return device; };
		VkInstance getInstance() const noexcept { return instance; };
		VkCommandPool getCommandPool()const noexcept { return commandPool; };
		VkQueue getGraphicsQueue() const noexcept { return graphicsQueue; };
		VkQueue getPresentQueue() const noexcept { return presentQueue; };
		struct QueueFamilyIndices {//对列簇索引
			int graphicsFamily = -1;//绘画列队簇
			int presentFamily = -1;//呈现列队簇
			bool isComplete() {
				return graphicsFamily >= 0 && presentFamily >= 0;
			}
		};
		//获取队列簇
		QueueFamilyIndices finQueueFamiles(const VkPhysicalDevice device,LveWindows& win);
		void clean(VkDevice device, VkInstance instance);
		void copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size);//拷贝缓冲区
		VkFormat findDepthFormat();//查询物理设备支持的深度格式
		void createImage(uint32_t width,uint32_t height,VkFormat format,VkImageTiling tiling,VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties,VkImage& image,VkDeviceMemory& imageMemory);
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);//查找内存类型
		void transitionImageLayout(
			VkImage image,
			VkFormat format,
			VkImageLayout oldLayout,
			VkImageLayout newLayout
		);//转换图像布局
	
	private:

		VkInstance instance = VK_NULL_HANDLE;//vk实例
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;//物理设备
		VkQueue graphicsQueue = VK_NULL_HANDLE;//检索列队句柄
		VkQueue presentQueue = VK_NULL_HANDLE;//呈现列队句柄
		VkCommandPool commandPool = VK_NULL_HANDLE;//命令池
		VkDevice device;  // 逻辑设备
		QueueFamilyIndices queueFamliesIndices;//列队簇索引
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;//设置创建的列队簇属性
		VkPhysicalDeviceFeatures deviceFeatures{};//指定使用的设备功能全部默认false（不启用任何可选特性）
		VkCommandBuffer commandBuffer;//命令缓冲区句柄

		std::vector<const char*> getRequiredExtensions();//获取所需扩展
		bool checkValidationLayerSupport();//检查验证层支持
		bool isDeviceSuitable(VkPhysicalDevice device);//检查显卡是否支持vk
		//检查显卡是否支持vk
		bool isDeviceSuitable2(VkPhysicalDevice device, LveWindows& win);
	
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	

		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);//寻找支持的模式[例如深度缓冲]

		bool hasStencilComponent(VkFormat format) {//告诉所选的深度格式是否包含模板缓冲分量
			return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		}
	};
}