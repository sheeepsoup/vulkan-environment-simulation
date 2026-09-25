# 🌄 Vulkan GPU Hydraulic Erosion & Ocean Research

<p align="center">
  Experimental Vulkan renderer for procedural terrain, GPU hydraulic erosion, atmospheric rendering, and FFT ocean simulation.
</p>

<p align="center">
  <a href="#english">English</a> · <a href="#chinese">中文</a>
</p>

---

## 📸 Gallery

<p align="center">
  <img width="2007" height="1280" alt="FFT ocean prototype" src="https://github.com/user-attachments/assets/d57e1f18-93ef-4d1d-86ef-b5a11d987680" />
  <br/>
  <sub>FFT Ocean Prototype</sub>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/de7d7099-27e5-4cbf-825d-2044da3f11c9" width="100%" alt="Terrain overview"/>
  <img src="https://github.com/user-attachments/assets/278b55c9-92d7-47fc-9864-90f8f4695d79" width="100%" alt="Terrain erosion result"/>
  <img src="https://github.com/user-attachments/assets/4ff2dd5c-5855-4e06-be60-cd35b529b77e" width="100%" alt="Terrain material rendering"/>
  <img src="https://github.com/user-attachments/assets/6d0e04a6-d846-4325-8835-53eac87491ce" width="100%" alt="Terrain shadow rendering"/>
  <br/>
  <sub>Procedural Terrain, Hydraulic Erosion, Material Blending, and Shadows</sub>
</p>

---

## <a id="english"></a>🇬🇧 English

## Overview

A Vulkan learning and rendering research project focused on procedural natural environments.

The project started with procedural terrain generation and GPU hydraulic erosion. It now also explores FFT ocean rendering, including spectrum generation, wave evolution, inverse FFT reconstruction, horizontal displacement, and water-surface lighting.

The goal is not only to generate visually interesting terrain, but also to study the relationship between erosion quality, GPU time budget, slope filtering, droplet allocation, and atomic-operation pressure.

## Current Features

### Terrain

- 🏔️ Multi-layer procedural terrain generation with FastNoiseLite
- 💧 GPU hydraulic erosion implemented with Vulkan compute shaders
- 🌊 Large-scale droplet simulation with configurable droplet counts and step limits
- 📉 Slope pre-pass for filtering nearly flat terrain before erosion
- ⚡ Early termination for low-impact droplets
- 🧪 GPU-side erosion statistics: droplet candidates, skipped droplets, movement distance, step count, erosion history, and flow accumulation
- 🪨 Procedural terrain material blending:
  - Grass
  - Dirt
  - Rock
  - Snow
- 🎨 Material selection based on height, slope, normal direction, flow accumulation, and procedural noise
- 🌫️ Distance-based atmospheric fog
- ☀️ Directional-light shadow-map prototype
- 🖱️ Free camera controls with WASD movement and mouse look
- 🛠️ ImGui controls for terrain, erosion, shadows, and ocean parameters

### FFT Ocean — Experimental

- 🌊 JONSWAP-inspired initial frequency spectrum generation
- 💨 Configurable wind direction, wind speed, fetch, gamma, directional spreading, high-frequency cutoff, and cross swell
- ⏱️ Time-domain spectrum evolution
- 🔁 GPU inverse FFT reconstruction
- 📈 Height-map reconstruction for vertical wave displacement
- ↔️ X/Y displacement maps for choppy horizontal wave motion
- ✨ Dynamic water normals, Fresnel reflection, directional sun highlights, and procedural sky reflection
- 🎛️ Runtime ocean tuning through ImGui

## Technical Pipeline

```text
FastNoiseLite
    ↓
Initial Terrain Height Field
    ↓
Slope Pre-pass
    ↓
GPU Hydraulic Erosion
    ↓
Height / Flow / Erosion Statistics
    ↓
Normal Reconstruction
    ↓
Procedural Terrain Materials
    ↓
Shadow Map + Fog + Terrain Rendering
```

```text
JONSWAP-style Spectrum Parameters
    ↓
H0 Initial Spectrum
    ↓
Time Evolution H(k, t)
    ↓
IFFT Height Spectrum
    ↓
Height Map + X/Y Displacement Maps
    ↓
Ocean Vertex Displacement
    ↓
Fresnel / Specular / Sky Reflection Rendering
```

## Erosion Research Direction

Current erosion optimization experiments include:

- Flat-region filtering based on slope
- Early exit for low-impact droplets
- Erosion-history tracking
- GPU-side statistics collection
- Reducing unnecessary height sampling
- Reducing unnecessary atomic updates
- Comparing terrain-detail preservation against GPU execution time
- Exploring more adaptive water-droplet allocation strategies

Historical benchmark progression for the same terrain test scene:

```text
4.886600 s → 3.957070 s → 1.700000 s → 1.039470 s
```

Actual performance depends on terrain resolution, erosion parameters, GPU power mode, droplet count, and shader configuration.

## Releases

### v0.1.0 — Terrain Stable

`v0.1.0` is the stable terrain-focused release.

Included:

- Procedural terrain generation
- GPU hydraulic erosion
- Slope pre-pass and early-exit optimization
- Terrain material blending
- Fog
- Directional shadow-map prototype
- ImGui terrain and erosion controls

This release intentionally does **not** include the FFT ocean system.

### Next Release — FFT Ocean Experimental

The next development release will introduce the current FFT ocean pipeline:

- Initial spectrum generation
- Wave evolution
- GPU IFFT
- Height and horizontal displacement maps
- Fresnel water shading
- Runtime ocean debugging controls

The ocean system is currently experimental and will continue to receive visual and performance improvements.

## Planned Features

- Better shadow filtering and stabilization
- PBR material refinement
- Rain, wet terrain, and water accumulation
- Rivers, lakes, coastlines, beaches, and ocean interaction
- Foam generation and nonlinear wave-crest treatment
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
- [Dear ImGui](https://github.com/ocornut/imgui)

### Build Steps

Compile GLSL shaders first:

```bat
compile.bat
```

Copy compiled shaders to the debug output directory when needed:

```bat
copy_shaders_to_x64.bat Debug
```

Then build and run the Vulkan solution with Visual Studio.

---

## <a id="chinese"></a>🇨🇳 中文

## 项目简介

这是一个基于 Vulkan 的自然环境渲染与实验项目，当前重点是程序化地形、GPU 水力侵蚀，以及正在开发中的 FFT 海洋模拟。

项目最初从地形生成与水力侵蚀开始，目前进一步探索海洋频谱生成、波浪演变、GPU IFFT、高度图重建、水平位移和水面光照。

项目不仅关注“生成一张看起来不错的地形”，也尝试研究侵蚀质量、GPU 时间预算、坡度筛选、水滴分配策略和原子操作竞争之间的关系。

## 当前功能

### 地形系统

- 🏔️ 使用 FastNoiseLite 多层噪声生成程序化地形
- 💧 使用 Vulkan Compute Shader 实现 GPU 水力侵蚀
- 🌊 支持可配置水滴数量与步数的大规模侵蚀模拟
- 📉 使用坡度预计算筛除近平坦区域
- ⚡ 对低收益水滴进行提前终止
- 🧪 在 GPU 端记录候选水滴、跳过水滴、移动距离、步数、侵蚀历史和水流量等实验数据
- 🪨 地形材质自动混合：
  - 草地
  - 泥土
  - 岩石
  - 雪地
- 🎨 材质会受到高度、坡度、法线方向、水流量和噪声影响
- 🌫️ 基于距离的环境雾效
- ☀️ 方向光阴影贴图原型
- 🖱️ WASD 移动与鼠标视角控制
- 🛠️ ImGui 调试面板，可调整地形、侵蚀、阴影和海洋参数

### FFT 海洋系统 — 实验中

- 🌊 基于 JONSWAP 思路生成初始海浪频谱
- 💨 可调整风向、风速、Fetch、Gamma、风向集中度、高频截止和交叉涌浪
- ⏱️ 基于时间推进海浪频谱
- 🔁 使用 GPU IFFT 还原空间域海浪
- 📈 高度图驱动垂直海浪位移
- ↔️ X/Y 位移图驱动水平挤压与 Choppy Wave 效果
- ✨ 水面法线、菲涅耳反射、方向光高光和程序化天空反射
- 🎛️ 可通过 ImGui 实时调试海洋参数

## 技术流程

```text
FastNoiseLite
    ↓
初始地形高度场
    ↓
坡度预计算
    ↓
GPU 水力侵蚀
    ↓
高度 / 流量 / 侵蚀统计
    ↓
法线重建
    ↓
程序化地形材质
    ↓
阴影贴图 + 雾效 + 地形渲染
```

```text
JONSWAP 风谱参数
    ↓
H0 初始频谱
    ↓
时间演变 H(k, t)
    ↓
IFFT 频谱还原
    ↓
高度图 + X/Y 水平位移图
    ↓
海洋顶点位移
    ↓
菲涅耳反射 / 高光 / 天空反射
```

## 当前研究方向

目前主要从这些方面优化侵蚀系统：

- 根据坡度筛除无意义的平坦区域水滴
- 提前结束低收益水滴
- 记录侵蚀历史
- 在 GPU 端收集实验统计数据
- 减少不必要的高度采样
- 减少不必要的原子操作
- 对比不同 GPU 时间预算下的地形细节保留效果
- 探索更加自适应的水滴分配策略

同一测试场景下的历史优化记录：

```text
4.886600 s → 3.957070 s → 1.700000 s → 1.039470 s
```

实际性能会受到地形分辨率、侵蚀参数、水滴数量、显卡功耗模式和着色器参数影响。

## 版本说明

### v0.1.0 — Terrain Stable

`v0.1.0` 是以稳定地形系统为核心的版本。

包含：

- 程序化地形生成
- GPU 水力侵蚀
- 坡度预计算与提前终止优化
- 地形材质混合
- 雾效
- 方向光阴影贴图原型
- ImGui 地形与侵蚀调试面板

该版本暂时**不包含 FFT 海洋系统**。

### 下一版本 — FFT Ocean Experimental

后续开发版本将加入目前正在实现的 FFT 海洋系统：

- 初始频谱生成
- 海浪频谱演变
- GPU IFFT
- 高度图和水平位移图
- 菲涅耳水面渲染
- 海洋 ImGui 调试参数

海洋系统目前仍处于实验阶段，后续会继续优化视觉效果和性能。

## 后续计划

- 优化阴影过滤与稳定性
- 完善 PBR 材质效果
- 雨水、潮湿地表与积水效果
- 河流、湖泊、海岸、沙滩与海洋交互
- 泡沫生成与非线性浪峰处理
- 树木、草地和生物群系系统
- 体积云与天气系统
- 地形区块加载与 LOD
- 更系统的侵蚀质量评估工具

## 编译

### 依赖库

- [Vulkan SDK](https://vulkan.lunarg.com/)
- [SDL3](https://github.com/libsdl-org/SDL)
- [GLM](https://github.com/g-truc/glm)
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite)
- [Dear ImGui](https://github.com/ocornut/imgui)

### 编译步骤

先编译 GLSL 着色器：

```bat
compile.bat
```

如需将着色器复制到调试目录：

```bat
copy_shaders_to_x64.bat Debug
```

最后使用 Visual Studio 编译并运行 Vulkan 解决方案。
