'use strict';

/*
 * TSACore.dll runtime tracer for ARM64EC environments
 *
 * 设计目标：
 *   1. 只 hook「导出函数」，避免直接按内部代码 RVA 下钩子。
 *   2. 变量抓取通过 .data/.bss 的静态偏移读取，不对内部函数做 call。
 *   3. 输出 JSONL 到文件，方便按 frameSequence 做离线分析。
 *   4. 运行时 attach，等待 TSACore.dll 加载后自动安装 hook。
 *
 * 关键说明：
 *   - hook 只走 Module.findExportByName()。
 *   - 变量读取仍然基于模块基址 + 数据 RVA；这不依赖代码翻译 thunk。
 *   - 若你想“绝对 export-only”，可把 includeRawGlobals 改成 false。
 *
 * 用法：
 *   1. 修改 CONFIG.logPath 为绝对路径
 *   2. frida -p <PID> -l scripts/tsacore_frida_trace.js
 *
 * 建议抓取场景：
 *   - 官方参数关闭：做一次慢速下笔 / 快速抬笔
 *   - 官方参数开启：做完全相同的动作
 *   - 对比同一类动作下的 seq / tx1 / tx2 / pressure / mode 变化
 */

const DEFAULT_CONFIG = {
  moduleName: 'TSACore.dll',

  // 强烈建议改成绝对路径，例如：
  // logPath: 'D:\\\\trace\\\\tsacore_trace.jsonl',
  logPath: 'tsacore_trace.jsonl',

  pollIntervalMs: 500,
  flushEveryLines: 20,
  periodicFlushMs: 1000,

  // 仅 leave 抓状态，避免热路径日志过重
  snapshotOnEnter: false,
  snapshotOnLeave: true,

  // 是否读取内部全局变量（data RVA）。
  // 仅影响“变量抓取”，不影响 hook；hook 始终只走导出函数。
  includeRawGlobals: true,

  // 仅对少量模式切换函数抓回溯，默认关闭。
  captureBacktrace: false,
  backtraceExports: [
    'EnterNoPressInkMode',
    'ExitNoPressInkMode',
    'EnterInkMode',
    'ExitInkMode',
    'EnterActiveStylusMode',
    'ExitActiveStylusMode'
  ],

  // 只 hook 导出函数；若某版本未导出，会自动记 skip。
  enabledExports: [
    'TSA_ASAProcess',
    'NoPressInkProcess',
    'HPP3_PressureProcess',
    'HPP3_PostPressureProcess',
    'HPP3_FakePressureDecreaseProcess',
    'ASAStaticStatusPreProcess',
    'HPP3_ASAStaticStatusPostProcess',
    'EnterNoPressInkMode',
    'ExitNoPressInkMode',
    'EnterInkMode',
    'ExitInkMode',
    'EnterActiveStylusMode',
    'ExitActiveStylusMode'
  ]
};

const RUNTIME_CONFIG = (typeof globalThis !== 'undefined' && globalThis.__TSACORE_TRACE_CONFIG__)
  ? globalThis.__TSACORE_TRACE_CONFIG__
  : {};

const CONFIG = Object.assign({}, DEFAULT_CONFIG, RUNTIME_CONFIG);
CONFIG.backtraceExports = Array.isArray(CONFIG.backtraceExports) ? CONFIG.backtraceExports.slice() : [];
CONFIG.enabledExports = Array.isArray(CONFIG.enabledExports) ? CONFIG.enabledExports.slice() : [];
CONFIG.resolvedExports = Array.isArray(CONFIG.resolvedExports) ? CONFIG.resolvedExports.slice() : [];
CONFIG.resolvedGetterExports = Array.isArray(CONFIG.resolvedGetterExports) ? CONFIG.resolvedGetterExports.slice() : [];
CONFIG.dumpGrids = !!CONFIG.dumpGrids;

// 这些偏移来自当前仓库的逆向文档，基于 TSACore.dll image base 0x180000000。
// 这里只读数据，不对内部函数做 hook。
const RAW_GLOBALS = {
  frameSequence:              { off: 0x00231a14, type: 'u32' },
  frameTimestampMs:           { off: 0x00231a20, type: 'u64' },

  tx1_signal_dim1:            { off: 0x00231160, type: 'u16' },
  tx1_signal_dim2:            { off: 0x00231162, type: 'u16' },
  tx1_signal_combined:        { off: 0x00231164, type: 'u16' },
  tx2_signal_dim1:            { off: 0x00231166, type: 'u16' },
  tx2_signal_dim2:            { off: 0x00231168, type: 'u16' },
  tx2_signal_combined:        { off: 0x0023116a, type: 'u16' },

  tx1_q10_x:                  { off: 0x00231130, type: 's32' },
  tx1_q10_y:                  { off: 0x00231134, type: 's32' },
  tx2_q10_x:                  { off: 0x00231148, type: 's32' },
  tx2_q10_y:                  { off: 0x0023114c, type: 's32' },

  asa_static_bits_cur:        { off: 0x00231950, type: 'u8'  },
  asa_static_bits_prev:       { off: 0x00231954, type: 'u8'  },
  real_press_flag:            { off: 0x00231964, type: 'u8'  },
  no_press_ink_flag:          { off: 0x00231965, type: 'u8'  },
  no_press_exit_debounce:     { off: 0x00231966, type: 'u8'  },
  no_press_enter_debounce:    { off: 0x00231967, type: 'u8'  },

  no_press_exit_thd_dim1:     { off: 0x00231968, type: 'u16' },
  no_press_enter_thd_dim1:    { off: 0x0023196a, type: 'u16' },
  no_press_exit_thd_dim2:     { off: 0x0023196c, type: 'u16' },
  no_press_enter_thd_dim2:    { off: 0x0023196e, type: 'u16' },

  no_press_abnormal_flags:    { off: 0x00231a18, type: 'u32' },
  cur_pressure_out:           { off: 0x00231b18, type: 'u32' },
  prev_pressure_out:          { off: 0x00231c18, type: 'u32' },
  rpt_static_bits:            { off: 0x00231b28, type: 'u8'  },
  prev_rpt_static_bits:       { off: 0x00231c28, type: 'u8'  },

  tx1_peak_signal_a:          { off: 0x00230a76, type: 'u16' },
  tx1_peak_signal_b:          { off: 0x00230c26, type: 'u16' },
  tx2_peak_signal_a:          { off: 0x00230dd6, type: 'u16' },
  tx2_peak_signal_b:          { off: 0x00230f86, type: 'u16' },

  tx1_valid_flag:             { off: 0x00230a85, type: 'u8'  },
  tx2_valid_flag:             { off: 0x00230c35, type: 'u8'  },

  tilt_learn_ok_flag:         { off: 0x00219eb4, type: 'u8'  },
  tilt_learn_min_tx2:         { off: 0x00219eb6, type: 'u16' },
  tilt_learn_max_tx2:         { off: 0x00219eb8, type: 'u16' },
  tilt_comp_scale_active:     { off: 0x00219eba, type: 'u16' },
  tilt_comp_scale_candidate:  { off: 0x00219ebc, type: 'u16' }
};

let gModuleBase = null;
let gHooksInstalled = false;
let gWriter = null;
let gPendingFlushLines = 0;
let gEventId = 0;
let gExportMap = Object.create(null);
let gResolvedExports = Object.create(null);
let gParsedExports = null;
let gParsedExportsByName = Object.create(null);
let gParsedExportsByOrdinal = Object.create(null);
const gDepthByTid = Object.create(null);

function nowMs() {
  return Date.now();
}

function nextEventId() {
  gEventId += 1;
  return gEventId;
}

function openWriterIfNeeded() {
  if (gWriter !== null) return;
  gWriter = new File(CONFIG.logPath, 'a');
}

function flushWriter() {
  if (gWriter === null || gPendingFlushLines === 0) return;
  try {
    gWriter.flush();
    gPendingFlushLines = 0;
  } catch (_) {
  }
}

function writeEvent(obj) {
  obj.eventId = nextEventId();
  obj.hostTsMs = nowMs();
  const line = JSON.stringify(obj) + '\n';
  try {
    openWriterIfNeeded();
    gWriter.write(line);
    gPendingFlushLines += 1;
    if (gPendingFlushLines >= CONFIG.flushEveryLines || obj.kind === 'meta' || obj.kind === 'hook' || obj.kind === 'diag') {
      flushWriter();
    }
  } catch (e) {
    send({ kind: 'trace-fallback', line: obj, error: String(e) });
  }
}

function findResolvedGetter(name) {
  let i = 0;
  for (i = 0; i < CONFIG.resolvedGetterExports.length; i += 1) {
    const item = CONFIG.resolvedGetterExports[i];
    if (!item || item.missing) continue;
    if (item.requested === name || item.name === name || item.ordinalName === name) {
      return item;
    }
  }
  return null;
}

function updateExportSnapshot() {
  gExportMap = Object.create(null);
  const names = [
    'ASA_GetTX1Siganl',
    'ASA_GetTX2Siganl',
    'ASA_GetTX1SiganlX',
    'ASA_GetTX1SiganlY',
    'ASA_GetTX2SiganlX',
    'ASA_GetTX2SiganlY',
    'ASA_GetRptInRange',
    'ASA_GetRptInk',
    'ASA_GetRptPressure',
    'ASA_GetRptXPos',
    'ASA_GetRptYPos',
    'ASA_GetRptButtonStatus',
    'ASA_GetEnterNoPressInkThold',
    'ASA_GetExitNoPressInkThold',
    'ASA_AnimationState'
  ];

  let i = 0;
  for (i = 0; i < names.length; i += 1) {
    const name = names[i];
    let p = null;

    const resolved = findResolvedGetter(name);
    if (resolved && gModuleBase !== null) {
      try {
        p = gModuleBase.add(resolved.rva);
      } catch (_) {
        p = null;
      }
    }

    if (p === null) {
      p = resolveExport(name);
    }

    if (p === null) {
      gExportMap[name] = null;
      continue;
    }

    try {
      let fn = null;
      if (name === 'ASA_GetRptButtonStatus') {
        fn = new NativeFunction(p, 'uint32', ['uint32']);
        gExportMap[name] = fn(0);
      } else {
        fn = new NativeFunction(p, 'uint32', []);
        gExportMap[name] = fn();
      }
    } catch (_) {
      gExportMap[name] = null;
    }
  }
}

function ptrToString(value) {
  try {
    return ptr(value).toString();
  } catch (_) {
    try {
      return String(value);
    } catch (_) {
      return null;
    }
  }
}

function getProcessModuleByName(moduleName) {
  try {
    const modules = Process.enumerateModules();
    let i = 0;
    for (i = 0; i < modules.length; i += 1) {
      const m = modules[i];
      if (m && m.name && m.name.toLowerCase() === String(moduleName).toLowerCase()) {
        return m;
      }
    }
  } catch (_) {
  }
  return null;
}

function parseModuleExports() {
  if (gParsedExports !== null) {
    return gParsedExports;
  }

  gParsedExports = [];
  gParsedExportsByName = Object.create(null);
  gParsedExportsByOrdinal = Object.create(null);

  try {
    const m = getProcessModuleByName(CONFIG.moduleName);
    if (!m) {
      return gParsedExports;
    }

    const imageBase = m.base;
    const peOffset = Memory.readU32(imageBase.add(0x3c));
    const nt = imageBase.add(peOffset);
    const signature = Memory.readU32(nt);
    if (signature !== 0x4550) {
      return gParsedExports;
    }

    const fileHeader = nt.add(4);
    const numberOfSections = Memory.readU16(fileHeader.add(2));
    const optionalHeader = fileHeader.add(20);
    const magic = Memory.readU16(optionalHeader);
    const dataDirBase = optionalHeader.add(magic === 0x20b ? 0x70 : 0x60);
    const exportRva = Memory.readU32(dataDirBase);
    const exportSize = Memory.readU32(dataDirBase.add(4));
    if (exportRva === 0 || exportSize === 0) {
      return gParsedExports;
    }

    const exportDir = imageBase.add(exportRva);
    const baseOrdinal = Memory.readU32(exportDir.add(0x10));
    const numberOfFunctions = Memory.readU32(exportDir.add(0x14));
    const numberOfNames = Memory.readU32(exportDir.add(0x18));
    const addressOfFunctions = Memory.readU32(exportDir.add(0x1c));
    const addressOfNames = Memory.readU32(exportDir.add(0x20));
    const addressOfNameOrdinals = Memory.readU32(exportDir.add(0x24));

    const namesByIndex = Object.create(null);
    let i = 0;
    for (i = 0; i < numberOfNames; i += 1) {
      const nameRva = Memory.readU32(imageBase.add(addressOfNames + (i * 4)));
      const ordIndex = Memory.readU16(imageBase.add(addressOfNameOrdinals + (i * 2)));
      let name = null;
      try {
        name = Memory.readUtf8String(imageBase.add(nameRva));
      } catch (_) {
        name = null;
      }
      namesByIndex[ordIndex] = name;
    }

    for (i = 0; i < numberOfFunctions; i += 1) {
      const funcRva = Memory.readU32(imageBase.add(addressOfFunctions + (i * 4)));
      if (funcRva === 0) {
        continue;
      }

      const ordinal = baseOrdinal + i;
      const name = namesByIndex[i] || null;
      const entry = {
        ordinal: ordinal,
        ordinalName: 'Ordinal_' + ordinal,
        name: name,
        address: imageBase.add(funcRva),
        rva: funcRva
      };

      gParsedExports.push(entry);
      gParsedExportsByOrdinal[ordinal] = entry;
      if (name !== null) {
        gParsedExportsByName[name] = entry;
      }
    }
  } catch (_) {
  }

  return gParsedExports;
}

function resolveExport(name) {
  try {
    if (Object.prototype.hasOwnProperty.call(gResolvedExports, name)) {
      return gResolvedExports[name];
    }
  } catch (_) {
  }

  let i = 0;
  for (i = 0; i < CONFIG.resolvedExports.length; i += 1) {
    const item = CONFIG.resolvedExports[i];
    if (!item || item.missing) continue;
    if (item.requested === name || item.name === name || item.ordinalName === name) {
      try {
        const addr = gModuleBase.add(item.rva);
        gResolvedExports[name] = addr;
        return addr;
      } catch (_) {
      }
    }
  }

  parseModuleExports();
  let entry = null;

  try {
    if (Object.prototype.hasOwnProperty.call(gParsedExportsByName, name)) {
      entry = gParsedExportsByName[name];
    }
  } catch (_) {
  }

  if (entry === null) {
    const m = /^Ordinal_(\d+)$/.exec(name);
    if (m) {
      const ord = parseInt(m[1], 10);
      if (!isNaN(ord) && Object.prototype.hasOwnProperty.call(gParsedExportsByOrdinal, ord)) {
        entry = gParsedExportsByOrdinal[ord];
      }
    }
  }

  if (entry !== null) {
    gResolvedExports[name] = entry.address;
    return entry.address;
  }

  try {
    if (typeof Module.findExportByName === 'function') {
      const p = Module.findExportByName(CONFIG.moduleName, name);
      if (p !== null) {
        gResolvedExports[name] = p;
        return p;
      }
    }
  } catch (_) {
  }

  return null;
}

function safeRead(addr, type) {
  try {
    switch (type) {
      case 'u8':  return Memory.readU8(addr);
      case 'u16': return Memory.readU16(addr);
      case 'u32': return Memory.readU32(addr);
      case 'u64': return Memory.readU64(addr).toString();
      case 's8':  return Memory.readS8(addr);
      case 's16': return Memory.readS16(addr);
      case 's32': return Memory.readS32(addr);
      case 'ptr': return ptrToString(Memory.readPointer(addr));
      default:    return null;
    }
  } catch (_) {
    return null;
  }
}

function readRawGlobal(name) {
  if (!CONFIG.includeRawGlobals) return null;
  if (gModuleBase === null) return null;
  const spec = RAW_GLOBALS[name];
  if (!spec) return null;
  return safeRead(gModuleBase.add(spec.off), spec.type);
}

function findResolvedEntry(list, name) {
  let i = 0;
  for (i = 0; i < list.length; i += 1) {
    const item = list[i];
    if (!item || item.missing) continue;
    if (item.requested === name || item.name === name || item.ordinalName === name) {
      return item;
    }
  }
  return null;
}

function readGridSnapshot() {
  if (!CONFIG.dumpGrids || gModuleBase === null) {
    return undefined;
  }

  const names = [
    'ASA_GetGridTiedOriTx1Ptr',
    'ASA_GetGridTiedOriTx2Ptr',
    'ASA_GetGridTiedDeTraceNoiseTx1Ptr',
    'ASA_GetGridTiedDeTraceNoiseTx2Ptr'
  ];
  const result = {};
  let i = 0;

  for (i = 0; i < names.length; i += 1) {
    const name = names[i];
    const entry = findResolvedEntry(CONFIG.resolvedGetterExports, name);
    if (!entry) {
      result[name] = null;
      continue;
    }

    let p = null;
    try {
      p = gModuleBase.add(entry.rva);
      const fn = new NativeFunction(p, 'pointer', ['uint32']);
      const buf = fn(0);
      if (buf.isNull()) {
        result[name] = { ptr: '0x0', values: null };
        continue;
      }

      const values = [];
      let j = 0;
      for (j = 0; j < 9 * 9; j += 1) {
        values.push(Memory.readS16(buf.add(j * 2)));
      }
      result[name] = {
        ptr: ptrToString(buf),
        values: values
      };
    } catch (_) {
      result[name] = null;
    }
  }

  return result;
}

function stateSnapshot() {
  updateExportSnapshot();

  const exportState = {
    tx1Combined: gExportMap.ASA_GetTX1Siganl,
    tx2Combined: gExportMap.ASA_GetTX2Siganl,
    tx1X: gExportMap.ASA_GetTX1SiganlX,
    tx1Y: gExportMap.ASA_GetTX1SiganlY,
    tx2X: gExportMap.ASA_GetTX2SiganlX,
    tx2Y: gExportMap.ASA_GetTX2SiganlY,
    rptInRange: gExportMap.ASA_GetRptInRange,
    rptInk: gExportMap.ASA_GetRptInk,
    rptPressure: gExportMap.ASA_GetRptPressure,
    rptX: gExportMap.ASA_GetRptXPos,
    rptY: gExportMap.ASA_GetRptYPos,
    rptButtonStatus0: gExportMap.ASA_GetRptButtonStatus,
    enterNoPressTh: gExportMap.ASA_GetEnterNoPressInkThold,
    exitNoPressTh: gExportMap.ASA_GetExitNoPressInkThold,
    animationState: gExportMap.ASA_AnimationState
  };

  if (!CONFIG.includeRawGlobals) {
    return {
      rawGlobalsDisabled: true,
      exports: exportState,
      grids: readGridSnapshot()
    };
  }

  return {
    seq: readRawGlobal('frameSequence'),
    frameTsMs: readRawGlobal('frameTimestampMs'),
    exports: exportState,
    grids: readGridSnapshot(),

    tx1: {
      dim1: readRawGlobal('tx1_signal_dim1'),
      dim2: readRawGlobal('tx1_signal_dim2'),
      combined: readRawGlobal('tx1_signal_combined'),
      peakA: readRawGlobal('tx1_peak_signal_a'),
      peakB: readRawGlobal('tx1_peak_signal_b'),
      valid: readRawGlobal('tx1_valid_flag'),
      q10x: readRawGlobal('tx1_q10_x'),
      q10y: readRawGlobal('tx1_q10_y')
    },

    tx2: {
      dim1: readRawGlobal('tx2_signal_dim1'),
      dim2: readRawGlobal('tx2_signal_dim2'),
      combined: readRawGlobal('tx2_signal_combined'),
      peakA: readRawGlobal('tx2_peak_signal_a'),
      peakB: readRawGlobal('tx2_peak_signal_b'),
      valid: readRawGlobal('tx2_valid_flag'),
      q10x: readRawGlobal('tx2_q10_x'),
      q10y: readRawGlobal('tx2_q10_y')
    },

    flags: {
      staticCur: readRawGlobal('asa_static_bits_cur'),
      staticPrev: readRawGlobal('asa_static_bits_prev'),
      rptStatic: readRawGlobal('rpt_static_bits'),
      rptStaticPrev: readRawGlobal('prev_rpt_static_bits'),
      realPress: readRawGlobal('real_press_flag'),
      noPressInk: readRawGlobal('no_press_ink_flag'),
      noPressExitDebounce: readRawGlobal('no_press_exit_debounce'),
      noPressEnterDebounce: readRawGlobal('no_press_enter_debounce'),
      abnormal: readRawGlobal('no_press_abnormal_flags'),
      tiltLearnOk: readRawGlobal('tilt_learn_ok_flag')
    },

    thresholds: {
      exitDim1: readRawGlobal('no_press_exit_thd_dim1'),
      enterDim1: readRawGlobal('no_press_enter_thd_dim1'),
      exitDim2: readRawGlobal('no_press_exit_thd_dim2'),
      enterDim2: readRawGlobal('no_press_enter_thd_dim2'),
      tiltMinTx2: readRawGlobal('tilt_learn_min_tx2'),
      tiltMaxTx2: readRawGlobal('tilt_learn_max_tx2'),
      tiltScaleActive: readRawGlobal('tilt_comp_scale_active'),
      tiltScaleCandidate: readRawGlobal('tilt_comp_scale_candidate')
    },

    pressure: {
      cur: readRawGlobal('cur_pressure_out'),
      prev: readRawGlobal('prev_pressure_out')
    }
  };
}

function containsString(list, value) {
  let i = 0;
  for (i = 0; i < list.length; i += 1) {
    if (list[i] === value) return true;
  }
  return false;
}

function maybeBacktrace(ctx, exportName) {
  if (!CONFIG.captureBacktrace) return undefined;
  if (!containsString(CONFIG.backtraceExports, exportName)) return undefined;
  try {
    return Thread.backtrace(ctx, Backtracer.ACCURATE)
      .map(DebugSymbol.fromAddress)
      .map((s) => s.toString());
  } catch (_) {
    return undefined;
  }
}

function attachExport(exportName) {
  if (!containsString(CONFIG.enabledExports, exportName)) return;

  writeEvent({
    kind: 'diag',
    event: 'attach-start',
    module: CONFIG.moduleName,
    exportName: exportName
  });

  const target = resolveExport(exportName);
  if (target === null) {
    writeEvent({
      kind: 'hook',
      event: 'skip-missing-export',
      module: CONFIG.moduleName,
      exportName: exportName
    });
    return;
  }

  writeEvent({
    kind: 'hook',
    event: 'attach',
    module: CONFIG.moduleName,
    exportName: exportName,
    address: ptrToString(target)
  });

  if (typeof target === 'undefined' || target === null) {
    writeEvent({
      kind: 'hook',
      event: 'skip-null-export',
      module: CONFIG.moduleName,
      exportName: exportName,
      reason: 'resolved export is null'
    });
    return;
  }

  if (typeof target.isNull === 'function' && target.isNull()) {
    writeEvent({
      kind: 'hook',
      event: 'skip-null-export',
      module: CONFIG.moduleName,
      exportName: exportName
    });
    return;
  }

  try {
    Interceptor.attach(target, {
      onEnter(args) {
        const tid = Process.getCurrentThreadId();
        const depth = gDepthByTid[tid] || 0;
        const enterTs = nowMs();
        const arg0 = args[0];

        this.__traceState = {
          exportName: exportName,
          tid: tid,
          depth: depth,
          enterTs: enterTs,
          arg0: arg0
        };

        gDepthByTid[tid] = depth + 1;

        const evt = {
          kind: 'call',
          phase: 'enter',
          module: CONFIG.moduleName,
          exportName: exportName,
          address: ptrToString(target),
          tid: tid,
          depth: depth,
          arg0: arg0 ? ptrToString(arg0) : null
        };

        const bt = maybeBacktrace(this.context, exportName);
        if (bt !== undefined) {
          evt.backtrace = bt;
        }
        if (CONFIG.snapshotOnEnter) {
          evt.state = stateSnapshot();
        }
        writeEvent(evt);
      },

      onLeave(retval) {
        const s = this.__traceState || {
          exportName: exportName,
          tid: Process.getCurrentThreadId(),
          depth: 0,
          enterTs: null,
          arg0: null
        };

        gDepthByTid[s.tid] = Math.max(0, (gDepthByTid[s.tid] || 1) - 1);

        const evt = {
          kind: 'call',
          phase: 'leave',
          module: CONFIG.moduleName,
          exportName: s.exportName,
          tid: s.tid,
          depth: s.depth,
          durationMs: s.enterTs === null ? null : (nowMs() - s.enterTs),
          retvalPtr: ptrToString(retval),
          retvalU32: retval.toUInt32(),
          arg0: s.arg0 ? ptrToString(s.arg0) : null
        };

        const bt = maybeBacktrace(this.context, s.exportName);
        if (bt !== undefined) {
          evt.backtrace = bt;
        }
        if (CONFIG.snapshotOnLeave) {
          evt.state = stateSnapshot();
        }
        writeEvent(evt);
        this.__traceState = null;
      }
    });
  } catch (e) {
    writeEvent({
      kind: 'diag',
      event: 'attach-failed',
      module: CONFIG.moduleName,
      exportName: exportName,
      address: ptrToString(target),
      error: String(e)
    });
  }
}

function dumpCandidateExports() {
  try {
    const interesting = [];
    let i = 0;

    for (i = 0; i < CONFIG.resolvedExports.length; i += 1) {
      const e = CONFIG.resolvedExports[i];
      if (!e || e.missing) continue;
      interesting.push({
        source: 'launcher',
        requested: e.requested,
        name: e.name || e.ordinalName || e.requested,
        ordinal: e.ordinal,
        rva: '0x' + e.rva.toString(16),
        address: gModuleBase ? ptrToString(gModuleBase.add(e.rva)) : null
      });
    }

    if (interesting.length === 0) {
      const exports = parseModuleExports();
      for (i = 0; i < exports.length; i += 1) {
        const e2 = exports[i];
        if (!e2) continue;
        const exportName = e2.name || e2.ordinalName;
        if (!exportName) continue;
        if (exportName.indexOf('ASA') >= 0 || exportName.indexOf('HPP3') >= 0 || exportName.indexOf('NoPress') >= 0 || exportName.indexOf('Ordinal_') === 0) {
          interesting.push({
            source: 'agent',
            name: exportName,
            ordinal: e2.ordinal,
            address: ptrToString(e2.address),
            rva: '0x' + e2.rva.toString(16)
          });
        }
      }
    }

    writeEvent({
      kind: 'diag',
      event: 'export-candidates',
      module: CONFIG.moduleName,
      count: interesting.length,
      exports: interesting.slice(0, 256)
    });
  } catch (e) {
    writeEvent({
      kind: 'diag',
      event: 'export-enumeration-failed',
      module: CONFIG.moduleName,
      error: String(e)
    });
  }
}

function installHooks(mod) {
  if (gHooksInstalled) return;
  gHooksInstalled = true;
  gModuleBase = mod.base;

  dumpCandidateExports();

  writeEvent({
    kind: 'meta',
    event: 'module-found',
    module: mod.name,
    base: mod.base.toString(),
    size: mod.size,
    path: mod.path,
    pid: Process.id,
    arch: Process.arch,
    pointerSize: Process.pointerSize,
    config: {
      logPath: CONFIG.logPath,
      includeRawGlobals: CONFIG.includeRawGlobals,
      dumpGrids: CONFIG.dumpGrids,
      snapshotOnEnter: CONFIG.snapshotOnEnter,
      snapshotOnLeave: CONFIG.snapshotOnLeave,
      captureBacktrace: CONFIG.captureBacktrace,
      enabledExports: CONFIG.enabledExports.slice()
    }
  });

  let i = 0;
  for (i = 0; i < CONFIG.enabledExports.length; i += 1) {
    attachExport(CONFIG.enabledExports[i]);
  }
}

function tryInstallHooks() {
  if (gHooksInstalled) return true;
  const mod = Process.findModuleByName(CONFIG.moduleName);
  if (mod !== null) {
    installHooks(mod);
    return true;
  }
  return false;
}

writeEvent({
  kind: 'meta',
  event: 'script-start',
  pid: Process.id,
  arch: Process.arch,
  pointerSize: Process.pointerSize,
  moduleName: CONFIG.moduleName,
  logPath: CONFIG.logPath,
  note: 'hooks use exported functions only; variables come from raw data RVAs when includeRawGlobals=true'
});

if (!tryInstallHooks()) {
  writeEvent({
    kind: 'meta',
    event: 'waiting-module',
    moduleName: CONFIG.moduleName,
    pollIntervalMs: CONFIG.pollIntervalMs
  });

  const timer = setInterval(function () {
    if (tryInstallHooks()) {
      clearInterval(timer);
    }
  }, CONFIG.pollIntervalMs);
}

setInterval(function () {
  flushWriter();
}, CONFIG.periodicFlushMs);
