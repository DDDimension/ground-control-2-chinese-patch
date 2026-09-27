# GC2 简中汉化 —— 方案 C 实施报告（Steam 1.0.0.8）

> 日期：2026-09-26
> 结论：**✅ 方案 C 实施成功 —— 汉化模块已跑通，零改动 `gcii.exe`**

---

## 一、核心突破：不用写 DLL，直接改常量

此前的判断是「要自己写一个 32 位 DLL，装 3 处 detour，实现词典查表」。
**实测发现这条路走错了** —— 有个简单得多的解法。

### 1.1 决定性发现：LG_GC2.dll 是「自包含完整汉化引擎」

从 `LG_GC2.dll` 的字符串表提取到：

```
0x0000a328  '%s\\LGCStringDict_01.txt'      ← 它自己加载词典
0x0000af4f  ' LGCStringDict::Initialize_txt' ← 类方法名
0x0000b0f8  'LGCStringDict.cpp'             ← 源码文件名
0x0000afc0  'xfhsm_res_CHI_Start'  / 'CHI_End'   ← 词典段落标记
0x0000b10c  'xfhsm_res_ENG_Start'  / 'ENG_End'
0x0000b162  'xfhsm_res_No.%08d'             ← 条目编号格式
0x0000b1b8  'Search_half_and_half'          ← 二分查找
```

→ **词典加载、解析、二分查找、hook 引擎，全在 DLL 里。** 不需要重写任何逻辑。

### 1.2 真正的「版本绑定」只在一处：6 个偏移常量

`func_1600`（RVA `0x1600`）的反汇编：

```asm
0x10001713  push 0x1000ca08           ; exe 目录
0x1000171e  push 0x1000a328           ; "%s\\LGCStringDict_01.txt"
0x10001736  call 0x1000248d           ; sprintf
0x10001745  mov  ecx, 0x1000d630
0x1000174a  call 0x10008e60           ; LGCStringDict::Initialize()

; ---- hook1 ----
0x10001751  push 0x1c49e2   ← ★ 常量 @ file 0x1757
0x10001756  push 0x1c49dd   ← ★ 常量 @ file 0x1752
0x1000175b  push 0x1000a31c ("GCII.exe")
0x10001765  call 0x100018f0           ; 算 hook 地址
0x1000178c  call 0x10001ac0           ; 写内存补丁

; ---- hook2 ----  @ file 0x17ac / 0x17b1
; ---- hook3 ----  @ file 0x1800 / 0x1805
```

**每个 32 位常量在 DLL 中只出现 1 次**，就在 `.text` 里。

### 1.3 解法：改 6 个 DWORD（共 24 字节）

| 位置 (file) | 原值 (1.0.0.6) | 新值 (1.0.0.8) | 名称 |
|---|---|---|---|
| `0x1752` | `0x1c49dd` | `0x1d04fd` | hook1 patch |
| `0x1757` | `0x1c49e2` | `0x1d0502` | hook1 detour |
| `0x17b1` | `0x1ffee7` | `0x20fb07` | hook2 patch |
| `0x17ac` | `0x1ffeee` | `0x20fb0e` | hook2 detour |
| `0x1805` | `0x1fffb1` | `0x20fbd1` | hook3 patch |
| `0x1800` | `0x1fffb7` | `0x20fbd7` | hook3 detour |

`LG_GC2.dll` MD5：`9db15346...` → `11595962...`
大小 61,440 B 不变（原地改写，不增不删）。

---

## 二、6 处 hook 目标点的上下文核对（逐指令一致）

| hook | 汉化版 1.0.0.6 | Steam 1.0.0.8 | 指令序列 |
|---|---|---|---|
| hook1 patch | `0x1c49dd` | `0x1d04fd` | `83 c4 10 84 c0 75 20 eb 14 c7 05 xx…06` |
| hook1 detour | `0x1c49e2` | `0x1d0502` | `75 20` |
| hook2 patch | `0x1ffee7` | `0x20fb07` | `8b f0 b9 08 00 00 00 33 d2 f3 a6 75 02` |
| hook2 detour | `0x1ffeee` | `0x20fb0e` | `33 d2` |
| hook3 patch | `0x1fffb1` | `0x20fbd1` | `8a 06 33 c9 84 c0 0f 84 ba 00 00 00` |
| hook3 detour | `0x1fffb7` | `0x20fbd7` | `0f 84 ba 00 00 00` |

**唯一差异**是各版本自己的全局变量地址（`0x6f359c` vs `0x714f04`、`0x6aa158` vs `0x6c8a50`），不影响 hook。

---

## 三、hook 语义（本轮实证修正）

⚠️ **修正**：此前报告称 hook2/hook3 是"字符串查表替换"，**不准确**。实际分工：

| hook | 真实作用 | 证据 |
|---|---|---|
| **hook1**<br>`ResDataDecompression01` | **资源路径劫持**<br>`guiskins.ice` → `LG_Data\guiskins.ice` | detour1 反汇编：`lstrcmpiA(path,"guiskins.ice")==0` → `wsprintfA(buf,"%s\\LG_Data\\guiskins.ice",…)` |
| **hook2**<br>`String01` | **词典查表回填** | detour2：`[esi+0x20]` → 用 `0x1000d630`（LGCStringDict 实例）查 → `[esi+0x20] = [eax+4]` |
| **hook3**<br>`String02` | **文本换行排版**<br>`\` → `\`+`\n` | detour3：遍历字符串，遇 `0x5C` 插 `0x0A`；再调渲染 API |

**hook2/hook3 的 patch 点落在同一个「8 字节魔数比较」循环里**：

```asm
mov  edi, 0x6c8a50        ; 指向 8 字节常量
mov  esi, eax             ; 待处理字符串
mov  ecx, 8               ; 比 8 字节
xor  edx, edx
repe cmpsb                ; 比较
jne  ...
mov  byte ptr [eax], dl   ; 相等则截断
```

实测该 8 字节常量 = **`"<empty>\0"`**（即字符串 `"<empty>"`），
两个版本一致 → 这是**空文本占位标记**，hook 在此拦截并从词典填充译文。

---

## 四、部署方案（零改动 `gcii.exe`）

### 4.1 部署结构

```
Ground Control II\
  gcii.exe                     ← 原版，一个字节不动 (md5 4f4f6d26…)
  dinput.dll                   ← 新增(替换)：代理注入器 (122,368 B)
  dinput_orig.dll              ← 新增：原 dinput.dll 的副本 (86,528 B)，供代理转发
  LGCStringDict_01.txt         ← 新增：词典 (609,744 B)  ★ 必须放这
  LG_Data\
    LG_GC2.dll                 ← 新增：改过 6 个偏移的 Steam 专用版
    LGCStringDict_01.txt       ← 新增：词典冗余
    guiskins.ice               ← 新增：中文皮肤
```

### 4.2 为什么词典必须放根目录（不是 `LG_Data\`）

`func_1600` 用 **`GetModuleFileNameA(NULL, …)`**（取**宿主 exe** 路径，不是 DLL 路径）
→ 取 exe 所在目录 → 拼 `%s\LGCStringDict_01.txt`。

→ **词典路径 = `gcii.exe` 同目录**。（汉化包里放 `LG_Data\` 是因为它走启动器改工作目录；
我们走 DLL 代理注入，所以必须放根目录。）

### 4.3 注入链

```
gcii.exe 启动
  → 加载根目录 dinput.dll（我们的代理）
  → DllMain(PROCESS_ATTACH)
      → LoadLibraryW("dinput_orig.dll")        ← 恢复原功能的 DirectInput
      → LoadLibraryW("LG_Data\LG_GC2.dll")     ← 装 hook
          → DllMain → func_1600(DLL_PROCESS_ATTACH)
              → 加载 LGCStringDict_01.txt
              → 在 0x1d04fd/0x20fb07/0x20fbd1 三处写补丁
```
（`GCII.exe` 校验：`GetModuleHandleA` 大小写不敏感 → `gcii.exe` 可通过）

---

## 五、实测结果

```
[2026-09-26 22:42:26] proxy: DllMain PROCESS_ATTACH
[2026-09-26 22:42:26] proxy: dinput_orig.dll loaded OK
[2026-09-26 22:42:26] proxy: LG_Data\LG_GC2.dll injected OK
```

| 检查项 | 结果 |
|---|---|
| 注入成功 | ✅ 三行日志全绿 |
| `LG_Try_Error.log` | ✅ 不存在（无报错） |
| 窗口标题 | ✅ `Ground Control II`（正常，非 `Error`） |
| `gcii.exe` md5 | ✅ `4f4f6d262392c2ead9ecc3ea4b644e6d`（原版未变） |
| 中文显示 | ⏳ **待肉眼确认** |

### 5.1 附带发现：副本测试路线不可行

尝试在 `D:\GC2_CN_TEST\` 建副本验证 → **游戏报 `Error` 无法启动**。
根因：`installscript.vdf` 显示游戏需读注册表

```
HKLM\SOFTWARE\Massive Entertainment AB\Ground Control II
    CDKEY = "LUD3-GYF2-GUP5-MUD2-9585"
```

**按绝对路径绑定** → 副本路径无效。故验证只能在 Steam 原目录做。

### 5.2 附带发现：「局部窗口 + 鼠标不能动」与汉化无关

对照实验：**撤除全部汉化文件后**（纯原版环境），症状**依然存在**
（`Title='Ground Control II'`，同表现）。

→ 这是游戏目录内已有 `dxvk.conf` / `dinput_inertia.ini` 环境的**既有问题**，
**不是汉化引入的**。`dxvk.conf` 注释里保留了完整的历史排查记录，
最新假设指向 `d3d9.modeCountCompatibility`（"画面缩成一块"的元凶）。

---

## 六、工作量总结（对比三轮预估）

| 轮次 | 预估 | 实际 |
|---|---|---|
| 初版 | 单人 数天~两周（需逆向整个 exe） | — |
| 定位后 | 1~3 天（写 DLL + 3 hook） | — |
| **本轮实测** | — | **约 2 小时**（改 24 字节 + 组装部署包 + 实测） |

**关键认知修正**：不需要写 DLL。LG_GC2.dll 自带全部逻辑，
**版本差异仅体现在 6 个偏移常量上** → 改常量即可。

---

## 七、产物清单

| 文件 | 说明 |
|---|---|
| `steam_module\LG_GC2.dll` | ★ Steam 1.0.0.8 专用汉化 DLL（改过 6 个偏移） |
| `deploy_steam\` | ★ 完整部署包（6 个文件） |
| `gc2cn_switch.py` | 汉化开关脚本（`on` / `off` / `status`） |
| `game_dir_state.json` | 游戏目录原始状态快照（用于校验/回滚） |
| `planC_step18~27_*.py` | 本轮的定位/构建/部署脚本 |

### 一键卸载

```bat
python gc2cn_switch.py off
```
→ 恢复原版 `dinput.dll`，移除全部新增文件，`gcii.exe` 始终未动。

---

## 八、待办

- ⏳ **肉眼确认界面中文**（本轮核心验证目标）
- ⬜ （可选）修「局部窗口 + 鼠标不能动」——属 DXVK 调优，与汉化无关
- ⬜ （可选）检查中文排版：字宽、换行、是否有方块
