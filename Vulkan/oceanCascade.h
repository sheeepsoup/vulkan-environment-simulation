#pragma once
#include<vulkan/vulkan.h>
#include"lve_device.h"
#include"spectrum.h"
#include"evolution.h"
#include"displacement.h"
#include"ifft.h"
namespace ocean_cascade {
    class OceanCascade {
    public:
		OceanCascade(lve::LveDevice& device, uint32_t oceanRange, float contribution, uint32_t spectrumResolution,
            const std::string& spectrumShaderPath, const std::string& evolutionShaderPath, const std::string& displacementShaderPath, 
            const std::string& ifftShaderPath, float oceanChoppiness);
        uint32_t oceanRange;//海洋范围
		float contribution;//不同海浪合并时的影响强度[小中大]

        spectrum::Spectrum spectrum;//频谱图
		evolution::Evolution evolution;//海水演变
		displacement::Displacement displacement;//位移图

		ifft::IFFT heightIFFT;//ifft生成海浪高度图
		ifft::IFFT displacementXIFFT;//ifft生成海浪位移图X
		ifft::IFFT displacementYIFFT;//ifft生成海浪位移图Y
		float oceanChoppiness;//海浪的凹凸程度,不同海浪的精细度可以不一样

        void update(VkCommandBuffer cmd, float time);


        void generateInitialSpectrum(
            const spectrum::SpectrumSettings& settings
        ) {
            spectrum.generateInitialSpectrum(settings);
        }

        VkImageView getHeightImageView() const {
            return heightIFFT.getHeightMapImageView();
        }

        VkImageView getDisplacementXImageView() const {
            return displacementXIFFT.getHeightMapImageView();
        }

        VkImageView getDisplacementYImageView() const {
            return displacementYIFFT.getHeightMapImageView();
        }

        VkImage getHeightImage() const {
            return heightIFFT.getHeightMapImage();
        }

        VkImage getDisplacementXImage() const {
            return displacementXIFFT.getHeightMapImage();
        }

        VkImage getDisplacementYImage() const {
            return displacementYIFFT.getHeightMapImage();
        }

        uint32_t getOceanRange() const {
            return oceanRange;
        }

        float getContribution() const {
            return contribution;
        }

	private:

    };



}