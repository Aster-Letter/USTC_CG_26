# 9. Mass Spring

> 作业步骤：
> - 查看[文档](./documents/README.md)
> - 在[作业框架](../../Framework3D/Ruzino)中编写作业代码，主要是 **mass_spring** 文件夹 和 **hw9_node_mass_spring.cpp**(位于Ruzino/source/Editor/geometry_nodes)
> - 按照[作业规范](../README.md)提交作业

## 作业递交

- 递交内容：程序代码、实验报告、动画演示，见[提交文件格式](#提交文件格式)
- 递交时间：2026年5月17日（周日）晚

## 要求



### 基础任务
- 实现基础的半隐式与隐式的质点弹簧仿真
- 在不同参数设定下进行仿真，对比结果
### 可选任务
- 实现基于惩罚力的布料与球之间的碰撞处理
- 复现[Liu et al. Siggraph 2013] 提出的基于Local-Global思想的质点弹簧系统加速算法
- 体网格的质点弹簧仿真
### 拓展任务
- 对模拟过程进行定量分析
- 并行化加速
  - OpenMP
  - Compute Shader
  - OpenCL
  - CUDA/HIP/OneAPI/SYCL...
- 用深度学习方法进行模拟，并进行定性定量分析
  - [PINN](https://zhuanlan.zhihu.com/p/590571656)
  - [Neural Operator](https://www.sciencedirect.com/science/article/pii/S0925231225011907)




## 提供的材料

### (1) 说明文档 `documents` [->](./documents/README.md) 

### (2) 作业项目 `Framework3D` [->](../../Framework3D/) 

### (3) 测试数据 `data` [->](./data/)

## 提交文件格式

完成作业之后，打包4类内容即可：

- 修改的程序代码
  - 修改的节点源文件
  - mass_spring 文件夹
- 仿真动画 (MP4, GIF)
- 实验报告 (PDF格式)
- stage.usdc； 在Ruzino目录下Assets文件夹中


### 注意事项
- 导入数据（网格和纹理）时使用**相对路径**，例如，将你的数据放在**可执行文件目录**下，直接通过 `FilePath = 'xxx.usda'` 或者 `FilePath = 'zzz.png'` 访问，或者定位到作业目录的 `data/` 文件夹中
- 请大家**尽量将算法相关代码都在节点定义文件中写完**，**避免**去额外创建其他的头文件/源文件将算法分离出去
- 节点的**输入&输出你都是可以修改**的，不要死脑经说“我这个明明要xxxx输入/输出，为什么节点没有xxxxx”