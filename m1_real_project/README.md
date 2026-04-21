# M1 Real LVGL Project

页面开发主工程，提供页面实现、页面路由、SDL 运行、截图导出、参考图校验的完整闭环。

## 目录结构

```text
m1_real_project/
├── CMakeLists.txt              # 构建入口
├── lv_conf.defaults            # LVGL 配置
├── src/
│   ├── main.c                  # 程序入口、SDL/LVGL 启动、截图导出
│   ├── page_registry.h         # 页面描述符与查找接口
│   ├── page_registry.c         # 页面 id → 函数映射
│   ├── home_page.c / .h        # 页面：工具箱仪表盘
│   └── token_page.c / .h       # 页面：Token 落地页
├── assets/                     # 页面静态资源（图片等）
├── references/                 # 参考图 (PNG)
├── artifacts/                  # 截图、diff、report 产物（按页面 id 分目录）
└── workflow/
    ├── WORKFLOW.md             # 校验工作流说明
    ├── page-task.schema.json   # 任务 JSON schema
    └── tasks/                  # 页面校验任务配置
        └── token.json
```

## 工程架构

### 数据流

```text
  页面代码 (.c)                 校验任务 (tasks/xxx.json)
       │                              │
       ▼                              ▼
  page_registry 注册            reference_image 路径
       │                              │
       ▼                              │
  cmake build                        │
       │                              │
       ▼                              │
  lvgl_m1_demo ──M1_PAGE──▶ 渲染指定页面
       │                              │
       │ headless + snapshot          │
       ▼                              │
  artifacts/xxx/                       │
    current.png ──────────────────────┘
    full.png                            │
       │                                │
       ▼                                ▼
  m1-page-validate.py ──────▶ diff.png + report.json
```

### 分层职责

**构建层** (`CMakeLists.txt` + `lv_conf.defaults`)

- 从 `lv_conf.defaults` 生成 `lv_conf.h`
- 通过 `add_subdirectory` 引用 `lv_port_linux_test/lvgl`
- 编译所有页面源码，链接为可执行文件 `lvgl_m1_demo`
- 当前已注册页面：`home`、`token`

```text
lvgl_m1_demo (executable)
  ├── src/main.c, page_registry.c, home_page.c, token_page.c
  └── lvgl (static library) → SDL2, SDL2_image, FreeType, lodepng
```

**运行层** (`main.c`)

初始化 LVGL + SDL 显示 (1280x800)，根据页面 id 创建 screen 并加载。

页面选择方式（优先级从高到低）：
1. 命令行 `--page <id>`
2. 环境变量 `M1_PAGE=<id>`
3. 缺省走 `m1_page_default()`（当前为 token）

截图模式（通过环境变量触发，互斥）：
- `LVGL_SCREENSHOT_OUT=<path>` — 导出 `lv_screen_active()` 的视口截图，然后退出
- `LVGL_SCREENSHOT_FULL_OUT=<path>` — 导出 `get_content_root()` 的整页内容截图，然后退出

截图流程：渲染 5 帧 → `lv_snapshot_take()` → BGRA 到 RGBA 合成（含 alpha 混合到白色背景）→ lodepng 编码 PNG。

其他命令行参数：
- `--list-pages` — 打印所有已注册页面 id 和名称

**注册层** (`page_registry.h` / `page_registry.c`)

```c
typedef struct {
    const char * id;                       // 页面 id，如 "home"、"token"
    const char * name;                     // 显示名称
    m1_page_create_fn_t create;            // 创建页面函数
    m1_page_content_root_fn_t get_content_root;  // 返回内容根节点
} m1_page_descriptor_t;
```

提供的接口：
- `m1_page_find(id)` — 按 id 查找，id 为 NULL 时返回默认页面
- `m1_page_default()` — 获取默认页面
- `m1_page_list(&count)` — 获取全部页面列表

**页面层** (`src/*.c`)

每个页面实现两个函数：
- `xxx_page_create(void)` — 创建 LVGL 对象树，返回 screen 对象
- `xxx_page_get_content_root(void)` — 返回内容区域的根节点（用于整页截图）

当前已有页面：
- `home` — 工具箱仪表盘：顶栏 + 侧边栏 + 8 个分类 + 26 张卡片，支持响应式布局
- `token` — Token 落地页：顶栏 + hero 区域 + 供应商 logo 行

**校验层** (`workflow/` + `tools/m1-page-validate.py`)

通过 task JSON 配置校验参数，脚本自动执行编译 → 截图 → diff → 输出报告。

判定条件（三个条件同时满足才 pass）：
- `diff_ratio` (差异像素占总像素比例) <= `max_diff_ratio`
- `mean_abs_diff` (三通道平均色差) <= `max_mean_abs_diff`
- 尺寸匹配，或 `require_size_match` 为 false

## 如何使用

### 查看可用页面

```bash
tools/lvgl-m1-real.sh list-pages
```

### 编译

```bash
tools/lvgl-m1-real.sh rebuild
```

### 运行

```bash
# 默认页面（当前为 token）
tools/lvgl-m1-real.sh run

# 指定页面
M1_PAGE=home tools/lvgl-m1-real.sh run
M1_PAGE=token tools/lvgl-m1-real.sh run
```

### 导出截图

```bash
# viewport 截图
M1_PAGE=token tools/lvgl-m1-real.sh screenshot \
  m1_real_project/artifacts/token/current.png

# 整页内容截图
M1_PAGE=token tools/lvgl-m1-real.sh screenshot-full \
  m1_real_project/artifacts/token/full.png
```

### 执行完整校验闭环

```bash
tools/m1-page-flow.sh run token
```

自动执行：编译 → 截图 → 对比参考图 → 输出 `diff.png` + `report.json`。

分步执行：

```bash
tools/m1-page-flow.sh build token       # 只编译
tools/m1-page-flow.sh screenshot token  # 只截图
tools/m1-page-flow.sh validate token    # 只校验（需要已有截图）
```

### 校验产物

闭环执行后，在 `artifacts/<page-id>/` 下生成：

| 文件 | 说明 |
|------|------|
| `current.png` | viewport 截图 |
| `full.png` | 整页内容截图 |
| `diff.png` | 三栏对比：参考 \| 当前 \| 热力图 |
| `report.json` | 结构化校验报告，含 pass/fail 判定及各项指标 |

`report.json` 关键字段：

```json
{
  "pass": true,
  "diff_ratio": 0.001234,
  "mean_abs_diff": 3.45,
  "changed_pixels": 1234,
  "total_pixels": 1024000,
  "reference_size": [1280, 800],
  "current_size": [1280, 800],
  "size_match": true
}
```

## 如何新增页面

**1. 编写页面代码**

新建 `src/xxx_page.c` 和 `src/xxx_page.h`：

```c
// xxx_page.h
lv_obj_t * xxx_page_create(void);
lv_obj_t * xxx_page_get_content_root(void);
```

实现要求：
- `xxx_page_create()` 创建 LVGL 对象树，返回 screen
- `xxx_page_get_content_root()` 返回可滚动内容区域的根节点，用于整页截图；如果页面不可滚动，返回 screen 即可

**2. 注册页面**

在 `src/page_registry.c` 的 `g_pages[]` 数组末尾添加：

```c
{
    .id = "xxx",
    .name = "XXX Page",
    .create = xxx_page_create,
    .get_content_root = xxx_page_get_content_root,
},
```

**3. 加入构建**

在 `CMakeLists.txt` 的 `add_executable` 中加入 `src/xxx_page.c`。

**4. 配置校验任务**

在 `workflow/tasks/` 下新建 `xxx.json`，参考 `workflow/page-task.schema.json`：

```json
{
  "page_id": "xxx",
  "page_name": "XXX Page",
  "entry_page": "xxx",
  "reference_image": "references/xxx-reference.png",
  "artifacts_dir": "artifacts/xxx",
  "viewport": { "width": 1280, "height": 800 },
  "validation": {
    "pixel_diff_threshold": 16,
    "max_diff_ratio": 0.22,
    "max_mean_abs_diff": 26.0,
    "require_size_match": false
  }
}
```

**5. 放置参考图**

将目标参考截图放到 `references/` 下，文件名与 task JSON 中 `reference_image` 一致。

**6. 运行闭环**

```bash
tools/m1-page-flow.sh run xxx
```

根据 `artifacts/xxx/report.json` 和 `artifacts/xxx/diff.png` 迭代修改页面代码，直到 `pass: true`。

## 关键文件

| 文件 | 职责 |
|------|------|
| `CMakeLists.txt` | 构建入口，管理源文件和依赖 |
| `lv_conf.defaults` | LVGL 运行配置（字体、SDL、截图等） |
| `src/main.c` | 程序入口，页面路由，截图导出 |
| `src/page_registry.c` | 页面注册中心，id → 函数映射 |
| `src/token_page.c` | Token 落地页（可作页面实现参考） |
| `workflow/WORKFLOW.md` | 校验工作流详细说明 |
| `workflow/tasks/token.json` | 校验任务配置示例 |
