# LVGL Agent 问题清单

## P0 — 运行时崩溃 / 功能错误

- [ ] **#1 字体路径硬编码** — `token_page.c`、`home_page.c`、`demo_page_page.c` 写死 `/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc`，导出到嵌入式必崩。应抽成统一 `ui_font.c`，路径由 board profile 或编译宏注入。
- [ ] **#2 代码生成器是纯规则解析，非 LLM** — `m1-generate-page.py` 只用 HTMLParser 提取标签文本，忽略 CSS/嵌套/布局。README 和 architecture.md 描述不符。
- [ ] **#3 token_page.c 字体缓存内存泄漏** — 缓存满后新字体不缓存也不释放；三个页面字体加载方式互不一致。

## P1 — 可靠性 / 可维护性

- [ ] **#4 page_registry.c 硬编码 256 上限** — 溢出静默截断无警告；默认页面用 `g_pages[1]` 硬编码索引。
- [ ] **#5 main.c 截图等待是魔法数字** — 固定 5 帧 80ms，复杂页面可能渲染不完，无自适应等待。
- [ ] **#6 m1-pipeline.sh 缺少中间产物校验** — generate 后不检查产物就跑 lint；build 失败后继续截图。
- [ ] **#7 m1-sync-generated-pages.py 竞态** — CMake 和 task-run 都调 sync，并行构建可能写坏注册文件；重复 page_id 不改退出码。
- [ ] **#8 CMakeLists.txt FreeType 检测脆弱** — 只检查 `/usr/bin/pkg-config` 固定路径。

## P2 — 功能缺失

- [ ] **#9 生成器不支持 CSS** — 颜色/间距/字号全部硬编码，HTML style 被忽略。
- [ ] **#10 生成器不支持嵌套布局** — 卡片式布局被拍平成一维列表，无 grid/flex 语义映射。
- [ ] **#11 task.json 多个字段未使用** — dpi、language、component_mode、allow_custom_draw、notes_file 未读取。
- [ ] **#12 可移植性检查太浅** — 只做正则，漏宏/拼接路径；不检查 malloc/printf；注释误报。
- [ ] **#13 导出包不完整** — PORTING.md 通用模板，缺字体依赖/资源清单/LVGL 版本；manifest 缺构建标志。
- [ ] **#14 没有 URL 直接输入能力** — 只能手动 curl，无自动抓取+资源下载流程。

## P3 — 工程化改进

- [ ] **#15 字体加载应统一** — 抽出 `ui_font.h/c`，提供 `ui_font_get(size)` 接口，按 profile 决定 FreeType 或内置。
- [ ] **#16 流水线缺 clean / validate-only 子命令**
- [ ] **#17 没有增量构建** — sync 全量扫描，workspace 大了会慢。
- [ ] **#18 没有回归测试** — 生成器改动后无法自动检测是否破坏已有页面。
- [ ] **#19 Board profile 缺版本号和约束字段** — 最大字号、内存限制、颜色格式等。
