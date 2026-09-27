# Ground Control II — Simplified Chinese Patch (Steam)

Unofficial Simplified Chinese localization for **Ground Control II** (Steam, build 707334 / in-game v1.0.0.8).
**Zero game files modified** — everything is added next to `gcii.exe`; delete the files to uninstall.

> **Looking for the black-screen/artifacts fix first?**
> On Windows 11 the game won't even start without the DXVK fix:
> **[ground-control-2-win11-fix](https://github.com/DDDimension/ground-control-2-win11-fix)** — install that first, then this patch.

---

## How it works

The game loads `dinput.dll` from its own folder first. Our proxy `dinput.dll` forwards every
DirectInput call to the real input library and, at load time, injects `LG_Data\LG_GC2.dll` —
a self-contained localization engine (dictionary loading, binary search, text hooks) that
translates the UI through three inline hooks. A Chinese UI skin pack (`guiskins.ice`) completes the menu rendering.

All 23 loose original game files were verified byte-for-byte against Steam's official depot
manifest (SHA-1) — nothing is touched.

## Requirements

- Steam **Ground Control II**, build 707334 (in-game v1.0.0.8) — hooks are adapted to this exact binary
- Windows 11 users: install **[ground-control-2-win11-fix](https://github.com/DDDimension/ground-control-2-win11-fix)** first
- This patch also bundles the wireless-USB startup-crash fix and smooth wheel-zoom by
  [Vegasq/Ground-Control-2-wireless-fix](https://github.com/Vegasq/Ground-Control-2-wireless-fix) (MIT)

## Installation

Copy **everything in this repository** into the game folder (merge `LG_Data/`):

```
.../steamapps/common/Ground Control II/
├── dinput.dll              ← proxy (loads the engine below)
├── dinput_orig.dll          ← real input library (wireless-crash fix + wheel inertia)
├── dinput_inertia.ini       ← wheel-zoom config
├── LGCStringDict_01.txt     ← Simplified Chinese dictionary
└── LG_Data/
    ├── LG_GC2.dll           ← localization engine (adapted to Steam 1.0.0.8)
    ├── LGCStringDict_01.txt ← dictionary copy (original pack layout)
    └── guiskins.ice         ← Chinese UI skin pack
```

Launch the game — the main menu should be in Simplified Chinese.

## Uninstall

Delete the 7 files above. **No Steam file verification needed** — originals were never touched.

## Verify / troubleshoot

A log `gc2cn_proxy.log` appears in the game folder. A healthy load looks like:

```
proxy: DllMain PROCESS_ATTACH (v5 fullscreen, no DXVK tweaks)
proxy: dinput_orig.dll loaded OK
proxy: LG_Data\LG_GC2.dll injected OK
```

| Symptom | Check |
|---|---|
| No log at all | `dinput.dll` not in the game root (subfolder?), or blocked by antivirus |
| Log stops after line 1 | `dinput_orig.dll` missing/corrupt |
| Line 3 says FAILED | `LG_Data\LG_GC2.dll` wrong path or quarantined — whitelist it |
| Injected OK but still English | Game must be build 707334 (v1.0.0.8); older builds have different hook addresses |
| **Picture squeezed into top-left corner** | Not related to this patch — the savegame resolution field was corrupted; set it back to **1024×768** or run Steam's file verification |
| Artifacts / frozen mouse | Install the DXVK fix first (link above) |

## Known limitations

- Subtitled cutscenes (.bik) are not covered by the dictionary.
- Steam v1.0.0.8 only; a game update would invalidate the 3 hook addresses
  (see `docs/技术原理-方案C实施报告.md` for how they were located).
- The bundled `dinput_orig.dll` is the MIT-licensed patch by Vegasq — required by the
  proxy as the forwarding target, and it fixes the wireless-device startup crash for free.

## Provenance & licensing

See **[docs/来源与授权.md](docs/来源与授权.md)** for the full breakdown:

| Component | Origin | License |
|---|---|---|
| `dinput.dll` (proxy) + hook-address adaptation | this project | MIT (this repo) |
| `dinput_orig.dll` + `dinput_inertia.ini` | [Vegasq/Ground-Control-2-wireless-fix](https://github.com/Vegasq/Ground-Control-2-wireless-fix) v1.1 | MIT |
| `LG_GC2.dll` engine | fan repack `GC2OE.CHS.PATCH`, adapted (6 DWORDs) to Steam 1.0.0.8 by this project | fan localization |
| `LGCStringDict_01.txt` / `guiskins.ice` | extracted from the game's **official** Simplified Chinese data | © Massive Entertainment / Ubisoft — for owners of the game |

**For players who own the game only.** The dictionary/skin data belongs to the publisher;
do not bundle this with game files or ship it to people who don't own GC2.

## Credits

- [Vegasq/Ground-Control-2-wireless-fix](https://github.com/Vegasq/Ground-Control-2-wireless-fix) (MIT) — dinput patch (Mykola "Nick" Yakovliev)
- The fan localization pack authors — LG_GC2 engine and UI skins
- Massive Entertainment / Ubisoft — Ground Control II and its official Chinese localization data

---

## 中文说明

# Ground Control II — 简中汉化补丁（Steam 版）

适用 **Ground Control II**（Steam，build 707334 / 游戏内 v1.0.0.8）的简体中文界面汉化。
**不修改游戏任何原版文件** —— 所有文件都是新增，删掉即卸载。

### 原理

游戏会优先加载自己目录里的 `dinput.dll`。本补丁的代理 `dinput.dll` 把 DirectInput 调用
转发给真正的输入库，并在加载时注入 `LG_Data\LG_GC2.dll` —— 一个自包含的汉化引擎
（词典加载、二分查找、3 处文本 hook），配合中文 UI 皮肤包完成界面中文化。
全部 23 个原版散装文件已与 Steam 官方清单逐字节比对（SHA-1 全一致）。

### 前置条件

- Windows 11 用户**必须先装 DXVK 修复**：**[ground-control-2-win11-fix](https://github.com/DDDimension/ground-control-2-win11-fix)**，否则游戏本身就跑不起来
- 游戏必须是 build 707334（v1.0.0.8），hook 地址按此版本适配

### 安装

把本仓库**全部内容**复制到游戏根目录（`LG_Data` 文件夹合并），启动游戏即为简体中文。

### 卸载

删除上面 7 个文件即可，**不需要 Steam 校验完整性**。

### 排障

游戏目录会生成 `gc2cn_proxy.log`，正常为三行（见上方英文表格）。
**画面缩左上角 / 鼠标被困**与汉化无关 —— 是存档分辨率字段被改坏，把
`profiles/0/0.profile` 的分辨率改回 **1024×768** 或用 Steam 校验完整性即可。

### 已知限制

- 过场动画字幕不在词典范围内
- 仅适配 Steam v1.0.0.8；游戏若更新，3 处 hook 地址需重新定位（见 docs/技术原理-方案C实施报告.md）
- 附带 Vegasq 的 MIT 补丁：修无线 USB 启动崩溃 + 滚轮惯性缩放（代理依赖它，不可删）

### 授权

详见 **[docs/来源与授权.md](docs/来源与授权.md)**。本仓库自身内容按 MIT 分发（见 LICENSE）；
`dinput_orig.dll` 为 MIT 第三方补丁；词典/皮肤数据版权归原发行商，**仅供游戏正版玩家使用**，
不得与游戏本体打包分发。
