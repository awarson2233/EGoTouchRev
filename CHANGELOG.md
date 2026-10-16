# Changelog

## [1.4.0](https://github.com/awarson2233/EGoTouchRev/compare/v1.3.0...v1.4.0) (2026-10-16)


### ✨ 新功能

* **tools:** 新增片上解算验证工具 ChipHidProbe / ChipHidProbeB，ghidra-mcp 配置跟随新路径 ([489552c](https://github.com/awarson2233/EGoTouchRev/commit/489552c4c640a6a9ca699d581d98e48f2d33dcb9))


### 🐛 修复

* IDLE 唤醒后首次触摸被吞与唤醒后手写笔失效，合入 TSACore 向量化优化 ([c61f64d](https://github.com/awarson2233/EGoTouchRev/commit/c61f64d7b3cc91800023178937acd3fdd423e32f))
* **pen:** 笔事件通道重开时把连接状态复位成未知，避免唤醒查询丢失后笔整段失效 ([f36ee02](https://github.com/awarson2233/EGoTouchRev/commit/f36ee02de1202c0b776f3a768aa8b6eeacdc0b75))
* **tsacore:** ctrl 控制字的复位位与充电器位对齐 ApDaemon，修复 IDLE 唤醒后首次触摸被吞 ([7ed7de3](https://github.com/awarson2233/EGoTouchRev/commit/7ed7de388b756a1b0221cd2e935caef9a3ab7a55))


### ⚡ 性能

* **tsacore:** 基线复位、SHB、坏点、CMF 与 Property 的逐格判定改成可向量化的写法 ([8e2984c](https://github.com/awarson2233/EGoTouchRev/commit/8e2984cd40937a6a17979721e167bcf96c14376d))
* **tsacore:** 轮廓插值的备忘表改成定长静态数组 ([d9cf324](https://github.com/awarson2233/EGoTouchRev/commit/d9cf32487cdadf655739e8ffd95fa5a68beb2b0c))
* **tsacore:** 适配层的死区与增益循环改成可向量化的写法 ([d22ecb3](https://github.com/awarson2233/EGoTouchRev/commit/d22ecb3aab20fdc028941109e0d2f4a91c3180ce))


### ♻️ 重构

* **tsacore:** 宿主接口改成类型化的 TsaCore.h，删掉 shim 与核里的死代码 ([8365e67](https://github.com/awarson2233/EGoTouchRev/commit/8365e671028a7d1593d40e0c5a33c5057b52d9e2))

## [1.3.0](https://github.com/awarson2233/EGoTouchRev/compare/v1.2.0...v1.3.0) (2026-10-15)


### ✨ 新功能

* **tsacore:** 按 OTP 面板 id 自动选用面板配置 ([33b7e13](https://github.com/awarson2233/EGoTouchRev/commit/33b7e13a17ecbc2b3c393d0d66f452e9e22371a8))
* **tsacore:** 按 OTP 面板 id 自动选用面板配置 ([1785220](https://github.com/awarson2233/EGoTouchRev/commit/1785220dabc34f1deb65306ea14451baa1e739df))


### 🐛 修复

* **recorder:** 日志文件名写成实际的 EGoTouchRecorder.txt ([530a4cf](https://github.com/awarson2233/EGoTouchRev/commit/530a4cf992c1c1387be8201a10b11bfdd389817f))

## [1.2.0](https://github.com/awarson2233/EGoTouchRev/compare/v1.1.2...v1.2.0) (2026-10-15)


### ✨ 新功能

* **device:** read the panel project id from the display driver OTP ([64a0dd1](https://github.com/awarson2233/EGoTouchRev/commit/64a0dd13c962e6b70aec455b0cac03991e3edcc9))
* **runtime:** route pen status, BT pressure and master-less frames to the TSACore solver ([2489d55](https://github.com/awarson2233/EGoTouchRev/commit/2489d55d58930f49eec891ba8eadb9cef1395f8b))
* **tools:** EGoTouchRecorder ([c62aea3](https://github.com/awarson2233/EGoTouchRev/commit/c62aea38100639151908bf1cfd99892354a4c52d))
* **touch:** feed stylus frame state, pen report and pen session into TSACore ([bee1158](https://github.com/awarson2233/EGoTouchRev/commit/bee1158dc183d88b78d9921ab063f5856883fbd2))
* **touch:** plug TSACore into the runtime; align AFE read and panel power handling with the stock driver ([8345c26](https://github.com/awarson2233/EGoTouchRev/commit/8345c262addac1938a1cd3de3eb3b806b521ecac))
* **touch:** replicate the full TSACore finger pipeline ([e937963](https://github.com/awarson2233/EGoTouchRev/commit/e937963b81a015e0c47c82f2e97478d57a258700))
* **TouchSolver/raw:** expose the full algorithm state for replay comparison ([a18f293](https://github.com/awarson2233/EGoTouchRev/commit/a18f2939d7703f004e96f5892ae06f62f555020c))
* **TouchSolver/raw:** reconstruct and compile 73 TSACore algorithm C sources cleanly under Clang -O3 ([6f69f21](https://github.com/awarson2233/EGoTouchRev/commit/6f69f2121ad28e95c679e98e00834762accc23f8))
* **TouchSolver/raw:** restore the TSA_ASAProcess shell behind a host stylus report bridge ([1134833](https://github.com/awarson2233/EGoTouchRev/commit/11348335533d15447f2a8ca248622ca5ed980aff))
* **TouchSolver:** add decompiled TSACore finger pipeline, analysis docs, and C++ reconstruction headers ([d6886df](https://github.com/awarson2233/EGoTouchRev/commit/d6886df5e71f79271344aedead7d56b80062e68b))
* **TsaRawReplay:** compare the whole TSACore state against a reference build ([16a576e](https://github.com/awarson2233/EGoTouchRev/commit/16a576ec563fab72687b05c9bf322e214065c4de))
* **TsaRawReplay:** print tilt intermediates, IIR speed/coefficient and recorded BT pressure on STY lines ([2dd25d9](https://github.com/awarson2233/EGoTouchRev/commit/2dd25d980160be805a0b44e8de2b017e207c5ba1))
* **TsaRawReplay:** replay pen paths with a synthetic pen or the real StylusPipeline ([ccb410f](https://github.com/awarson2233/EGoTouchRev/commit/ccb410f72e0c8584763c13b955328179091c297b))


### 🐛 修复

* **stylus:** restore the ASA IIR coefficients and interpolation span ([b59f67d](https://github.com/awarson2233/EGoTouchRev/commit/b59f67d0b88d02395cab8757822a4d41fea64995))
* **stylus:** take the tilt length and ratio table from the ASA flash parameters ([2e4ba7e](https://github.com/awarson2233/EGoTouchRev/commit/2e4ba7e72df1da5cf30a2d011921c40a54934dec))
* **touch:** hand the stylus tilt to TSACore without negating it ([8055285](https://github.com/awarson2233/EGoTouchRev/commit/80552858098fad414345ddc1824f55e0e5385754))
* **TouchSolver/raw:** separate the two g_lastStylusTimeStamp statics and correct the AsaOut status offset ([178627b](https://github.com/awarson2233/EGoTouchRev/commit/178627b6ec1b14731113dfd56820d2bf762f80e7))
* **tsacore raw:** prune the CMF 2D/PCA filter ([76b381d](https://github.com/awarson2233/EGoTouchRev/commit/76b381d452294ce8e54ba79ee12fe6bed2ed9fc3))
* **tsacore raw:** read the slot-1 tuning block behind pDim1PitchMap with 8-byte elements ([4b8c027](https://github.com/awarson2233/EGoTouchRev/commit/4b8c027f5aac35fa3aa5ee48d43eb15f4c141482))
* **tsacore raw:** read the stylus XY correction constants as floats ([8077ad0](https://github.com/awarson2233/EGoTouchRev/commit/8077ad068d37a183c49e2024a533622acb822aa8))
* **tsacore raw:** restore the float math of the TZ contour interpolation ([a6fcd8b](https://github.com/awarson2233/EGoTouchRev/commit/a6fcd8bfa769ee5b68ce87cd960d36bca70b7b9c))
* **tsacore raw:** round the 3/4 lower bounds of the threshold entries toward zero ([6121110](https://github.com/awarson2233/EGoTouchRev/commit/612111002a071a1c62c90719fb3c46155402bdee))
* **tsacore raw:** write the touch-mode debounce entries as 16-bit ([afd0ae2](https://github.com/awarson2233/EGoTouchRev/commit/afd0ae2cdc0bf90ce4be3c02184d5e034483d274))
* **tsacore:** read the AFE scan-rate parameters from the right FlashPrmt offsets ([0b01291](https://github.com/awarson2233/EGoTouchRev/commit/0b01291477367ef68ba2c6b06ef2272185e930f2))
* **TsaRawReplay:** keep the recording probe of a coverage run from writing default.profraw into the repo ([9d15519](https://github.com/awarson2233/EGoTouchRev/commit/9d155191df40a84ed2483d1d917719547b5b41f1))
* **TsaRawReplay:** run the stylus scenario for every recording and accept comma-separated recording lists ([54783ef](https://github.com/awarson2233/EGoTouchRev/commit/54783ef7cdfde9893e4df4d97383bfda5283774f))


### ⚡ 性能

* **tools:** cache reference replays and run comparisons in parallel ([cfb5294](https://github.com/awarson2233/EGoTouchRev/commit/cfb52949115988799767930df31869ba9b9571a4))
* **tools:** compare outputs only, on three scenarios by default ([252e955](https://github.com/awarson2233/EGoTouchRev/commit/252e9557c7634dcf589c7ce5f579c313d50a398a))
* **tsacore:** batch CMF clustering runs; unconditional per-cell updates ([2608414](https://github.com/awarson2233/EGoTouchRev/commit/26084143955ce28d5aad3ecfe41c58a228a0739d))
* **tsacore:** branchless flat mask and local cluster state in CMF NoiseCal ([b546d4c](https://github.com/awarson2233/EGoTouchRev/commit/b546d4c564b129b1db62c9c638821b3c656afbf0))
* **tsacore:** compute only what is read in raw2raw, contour, Self and Property ([f4a3369](https://github.com/awarson2233/EGoTouchRev/commit/f4a33699ebda8fcdebc6b80a8ea8020f07bc69dc))
* **tsacore:** drop write-only TZ morphology and CMF side outputs ([d08f486](https://github.com/awarson2233/EGoTouchRev/commit/d08f48658b12151a6daaea9253124c92e5a92b4e))
* **tsacore:** hoist flash parameters, screen peak rows, memoize contour samples ([bb1f06d](https://github.com/awarson2233/EGoTouchRev/commit/bb1f06db6f198f464e610ec225cb5898db6ce52a))
* **tsacore:** row-level contour memo, narrower Property scans, chunked lookups ([5e2da20](https://github.com/awarson2233/EGoTouchRev/commit/5e2da2035bcac5089aa417ad3ec7f287bd597698))
* **tsacore:** screen sensor-broken Dif checks per 16-cell chunk ([f6632b5](https://github.com/awarson2233/EGoTouchRev/commit/f6632b5d6f723d2c8feef3ebf4bed0ef55ccf797))
* **tsacore:** step profiling and a branchless Self::Process ([91fa36e](https://github.com/awarson2233/EGoTouchRev/commit/91fa36e3b115af37c1d1a1ebfda0c30674c8a2e4))
* **tsacore:** time the whole frame without per-step timer overhead ([e571d39](https://github.com/awarson2233/EGoTouchRev/commit/e571d399044cd9b707411024c9782715d24cb36e))
* **tsacore:** vectorizable per-cell timers in TSABuffer; bench the Release build ([412148f](https://github.com/awarson2233/EGoTouchRev/commit/412148f1451ee600f2ed4d0397061b2ac6fec4f9))


### ♻️ 重构

* make TSACore the only finger solver; drop EGoTouchApp and the old tests ([3bc8d07](https://github.com/awarson2233/EGoTouchRev/commit/3bc8d07fe5409dbea7e58be58b83d4ce6e90ed1b))
* **tsacore:** drop specialControlFlags and the Raw naming ([6de9e97](https://github.com/awarson2233/EGoTouchRev/commit/6de9e975ac44be9e7e7b5d6b8293992b4b86a018))
* **tsacore:** name the fieldXXX members, drop write-only ones ([187ecaf](https://github.com/awarson2233/EGoTouchRev/commit/187ecaffafe51a9f3621b2ee8eb30f8ae6b9edeb))
* **tsacore:** replace the memory image with C++ state objects ([230e4f4](https://github.com/awarson2233/EGoTouchRev/commit/230e4f4bb49afc74391f6d9acdc76c50d6d18760))
* **tsacore:** retire the raw C layer ([a200784](https://github.com/awarson2233/EGoTouchRev/commit/a2007841b9d8439067d3126de2cfd181c673aba9))
* **tsacore:** rewrite anti-touch rejection (AntiTouch) in C++ ([e281312](https://github.com/awarson2233/EGoTouchRev/commit/e2813129dba4e7cc31b7579dbce050af323cee6a))
* **tsacore:** rewrite MISC and the state registry in C++ ([7d10ea9](https://github.com/awarson2233/EGoTouchRev/commit/7d10ea93f13c084cbc0b2bb97e50597411df303b))
* **tsacore:** rewrite peak detection and the peak tables in C++ ([1943f20](https://github.com/awarson2233/EGoTouchRev/commit/1943f20e3795777f84860055c1bbfcac1e2a98cd))
* **tsacore:** rewrite stylus palm rejection (StylusAft) in C++ ([8e3603e](https://github.com/awarson2233/EGoTouchRev/commit/8e3603e6f4196c50cfd2c2339a3130ef813ffb36))
* **tsacore:** rewrite the AFE interface and scan scheduling in C++ ([6f7e06e](https://github.com/awarson2233/EGoTouchRev/commit/6f7e06ee4f1e99bce3746b1bda8efb4fceaf7fc3))
* **tsacore:** rewrite the ASA stylus bridge in C++ ([1c2ce26](https://github.com/awarson2233/EGoTouchRev/commit/1c2ce26f08069e002d29b398cd95feb67b99f737))
* **tsacore:** rewrite the baseline in C++ ([28709d4](https://github.com/awarson2233/EGoTouchRev/commit/28709d42ec33c532ba5f789ccc7b2a0fa7dc1c8c))
* **tsacore:** rewrite the binary-image morphology (Image) in C++ ([eed20b5](https://github.com/awarson2233/EGoTouchRev/commit/eed20b50b22a3406c30cedb83e62947d3d004d73))
* **tsacore:** rewrite the centroid and edge correction (CTD) in C++ ([9d0d267](https://github.com/awarson2233/EGoTouchRev/commit/9d0d267ea65d379d314adbe8637f2ed1d420a859))
* **tsacore:** rewrite the common mode filter in C++ ([5f8fa75](https://github.com/awarson2233/EGoTouchRev/commit/5f8fa759ca10d972d52a3d0eb08de46a49d5cb6f))
* **tsacore:** rewrite the coordinate filter and touch mode in C++ ([6ae539c](https://github.com/awarson2233/EGoTouchRev/commit/6ae539c4913d181e11b3bc442087a33d60f6bfa9))
* **tsacore:** rewrite the edge recovery (ER) in C++ ([ae85818](https://github.com/awarson2233/EGoTouchRev/commit/ae85818f3225fd78938cbb796a0e1e786779284f))
* **tsacore:** rewrite the exception state machine (Exception, FIException) in C++ ([328399c](https://github.com/awarson2233/EGoTouchRev/commit/328399c5932ec99b238c07f52a23b8f63c306fc9))
* **tsacore:** rewrite the grid buffers and module allocation in C++ ([1b6fb27](https://github.com/awarson2233/EGoTouchRev/commit/1b6fb271e4d7d174cedcbd83459533042536b37f))
* **tsacore:** rewrite the grip filter (GripFilter) in C++ ([3058259](https://github.com/awarson2233/EGoTouchRev/commit/305825933c9f266e9edf90a6a8cb561d1310d446))
* **tsacore:** rewrite the grip hand gesture (HandGesture) in C++ ([f14727f](https://github.com/awarson2233/EGoTouchRev/commit/f14727f335e4d3da9f84102e90f2986d83cf552c))
* **tsacore:** rewrite the hardware analyzer in C++ ([269d031](https://github.com/awarson2233/EGoTouchRev/commit/269d0313b8209ff5ee70b8ea3fd9f0532c650384))
* **tsacore:** rewrite the hot-zone touch adjustment (HotZoneAdjust) in C++ ([bd38b3a](https://github.com/awarson2233/EGoTouchRev/commit/bd38b3afa49042e3d12aa8bc4cb1444f8e78b385))
* **tsacore:** rewrite the inner-corner signal ratio (TouchZone) in C++ ([17c0b0f](https://github.com/awarson2233/EGoTouchRev/commit/17c0b0f7c79e5ad8b895682c260d0c2827cf1332))
* **tsacore:** rewrite the parameter tables in C++ ([5c06a86](https://github.com/awarson2233/EGoTouchRev/commit/5c06a86c28a98f6eec2b6e903c8b22a8d9dcc728))
* **tsacore:** rewrite the per-cell zone queries (Sensor) in C++ ([e139b94](https://github.com/awarson2233/EGoTouchRev/commit/e139b94bb82441dc80b0f9cba7e0f8ca7e24d4f5))
* **tsacore:** rewrite the per-frame property statistics in C++ ([ba69dd5](https://github.com/awarson2233/EGoTouchRev/commit/ba69dd526818d68bc9972a1b79218b040e145605))
* **tsacore:** rewrite the RF noise detection (RFNoise) in C++ ([5d54389](https://github.com/awarson2233/EGoTouchRev/commit/5d54389e1fc462e07de746701dfa5327673d82da))
* **tsacore:** rewrite the self data, grid IIR, rawdata and line filters in C++ ([9bedc1e](https://github.com/awarson2233/EGoTouchRev/commit/9bedc1eb98b17f58f2e313a78ef5352dfa264758))
* **tsacore:** rewrite the sign-split cell list (PosnSpecial) in C++ ([72975ef](https://github.com/awarson2233/EGoTouchRev/commit/72975efaf7109c92e58b303c814cfb796489fa7a))
* **tsacore:** rewrite the static state, output, MS idle and data switch in C++ ([095a53f](https://github.com/awarson2233/EGoTouchRev/commit/095a53f16d14c4296e72c22446ff20d977bab8f6))
* **tsacore:** rewrite the sub touch zones (SubTZ) in C++ ([c95992c](https://github.com/awarson2233/EGoTouchRev/commit/c95992c495ff80a7f1f07345600b04ab872e2aa3))
* **tsacore:** rewrite the touch action (TouchAction) in C++ ([b36bcc7](https://github.com/awarson2233/EGoTouchRev/commit/b36bcc742b0971f1892b493258fdaa6e06e546df))
* **tsacore:** rewrite the touch event state machine (TE) in C++ ([bf19901](https://github.com/awarson2233/EGoTouchRev/commit/bf19901dd6daf4705e8ec002e04ff594758c5157))
* **tsacore:** rewrite the touch ID tracking (IDT) in C++ ([9a05047](https://github.com/awarson2233/EGoTouchRev/commit/9a0504732b6be0b269bab4650d3125ba1dfde609))
* **tsacore:** rewrite the touch report decision (TouchReport) in C++ ([60df064](https://github.com/awarson2233/EGoTouchRev/commit/60df064f7920ebc5fa8772c947068200cb0fefa2))
* **tsacore:** rewrite the touch size (TS) in C++ ([b88bf4a](https://github.com/awarson2233/EGoTouchRev/commit/b88bf4aa01b70514edd17f80a27066a545439c90))
* **tsacore:** rewrite the touch tables (TouchData / PrevTouchData) in C++ ([f700db7](https://github.com/awarson2233/EGoTouchRev/commit/f700db7cc56dee58b28f62f5d5d2d53940afe69f))
* **tsacore:** rewrite the touch thresholds (TouchThold) in C++ ([56c9407](https://github.com/awarson2233/EGoTouchRev/commit/56c9407a8aad721b193997bc1ba040195ffa5427))
* **tsacore:** rewrite the touch zones (TZ) in C++ ([5377c28](https://github.com/awarson2233/EGoTouchRev/commit/5377c28af9cdd014594e4d9a05c7cf8f21652f97))
* **tsacore:** rewrite the touch-down / lift-off debounce (DB) in C++ ([b742d55](https://github.com/awarson2233/EGoTouchRev/commit/b742d553db4585d78a9b0aa5353140d2d28af8ab))
* **tsacore:** rewrite the TP sensor correction in C++ ([b63794c](https://github.com/awarson2233/EGoTouchRev/commit/b63794c1e4c3e77d4f64b89fcf4ced9108a93b5c))
* **tsacore:** rewrite the TSA pipeline, scenarios and report API in C++ ([75000ac](https://github.com/awarson2233/EGoTouchRev/commit/75000acb4da5d86e558f75fbdff6833db4378ee4))
* **tsacore:** rewrite thenar / side palm detection (Thenar) in C++ ([5934c60](https://github.com/awarson2233/EGoTouchRev/commit/5934c6082d56ac14f5dccb248a7b3978dd9af719))
* **tsacore:** start the C++ rewrite with the math and IIR helpers ([ee2a6e7](https://github.com/awarson2233/EGoTouchRev/commit/ee2a6e7141126a27169e34ac539ba9ba911d85a8))


### 📝 文档

* **finger_algorithms:** add the TSACore raw integration plan and status ([d311d35](https://github.com/awarson2233/EGoTouchRev/commit/d311d3526422e56a811e068b8013062f6dd2979a))
* **finger_algorithms:** describe the stylus state seam into TSACore ([e0e9fe4](https://github.com/awarson2233/EGoTouchRev/commit/e0e9fe474af7940a7a0f5f6500e01aacf686cf35))
* **tsacore:** describe the reference comparison and the C++ rewrite rules ([5b3beaf](https://github.com/awarson2233/EGoTouchRev/commit/5b3beafbd2b62310de353907a21c0c938077504f))
* **tsacore:** performance, profiling and benchmark section ([f914496](https://github.com/awarson2233/EGoTouchRev/commit/f914496f79bc58d3d6e8bc2fc8bcdcc6081cc8a3))
