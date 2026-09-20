//后面着色器换成slang



#include<SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>
#include<vulkan/vulkan.h>
#include<cstdlib>
#include<iostream>
#include<stdexcept>
#include"lve_windows.h"
#include"lve_pipeline.h"
#include"lve_device.h"
#include"lve_swapChain.h"
#include"lve_renderPass.h"
#include"lve_renderer.h"
#include"lve_model.h"
#include"lve_uniform.h"
#include"lve_camera.h"
#include"lve_compute.h"
#include"lve_terrain.h"
#include "slope.h"
#include "shadow.h"
#include <set>
#include<vector>

#include"imgui/imgui.h"
#include"imgui/imgui_impl_vulkan.h"
#include"imgui/imgui_impl_sdl3.h"
//----------------------------------------------------------------------------------------
// 参数控制
int  g_seed = 110514;//地图种子
bool g_regenTerrain = false;//是否重新生成种子地形
const float HEIGHT_FIXED_SCALE = 10000.0f;//这个用于int->float还原,不用改
const float cameraMaxSeeDistance = 450.0f;
glm::vec3 lightDir = glm::normalize(glm::vec3(0.137f, -0.33f, 0.0066f));//光源方向
const int   g_terrainScale = 2;//地形缩放大小
//----------------------------------------------------------------------------------------
//本地无限地形生成开关
bool unlimitedArea = false;
//----------------------------------------------------------------------------------------
lve::LveWindows win(1366,768,"从零开始的vulkan生活");//窗口
lve::Lvepipeline pipeLine("shader/simple_shader.vert.spv", "shader/simple_shader.frag.spv");
lve::LveDevice device;
lve::LveSwapChain swapChain;
lve::LveRenderPass renderPass;
lve::LveRenderer renderer;
lve::LveModel model;
lve::LveUniform uniform;
lve::LveCamera camera;
lve::LveCompute compute(device, "shader/compute.comp.spv");
lve::LveTerrain terrain;
shadow::Shadow shadowObj(device, "shader/shadow.vert.spv");
slope::Slope slopeCompute(device, "shader/slope.comp.spv");

uint32_t currentFrame = 0;//当前帧


#pragma region imgui相关
void initImGui() {
	// 检查 imgui.h 和 imgui.cpp 的版本是否一致
	IMGUI_CHECKVERSION();

	// 创建 ImGui 上下文
	ImGui::CreateContext();

	// 设置默认深色主题
	ImGui::StyleColorsDark();

	// 初始化 SDL3 输入后端
	if (!ImGui_ImplSDL3_InitForVulkan(win.win)) {
		throw std::runtime_error(
			"failed to initialize ImGui SDL3 backend");
	}

	ImGui_ImplVulkan_InitInfo initInfo{};

	initInfo.ApiVersion = VK_API_VERSION_1_0;

	initInfo.Instance =
		device.getInstance();

	initInfo.PhysicalDevice =
		device.getPhysicalDevice();

	initInfo.Device =
		device.getDevice();

	initInfo.QueueFamily =
		swapChain.getQueueFamilyIndices_what(0);

	initInfo.Queue =
		device.getGraphicsQueue();

	// 让 ImGui 后端自动创建描述符池。
	// 不会使用 LveUniform 中的描述符池。
	initInfo.DescriptorPoolSize =
		IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE;

	// Vulkan 交换链至少拥有的图片数
	initInfo.MinImageCount = 2;

	// 你的实际交换链图片数量
	initInfo.ImageCount =
		swapChain.getSwapChainImageCount();

	// 你的 ImGui 版本是 1.92.9，
	// RenderPass 需要放在 PipelineInfoMain 中
	initInfo.PipelineInfoMain.RenderPass =
		renderPass.getRenderPass();

	initInfo.PipelineInfoMain.Subpass = 0;

	initInfo.PipelineInfoMain.MSAASamples =
		VK_SAMPLE_COUNT_1_BIT;

	if (!ImGui_ImplVulkan_Init(&initInfo)) {
		throw std::runtime_error(
			"failed to initialize ImGui Vulkan backend");
	}
}
float bias = 0.0015;

#pragma endregion

#pragma region 消息回调
VkDebugUtilsMessengerEXT callback;
//回调函数
static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "error: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

#pragma endregion

//计算光线的矩阵[正交矩阵]a
glm::mat4 computeLightViewProj(glm::vec3 lightDir, glm::vec3 sceneCenter, float extent) {
	glm::vec3 dir = glm::normalize(lightDir);
	float lightDist = extent * 2.0f;
	glm::vec3 lightPos = sceneCenter + dir * lightDist;   // 虚拟 eye，只定方向
	glm::vec3 up = (std::abs(dir.z) > 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);

	glm::mat4 view = glm::lookAt(lightPos, sceneCenter, up);

	float near = lightDist - extent * 1.5f;
	float far = lightDist + extent * 1.5f;
	glm::mat4 proj = glm::ortho(-extent, extent, -extent, extent, near, far);
	return proj * view;
}

std::vector<float> initialHeightData;//单纯备份地形的,用于imgui测试侵蚀次数
int g_erosionExtent = 2000000;   // 可调侵蚀次数
bool g_rerunErosion = false;     // 是否重新跑
//单纯用在imgui里测试不同侵蚀次数的效果
void runErosionOnce(int extent) {
	terrain.EROSON_EXTENT = extent;
	compute.EROSON_EXTENT = extent;

	std::vector<int32_t> heightUint(terrain.getHeightData().size());
	std::vector<uint32_t> flowUint(terrain.getHeightData().size(), 0);
	std::vector<uint32_t> ersionUint(terrain.getHeightData().size(), 0);
	lve::LveCompute::ResearchStats researchStats{};

	for (size_t i = 0; i < initialHeightData.size(); i++) {
		heightUint[i] = static_cast<int32_t>(initialHeightData[i] * HEIGHT_FIXED_SCALE + 0.5f);
	}

	compute.runErosionSync(device, 0, terrain.getMapVertexNum(),
		heightUint, flowUint, ersionUint, researchStats,
		sizeof(int32_t) * terrain.getHeightData().size(), slopeCompute.getStep(), slopeCompute.getSlopeResolution());

	// ★ 更新顶点时，把地形缩放乘回去
	for (size_t i = 0; i < heightUint.size(); i++) {
		terrain.getVertices()[i].pos.z =
			(heightUint[i] / HEIGHT_FIXED_SCALE) * g_terrainScale;
		terrain.getHeightData()[i] =
			heightUint[i] / HEIGHT_FIXED_SCALE;
	}

	// 流量也更新一下（不涉及缩放）
	const uint32_t maxFlow = *std::max_element(flowUint.begin(), flowUint.end());
	const float maxValue = std::log1p(static_cast<float>(maxFlow));
	for (size_t i = 0; i < flowUint.size(); i++) {
		float flow = std::log1p(static_cast<float>(flowUint[i]));
		terrain.getVertices()[i].flow = maxValue > 0.0f ? flow / maxValue : 0.0f;
	}

	terrain.updateChunkDate(heightUint, HEIGHT_FIXED_SCALE);
	terrain.calculateNormal();

	vkDeviceWaitIdle(device.getDevice());
	model.clean(device.getDevice());
	model.createVertexBufferWithStaging(device, terrain.getVertices());
	model.createIndexBufferWithStaging(device, terrain.getIndices());
}

//单纯用于imgui调试的重新生成地形
void regenerateTerrain(int newSeed) {
	// 1. 重新生成
	terrain.processArea(newSeed);

	// 2. 备份初始高度（用于后续侵蚀对比）
	initialHeightData = terrain.getHeightData();

	// 3. 重新侵蚀
	std::vector<int32_t>  heightUint(terrain.getHeightData().size());
	std::vector<uint32_t> flowUint(terrain.getHeightData().size(), 0);
	std::vector<uint32_t> ersionUint(terrain.getHeightData().size(), 0);
	lve::LveCompute::ResearchStats stats{};

	for (size_t i = 0; i < initialHeightData.size(); i++) {
		heightUint[i] = static_cast<int32_t>(initialHeightData[i] * HEIGHT_FIXED_SCALE + 0.5f);
	}

	compute.runErosionSync(device, 0, terrain.getMapVertexNum(),
		heightUint, flowUint, ersionUint, stats,
		sizeof(int32_t) * terrain.getHeightData().size(), slopeCompute.getStep(), slopeCompute.getSlopeResolution());

	terrain.updateHeightFlow(heightUint, flowUint, HEIGHT_FIXED_SCALE);
	terrain.updateChunkDate(heightUint, HEIGHT_FIXED_SCALE);
	terrain.calculateNormal();

	// 4. 生成海洋 + 缩放
	terrain.processOcean();
	terrain.SetModelSize(2);

	// 5. 重新上传顶点缓冲
	vkDeviceWaitIdle(device.getDevice());
	model.clean(device.getDevice());
	model.createVertexBufferWithStaging(device, terrain.getVertices());
	model.createIndexBufferWithStaging(device, terrain.getIndices());

	std::cout << "regenerated with seed " << newSeed << "\n";
}


void clean() {


	vkDeviceWaitIdle(device.getDevice());
	pipeLine.clean(device.getDevice());
	swapChain.cleanupSwapChain(device.getDevice());
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
	shadowObj.clean();
	renderPass.clean(device.getDevice());
	renderer.clean(device.getDevice());
	win.cleanSurface(device.getInstance());
	model.clean(device.getDevice());
	compute.clean();
	slopeCompute.clean();
	uniform.clean(device.getDevice(),renderer.getMaxFramesInFlight());
	auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(device.getInstance(), "vkDestroyDebugUtilsMessengerEXT");
	if (vkDestroyDebugUtilsMessengerEXT) {
		vkDestroyDebugUtilsMessengerEXT(device.getInstance(), callback, nullptr);
	}
	device.clean(device.getDevice(), device.getInstance());
}

void writeBMP(const char* filename, const std::vector<uint8_t>& pixels, int w, int h) {
	// pixels 是 RGB 数据，每像素 3 字节
	int rowSize = (w * 3 + 3) & ~3;   // BMP 每行对齐到 4 字节
	int dataSize = rowSize * h;
	int fileSize = 54 + dataSize;

	std::ofstream f(filename, std::ios::binary);

	// BMP 文件头 (14 字节)
	uint8_t fileHeader[14] = {};
	fileHeader[0] = 'B';
	fileHeader[1] = 'M';
	*(uint32_t*)&fileHeader[2] = fileSize;
	*(uint32_t*)&fileHeader[10] = 54;
	f.write((char*)fileHeader, 14);

	// BMP 信息头 (40 字节)
	uint8_t infoHeader[40] = {};
	*(uint32_t*)&infoHeader[0] = 40;
	*(int32_t*)&infoHeader[4] = w;
	*(int32_t*)&infoHeader[8] = h;
	*(uint16_t*)&infoHeader[12] = 1;      // planes
	*(uint16_t*)&infoHeader[14] = 24;     // 24 位
	*(uint32_t*)&infoHeader[20] = dataSize;
	f.write((char*)infoHeader, 40);

	// 像素数据（BMP 是从下到上存）
	std::vector<uint8_t> row(rowSize, 0);
	for (int y = h - 1; y >= 0; y--) {
		for (int x = 0; x < w; x++) {
			int i = (y * w + x) * 3;
			row[x * 3 + 0] = pixels[i + 2]; // B
			row[x * 3 + 1] = pixels[i + 1]; // G
			row[x * 3 + 2] = pixels[i + 0]; // R
		}
		f.write((char*)row.data(), rowSize);
	}
	f.close();
}


int main() {
	//初始化
	
	
	//3.创建实例


	device.createInstance();
	//创建回调
	// 创建回调
	if (enableValidationLayers) {
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugUtilsCallback;  // 你的回调函数
		debugCreateInfo.pUserData = nullptr;  // 可传递自定义数据

		// 加载创建函数
		auto vkCreateDebugUtilsMessengerEXT =
			(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(device.getInstance(), "vkCreateDebugUtilsMessengerEXT");

		if (vkCreateDebugUtilsMessengerEXT) {
			VkResult debugResult = vkCreateDebugUtilsMessengerEXT(device.getInstance(), &debugCreateInfo, nullptr, &callback);
			if (debugResult != VK_SUCCESS) {
				std::cout << "create callback error!! error code: " << debugResult << std::endl;
			}
		}
		else {
			std::cout << "unable load vkCreateDebugUtilsMessengerEXT" << std::endl;
		}
	}

	//创建表面
	win.createWindowSurface(device.getInstance(), win.win);
	device.pickPhysicalDevice();//选择物理设备
	//创建逻辑设备
	//创建队列
	device.createQueueFamiliesIndices(win);
	//创建逻辑设备
	device.createLogicalDevice();
	//检索列队句柄
	device.createArrHandle();
	
	//创建交换链
	swapChain.createSwapChain( device,win);
	
	

	//前提都设置好了---------------------------------------------------------------------------开始整画面

	pipeLine.initPipeline();//初始化管线,设置视口等信息
	device.createCommandPool(swapChain.getQueueFamilyIndices_what(0));//创建命令池
	//深度缓冲
	swapChain.createDepthResources(device);
	
	//创建阴影的管线啥的
	shadowObj.init(2048);
	glm::mat4 lightViewProj = computeLightViewProj(//灯光矩阵
		lightDir,
		glm::vec3(0.0f, 0.0f, 0.0f),
		800.0f);


	//渲染过程----------------------------------
	pipeLine.createShader(device.getDevice());//创建着色器模块
	

	using Clock = std::chrono::steady_clock;

	auto start = Clock::now();
	//初始化地形
	terrain.processArea(g_seed);
	//生成坡度图像
	slopeCompute.init(
		512,                                    // 坡度图分辨率
		terrain.getMapVertexNum(),              // 高度图边长（顶点数）
		terrain.getBlockDist() / (20 - 1)       // step（顶点间距）
	);
	compute.setSlopeBuffers(slopeCompute.getSlopeBuffer());//把坡度图像内存拷到compute类里
	//备份一下地形,方便后面调整侵蚀恢复原样
	initialHeightData = terrain.getHeightData();



	auto afterTerrain = Clock::now();
	std::cout << "terrain generation: "<< std::chrono::duration<float>(afterTerrain - start).count() << " seconds\n";

	//创建计算着色器
	VkDeviceSize computeBufferSize = sizeof(int32_t) * terrain.getHeightData().size();
	compute.init(renderer.getMaxFramesInFlight(), computeBufferSize);

	
	
	//侵蚀模拟计算
	std::vector<int32_t> heightUint(terrain.getHeightData().size());//高度数据
	std::vector<uint32_t> flowUint(	terrain.getHeightData().size(), 0);//流量数据
	std::vector<uint32_t> ersionUint(terrain.getHeightData().size(), 0);//侵蚀数据
	lve::LveCompute::ResearchStats researchStats{};//实验数据
	auto erosionStart = Clock::now();
	for (size_t i = 0; i < 	terrain.getHeightData().size(); i++) {
		heightUint[i] = static_cast<int32_t>(	terrain.getHeightData()[i] * HEIGHT_FIXED_SCALE + 0.5f);
	}

	//把坡度图像扔进计算着色器里
	compute.updateStorageBuffer(//更新高度图
		0,
		heightUint.data(),
		computeBufferSize
	);
	slopeCompute.runSlopeSync(device, 0,//存储坡度图
		compute.getStorageBuffer(0),
		terrain.getMapVertexNum());

	//计算侵蚀
	compute.runErosionSync(device, 0, terrain.getMapVertexNum(), heightUint, flowUint, ersionUint,researchStats,
		computeBufferSize, slopeCompute.getStep(), slopeCompute.getSlopeResolution());
	terrain.updateHeightFlow(heightUint, flowUint, HEIGHT_FIXED_SCALE);
	terrain.updateChunkDate(heightUint, HEIGHT_FIXED_SCALE);

	
	auto erosionEnd = Clock::now();

	std::cout << "GPU erosion: "
		<< std::chrono::duration<float>(
			erosionEnd - erosionStart).count()
		<< " seconds\n";
	// 重新计算法线
	auto normalStart = Clock::now();
	terrain.calculateNormal(); //
	auto normalEnd = Clock::now();
	std::cout << "normal calculation: "
		<< std::chrono::duration<float>(
			normalEnd - normalStart).count()
		<< " seconds\n";

	//生成海洋
	terrain.processOcean();

	//放大地形
	terrain.SetModelSize(g_terrainScale);

	//[2选1]具体区别看model.h
	//model.createVertexBuffer(device);//创建顶点缓冲区
	model.createVertexBufferWithStaging(device, terrain.getVertices());//创建顶点缓冲区,使用staging buffer
	model.createIndexBufferWithStaging(device, terrain.getIndices());//创建索引缓冲区


	pipeLine.distritbutePipeline();//分配管线

	//创建布局
	uniform.init(device, renderer.getMaxFramesInFlight(), shadowObj.getImageView(), shadowObj.getSampler());

	//管线布局
	pipeLine.createPipelineLayout(device.getDevice(),uniform.getDescriptorSetLayout());//创建管线布局



	renderPass.createRenderPass(swapChain.getSwapChainSurfaceFormat(), device.findDepthFormat(),device.getDevice());//创建渲染通道
	//初始化imgui
	initImGui();


	//创建图像管线--------------------------------------------------


	pipeLine.createOthers();//创建其他管线相关信息,如视口,裁切矩形等

	pipeLine.createpipeline(renderPass.getRenderPass(), device.getDevice(),renderer.getGraphicsPipeline());//创建图像管线

	//绘制部分+--------------------------------------------------------------------
	//创建帧缓冲对象

	swapChain.createFrameBuffer(device, renderPass.getRenderPass());



	renderer.createCommandBuffers(device.getDevice(), device.getCommandPool(), swapChain.getSwapChainImageCount());//创建命令缓冲区)


	renderer.createSignalSemaphore(device.getDevice(), swapChain.getSwapChainImageCount());//创建信号量

	//初始化摄像机
	camera.setViewDirection(
		glm::vec3{ 2.0f, 2.0f, 80.0f },   // 摄像机位置
		glm::vec3{ -1.0f, -1.0f, -1.0f }, // 摄像机方向
		glm::vec3{ 0.0f, 0.0f, 1.0f });   // Z 轴向上

	camera.setPerspectiveProjection(
		glm::radians(45.0f),
		swapChain.getSwapChainExtent().width / static_cast<float>(swapChain.getSwapChainExtent().height),
		0.1f,//近裁截面
		cameraMaxSeeDistance);//远裁截面


	SDL_Event event;
	SDL_SetWindowRelativeMouseMode(win.win, true);//启用相对鼠标模式
	Uint64 lastTime = SDL_GetTicks();
	glm::mat4 modelMatrix{ 1.0f };

	modelMatrix = glm::translate(modelMatrix,glm::vec3{ 0.0f, 0.0f, 0.0f });

	modelMatrix = glm::rotate(modelMatrix,glm::radians(45.0f),glm::vec3{ 0.0f, 0.0f, 1.0f });

	modelMatrix = glm::scale(modelMatrix,glm::vec3{ 1.0f, 1.0f, 1.0f });
	 modelMatrix = { 1.0f };
	uint32_t maxErosion = *std::max_element(ersionUint.begin(), ersionUint.end());//debug测试
	std::cout << "max erosion: " << maxErosion << std::endl;
	std::cout << "vertexNUm:" << terrain.getMapVertexNum() << std::endl;

	bool showCurios = false;//是否显示鼠标


	//保存坡度图,自己打开图片看看对不对
	/*{
		float* data = (float*)slopeCompute.getSlopeBufferMapped(0);
		uint32_t res = slopeCompute.getSlopeResolution();

		float maxS = 0.0f;
		for (uint32_t i = 0; i < res * res; i++) {
			maxS = std::max(maxS, data[i]);
		}
		std::cout << "slope max = " << maxS << std::endl;

		float invMax = (maxS > 0.0001f) ? (255.0f / maxS) : 0.0f;

		std::vector<uint8_t> pixels(res * res * 3);
		for (uint32_t i = 0; i < res * res; i++) {
			uint8_t v = (uint8_t)glm::clamp(data[i] * invMax, 0.0f, 255.0f);
			pixels[i * 3 + 0] = v;
			pixels[i * 3 + 1] = v;
			pixels[i * 3 + 2] = v;
		}

		writeBMP("slope_debug.bmp", pixels, res, res);
		std::cout << "slope_debug.bmp saved\n";

	}*/

	while (1) {
		const Uint64 currentTime = SDL_GetTicks();

		const float dealtTime =
			static_cast<float>(currentTime - lastTime) / 1000.0f;
		lastTime = SDL_GetTicks();
		while (SDL_PollEvent(&event)) {//处理事件
			ImGui_ImplSDL3_ProcessEvent(&event);//将事件传递给ImGui
			if (event.type == SDL_EVENT_QUIT) {
				clean();
				return EXIT_SUCCESS;
			}
			if (event.type == SDL_EVENT_MOUSE_MOTION && !showCurios) {
				camera.rotate(
					event.motion.xrel,
					event.motion.yrel);
			}
			if (event.type == SDL_EVENT_KEY_DOWN &&event.key.scancode == SDL_SCANCODE_TAB &&!event.key.repeat) {
				showCurios = !showCurios;
				SDL_SetWindowRelativeMouseMode(win.win,!showCurios);
			}
		}
		const bool* keyboardState = SDL_GetKeyboardState(nullptr);

		if (keyboardState[SDL_SCANCODE_W]) {
			camera.forward_and_behind(true, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_S]) {
			camera.forward_and_behind(false, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_A]) {
			camera.right_and_left(false, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_D]) {
			camera.right_and_left(true, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_SPACE]) {
			camera.up_and_down(true, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_LSHIFT]) {
			camera.up_and_down(false, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_ESCAPE]) {
			clean();
			return EXIT_SUCCESS;
		}
		

		//imgui相关
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();
		if (showCurios) {
			ImGui::Begin("Water Drop Research");
			ImGui::SetWindowFontScale(1.5f);

			ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
			ImGui::Text("totalCandidates: %u", researchStats.totalCandidates);
			ImGui::Text("erosionSkipped: %u", researchStats.erosionSkipped);
			ImGui::Text("flatSlopeStopped: %u", researchStats.flatSlopeStopped);

			ImGui::Separator();

			// ★ 侵蚀次数滑块
			ImGui::SliderInt("Erosion Extent", &g_erosionExtent, 100000, 5000000, "%d");

			// ★ 重跑按钮
			if (ImGui::Button("Rerun Erosion")) {
				g_rerunErosion = true;
			}
			ImGui::Separator();

			// ★ 种子
			ImGui::InputInt("Seed", &g_seed);
			ImGui::SameLine();
			if (ImGui::Button("Regenerate")) {
				g_regenTerrain = true;
			}

			ImGui::Separator();
			// ★ 显示当前状态
			ImGui::SameLine();
			ImGui::Text("(current: %d)", g_erosionExtent);

			ImGui::Separator();

			ImGui::SliderFloat("bias", &bias, 0.0f, 0.02f, "%.5f");
			ImGui::SliderFloat("lightDirX", &lightDir.x, -1.0f, 1.0f, "%.5f");
			ImGui::SliderFloat("lightDirY", &lightDir.y, -1.0f, 1.0f, "%.5f");
			ImGui::SliderFloat("lightDirZ", &lightDir.z, -1.0f, 1.0f, "%.5f");


			ImGui::TextUnformatted("Press TAB to close UI");
			ImGui::End();
		}
		ImGui::Render();

		//更新光源矩阵[这个release记得删,单纯debug的时候用](但是如果要做光源一直动就不用删了)
		lightViewProj = computeLightViewProj(//灯光矩阵
			lightDir,
			glm::vec3(0.0f, 0.0f, 0.0f),
			800.0f);

		renderer.run(device.getDevice(), swapChain, device.getGraphicsQueue(), device.getPresentQueue(),
			currentFrame, renderPass.getRenderPass(),model,uniform.getDescriptorSets(),pipeLine.getPipelineLayout(),
			uniform, modelMatrix,camera.getView(),camera.getProjection(),compute,camera.getPos(),terrain.getIndices(),terrain, cameraMaxSeeDistance, shadowObj,
			lightViewProj, glm::vec4(bias, lightDir));
		
		if (g_rerunErosion) {
			g_rerunErosion = false;
			auto t0 = Clock::now();
			runErosionOnce(g_erosionExtent);
			auto t1 = Clock::now();
			std::cout << "rerun erosion: "
				<< std::chrono::duration<float>(t1 - t0).count()
				<< " seconds\n";
		}
		if (g_regenTerrain) {
			g_regenTerrain = false;
			auto t0 = Clock::now();
			regenerateTerrain(g_seed);
			auto t1 = Clock::now();
			std::cout << "regenerate: "
				<< std::chrono::duration<float>(t1 - t0).count()
				<< " seconds\n";
		}
	}

	system("pause");
	clean();


	return EXIT_SUCCESS;

}
