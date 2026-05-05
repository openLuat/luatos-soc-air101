# Project Guidelines — LuatOS Air101/Air103/Air601

> 本仓库已于 2024-08-15 封存，不再更新维护。仅修复关键Bug。

## 项目概览

嵌入式 C 固件，面向平头哥 C-Sky XT804 内核 SoC（Air101/Air103/Air601/W800/W801/W806），集成 LuatOS Lua 运行时。

## 构建

- **构建系统**: xmake (≥ 2.6.3)
- **工具链**: C-Sky ELF cross-compiler (`csky-elfabiv2`)，xmake 自动下载
- **编译命令**: `xmake -y`
- **输出**: `build/out/` 目录下的 `.soc` / `.fls` 文件
- **LuatOS 依赖**: 需要在同级目录存在 `../LuatOS/` 仓库（https://gitee.com/openLuat/LuatOS）
- **目标切换**: 修改 `app/port/luat_conf_bsp.h` 中的板级宏（`AIR101` / `AIR103` / `AIR601`）
- 详见 [编译说明.md](../编译说明.md)

## 架构

```
app/port/          — HAL 驱动适配层（命名: luat_<外设>_air101.c）
app/network/       — WLAN/网络集成
app/custom/        — 用户自定义代码入口
platform/          — 底层平台代码（arch/drivers/sys）
src/               — 上层子系统（os/network/bt/app）
include/           — 公共头文件
ld/                — 链接脚本（按芯片: AIR101.ld, AIR103.ld, AIR601.ld）
partition/         — Flash 分区表（CSV 格式，含注释头定义 flash_base/flash_size/ram_end）
soc_tools/         — 芯片元数据 JSON + 烧录工具
cloudbuild/        — 云编译元数据
```

## 代码约定

- **语言标准**: C99（`-std=gnu99`）
- **优化等级**: `-Os`（最小体积优先）
- **编译器标志**: `-mcpu=ck804ef -mhard-float -fdata-sections -ffunction-sections`
- **端口文件命名**: `luat_<模块名>_air101.c`（如 `luat_uart_air101.c`）
- **BSP 配置**: 通过 `luat_conf_bsp.h` 宏开关控制外设/组件的编译
- **Werror 启用项**: `unused-value`, `array-bounds`, `return-type`, `overflow`, `empty-body`, `old-style-declaration`, `implicit-function-declaration`
- 使用 `__LUATOS__` 和 `__USER_CODE__` 宏区分 LuatOS 模式与纯 C SDK 模式
- mbedTLS 配置见 `app/port/mbedtls_config_air101.h`

## 分区表格式

```csv
# flash_base = 0x08000000
# flash_size = 2048K
# ram_end = 0x20028000
# Name,   Type,   Offset,     Size
secboot,  boot,   0x000000,   64K
app,      app,    0x011000,   1788K
```

`buildx.lua` 解析分区 CSV 文件，支持十六进制偏移和带单位的大小（`64K`, `1M`）。

## 部署

- `deploy.py`: 从 `luat_conf_bsp.h` 读取 `LUAT_BSP_VERSION`，将 `.soc` 文件通过 SSH 部署
- 量产烧录: 见 [tools/量产脚本工具.md](../tools/量产脚本工具.md)

## 常见陷阱

- LuatOS 主仓库必须位于 `../LuatOS/`，否则编译失败
- Air101 仅 2MB Flash / 288KB RAM，注意资源预算
- `luat_conf_bsp.h` 是日常开发配置；`luat_conf_bsp_air103.h` / `luat_conf_bsp_air601.h` 仅用于发布构建覆盖
- 链接脚本与分区表必须匹配，修改分区后需同步更新对应 `.ld` 文件
