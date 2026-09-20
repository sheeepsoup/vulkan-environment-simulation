#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "lve_device.h"
namespace lve {
    class LveModel;
    class LveTerrain;
}
namespace shadow {

    class Shadow {
    public:
        Shadow(lve::LveDevice& device, const std::string& shadowShaderPath);
        ~Shadow();

        void init(uint32_t shadowMapSize);
        void updatePushConstant(VkCommandBuffer cmd, const glm::mat4& lightViewProj);

        VkRenderPass  getRenderPass()  const { return shadowRenderPass; }
        VkFramebuffer getFramebuffer() const { return shadowFramebuffer; }
        VkPipeline    getPipeline()    const { return shadowPipeline; }
        VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

        // 主渲染采样用
        VkImageView getImageView() const { return shadowImageView; }
        VkSampler   getSampler()   const { return shadowSampler; }
        void recordShadowPass(VkCommandBuffer cmd,
            const glm::mat4& lightViewProj,
            lve::LveModel& model,
            lve::LveTerrain& terrain);

        void clean();
        
    private:
        lve::LveDevice& device;
        std::string shadowShaderPath;
        uint32_t shadowMapSize = 2048;

        struct PushConstantData {
            glm::mat4 lightViewProj;
        };

        // 管线
        VkShaderModule   shadowShaderModule = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline       shadowPipeline = VK_NULL_HANDLE;

        // 深度图
        VkImage        shadowImage = VK_NULL_HANDLE;
        VkDeviceMemory shadowImageMemory = VK_NULL_HANDLE;
        VkImageView    shadowImageView = VK_NULL_HANDLE;
        VkSampler      shadowSampler = VK_NULL_HANDLE;
        VkFormat       shadowDepthFormat = VK_FORMAT_UNDEFINED;

        // RenderPass + Framebuffer
        VkRenderPass  shadowRenderPass = VK_NULL_HANDLE;
        VkFramebuffer shadowFramebuffer = VK_NULL_HANDLE;

        void createPipelineLayout();
        void createShadowPipeline();
        void createDepthResources();
        void createRenderPass();
        void createFramebuffer();
        void createSampler();

        std::vector<char> readFile(const std::string& filename);
        VkShaderModule createShaderModule(const std::vector<char>& code);
    };

}