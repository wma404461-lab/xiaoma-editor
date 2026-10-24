# 小马编辑 · PonyEdit

UE 场景制作编辑器插件。包含导入、命名、材质、Pivot、散布及编辑面板相关 C++ 源码；来自已有 PonyEdit 工程，插件描述版本 0.1.0。以实际源码实现为准，界面入口不代表每项完整流程均已验收。

## 使用源码

将本仓库放入自己的 C++ UE 项目 `Plugins/PonyEdit`，重新生成项目文件，用 Visual Studio 构建 Editor 目标，再在插件列表启用。现有工程面向 UE 5.6；其他版本需自行核验 API 兼容性。本次只整理源文件和模块依赖，没有启动编辑器或重新做完整引擎编译。

`Source/PonyEditEditor` 为编辑器模块，`Source/PonyEditRuntime` 为设置和运行时模块。需要安装合法取得的 Unreal Engine 与 C++ 工具链；不包含引擎源码、引擎二进制、工程素材和缓存。

自制插件源码按 MIT 发布；Unreal Engine 仍受其自身许可约束。
