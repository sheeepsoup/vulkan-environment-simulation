#include"lve_renderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"


namespace lve {
	void LveRenderer::createCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount) {

		//创建多个命令缓冲区
		commandBuffers.resize(commandBufferCount);
		VkCommandBufferAllocateInfo allocInfo{};//命令缓冲区分配信息
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;//命令缓冲区分配信息
		allocInfo.commandPool = commandPool;//命令池
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;//主命令缓冲区[指定分配主的还是辅助的]
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();//分配size个命令缓冲区
		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {//分配命令缓冲区
			throw std::runtime_error("failed to allocate command buffers!");
		}

	}
	void LveRenderer::createSignalSemaphore(VkDevice device,uint32_t swapChainImageCount) {
		//创建多个信号量和栅栏
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(swapChainImageCount);//根据
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		VkSemaphoreCreateInfo semaphoreInfo{};//信号量信息
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;//信号量信息

		VkFenceCreateInfo fenceInfo{};//栅栏信息
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;//栅栏创建时就处于信号状态,这样第一次渲染的时候就不会阻塞

		// 每个“飞行帧”一个：图片可用信号量和 Fence
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(
				device,
				&semaphoreInfo,
				nullptr,
				&imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(
					device,
					&fenceInfo,
					nullptr,
					&inFlightFences[i]) != VK_SUCCESS) {

				throw std::runtime_error("failed to create frame sync objects!");
			}
		}

		// 每张交换链图片一个：渲染完成信号量
		for (size_t i = 0; i < swapChainImageCount; i++) {
			if (vkCreateSemaphore(
				device,
				&semaphoreInfo,
				nullptr,
				&renderFinishedSemaphores[i]) != VK_SUCCESS) {

				throw std::runtime_error("failed to create render finished semaphore!");
			}
		}
	}
	void LveRenderer::run(VkDevice device, LveSwapChain& swapChain, VkQueue graphicsQueue, VkQueue presentQueue, uint32_t& currentFrame,
		VkRenderPass &renderPass, LveModel& model,const std::vector<VkDescriptorSet> descriptorSets, VkPipelineLayout pipelineLayout,LveUniform &uniform,
		const glm::mat4 modelMatirx,const glm::mat4 view,const glm::mat4 proj,LveCompute& compute,glm::vec3 cameraPos, std::vector<uint32_t>& indices,
		LveTerrain& terrain, float renderDistance, shadow::Shadow& shadow, const glm::mat4& lightViewProj,glm::vec4 renderParams, 
		float time, ocean::Ocean& oceanObj, float oceanHeight, float horizontal_displacement_intensity, ocean_cascade::OceanCascade& bigOceanCascade,
		ocean_cascade::OceanCascade& middleOceanCascade, ocean_cascade::OceanCascade& smallOceanCascade, ocean_cascade::OceanCascade& tinyOceanCascade) {
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);//等待栅栏,直到渲染完成
		vkAcquireNextImageKHR(device, swapChain.getSwapChain(), UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);//获取下一张交换链图片的索引,并将imageAvailableSemaphore信号量设置为在图像可用时发出信号
		vkResetFences(device, 1, &inFlightFences[currentFrame]);//重置栅栏,以便下一次使用
		//获取交换链图像
		uniform.updateUniformBuffer(currentFrame, swapChain.getSwapChainExtent(), modelMatirx, view,proj,lightViewProj, cameraPos, renderParams);//更新上传ubo
	
		vkResetCommandBuffer(commandBuffers[currentFrame], 0);//重置命令缓冲区,以便重新记录命令缓冲区
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex,
			renderPass,swapChain.getSwapChainFrameBuffer(imageIndex),swapChain.getSwapChainExtent(),model,pipelineLayout,currentFrame,descriptorSets,compute,
			indices,terrain,renderDistance,cameraPos, shadow, lightViewProj,time,oceanObj, oceanHeight,horizontal_displacement_intensity, bigOceanCascade,
			middleOceanCascade, smallOceanCascade, tinyOceanCascade);//记录命令缓冲区

		//提交命令缓冲区
		VkSubmitInfo submitInfo{};//提交信息
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };//等待的信号量
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };//等待的阶段
		submitInfo.waitSemaphoreCount = 1;//等待的信号量数量
		submitInfo.pWaitSemaphores = waitSemaphores;//等待的信号量
		submitInfo.pWaitDstStageMask = waitStages;//等待的阶段
		submitInfo.commandBufferCount = 1;//提交的命令缓冲区数量
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];//提交的命令缓冲区

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex] };//执行完毕后触发的信号量
		submitInfo.signalSemaphoreCount = 1;//信号量数量
		submitInfo.pSignalSemaphores = signalSemaphores;//信号量
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {//提交命令缓冲区
			throw std::runtime_error("failed to submit draw command buffer!");
		}


		//提交渲染结果
		//提交回交换链
		VkPresentInfoKHR presentInfo{};//提交信息
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;//提交信息

		presentInfo.waitSemaphoreCount = 1;//等待的信号量数量
		presentInfo.pWaitSemaphores = signalSemaphores;//等待的信号量
		VkSwapchainKHR swapChains[] = { swapChain.getSwapChain()};//交换链
		presentInfo.swapchainCount = 1;//交换链数量
		presentInfo.pSwapchains = swapChains;//交换链
		presentInfo.pImageIndices = &imageIndex;//交换链图像索引
		presentInfo.pResults = nullptr;//结果
		vkQueuePresentKHR(presentQueue, &presentInfo);//提交回交换链

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;//前进刀下一帧
	};

	void LveRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkRenderPass renderPass,
		VkFramebuffer framebuffer, VkExtent2D extent, LveModel& model, VkPipelineLayout pipelineLayout, uint32_t currentFrame,
		const std::vector<VkDescriptorSet> descriptorSets, LveCompute& compute, std::vector<uint32_t>& indices, LveTerrain& terrain,
		float renderDistance, glm::vec3 cameraPos, shadow::Shadow& shadow, const glm::mat4& lightViewProj,
		float time,ocean::Ocean &oceanObj,float oceanHeight,float horizontal_displacement_intensity, ocean_cascade::OceanCascade& bigOceanCascade, 
		ocean_cascade::OceanCascade& middleOceanCascade, ocean_cascade::OceanCascade& smallOceanCascade, ocean_cascade::OceanCascade& tinyOceanCascade) {
		{
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;//~
			beginInfo.flags = 0;//使用的行为默认
			beginInfo.pInheritanceInfo = nullptr;//用于辅助缓冲区,继承主缓冲区时用的

			if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {//开始记录命令缓冲区
				throw std::runtime_error("failed to begin recording command buffer!");
			}
		
			bigOceanCascade.update(commandBuffer, time);
			middleOceanCascade.update(commandBuffer, time);
			smallOceanCascade.update(commandBuffer, time);
			tinyOceanCascade.update(commandBuffer, time);

			oceanObj.recordHeightMapReadyForGraphics(commandBuffer);

			oceanObj.recordHeightMapReadyForGraphics(commandBuffer);//将ifft计算结果传给graphics绘画海洋
			//记录阴影的渲染通道
			shadow.recordShadowPass(commandBuffer, lightViewProj, model, terrain);

			//启动渲染通道
			VkRenderPassBeginInfo renderPassBeginInfo{};//渲染通道开始信息
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;//渲染通道开始信息
			renderPassBeginInfo.renderPass = renderPass;//渲染通道
			renderPassBeginInfo.framebuffer = framebuffer;//帧缓冲区
			renderPassBeginInfo.renderArea.offset = { 0, 0 };//渲染区域偏移量
			renderPassBeginInfo.renderArea.extent = extent;//渲染区域大小

			VkClearValue clearValues[2]{};

			clearValues[0].color = { {0.38f, 0.62f, 0.82f, 1.0f} };

			clearValues[1].depthStencil = {
				1.0f,
				0
			};

			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;


			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);//开始渲染通道

			//视图暂时用静态的,后面while再加动态
			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(extent.width);
			viewport.height = static_cast<float>(extent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = extent;
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			
			//绑定图像管线
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);//绑定图像管线
			model.bindVertex(commandBuffer);//绑定顶点
			model.bindIndexBuffer(commandBuffer);//绑定索引缓冲
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);//绑定描述符集
			//绘制三角形
			terrain.drawVisibleChunks(commandBuffer, cameraPos, renderDistance);
			//terrain.drawOcean(commandBuffer);[旧版]
			oceanObj.draw(
				commandBuffer,
				descriptorSets[currentFrame],
				oceanHeight,
				oceanHeight * horizontal_displacement_intensity
			);

			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),commandBuffer);//绘制imgui也扔到缓冲区
			//结束渲染通道
			vkCmdEndRenderPass(commandBuffer);

			if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {//结束命令缓冲区
				throw std::runtime_error("failed to record command buffer!");
			}

		}
	};

	void LveRenderer::clean(VkDevice device) {
		if (graphicsPipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(
				device,
				graphicsPipeline,
				nullptr
			);

			graphicsPipeline = VK_NULL_HANDLE;
		}

		// 有多少个就销毁多少个
		for (VkSemaphore semaphore :
		renderFinishedSemaphores) {

			vkDestroySemaphore(
				device,
				semaphore,
				nullptr
			);
		}

		renderFinishedSemaphores.clear();

		// 这些按飞行帧数量销毁
		for (size_t i = 0;
			i < MAX_FRAMES_IN_FLIGHT;
			++i) {

			vkDestroySemaphore(
				device,
				imageAvailableSemaphores[i],
				nullptr
			);

			vkDestroyFence(
				device,
				inFlightFences[i],
				nullptr
			);
		}

		imageAvailableSemaphores.clear();
		inFlightFences.clear();

	}
}
