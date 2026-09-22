#include"lve_device.h"
namespace lve {
	void LveDevice::createInstance() {
		//1.创建应用信息
		VkApplicationInfo appInfo{};
		appInfo.apiVersion = VK_API_VERSION_1_3;//使用1.3.0版本的vulkan
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 3, 0);//支持1.3.0版本的vulkan
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;//说明使用这个结构,用于方便vk读取[不是程序员管理]
		appInfo.pApplicationName = "从零开始的vulkan生活";//程序名
		appInfo.pEngineName = nullptr;//使用的引擎名
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);//引擎版本,没有就用vk1.0.0
		appInfo.pNext = nullptr;

		//2.填充实例创建信息
		VkInstanceCreateInfo creatInfo{};
		creatInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;//老样子
		creatInfo.pApplicationInfo = &appInfo;//应用信息


		auto extensions_get = getRequiredExtensions();
		creatInfo.enabledExtensionCount = static_cast<uint32_t>(extensions_get.size());//开启的拓展数量
		creatInfo.ppEnabledExtensionNames = extensions_get.data();//对应的拓展名称

		//分析验证层是否启用
		if (enableValidationLayers) {
			//开启验证层了
			if (checkValidationLayerSupport()) {//检查验证层是否支持
				creatInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				creatInfo.ppEnabledLayerNames = validationLayers.data();
			}
			else
			{
				creatInfo.enabledLayerCount = 0;//开启的验证层数量
				creatInfo.ppEnabledLayerNames = nullptr;//验证层名称列表
				std::cout << "EnableLayer is not support!!" << std::endl;
			}
		}


		creatInfo.pNext = nullptr;
		creatInfo.flags = 0;//标志位,目前填0


		VkResult result = vkCreateInstance(&creatInfo, nullptr, &instance);
		if (result != VK_SUCCESS) {//检查是否成功
			std::cout << "create instance error!!" << result << std::endl;
			system("pause");
		}
	}

	std::vector<const char*> LveDevice::getRequiredExtensions() {//获取消息回调
		// 1. 获取 SDL 要求的 Vulkan 实例扩展
		Uint32 sdlExtensionCount = 0;
		const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

		// 2. 创建一个新的 vector，用来存放最终的扩展列表
		std::vector<const char*> extensions;

		// 3. 将 SDL 的扩展添加到 vector 中
		for (Uint32 i = 0; i < sdlExtensionCount; ++i) {
			extensions.push_back(sdlExtensions[i]);
		}

		// 4. ★ 检查是否支持调试扩展，如果支持就添加上去 ★
		//    这部分逻辑和你在验证层部分做的检查类似
		uint32_t extensionPropertyCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionPropertyCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionPropertyCount, availableExtensions.data());

		bool debugUtilsSupported = false;
		for (const auto& ext : availableExtensions) {
			if (strcmp(ext.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
				debugUtilsSupported = true;
				break;
			}
		}

		if (debugUtilsSupported) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool LveDevice::checkValidationLayerSupport() {//检查所请求的验证层
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);//列出可用的验证层数量

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());//获取对应的验证层数据

			for (const char* layerName : validationLayers) {//遍历验证层里面
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers) {//遍历可用列表里面
					if (strcmp(layerName, layerProperties.layerName) == 0) {//strcmp,比较字符串是否相等,==0代表相等
						layerFound = true;
						break;
					}
				}

				if (!layerFound) {
					return false;
				}
			}
	}
	bool LveDevice::isDeviceSuitable(VkPhysicalDevice device) {
		//vkGetPhysicalDeviceProperties遍历来获取对应属性 基本的设备属性像name,type以及Vulkan版本都可通过此方法获取
		//检查列簇
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamlies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamlies.data());

		bool graphicsFound = false;
		for (uint32_t i = 0; i < queueFamilyCount; i++) {
			if (queueFamlies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) { //&表示包含就行(省去了再一次for循环后==)
				graphicsFound = true;
				break;
			}
		}
		//检查交换链拓展
		//暂时留空
		bool swapchainSupported = true;

		return graphicsFound && swapchainSupported;
	}
	void LveDevice::pickPhysicalDevice() {//获取物理设备
		uint32_t deviceCount = 0;//物理设备数量
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			std::cout << "failed to find GPUs with Vulkan support!" << std::endl;
		}
		std::vector<VkPhysicalDevice> devices_arr(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices_arr.data());//分配存储的句柄

		//寻找可用vk的显卡
		std::vector<VkPhysicalDevice> SuitableDevice;

		for (const auto& nowDevice : devices_arr) {
			if (isDeviceSuitable(nowDevice)) {
				SuitableDevice.push_back(nowDevice);
			}
		}
		if (SuitableDevice.size() == 0) {
			std::cout << "failed to find a suitable GPU!" << std::endl;
		}
		// 打印选中的显卡名称
		for (const auto& selectDevice : SuitableDevice) {
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(selectDevice, &props);
			std::cout << "suitable GPU: " << props.deviceName << std::endl;
		}
		std::cout << "defate us twice GPU" << std::endl;
		physicalDevice = SuitableDevice[1];
	}

	bool LveDevice::isDeviceSuitable2(VkPhysicalDevice device,LveWindows& win) {
		QueueFamilyIndices indices = finQueueFamiles(device, win);

		bool extensionsSupported = checkDeviceExtensionSupport(device);

		return indices.isComplete() && extensionsSupported;
	}

	LveDevice::QueueFamilyIndices LveDevice::finQueueFamiles(const VkPhysicalDevice device,LveWindows& win) {
		QueueFamilyIndices indices;
		uint32_t queueFamlityCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamlityCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamlityCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamlityCount, queueFamilies.data());//获取列队簇

		//遍历填充
		int i = 0;//对应的队列位置
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}
			VkBool32 presentSupport = false;//寻找窗口呈现列队簇的支持
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, win.getSurface(), &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}
			if (indices.isComplete()) {
				break;
			}
			i++;
		}

		return indices;
	}
	bool LveDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) {//检查是否支持拓展
		uint32_t extensionCount;//依旧
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}
	void LveDevice::createQueueFamiliesIndices(LveWindows& win) {
		queueFamliesIndices = finQueueFamiles(physicalDevice, win);//获取队列簇
		std::set<int> uniqueQueueFamilies = { queueFamliesIndices.graphicsFamily, queueFamliesIndices.presentFamily };//去重
		float queuePriority = 1.0f;
		
		for (int queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo qci{};
			qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;//结构体类型
			qci.queueFamilyIndex = queueFamily;//列队簇索引[上面已经获取了]
			qci.queueCount = 1;//申请创建一个队列用于下面的操作
			qci.pQueuePriorities = &queuePriority;//设置队列的优先级,只有一个还是要设置为1.0f
			queueCreateInfos.push_back(qci);
		}

	}
	void LveDevice::createLogicalDevice() {
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;//结构体属性
		createInfo.pQueueCreateInfos = queueCreateInfos.data();//添加队列的属性指针
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());//队列创建信息结构体的数量
		deviceFeatures.fillModeNonSolid = VK_TRUE;
		createInfo.pEnabledFeatures = &deviceFeatures;//添加指定的设备功能属性
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());//启用的拓展数量
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {//正式开始创建
			throw std::runtime_error("failed to create logical device!");
		}

	}

	void LveDevice::createArrHandle() {
		vkGetDeviceQueue(device, queueFamliesIndices.graphicsFamily, 0, &graphicsQueue);
		vkGetDeviceQueue(device, queueFamliesIndices.presentFamily, 0, &presentQueue);
	}
	void LveDevice::createCommandPool(uint32_t queueFamilyIndex) {
		//创建命令池
	//定义在最上面
		VkCommandPoolCreateInfo poolInfo{};//命令池信息
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;//命令池信息
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//允许单独重置命令缓冲区中某个不影响其他的
		poolInfo.queueFamilyIndex = queueFamilyIndex;//指定命令池的列队簇索引,命令缓冲区将从这个列队簇中分配
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {//创建命令池
			throw std::runtime_error("failed to create command pool!");
		}

	}

	void LveDevice::clean(VkDevice device,VkInstance instance) {
		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);//清除命令缓冲区
		vkDestroyCommandPool(device, commandPool, nullptr);//清除命令池
		vkDestroyDevice(device, nullptr);//清除逻辑设备)

		vkDestroyInstance(instance, nullptr);		//销毁实例

	}

	void LveDevice::copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size) {
		VkCommandBufferAllocateInfo allocInfo{};//命令缓冲区分配信息
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;//指定结构体类型
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//指定命令缓冲区的级别,主命令缓冲区
		allocInfo.commandPool = commandPool;//指定命令池,命令缓冲区将从这个命令池中分配
		allocInfo.commandBufferCount = 1;//指定要分配的命令缓冲区数量

	
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);//分配命令缓冲区

		VkCommandBufferBeginInfo beginInfo{};//命令缓冲区开始信息
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;//指定结构体类型
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;//指定命令缓冲区的使用方式,一次性提交

		vkBeginCommandBuffer(commandBuffer, &beginInfo);//记录命令缓冲区信息

		VkBufferCopy copyRegion{};//缓冲区拷贝信息
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);//执行缓冲区拷贝命令

		vkEndCommandBuffer(commandBuffer);//结束命令缓冲区记录

		VkSubmitInfo submitInfo{};//提交信息
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);//提交命令缓冲区到图形队列
		vkQueueWaitIdle(graphicsQueue);//等待图形队列执行完毕


	};

	void LveDevice::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
		VkDeviceMemory& bufferMemory) {//创建缓冲区
		VkBufferCreateInfo bufferInfo{};//创建缓冲区的结构体
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;//结构体类型
		bufferInfo.size = size;//缓冲区大小
		bufferInfo.usage = usage;//缓冲区用途
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;//缓冲区共享模式:独占模式 从图形队列中使用

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {//创建缓冲区
			throw std::runtime_error("failed to create buffer!");
		}

		VkMemoryRequirements memRequirements;//内存需求结构体
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);//获取缓冲区内存需求

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	VkImageView LveDevice::createImageView(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags) {

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;

		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView = VK_NULL_HANDLE;

		if (vkCreateImageView(
			device,
			&viewInfo,
			nullptr,
			&imageView) != VK_SUCCESS) {

			throw std::runtime_error("failed to create image view!");
		}

		return imageView;
	}

	void LveDevice::createImage(
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory) {

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;

		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;

		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;

		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;

		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateImage(
			device,
			&imageInfo,
			nullptr,
			&image) != VK_SUCCESS) {

			throw std::runtime_error("failed to create image!");
		}

		VkMemoryRequirements memoryRequirements{};
		vkGetImageMemoryRequirements(
			device,
			image,
			&memoryRequirements
		);

		VkMemoryAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memoryRequirements.size;

		allocateInfo.memoryTypeIndex = findMemoryType(
			memoryRequirements.memoryTypeBits,
			properties
		);

		if (vkAllocateMemory(
			device,
			&allocateInfo,
			nullptr,
			&imageMemory) != VK_SUCCESS) {

			vkDestroyImage(device, image, nullptr);
			image = VK_NULL_HANDLE;

			throw std::runtime_error("failed to allocate image memory!");
		}

		if (vkBindImageMemory(
			device,
			image,
			imageMemory,
			0) != VK_SUCCESS) {

			vkFreeMemory(device, imageMemory, nullptr);
			vkDestroyImage(device, image, nullptr);

			imageMemory = VK_NULL_HANDLE;
			image = VK_NULL_HANDLE;

			throw std::runtime_error("failed to bind image memory!");
		}
	}
	void LveDevice::endSingleTimeCommands(
		VkCommandBuffer commandBuffer
	) {
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to end single-time command buffer"
			);
		}

		VkSubmitInfo submitInfo{};
		submitInfo.sType =
			VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		if (vkQueueSubmit(
			getGraphicsQueue(),
			1,
			&submitInfo,
			VK_NULL_HANDLE
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to submit single-time command buffer"
			);
		}

		vkQueueWaitIdle(getGraphicsQueue());

		vkFreeCommandBuffers(
			getDevice(),
			getCommandPool(),
			1,
			&commandBuffer
		);
	}
	VkCommandBuffer LveDevice::beginSingleTimeCommands() {
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType =
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;

		allocInfo.level =
			VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		allocInfo.commandPool = getCommandPool();
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;

		if (vkAllocateCommandBuffers(
			getDevice(),
			&allocInfo,
			&commandBuffer
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to allocate single-time command buffer"
			);
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType =
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		beginInfo.flags =
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (vkBeginCommandBuffer(
			commandBuffer,
			&beginInfo
		) != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to begin single-time command buffer"
			);
		}

		return commandBuffer;
	}
	void LveDevice::transitionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout
	) {
		VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barrier.image = image;

		barrier.subresourceRange.aspectMask =
			VK_IMAGE_ASPECT_COLOR_BIT;

		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
			newLayout == VK_IMAGE_LAYOUT_GENERAL) {

			barrier.srcAccessMask = 0;

			barrier.dstAccessMask =
				VK_ACCESS_SHADER_READ_BIT |
				VK_ACCESS_SHADER_WRITE_BIT;

			sourceStage =
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			destinationStage =
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		else {
			throw std::runtime_error(
				"unsupported image layout transition"
			);
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage,
			destinationStage,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier
		);

		endSingleTimeCommands(commandBuffer);
	}
	uint32_t LveDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;//物理设备内存属性结构体
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);//获取物理设备内存属性
		//寻找buffer允许使用的内存类型
		//寻找CPU可写,自动保持一致的特殊内存属性
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {//检查是否将相应的位设置为1来确定适合的内存类型
				return i;
			}
		}
		throw std::runtime_error("failed to find suitable memory type!");
	}

	VkFormat LveDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
		for (VkFormat format : candidates) {
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
				return format;
			}
		}
		throw std::runtime_error("failed to find supported format!");
	}

	VkFormat  LveDevice::findDepthFormat() {
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
}