# 10. SPH Fluid

> 作业步骤：
> - 查看[文档](./documents/README.md)
> - 在[作业框架](../../Framework3D/Ruzino)中编写作业代码，主要是 **sph_fluid**文件夹 和 **hw10_sph_fluid.cpp** (位于Ruzino\source\Editor\geometry_nodes)
> - 按照[作业规范](../README.md)提交作业

## 作业递交

- 递交内容：程序代码、实验报告、动画演示，见[提交文件格式](#提交文件格式)
- 递交时间：2026年5月24日（周日）晚

## 要求

### 基础任务
- 实现弱可压缩SPH流体仿真（WCSPH）的完整流程（包括密度估计、粘性力计算、压力计算、速度与位置更新）
- 测试不同参数和边界条件设置下的仿真效果
- 从粒子重建表面，渲染结果 (这部分已经写好了的，但是你得连上对应节点)

### 可选任务
- 实现隐式不可压缩的SPH流体仿真方法 IISPH

### 拓展任务
- 对模拟过程进行定量分析
- 并行化加速
  - OpenMP
  - Compute Shader
  - OpenCL
  - CUDA/HIP/OneAPI/SYCL...
- 用深度学习方法进行模拟，并进行定性定量分析


## 提供的材料

### (1) 说明文档 `documents` [->](./documents/README.md) 

### (2) 作业项目 `Framework3D` [->](../../Framework3D/) 

## 提交文件格式

完成作业之后，打包如下内容即可：

- 修改的程序代码
  - 修改的节点源文件
  - sph_fluid 文件夹
- 仿真动画 (MP4, GIF)
- 实验报告 (PDF格式)
- stage.usdc； 在Ruzino目录下Assets文件夹中
- AI 对话记录
