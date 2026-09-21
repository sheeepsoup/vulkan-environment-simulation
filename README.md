# 🌄 Vulkan GPU Hydraulic Erosion

<p align="center">
  Experimental Vulkan terrain renderer with GPU hydraulic erosion, procedural materials, shadows, and atmospheric rendering.
</p>

<p align="center">
  <a href="#english">English</a> · <a href="#chinese">中文</a>
</p>

---

## 📸 Gallery

<p align="center">
  <img src="https://github.com/user-attachments/assets/de7d7099-27e5-4cbf-825d-2044da3f11c9" width="100%" alt="Terrain overview"/>
  <img src="https://github.com/user-attachments/assets/278b55c9-92d7-47fc-9864-90f8f4695d79" width="100%" alt="Terrain erosion result"/>
  <img src="https://github.com/user-attachments/assets/4ff2dd5c-5855-4e06-be60-cd35b529b77e" width="100%" alt="Terrain material rendering"/>
  <img src="https://github.com/user-attachments/assets/6d0e04a6-d846-4325-8835-53eac87491ce" width="100%" alt="Terrain shadow rendering"/>
</p>

---

## <a id="english"></a>🇬🇧 English

## Introduction

A Vulkan learning and experimental rendering project focused on procedural terrain and GPU hydraulic erosion.

The current goal is not only to render terrain, but also to explore how erosion quality, GPU time, water-droplet allocation, slope filtering, and atomic-operation pressure affect the final landform.

## Current Features

- 🏔️ **Procedural terrain generation** using multi-layer FastNoiseLite noise
- 💧 **GPU hydraulic erosion** implemented with Vulkan compute shaders
- 🌊 Supports large-scale water-droplet erosion simulation with configurable droplet counts and step limits
- 📉 **Slope pre-pass** for identifying nearly flat regions before erosion
- ⚡ Early termination for droplets with low terrain influence
- 🧪 Erosion, flow, step-count, and movement-distance buffers for experimentation and profiling
- 🪨 Terrain material blending for **grass, dirt, rock, and snow**
- 🏔️ Material selection influenced by height, slope, normal direction, flow accumulation, and procedural noise
- 🌫️ Distance fog for atmospheric depth
- ☀️ Directional-light shadow-map prototype
- 🖱️ Free camera controls: WASD movement and mouse look
- 🛠️ ImGui debug controls for terrain, erosion, and rendering parameters
- 🌊 Basic ocean/water surface prototype

## Current Research Direction

The erosion system is being optimized through:

- Flat-region filtering based on terrain slope
- Droplet early-exit conditions
- Erosion-history tracking
- GPU-side statistics collection
- Reducing unnecessary sampling and atomic updates
- Comparing terrain detail preservation against GPU execution time

Historical benchmark progression for the same test scene:

```text
4.886600 s → 3.957070 s → 1.700000 s → 1.039470 s
```

Actual performance depends on GPU power mode, terrain size, droplet count, and shader parameters.

## Planned Features

- FFT ocean simulation
- Better shadow filtering and stability
- PBR material refinement
- Rain, wetness, and water accumulation
- Rivers, lakes, coastlines, and beaches
- Vegetation system: trees, grass, and biome placement
- Volumetric clouds and weather
- Terrain chunk streaming and LOD
- More systematic erosion-quality evaluation tools

## Building

### Dependencies

- [Vulkan SDK](https://vulkan.lunarg.com/)
- [SDL3](https://github.com/libsdl-org/SDL)
- [GLM](https://github.com/g-truc/glm)
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite)
- Dear ImGui

### Notes

Compile the GLSL shaders before running the program:

```bat
compile.bat
```

Then build the Vulkan solution with Visual Studio.

---

## <a id="chinese"></a>🇨🇳 中文

## 简介

这是一个基于 Vulkan 的地形渲染与 GPU 水力侵蚀实验项目。

项目不仅关注“生成一张地形”，也在探索侵蚀质量、GPU 时间、水滴分配、坡度筛选和原子操作竞争之间的关系。

## 当前功能

- 🏔️ 使用 FastNoiseLite 多层噪声生成程序化地形
- 💧 使用 Vulkan Compute Shader 实现 GPU 水力侵蚀
- 🌊 支持可配置水滴数量与步数的大规模侵蚀模拟
- 📉 使用坡度预计算识别近平坦区域
- ⚡ 对影响很小的水滴进行提前终止
- 🧪 记录侵蚀次数、流量、移动距离和步数等实验数据
- 🪨 根据草地、泥土、岩石、雪地进行地形材质混合
- 🏔️ 材质会受到高度、坡度、法线方向、水流量和噪声影响
- 🌫️ 基于距离的环境雾效
- ☀️ 方向光阴影贴图原型
- 🖱️ WASD 移动与鼠标视角控制
- 🛠️ ImGui 调试面板，可调整地形、侵蚀和渲染参数
- 🌊 基础海洋/水面原型

## 当前研究方向

目前主要从这些方面优化侵蚀系统：

- 根据坡度过滤无意义的平坦区域水滴
- 提前结束低收益水滴
- 记录每个区域的侵蚀历史
- 在 GPU 端收集实验统计数据
- 减少不必要的高度采样与原子操作
- 对比不同时间预算下的地形细节保留效果

同一测试场景下的历史优化记录：

```text
4.886600 s → 3.957070 s → 1.700000 s → 1.039470 s
```

实际性能会受到显卡功耗模式、地形尺寸、水滴数量和着色器参数影响。

## 后续计划

- FFT 海洋模拟
- 优化阴影稳定性与阴影过滤
- 完善 PBR 材质效果
- 雨水、潮湿地表与积水效果
- 河流、湖泊、海岸与沙滩
- 树木、草地和生物群系系统
- 体积云与天气系统
- 地形区块加载与 LOD
- 更完整的侵蚀质量评估工具

## 编译

### 依赖库

- [Vulkan SDK](https://vulkan.lunarg.com/)
- [SDL3](https://github.com/libsdl-org/SDL)
- [GLM](https://github.com/g-truc/glm)
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite)
- Dear ImGui

运行前先编译 GLSL 着色器：

```bat
compile.bat
```

然后使用 Visual Studio 编译 Vulkan 解决方案。
