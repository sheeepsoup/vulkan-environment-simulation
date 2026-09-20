#include"lve_uniform.h"
#include<array>
namespace lve {
	void LveUniform::createDescriptorSetLayout(VkDevice device) {
		std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

		bindings[0].binding = 0;
		bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bindings[0].descriptorCount = 1;
		bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;  // ★ 加 fragment

		bindings[1].binding = 1;
		bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;  // ★ 阴影图
		bindings[1].descriptorCount = 1;
		bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		bindings[1].pImmutableSamplers = nullptr;



		VkDescriptorSetLayoutCreateInfo layoutInfo{};//信息
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 2;
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {//创建
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}
	void LveUniform::clean(VkDevice device, uint32_t MAX_FRAMES_IN_FLIGHT) {
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	}
	void LveUniform::createUniformBuffer(uint32_t MAX_FRAMES_IN_FLIGHT, LveDevice& device) {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		//为每一帧创建独立的ubo
		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {//为每个飞行的帧加个ubo
			device.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(device.getDevice(), uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);//标记位置
		}

	}
	void LveUniform::updateUniformBuffer(uint32_t currentImage, VkExtent2D extent, const glm::mat4& modelMatrix,
		const glm::mat4& view, const glm::mat4& proj, const glm::mat4& lightViewProj, glm::vec3 cameraPos,glm::vec4 renderParams) {

		//UBO
		UniformBufferObject ubo{};
		ubo.model = modelMatrix;
		ubo.view = view;
		ubo.proj = proj;
		ubo.lightViewProj = lightViewProj;
		ubo.cameraPos = glm::vec4(cameraPos, 1.0f);
		ubo.renderParams = renderParams;
		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}
	void LveUniform::createDescriptorPool(uint32_t MAX_FRAMES_IN_FLIGHT, VkDevice device) {
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;


		VkDescriptorPoolCreateInfo poolInfo{};//信息
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 2;
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);//最大数量
		poolInfo.flags = 0;//保持默认
		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {//创建
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}
	void LveUniform::createDescriptorSets(uint32_t MAX_FRAMES_IN_FLIGHT, VkDevice device) {
		layouts.resize(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);//描述符集
		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {//创建
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {//填充描述符池
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr; // Optional
			descriptorWrite.pTexelBufferView = nullptr; // Optional
			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			imageInfo.imageView = shadowImageView;   // 从 Shadow 传进来
			imageInfo.sampler = shadowSampler;

			VkWriteDescriptorSet imageWrite{};
			imageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			imageWrite.dstSet = descriptorSets[i];
			imageWrite.dstBinding = 1;
			imageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			imageWrite.descriptorCount = 1;
			imageWrite.pImageInfo = &imageInfo;
			vkUpdateDescriptorSets(
				device,
				1,
				&imageWrite,
				0,
				nullptr
			);
		}


	}
	void LveUniform::init(LveDevice &device,uint32_t MAX_FRAMES_IN_FLIGHT, VkImageView shadowImageView,VkSampler shadowSampler) {
		//给赋值
		this->shadowImageView = shadowImageView;
		this->shadowSampler = shadowSampler;
		createDescriptorSetLayout(device.getDevice());
		createUniformBuffer(MAX_FRAMES_IN_FLIGHT, device);
		createDescriptorPool(MAX_FRAMES_IN_FLIGHT, device.getDevice());
		createDescriptorSets(MAX_FRAMES_IN_FLIGHT, device.getDevice());
	}
}