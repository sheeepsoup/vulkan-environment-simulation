#include"oceanCascade.h"

namespace ocean_cascade {
	//构建函数
	OceanCascade::OceanCascade(lve::LveDevice& device, uint32_t oceanRange, float contribution, uint32_t spectrumResolution,
		const std::string& spectrumShaderPath, const std::string& evolutionShaderPath, const std::string& displacementShaderPath,
		const std::string& ifftShaderPath, float oceanChoppiness)
		: oceanRange(oceanRange), contribution(contribution), oceanChoppiness(oceanChoppiness),
		spectrum(device, spectrumShaderPath, spectrumResolution, static_cast<float>(oceanRange)),
		evolution(device, this->spectrum, evolutionShaderPath, spectrumResolution, static_cast<float>(oceanRange)),
		displacement(device, this->evolution, displacementShaderPath, spectrumResolution, static_cast<float>(oceanRange)),
		heightIFFT(device, evolution.getHtSpectrumImageView(), ifftShaderPath, spectrumResolution),
		displacementXIFFT(device, displacement.getDisplacementXView(), ifftShaderPath, spectrumResolution),
		displacementYIFFT(device, displacement.getDisplacementYView(), ifftShaderPath, spectrumResolution) {
	}
	void OceanCascade::update(VkCommandBuffer commandBuffer, float time) {
		evolution.recordEvolutionCommands(commandBuffer, time);//海洋波浪演化
		displacement.recordDisplacementCommands(
			commandBuffer,
			this->oceanChoppiness // choppiness，先从 0.8 ~ 1.2 测
		);//海洋位移计算
		heightIFFT.recordIFFTCommands(commandBuffer);//ifft计算[高度的]
		//下面是计算水平位移的,来改变海浪顶点的水平坐标
		// 新增：Dx -> X 位移图
		displacementXIFFT.recordIFFTCommands(commandBuffer);
		// 新增：Dy -> Y 位移图
		displacementYIFFT.recordIFFTCommands(commandBuffer);
		
	}
}