# LVGL Agent Workspace

基于 **LVGL + SDL** 的自动化 UI 开发工作仓库。核心能力：**从 HTML 页面自动生成嵌入式 LVGL C 代码**，并通过 SDL 模拟器进行像素级视觉回归验证。当前代码生成器为规则引擎（rule_based_html_v1），LLM 驱动生成为后续目标。

## 核心流程

```text
HTML 页面设计                  Board Profile (分辨率/字体/约束)
      │                                │
      └──────────┬─────────────────────┘
                 ▼
         代码生成（规则引擎 / LLM） (m1-generate-page.py)
                 │
                 ▼
         generated/<page>.c/.h
                 │
                 ├── 可移植性检查 (m1-portability-lint.py)
                 │
                 ▼
         CMake 编译 → SDL 模拟器渲染 (headless)
                 │
                 ▼
         截图 (lv_snapshot_take → PNG)
                 │
                 ▼
         像素级 diff 校验 (m1-page-validate.py)
                 │
                 ▼
         report.json (pass/fail) + diff.png (热力图)
                 │
                 ▼
         导出可移植交付包 → 嵌入式固件项目
```

## 3 分钟上手

第一次接触这个仓库，先不要自己建 task，先跑通内置 demo：

### 1. 安装核心依赖

```bash
sudo apt update
sudo apt install -y \
  build-essential cmake pkg-config \
  python3 python3-pip python3-pil \
  libsdl2-dev libsdl2-image-dev libfreetype6-dev
```

### 2. 做一次环境自检

```bash
tools/m1-pipeline.sh doctor
```

`doctor` 会检查：
- `cmake`、`pkg-config`、`python3`
- `Pillow`
- `SDL2` / `SDL2_image` / `FreeType`（包含仓库内置的 `SDL2_image` fallback）
- 是否存在 HTML 参考图渲染工具（`chromium` 或 `wkhtmltoimage`）
- `m1_real_project` 能否成功 `cmake configure`

### 3. 跑内置 quickstart

```bash
tools/m1-pipeline.sh quickstart
```

这条命令会直接跑仓库内置 demo task，成功后重点看：
- `workspace/tasks/demo_v1/artifacts/current.png`
- `workspace/tasks/demo_v1/artifacts/diff.png`
- `workspace/tasks/demo_v1/artifacts/report.json`

说明：
- `quickstart` 优先使用仓库已提供的 `reference/reference.png`，所以即使本机还没装浏览器截图工具，也能先跑通第一条闭环
- 只有你自己新建 task，且 `reference.render_from_html = true` 时，才需要额外安装 `chromium` / `google-chrome` / `wkhtmltoimage`

如果这里已经跑通，你再开始创建自己的 task。

## 从 HTML 或图片生成自己的 LVGL 页面

整个流程围绕 **任务 (task)** 组织。每个任务对应一个页面，包含输入 HTML、生成的 C 代码、校验产物和导出包。

### 1. 初始化任务

```bash
tools/m1-pipeline.sh init workspace/tasks/my_page_v1
```

如果你的输入是一张设计稿或页面截图，可以直接初始化图片任务：

```bash
tools/m1-pipeline.sh init workspace/tasks/my_image_page_v1 \
  --source-type image \
  --image /path/to/source.png
```

这会创建任务目录结构和 `task.json` 配置文件：

```text
workspace/tasks/my_page_v1/
├── task.json              # 任务配置（视口、profile、校验阈值等）
├── input/
│   ├── index.html         # HTML 输入，图片任务会先自动生成这个草稿
│   ├── source.png         # 图片任务时的源截图（文件名按原始扩展名变化）
│   ├── assets/            # 页面引用的图片等资源
│   └── notes.md           # 布局备注、约束说明
├── reference/             # 视觉参考图
├── generated/             # LLM 生成的 C 代码（自动填充）
├── artifacts/             # 截图、diff、report（自动生成）
└── export/                # 可移植交付包（自动生成）
```

### 2. 放入页面内容

有两种输入方式：

**方式 A：已有 HTML 文件**

直接复制到任务的 `input/` 目录：

```bash
cp /path/to/your_page.html workspace/tasks/my_page_v1/input/index.html
```

如果 HTML 中引用了图片等资源，一并放到 `input/assets/` 下：

```bash
cp logo.png workspace/tasks/my_page_v1/input/assets/
```

**方式 B：只有一个 URL**

先把页面内容抓取到 `input/index.html`：

```bash
# 简单页面（纯 HTML，无复杂 JS 渲染）
curl -L "https://example.com/your-page" -o workspace/tasks/my_page_v1/input/index.html

# 需要 JS 渲染的 SPA 页面，用浏览器导出：
# 1. 在浏览器中打开目标 URL
# 2. 右键 → 另存为 → 仅 HTML（或 Ctrl+S）
# 3. 将保存的 .html 文件复制到 input/index.html
```

如果页面有需要保留的图片，手动下载到 `input/assets/` 并修改 HTML 中的 `src` 路径为相对路径（如 `assets/logo.png`）。

> 提示：生成器解析 HTML 中的 `h1/h2/h3/p/button/a/img` 等标签，转换为对应的 LVGL 控件。复杂的 CSS 布局和 JS 交互不会被转换，只提取结构和文本内容。

**方式 C：只有一张图片**

如果你初始化的是图片任务：

```bash
tools/m1-pipeline.sh init workspace/tasks/my_image_page_v1 \
  --source-type image \
  --image /path/to/source.png
```

生成前可以先手动看一眼自动草拟的 HTML：

```bash
tools/m1-pipeline.sh draft-html workspace/tasks/my_image_page_v1/task.json
```

说明：
- 这一步会把截图粗略拆成几个视觉区域，再写入 `input/index.html`
- 当前是启发式草稿生成，不是视觉大模型逆向还原
- 产出的 HTML 适合继续人工修改，然后再走现有 `HTML -> LVGL` 流水线
- 图片任务默认把源图本身当作 `reference.image`，不会依赖浏览器渲染 HTML 参考图

### 3. 生成 LVGL C 代码

```bash
tools/m1-pipeline.sh generate workspace/tasks/my_page_v1/task.json
```

生成器读取 `task.json` 中的 profile 配置，并使用：
- HTML 任务：直接读取 `input/index.html`
- 图片任务：先从 `input/source.*` 草拟 `input/index.html`，再继续生成

输出：
- `generated/<page_id>_page.c` — LVGL 页面实现
- `generated/<page_id>_page.h` — 页面头文件
- `generated/manifest.json` — 生成元数据
- `generated/codegen_prompt.md` — 生成时使用的上下文摘要

生成的代码遵循 [LLM 代码生成规则](docs/llm_codegen_rules.md)，确保可移植性。

### 4. 编译 + 截图 + 校验（一键完成）

```bash
tools/m1-pipeline.sh run workspace/tasks/my_page_v1/task.json
```

`run` 命令自动执行完整流水线：
1. 调用 `generate` 生成 C 代码（如果不是 legacy 兼容任务）
2. 运行可移植性检查
3. 编译 → SDL 模拟器渲染 → 截图 → 像素级 diff 校验

当前默认使用单页隔离模式：
- 每个 task 使用独立 build 目录，例如 `m1_real_project/build/demo-v1/`
- 当前 task 只注册并编译自己的 generated page，不再把其它 workspace 页面一起编进来
- 这样可以避免“别的页面编译失败影响当前页面运行”

注意：
- 对于你自己新建的 task，默认 `reference.render_from_html = true`
- 这意味着第一次 `run` 需要本机安装 `chromium` / `google-chrome` / `wkhtmltoimage` 之一
- 如果你没有这些工具，可以先手动提供 `reference/reference.png`，并把 `render_from_html` 改为 `false`
- 图片任务默认 `render_from_html = false`，直接拿源图做参考图

校验产物输出到 `artifacts/`：

| 文件 | 说明 |
|------|------|
| `current.png` | viewport 截图 |
| `full.png` | 整页内容截图 |
| `diff.png` | 三栏对比：参考 \| 当前 \| 热力图 |
| `report.json` | 结构化校验报告，含 pass/fail 判定 |

### 5. 导出交付包

```bash
tools/m1-pipeline.sh export workspace/tasks/my_page_v1/task.json
```

将生成的 `.c/.h` 和资源打包到 `export/portable_bundle/`，可直接复制到嵌入式固件项目中使用。

## task.json 配置说明

`task.json` 是每个任务的核心配置，控制生成、校验和导出行为：

```jsonc
{
  "page_id": "demo_page",           // 页面标识，用于生成函数名和文件名
  "page_name": "Demo Page",         // 人类可读名称
  "input": {
    "source_type": "html",
    "html_entry": "input/index.html", // HTML 输入文件或图片任务的草稿输出
    "image_entry": "input/source.png" // 图片任务时的源截图
  },
  "target": {
    "profile": "../../../profiles/sim_1280x800.json",  // 板级 profile
    "viewport": { "width": 1280, "height": 800 },
    "language": "zh-CN"
  },
  "generation": {
    "allow_custom_draw": false,       // 是否允许自定义绘制
    "allow_freetype": false,         // 是否允许 FreeType 运行时字体
    "allow_filesystem_assets": false, // 是否允许文件系统资源加载
    "component_mode": "portable"     // 生成模式：portable = 可移植
  },
  "validation": {
    "max_diff_ratio": 0.18,          // 最大差异像素占比
    "max_mean_abs_diff": 22.0        // 最大平均色差
  },
  "failure_policy": {
    "max_iterations": 8,             // 最大迭代次数
    "stop_on_build_error": true
  }
}
```

完整 schema 见 `workspace/task.schema.json`。

## Board Profile

Board profile 定义目标硬件的约束，位于 `profiles/` 目录：

| Profile | 分辨率 | 用途 |
|---------|--------|------|
| `sim_1280x800.json` | 1280x800 | 桌面模拟器开发 |
| `esp32_480x320.json` | 480x320 | ESP32 嵌入式板 |
| `stm32_800x480.json` | 800x480 | STM32 嵌入式板 |

Profile 控制代码生成时的字体选择、资源策略和 API 约束，防止生成器依赖桌面专有特性。详见 [docs/board_profiles.md](docs/board_profiles.md)。

## 生成代码规范

LLM 生成的页面代码必须遵循以下约束（详见 [docs/llm_codegen_rules.md](docs/llm_codegen_rules.md)）：

- 每个页面导出 `xxx_page_create()` 和 `xxx_page_get_content_root()` 两个函数
- 禁止使用 SDL/模拟器专有 API
- 禁止硬编码绝对路径
- 字体选择由 board profile 决定
- 资源路径必须相对于任务或导出包
- 生成的代码必须同时通过视觉校验和可移植性检查

## 目录结构

```text
lvgl_agent/
├── m1_real_project/       # 主工程：运行时、注册中心、手写示例页面
│   ├── src/               # main.c, page_registry, token_page, home_page
│   ├── workflow/          # 页面级任务 schema 和配置
│   ├── references/        # 参考图 (PNG)
│   ├── artifacts/         # 截图、diff、report 产物
│   └── assets/            # 图片资源
├── workspace/             # LLM 任务驱动工作区
│   ├── task.schema.json   # 任务定义 schema
│   └── tasks/             # 各任务目录 (demo_v1, token_landing_v1, ...)
│       └── <task_id>/     # 每个任务的完整生命周期目录
├── profiles/              # 板级 profile (sim, esp32, stm32)
├── tools/                 # 流水线脚本 (Bash + Python)
├── lv_port_linux_test/    # LVGL v9.6.0-dev + SDL 模拟器（上游）
└── docs/                  # 架构文档、生成规则、部署指南
```

## 环境搭建

推荐 Ubuntu 22.04/24.04, Linux x86_64。

核心依赖见上面的“3 分钟上手”。  
如果你要为新 task 自动生成 HTML 参考图，还需要额外安装下面任意一种工具：
- `chromium`
- `chromium-browser`
- `google-chrome`
- `wkhtmltoimage`

## 工程内部架构

### 分层职责

- **页面层** (`m1_real_project/src/`) — 每个页面实现 `xxx_page_create()` 和 `xxx_page_get_content_root()`
- **注册层** (`page_registry.c`) — 页面 id → 函数映射，workspace 任务通过 `m1-sync-generated-pages.py` 自动注册
- **运行层** (`main.c`) — LVGL + SDL 初始化，通过 `M1_PAGE` 环境变量选择页面，支持 viewport 和整页两种截图模式
- **校验层** (`m1-page-validate.py`) — PIL 像素级 diff，输出三栏对比图和结构化 JSON 报告
- **构建层** (`CMakeLists.txt`) — 从 `lv_conf.defaults` 生成配置，按 task 输出独立 `build/<task_id>/lvgl_m1_demo`

### CMake 依赖

```text
lvgl_m1_demo (executable)
  ├── src/main.c, page_registry.c, home_page.c, token_page.c
  ├── generated_page_registry.c (workspace 任务自动生成)
  └── lvgl (static library) → SDL2, SDL2_image, FreeType, lodepng
```

## 工具脚本一览

| 脚本 | 说明 |
|------|------|
| `tools/m1-doctor.py` | 环境自检，检查依赖、仓库内 SDL2_image fallback、浏览器截图工具和 CMake configure |
| `tools/m1-pipeline.sh` | 任务流水线统一入口（doctor/quickstart/init/draft-html/generate/render-ref/sync/lint/run/export） |
| `tools/m1-image-to-html.py` | 图片 → HTML 草稿 |
| `tools/m1-generate-page.py` | HTML → LVGL C 代码生成 |
| `tools/m1-render-html-ref.py` | HTML 参考图渲染 |
| `tools/m1-page-validate.py` | PIL 像素级 diff 校验器 |
| `tools/m1-portability-lint.py` | 可移植性静态检查 |
| `tools/m1-sync-generated-pages.py` | workspace 任务自动注册桥接 |
| `tools/m1-task-init.py` | 任务目录初始化 |
| `tools/m1-task-run.py` | 任务执行器 |
| `tools/m1-export-page.py` | 页面导出打包 |
| `tools/lvgl-m1-real.sh` | 主工程编译/运行/截图（底层） |
| `tools/m1-page-flow.sh` | 页面级校验闭环（底层） |

## 手动操作参考

以下命令用于调试或单步执行，日常使用推荐通过 `m1-pipeline.sh` 驱动。

```bash
# 环境自检
tools/m1-pipeline.sh doctor

# 跑内置 demo
tools/m1-pipeline.sh quickstart

# 查看已注册页面
tools/lvgl-m1-real.sh list-pages

# GUI 运行（需要桌面环境）
M1_PAGE=token tools/lvgl-m1-real.sh run

# 单独截图
M1_PAGE=token tools/lvgl-m1-real.sh screenshot output.png

# 单独校验
tools/m1-page-flow.sh validate token
```

对于 workspace task 页面，`tools/lvgl-m1-real.sh` 现在支持仅通过 `M1_PAGE` 自动推导运行上下文：

```bash
M1_PAGE=demo_page tools/lvgl-m1-real.sh run
M1_PAGE=stitch_smart_home_panel tools/lvgl-m1-real.sh screenshot output.png
```

行为说明：
- 自动从 `workspace/tasks/*/task.json` 反查对应的 `page_id`
- 自动推导对应的隔离 build 目录，例如 `m1_real_project/build/demo-v1/`
- 如果未显式传入 `M1_TASK_JSON`，会自动读取 task 里的 `target.viewport`
- 如果目标二进制尚未构建，会自动执行 `configure + build`

手动传参仍然有效，并且优先级更高：
- `M1_TASK_JSON`
- `M1_BUILD_DIR`
- `M1_VIEWPORT_WIDTH`
- `M1_VIEWPORT_HEIGHT`

## 文档索引

| 文档 | 说明 |
|------|------|
| [docs/architecture.md](docs/architecture.md) | 系统架构与执行阶段 |
| [docs/llm_codegen_rules.md](docs/llm_codegen_rules.md) | LLM 代码生成约束规则 |
| [docs/board_profiles.md](docs/board_profiles.md) | Board profile 配置说明 |
| [docs/lvgl-sdl-cross-machine-deployment.md](docs/lvgl-sdl-cross-machine-deployment.md) | 跨机器部署指南 |
