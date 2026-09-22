# Aether 编码规范（Microsoft / vge 式风格）

## 1. 总则

- 命名空间：PascalCase，与模块/目录同名。根命名空间 `Aether`（容器、数学等基础类型直接位于根）；模块命名空间 `Aether::RHI`、`Aether::Platform`、`Aether::Engine`（`UI`、`Shaders` 为其子命名空间）、`Aether::Math`、后端 `Aether::D3D11` / `Aether::WebGPU`。Demo 用自己的顶层命名空间（`Shmup`、`DSP`、`DSPArt`）。缩写词保持全大写（`RHI`、`UI`、`DSP`）。
- 大括号：Allman 风格，独占一行。缩进 4 空格。
- 头文件保护：`#pragma once`。
- 注释：`//` 行注释；公共类与复杂算法必须有说明。

## 2. 命名

| 元素 | 规则 | 示例 |
|---|---|---|
| 类 / 结构 / 枚举类型 | PascalCase | `RenderPipelineDesc`, `SpriteBatch` |
| 函数 / 方法 | PascalCase，动词开头 | `BeginFrame()`, `IsKeyDown()`, `DrawIndexed()` |
| 类成员变量 | `m` + PascalCase | `mDevice`, `mClearColor`, `mFrameStarted` |
| 公开 POD 字段 | PascalCase，无前缀 | `Position`, `FovYDeg`, `InitialData` |
| 数学向量分量 | 小写（惯例例外） | `x, y, z, w`、`r, g, b, a` |
| 局部变量 / 参数 | camelCase | `clearColor`, `deltaTime` |
| 常量 | 全大写下划线 | `MAX_SPRITES`, `PI`, `DEG_TO_RAD` |
| 文件级全局 | `g` + PascalCase | `gRenderer`, `gMusicMutex` |
| 函数级静态 | `s` + PascalCase | `sDesc`, `sFrameCount` |
| 宏 | `AETHER_` 前缀全大写 | `AETHER_ASSERT`, `AETHER_PLATFORM_WEB` |
| 枚举值 | PascalCase | `BackendType::D3D11` |
| 文件 / 目录 | PascalCase 文件名 + PascalCase 模块目录 | `Engine/Public/Renderer.h` |

## 3. 模块与包含

- 模块：`Core`、`RHI`、`D3D11`、`WebGPU`、`Platform`、`Engine`、`Demo/*`。
- 只有对外接口进 `Public/`，实现细节进 `Private/`。后端模块（D3D11/WebGPU）全部私有，公开面只有 RHI 工厂。
- include 一律相对**目标模块根**：本模块的写 `"Renderer.h"`；跨模块写相对对方模块根的路径，如 `"Container/Array.h"`、`"RHI.h"`、`"Platform.h"`，由构建系统注入对方模块的 Public 路径。
- 禁止跨模块包含对方的 `Private/` 头（构建系统层面不可达）。
- 平台相关代码保留平台原生风格：Win32/D3D11 处的 `HWND`、`WNDCLASSEX`、`HRESULT`、`ComPtr`；WebGPU/Emscripten 处的 `WGPU*`、`EM_JS`。自研标识符仍按本规范。

## 4. 基础库（禁用 STL 容器）

禁止 `std::vector/string/map/unordered_map/shared_ptr/unique_ptr/function/optional` 与 `<algorithm>`。统一用 Core：

| 自研 | 替代 | 要点 |
|---|---|---|
| `Array<T>` | `std::vector` | `Add/EmplaceAdd/RemoveAt/RemoveAtSwap/RemoveIf/Insert/Resize/Reserve/Count/IsEmpty/Find/Contains/Sort/Data/First/Last`，范围 for 用小写 `begin/end` |
| `String` | `std::string` | `CStr()/Length()/IsEmpty()/Find/Sub/StartsWith/EndsWith`，`String::Format(fmt, ...)`，`==/!=/+` |
| `HashMap<K,V>` | `map/unordered_map` | `Add(k,v)`（存在则覆盖）、`FindOrAdd(k)`（缺省构造，等价 `map[k]`）、`Find/Contains/Remove/Count`；遍历 `for (auto& e : map) e.Key / e.Value` |
| `RefCounted` + `RefPtr<T>` | `std::shared_ptr` | `MakeRef<T>(...)` 构造；`Get()/Reset()/IsValid()`；下跨类型 `StaticCastRef<Dst>(src)` 或 `static_cast` 裸指针 |
| `UniquePtr<T>` + `MakeUnique` | `std::unique_ptr` | move-only，`Reset/Release/Get` |
| `Function<Sig>` | `std::function` | move-only 闭包，`operator()` 调用 |
| `Random` | `mt19937 + distributions` | `NextU32/NextU64/NextF32/Range(i32,i32)/Range(f32,f32)`，确定性种子 |
| `Math::` | `<cmath>` 封装 | `Vec2/Vec3/Vec4/Mat4/Color`、`Min/Max/Clamp/Abs/Lerp/Saturate`、`PI/TWO_PI/DEG_TO_RAD` |

保留允许：C 运行时（`cstdio/cmath/cstring/cassert/cstdint/cstdlib`）、`printf`、平台 SDK 头。数学矩阵为**列主序**、D3D 深度约定（勿改语义）。

## 5. 线程纪律（禁止裸用 std 并发原语）

引擎代码**禁止**直接使用 `std::thread / std::mutex / std::atomic / std::condition_variable / std::lock_guard`，一律走封装层，便于日后按平台替换原生实现（SRWLOCK、WaitOnAddress、fiber 等）：

| 自研 | 替代 | 要点 |
|---|---|---|
| `Atomic<T>`（Core，`Threading/Atomic.h`） | `std::atomic` | 显式内存序命名（`LoadRelaxed/LoadAcquire/StoreRelease/FetchAddAcqRel/CompareExchange*`），无隐式转换 |
| `Platform::Thread` | `std::thread` | `Run(entry, name, priority)/Join/Detach`；静态 `SetCurrentThreadName/SetCurrentThreadPriority/SleepMillis/YieldCpu` |
| `Platform::Mutex` + `ScopedLock` | `std::mutex` + `lock_guard` | 冷路径粗粒度临界区；短热临界区用 `SpinLock`/原子 |
| `Platform::SpinLock` + `ScopedSpinLock` | 自旋 | 仅限极短临界区（几十周期），禁止跨系统调用/分配/回调持有 |
| `Platform::Event` | `CreateEvent`/condvar | auto-reset；`Signal/Reset/Wait(timeout)`；Win32 下 `NativeHandle()` 暴露 `HANDLE` |

层次：`Core::Atomic` 是最底层（`RefCounted` 需要）；其余触 OS 的同步原语、线程、JobSystem 都在 `Platform::Threading`。参照样板：`Engine/Private/Audio.cpp`（锁纪律：热点标量用 atomic、队列冷路径用 mutex）。Web 构建保持单线程，线程 API 编译通过但不应创建线程。

## 6. 转换对照（旧 → 新）

```
init() / begin_frame()          → Init() / BeginFrame()
device_ / clear_color_          → mDevice / mClearColor
position（POD 字段）            → Position
clear_color（局部）             → clearColor
kMaxSprites                     → MAX_SPRITES
g_renderer / s_count            → gRenderer / sCount
std::vector<X> v; v.push_back   → Array<X> v; v.Add
emplace_back                    → EmplaceAdd
erase(begin()+i)                → RemoveAt(i)（保持序） / RemoveAtSwap(i)（O(1)）
std::map[k] = v                 → map.FindOrAdd(k) = v
map.insert({k,v}) / emplace     → map.Add(k, v)
for (auto& [k, v] : m)          → for (auto& e : m) ... e.Key, e.Value
std::make_shared<X>             → MakeRef<X>
std::make_unique<X>             → MakeUnique<X>
std::static_pointer_cast<D>(s)  → StaticCastRef<D>(s)
std::to_string(x)               → String::Format(...)（按显示精度选格式）
std::clamp/min/max              → Math::Clamp/Min/Max
uniform_real_distribution       → rng.Range(lo, hi)
```
