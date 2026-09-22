# Aether 引擎架构 — 并发模型

## 模块层次（自底向上）

```
Core      零 OS 依赖: 容器 / 数学 / Atomic (唯一并发原语, RefCounted 需要)
RHI       抽象设备接口 (WebGPU 形状), 资源全部 RefCounted
Platform  触 OS 的一切: 窗口/消息泵 + Threading HAL + JobSystem
Engine    渲染器 / SpriteBatch / 输入 / 音频 / UI + System 框架 + EngineLoop
Demo      FlightShmup / DysonSphere (作为一组 System 跑在调度器上)
```

跨模块规则：只能向下依赖；D3D11/WebGPU 后端全私有，公开面只有 RHI 工厂。

## 线程 HAL（Source/Platform/Public/Threading/）

所有平台相关能力（即便底层用 std 实现）必须经过封装层，禁止引擎代码直接
使用 `std::thread/mutex/atomic/condition_variable`：

| 类型 | 用途 | 备注 |
|---|---|---|
| `Core::Atomic<T>` | 无锁热点标量/计数 | 显式内存序命名（`LoadRelaxed/StoreRelease/FetchAddAcqRel/CompareExchange*`） |
| `Platform::Thread` | OS 线程 | `Run(entry, name, priority)`；`ThreadPriority::Audio` 走 MMCSS |
| `Platform::Mutex` + `ScopedLock` | 冷路径粗锁 | 队列/慢速状态 |
| `Platform::SpinLock` | 极短临界区 | 禁止跨系统调用/分配持有 |
| `Platform::Event` | auto-reset 事件 | Win32 原生 HANDLE（WASAPI 需要），其他平台 condvar 模拟 |
| `Platform::CpuInfo` | 核数 / cache line | JobSystem 粒度与防伪共享 padding |

实现集中在 `Source/Platform/Private/Threading/Threading.cpp`（std 默认实现 +
Win32 特化）。日后替换为 SRWLOCK / WaitOnAddress / fiber 只动实现文件。

## JobSystem（Platform::Jobs）

- 固定 worker 池（逻辑核数 - 1，`Jobs::Init(n)` 可覆盖）；主线程通过
  `WaitFor` 的帮忙执行（work-creeping）参与计算，等待不空转。
- 每 worker 一条互斥保护的双端队列（本地 LIFO / 窃取 FIFO / 公共提交口）。
  `JobQueue` 是独立接缝，日后可整体换成 Chase-Lev 无锁实现。
- `ParallelFor(begin, end, grain, fn, counter*)`：分块派发，`fn(rangeBegin,
  rangeEnd, threadIndex)`；小范围或单核时内联执行零开销。
- `Jobs::ThreadIndex()`：0 = 主/外部线程，1..N = worker。用于索引每线程
  状态（SpriteBatch bin、戴森球网格 part）。
- `Jobs::ScratchAlloc/ScratchReset`：每线程线性 arena，cache-line 对齐，
  帧首（无 job 在飞时）重置。
- **Web 构建**：所有入口内联同步执行，API 完全一致，不创建任何线程。

## System 框架（Engine::SystemScheduler）

一个 System 是一段有名字的更新逻辑，声明自己属于哪个 Phase、读/写哪些
Blackboard 标签：

```cpp
Engine::SystemDef def;
def.Name = "Factory";
def.SysPhase = Engine::Phase::Simulation;
def.Update = [this](Engine::SystemContext& ctx) { ... };
def.Reads  = {Engine::TypeIdOf<TagGrid>()};
def.Writes = {Engine::TypeIdOf<TagFactory>()};
scheduler.Register(std::move(def));
```

调度规则（`Build()` 启动时一次拓扑分层）：
- 同一 Phase 内：读写标签冲突的系统按**注册序**串行（Kahn 分层），
  无冲突的系统放进同一波并发执行（跑在 JobSystem 上）。
- 波与波之间隐式同步；Phase 之间同步点由 `EngineLoop` 保证。
- 注册序即语义序：迁移旧代码时按原 `Update()` 的语句顺序注册即可保序。

Phase 顺序：`Input → Simulation → RenderPrep → FrameBegin → RenderSubmit → UI → FrameEnd`。

**引擎自己的服务也是 System**，由 `EngineLoop` 在构造时注册（先于游戏的
startup 回调，注册序即语义序）：

| 引擎 System | Phase | 职责 |
|---|---|---|
| `EngineTimer` | Input | 更新 Timer，填 `ctx.Delta/UnscaledDelta/Elapsed`（写 `Tags::Time`） |
| `EngineInput` | Input | `Input::CaptureSnapshot` 填本帧快照（写 `Tags::Input`） |
| `EngineFrameBegin` | FrameBegin | `BeginFrame` 打开渲染 pass，写 `ctx.FrameActive` |
| `EngineUI` | UI | 默认开着：清批次 → `UI::Draw` → 屏幕空间冲刷；游戏自己集成 UI 时 `SetAutoUI(false)` |
| `EngineFrameEnd` | FrameEnd | `EndFrame` 提交 + Present |

契约：游戏系统若与引擎系统**同相位**读它写的东西（Input 相位读输入、
读 dt），必须在 `Reads` 里声明 `Tags::Input` / `Tags::Time` 才能被排到
引擎系统之后；相位边界之后的相位天然有序。RenderSubmit/UI 系统需检查
`ctx.FrameActive`，为 false 时早退（如窗口 resize 竞态）。

`EngineLoop::Tick` 因此只剩三件事：渲染器就绪门控（未就绪时跑初始化
状态机）、`Jobs::ScratchReset`（帧首重置 arena）、按序跑完全部 Phase。
除了调度器本身，引擎没有任何帧内硬编码逻辑。
RenderSubmit/UI 在 `BeginFrame/EndFrame` 之间执行（主线程单点提交 GPU）。

`Blackboard` 是类型键控的服务定位器（`TypeIdOf<T>()`），系统发布指针、
按标签声明访问面；标签只影响调度顺序，不代理数据访问。

## 引擎帧循环（Engine::EngineLoop）

```
Tick():
  Renderer.Tick()            // 设备初始化状态机/调试队列 (就绪门控)
  (首次就绪) startup 回调     // 创建游戏资源、注册游戏 System
  Jobs::ScratchReset()       // 帧首重置每线程 arena
  SystemContext ctx          // Input 指向成员快照, 由 EngineInput 系统填充
  依序 RunPhase: Input → Simulation → RenderPrep →
                 FrameBegin → RenderSubmit → UI → FrameEnd
```

引擎服务全部以 System 形式参与调度（见上节）；`InputSnapshot` 帧首由
`EngineInput` 系统捕获、当帧只读，worker 线程可安全访问；旧的 `Input::`
静态查询 API 保留（快照捕获时同步刷新）。

## 并行渲染管线

- **SpriteBatch 多线程累积**：`Add/AddUv/AddQuad` 写入调用线程的 bin
  （`Jobs::ThreadIndex()` 索引），无锁；`Render()` 按 bin 升序合并
  （确定性），(bin,index) 全序引用排序（任意排序算法皆稳定），顶点构建
  `ParallelFor`（每精灵 32 float 互不重叠），主线程单点提交。
- **Pass A / Pass B 分批**：深空背景走独立 SpriteBatch 在 3D pass 前
  冲刷；游戏层精灵在 RenderPrep 阶段跨线程累积，UI 之后一次冲刷。
- **呈现流水线**：D3D11 3 缓冲 FLIP_DISCARD + `MaximumFrameLatency(3)`，
  CPU 组装 N+2 帧时 GPU 仍在画 N 帧；跨帧上传安全由 `Map(WRITE_DISCARD)`
  重命名（D3D11）与队列顺序（WebGPU）保证。

## DSP demo 的系统划分

| System | 说明 | 并行性 |
|---|---|---|
| PlayerInput | 输入→建造/相机意图 + 太阳方位 | Simulation 链首 |
| Universe / Mecha | 轨道自转 / 机甲运动 | 串行链 |
| Factory | 生产线模拟（读上一帧电力满足率，1 帧滞后握手） | 串行（保皮带确定性） |
| Power / Tech / Dyson | 电力结算 / 科技 / 戴森球增长 | **三者并发** |
| CameraAmbience | 相机跟随 + 发射 VFX 差分 + 音频混音 | Simulation 链尾 |
| DspRenderPrep | 矿脉 13,824 格遍历、O(n²) 电塔弧线、逐建筑精灵、戴森球帆群重建，全部 ParallelFor 进各线程 bin | **重度并行** |
| DspRenderSubmit | Pass A 冲刷 → 3D pass → UI → Pass B 冲刷 → perf 统计 | 主线程 |

## 性能观察

DSP Main 旧帧循环的 `[perf]` 统计保留在 `SysRenderSubmit`：每 120 帧
输出平均/峰值帧时间与精灵数。对比并行化基线时重点看 RenderPrep 相位
吞吐与帧时间方差。

## 已知边界 / 后续方向

- Web 构建单线程回退：Scheduler 的"并发波"在 Web 上退化为顺序执行
  （JobSystem 内联），语义不变。
- FactorySystem 内部仍串行（皮带按数组序结算，行为确定性优先）；
  如需并行可做分阶段双缓冲（读旧写新，按拓扑序分波）。
- UI 立即模式在 RenderSubmit/UI 相位读 `Input` 静态 API 与鼠标状态，
  保持主线程执行。
- 3D pass（World3D）尚未并行组装；戴森球帆群已并行，其余静态网格
  低频重建无需并行。
