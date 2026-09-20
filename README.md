# 🌄 Vulkan 环境渲染模拟器
<p align="center">
  <img src="https://github.com/user-attachments/assets/16667eff-fd7d-44ea-857f-8fa578816c2a" alt="地形渲染全景" width="100%"/>
</p>

<p align="center">
  <a href="#english">English</a> · <a href="#chinese">中文</a>
</p>

---

## <a id="english"></a>🇬🇧 English

## 📖 Introduction

A graphics rendering project learning Vulkan from scratch, aiming to build a real-time rendering simulator for natural environments including **terrain, vegetation, water bodies, clouds, and oceans**.

> This project is planned for long-term development. Welcome to star and follow my progress! ⭐

Currently completed: terrain generation and hydraulic erosion simulation. More environmental elements will be added progressively.

---

## ✨ Current Features

- 🏔️ **Procedural Terrain Generation** — Multi-layer noise blending based on FastNoiseLite for highly detailed terrain
- 💧 **GPU Hydraulic Erosion Simulation** — Compute shader simulation with 200,000 droplets, creating natural gullies and sediment deposits
- 🎨 **Intelligent Texture Blending** — Automatically blends grass/dirt/rock/snow based on slope, height, normal direction, and water flow
- 🌫️ **Atmospheric Fog** — Distance-based fog rendering for enhanced scene depth
- 🖱️ **Free Camera Control** — WASD movement + mouse rotation for full 3D observation

---

## 📸 Gallery

<img width="1370" height="815" alt="Terrain Overview" src="https://github.com/user-attachments/assets/16667eff-fd7d-44ea-857f-8fa578816c2a" />

## ✨ Features

> Erosion-simulated terrain generation
> Unique terrain texture algorithm

### 🧪 Latest Research Update

The erosion shader now tracks how many droplets have affected each terrain location. When erosion accumulates too heavily in the same area, subsequent droplets can exit early. This avoids excessive droplet accumulation and redundant computation on flat terrain.

- **Before:** `4.886600 s`
- **After:** `3.95707 s`
- **Improvement:** approximately `19%` faster GPU erosion

---

### Key Milestones

- **Phase 1**: Basic Vulkan setup (SDL3 + Vulkan), simple triangle rendering
- **Phase 2**: Terrain generation with FastNoiseLite, multi-layer noise blending
- **Phase 3**: CPU-based hydraulic erosion (later replaced by GPU version)
- **Phase 4**: GPU compute shader erosion with parallel droplet simulation
- **Phase 5**: Advanced texture blending based on slope, height, and water flow
- **Phase 6**: (Planned) Infinite terrain chunk loading

---

### 🔮 Planned Features

- **Phase 6**: Infinite terrain chunk loading with LOD
- **Phase 7**: Vegetation system (trees and grass)
- **Phase 8**: Lake and ocean rendering
- **Phase 9**: Dynamic cloud system

---

### Notes on the Code

> The commented-out CPU erosion code in `lve_model.cpp` is a remnant of the initial implementation.
> It is kept to show the learning path and for potential performance comparisons in the future.

---

## 🛠️ Building

### Dependencies
- [SDL3](https://github.com/libsdl-org/SDL) — Window and input management
- [Vulkan SDK](https://vulkan.lunarg.com/) — Graphics and compute API
- [GLM](https://github.com/g-truc/glm) — Mathematics library
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) — Procedural noise generation



## <a id="chinese"></a>🇨🇳 中文

# 🌄 Vulkan 环境渲染模拟器

<p align="center">
  <img src="https://github.com/user-attachments/assets/16667eff-fd7d-44ea-857f-8fa578816c2a" alt="地形渲染全景" width="100%"/>
</p>


## 📖 简介
这是一个从零开始学习 Vulkan 的图形渲染项目，目标是实现一个包含**地形、植被、水体、云层、海洋**等自然环境的实时渲染模拟器。
> 项目预计持续开发很长时间，欢迎收藏关注我的进展！⭐
目前已完成地形生成与水力侵蚀模拟，后续将逐步添加更多环境要素。
----


## ✨ 当前功能
- 🏔️ **程序化地形生成** — 基于 FastNoiseLite 的多层噪声叠加，生成高细节地形
- 💧 **GPU 水力侵蚀模拟** — 计算着色器并行模拟 20 万水滴，形成自然沟壑与沉积
- 🎨 **智能纹理混合** — 根据坡度、高度、法线方向和水流流量，自动混合草地/泥土/岩石/雪地
- 🌫️ **环境雾效** — 基于距离的雾效渲染，增强场景层次感
- 🖱️ **自由摄像机控制** — WASD 移动 + 鼠标旋转，全方位观察地形


## 📸 效果展示
<img width="1370" height="815" alt="faec3c47076a1febc4e78b21814e1a55" src="https://github.com/user-attachments/assets/16667eff-fd7d-44ea-857f-8fa578816c2a" />
## ✨ 功能特性
> 侵蚀模拟的地形生成
> 独特的地形纹理算法

### 🧪 最新研究进展

侵蚀计算着色器现在会记录每个地形位置累计的水滴侵蚀次数。当同一区域的侵蚀累积过多时，后续水滴会提前退出模拟，避免大量水滴在平原区域反复堆积并产生无效计算。

- **优化前：** `4.886600 s`
- **优化后：** `3.95707 s`
- **性能提升：** GPU 侵蚀计算速度约提升 `19%`


### 重要里程碑
- **第一阶段**：基础Vulkan环境搭建（SDL3 + Vulkan），实现简单三角形渲染
- **第二阶段**：使用FastNoiseLite生成地形，多层噪声叠加
- **第三阶段**：CPU端水力侵蚀模拟（后被GPU版本替代）
- **第四阶段**：GPU计算着色器实现侵蚀，支持水滴并行模拟
- **第五阶段**：基于坡度、高度、水流量的高级纹理混合
- **第六阶段**：（计划中）实现无限地形区块加载


### 🔮 计划中
- **第六阶段**：无限地形区块加载（LOD）
- **第七阶段**：树木与草地植被系统
- **第八阶段**：湖泊与海洋渲染
- **第九阶段**：动态云层系统


### 关于代码的一些说明
> `lve_model.cpp` 中保留了被注释掉的CPU侵蚀代码，这是最初实现方案的遗留。
> 保留它一方面是为了展示学习路径，另一方面也方便将来做性能对比。
> 


## 🛠️ 编译构建
### 依赖库
- [SDL3](https://github.com/libsdl-org/SDL) — 窗口与输入管理
- [Vulkan SDK](https://vulkan.lunarg.com/) — 图形与计算 API
- [GLM](https://github.com/g-truc/glm) — 数学运算库
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) — 程序化噪声生成

