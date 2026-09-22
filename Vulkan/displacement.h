#pragma once

#include "lve_device.h"
#include "evolution.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>

namespace displacement {

    class Displacement {
    public:
        struct PushConstant {
            uint32_t resolution;
            uint32_t oceanRange;
            float choppiness;//频域位移系数
            float padding;
        };

        Displacement(
            lve::LveDevice& device,
            const evolution::Evolution& evolutionObj,
            const std::string& computeShaderPath,
            uint32_t resolution,
            uint32_t oceanRange
        );

        ~Displacement();

        Displacement(const Displacement&) = delete;
        Displacement& operator=(const Displacement&) = delete;

        // 每帧在 evolution 之后、IFFT 之前调用
        void recordDisplacementCommands(
            VkCommandBuffer commandBuffer,
            float choppiness
        );

        // 给两个 IFFT 对象作为输入：
        // Dx(k,t) -> IFFT -> displacementXMap
        // Dy(k,t) -> IFFT -> displacementYMap
        VkImageView getDisplacementXView() const {
            return displacementXImageView;
        }

        VkImageView getDisplacementYView() const {
            return displacementYImageView;
        }

        VkImage getDisplacementXImage() const {
            return displacementXImage;
        }

        VkImage getDisplacementYImage() const {
            return displacementYImage;
        }

        uint32_t getResolution() const {
            return resolution;
        }

        uint32_t getOceanRange() const {
            return oceanRange;
        }

        void clean();

    private:
        void createDescriptorSetLayout();
        void createDescriptorPool();
        void createDescriptorSet();

        void createSpectrumImages();
        void createImage(
            VkImage& image,
            VkDeviceMemory& imageMemory,
            VkImageView& imageView
        );

        void createComputePipeline(const std::string& computeShaderPath);

        static std::vector<char> readFile(const std::string& filename);
        VkShaderModule createShaderModule(const std::vector<char>& code);

    private:
        lve::LveDevice& device;
        const evolution::Evolution& evolutionObj;

        uint32_t resolution;
        uint32_t oceanRange;

        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline pipeline = VK_NULL_HANDLE;

        // Dx(k,t)：X 方向位移频谱
        VkImage displacementXImage = VK_NULL_HANDLE;
        VkDeviceMemory displacementXImageMemory = VK_NULL_HANDLE;
        VkImageView displacementXImageView = VK_NULL_HANDLE;

        // Dy(k,t)：Y 方向位移频谱
        VkImage displacementYImage = VK_NULL_HANDLE;
        VkDeviceMemory displacementYImageMemory = VK_NULL_HANDLE;
        VkImageView displacementYImageView = VK_NULL_HANDLE;
    };

} // namespace displacement