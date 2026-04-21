# 7. Rasterization and Path Tracing

> 作业步骤：
> - 查看[作业文档](./docs/README.md)
> - 在[项目目录](../../Framework3D/)中编写作业代码
> - 按照[作业规范](../README.md)提交作业

## 作业递交

- 递交内容：程序代码、实验报告及 `stage.usdc` 文件，见[提交文件格式](#提交文件格式)
- 递交时间：2026年4月26日（周日）晚

## 要求
- 基础任务
  - (Rast.) 实现 Blinn-Phong 着色模型 ([参考资料](https://learnopengl-cn.github.io/02%20Lighting/03%20Materials/))
    - 正确计算法线贴图
    - 正确计算着色公式
  - (Rast.) 实现 Shadow Mapping 算法 ([参考资料](https://learnopengl-cn.github.io/05%20Advanced%20Lighting/03%20Shadows/01%20Shadow%20Mapping/))
  - (P.T.) 矩形光源相关内容 （相交计算，采样计算，Irradiance计算）
  - (P.T.) 路径追踪算法中着色递归计算与Russian Roulette
- 可选任务
  - (Rast. )实现 Percentage Close Soft Shadow ([参考资料](https://zhuanlan.zhihu.com/p/478472753))
  - (Rast.) 实现 Screen Space Ambient Occlusion ([参考资料](https://learnopengl-cn.github.io/05%20Advanced%20Lighting/09%20SSAO/#ssao))
  - (Rast.) 实现Displacement Mapping ([Ref](https://zhuanlan.zhihu.com/p/369442463))
  - (P.T.) 实现更复杂的BRDF模型并进行MIS ([Ref](https://zhuanlan.zhihu.com/p/379681777))
- 拓展任务
  - 全面对比光栅 & 光追效果、时间等效果
  - 修改参数，对比效果
  - 修改光栅光追渲染方法，实现非真实感渲染 [Ref](https://zhuanlan.zhihu.com/p/142145970)



## 提供的材料

### (1)说明文档 
本次作业的要求说明和一些辅助资料
- 作业文档(今年的) [->](./docs/)
- 光栅化去年的文档(仅供参考) [->](./docs-rast/README.md)
- 光追去年年的文档(仅供参考) [->](https://github.com/USTC-CG/USTC_CG_25/blob/main/Homeworks/7_path_tracing/rtfd.pdf)
### (2)作业框架 [->](../../Framework3D/)

### (3)测试数据 [->](./data/)
> 注：其中data_c中1.usda将**法线贴图**替换为了**位移贴图**，即你在shader中访问的法线贴图实际上为位移贴图！
> data_a中数据建议在PT中使用！


## 提交文件格式

完成作业之后，打包三类内容即可：

- 修改的程序代码
  - `Ruzino\source\Plugins\hd_RUZINO_GL` 文件夹
  - `Ruzino\source\Plugins\hd_RUZINO_Embree` 文件夹
  - 其他你修改过的东西

- 报告：请**提交PDF**格式

- stage.usdc； 在Ruzino目录下Assets文件夹中

### 注意事项

- 导入数据（网格和纹理）时使用**相对路径**，例如，将你的数据放在**可执行文件目录**下，直接通过 `FilePath = 'xxx.usda'` 或者 `FilePath = 'zzz.png'` 访问，或者定位到作业目录的 `data/` 文件夹中
- 请大家**尽量将算法相关代码都在节点定义文件中写完**，**避免**去额外创建其他的头文件/源文件将算法分离出去
- 节点的**输入&输出你都是可以修改**的，不要死脑经说“我这个明明要xxxx输入/输出，为什么节点没有xxxxx”