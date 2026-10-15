#requires -Version 7
<#
.SYNOPSIS
  判断当前工作区的 TSACore 与参考版本在算法上是否等价。

.DESCRIPTION
  把同一批录制分别喂给两份 TsaRawReplay：一份从参考提交（默认是标签 tsa-reference）构建，一份从当前
  工作区构建。每份录制默认跑纯回放、长时间静止、注入模拟笔三个场景（-Full 再加充电器、悬停笔、经
  StylusPipeline 的真实笔），比较两样东西：

    触点 CSV        每帧上报的触点（数量、ID、事件、坐标）
    stderr          闭环请求 / AFE 动作、笔状态等逐帧诊断

  逐字节相同才算通过。比较的是算法的输出而不是内部状态，内部状态的布局可以随重构改变。

  参考版本检出在 build/tsa-reference/<提交> 下（git worktree），只在第一次用到时构建；它的回放结果按
  录制（路径、大小、修改时间）与场景参数缓存在同一目录下，参考提交不变就不再重跑。所有回放按处理器数并行。

.PARAMETER Full
  跑全部六个场景。

.PARAMETER State
  另外逐帧比较 TSACore 全部算法状态（.bss + 堆缓冲）的摘要；不同的时候在第一次出现差异的那一帧把两边的
  完整状态导出来，按符号名列出不同的字段。只在两边状态布局相同、需要定位差异时用。

.PARAMETER Reference
  参考提交。行为有意变更（修了真实的缺陷）之后，把标签挪到新的提交上。

.PARAMETER Recordings
  录制文件或目录，可以给多个。

.PARAMETER Only
  只跑名字（录制名/场景名）含这个子串的项。

.PARAMETER SelfCheck
  额外把参考版本换一种堆布局再跑一遍纯回放，确认摘要本身是确定的。

.PARAMETER Coverage
  不做比较，改为统计这批录制与场景对当前算法代码的覆盖率，结果写到 build/tsa-coverage。

.PARAMETER Benchmark
  不做比较，改为计时：以服务实际安装的配置 -BenchPreset（默认 arm64-Release）加 EGO_TSACORE_PROFILE
  构建到 build/tsa-bench-release，逐个串行回放（纯回放场景），跑 -Rounds 轮取最快的一轮打印每帧耗时（只计
  整帧，--profile-frame），再另跑几轮分步计时，打印耗时最多的 -Top 个步骤（步骤耗时含其子步骤，也含各段
  计时自身的开销）。回放钉在 -Affinity 指定的核上（默认 0xF0，Snapdragon
  8cx Gen 3 的四个大核）并以高优先级运行。

.EXAMPLE
  pwsh Tools/TsaRawReplay/Compare-Reference.ps1
  pwsh Tools/TsaRawReplay/Compare-Reference.ps1 -Recordings C:\ProgramData\EGoTouchRev\exports\dvr, D:\rec\pen.dvrbin -SelfCheck
  pwsh Tools/TsaRawReplay/Compare-Reference.ps1 -Coverage
#>
[CmdletBinding()]
param(
    [string]$Reference = 'tsa-reference',
    [string]$Preset = 'arm64-Debug',
    [string]$BenchPreset = 'arm64-Release',
    [string[]]$Recordings = @('C:\ProgramData\EGoTouchRev\exports\dvr'),
    [string]$Only = '',
    [switch]$SelfCheck,
    [switch]$Coverage,
    [switch]$Full,
    [switch]$State,
    [switch]$Benchmark,
    [int]$Rounds = 5,
    [int]$Top = 20,
    [long]$Affinity = 0xF0
)

$ErrorActionPreference = 'Stop'
$root = (git -C $PSScriptRoot rev-parse --show-toplevel).Trim()

function Invoke-Native {
    param([string]$What, [scriptblock]$Command)
    $output = & $Command 2>&1
    if ($LASTEXITCODE -ne 0) {
        $output | Select-Object -Last 30 | ForEach-Object { Write-Host $_ }
        throw "$What failed (exit $LASTEXITCODE)"
    }
}

function Get-RecordingFiles {
    # 经 pwsh -File 调用时，逗号分隔的列表会整个落进第一个元素
    foreach ($item in ($Recordings -split ',' | ForEach-Object Trim | Where-Object { $_ })) {
        if (Test-Path $item -PathType Container) {
            Get-ChildItem $item -Filter *.dvrbin | Sort-Object Name | ForEach-Object FullName
        } elseif (Test-Path $item) {
            (Resolve-Path $item).Path
        } else {
            throw "recording not found: $item"
        }
    }
}

# 启动一次回放，不等它结束
function Start-Replay {
    param([string]$Exe, [string]$Recording, [object[]]$ReplayArgs, [string]$OutBase, [string]$ProfileFile = '', [switch]$Digest)
    $argList = @("`"$Recording`"") + ($ReplayArgs | ForEach-Object { "$_" })
    if ($Digest) { $argList += @('--state-digest', "`"$OutBase.dig`"") }
    if ($ProfileFile) { $env:LLVM_PROFILE_FILE = $ProfileFile }
    try {
        Start-Process -FilePath $Exe -ArgumentList $argList -NoNewWindow -PassThru `
            -RedirectStandardOutput "$OutBase.csv" -RedirectStandardError "$OutBase.err"
    } finally {
        if ($ProfileFile) { Remove-Item Env:LLVM_PROFILE_FILE }
    }
}

# 回放排队，最多同时跑处理器数个
$script:running = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
function Start-Queued {
    param([string]$Exe, [string]$Recording, [object[]]$ReplayArgs, [string]$OutBase, [string]$ProfileFile = '', [switch]$Digest)
    while ($script:running.Count -ge [Environment]::ProcessorCount) {
        $script:running.RemoveAll({ param($p) $p.HasExited }) | Out-Null
        if ($script:running.Count -ge [Environment]::ProcessorCount) { Start-Sleep -Milliseconds 20 }
    }
    $p = Start-Replay $Exe $Recording $ReplayArgs $OutBase $ProfileFile -Digest:$Digest
    $null = $p.Handle   # 先取句柄，退出后才读得到 ExitCode
    $script:running.Add($p)
    $p
}

function Test-SameFile {
    param([string]$A, [string]$B)
    if ($A.EndsWith('.err')) {
        # 开头的 loaded 行描述输入（帧数、面板机型），不是算法输出
        $body = { param($p) @([System.IO.File]::ReadAllLines($p) | Where-Object { -not $_.StartsWith('loaded ') }) -join "`n" }
        return (& $body $A) -ceq (& $body $B)
    }
    (Get-FileHash $A -Algorithm SHA256).Hash -eq (Get-FileHash $B -Algorithm SHA256).Hash
}

# 两个文本文件第一处不同的行
function Get-FirstDifference {
    param([string]$A, [string]$B)
    $la = [System.IO.File]::ReadAllLines($A)
    $lb = [System.IO.File]::ReadAllLines($B)
    $n = [Math]::Min($la.Length, $lb.Length)
    for ($i = 0; $i -lt $n; $i++) {
        if ($la[$i] -cne $lb[$i]) { return [pscustomobject]@{ Line = $i + 1; A = $la[$i]; B = $lb[$i] } }
    }
    if ($la.Length -ne $lb.Length) {
        return [pscustomobject]@{ Line = $n + 1; A = ($la.Length -gt $n ? $la[$n] : '<end>'); B = ($lb.Length -gt $n ? $lb[$n] : '<end>') }
    }
    $null
}

# 录制的帧数：让工具读一遍但不喂帧
function Get-RecordingInfo {
    param([string]$Exe, [string]$Recording)
    # 覆盖率构建的程序退出时会在当前目录写 default.profraw；这次运行不计入覆盖率
    $env:LLVM_PROFILE_FILE = Join-Path ([System.IO.Path]::GetTempPath()) 'tsa-recording-info.profraw'
    try {
        $text = & $Exe $Recording --quiet --to 0 2>&1 | Out-String
    } finally {
        Remove-Item Env:LLVM_PROFILE_FILE
    }
    $frames = if ($text -match 'loaded (\d+) frames') { [int]$Matches[1] } else { 0 }
    [pscustomobject]@{ Frames = $frames }
}

function Get-Scenarios {
    param($Info)
    $n = $Info.Frames
    $quarter = [int]($n / 4)
    $half = [int]($n / 2)
    $threeQuarters = [int]($n * 3 / 4)
    $list = @(
        @{ Name = 'plain';     Args = @('--afe') }
        @{ Name = 'charger';   Args = @('--afe', '--charger') }
        @{ Name = 'idle';      Args = @('--afe', '--idle-exit', $half, '--hold', $quarter, 600) }
        @{ Name = 'pen-write'; Args = @('--afe', '--pen', 1280, 800, $quarter, $threeQuarters, '--pen-word', '0102', '--pen-tilt', 25, -15) }
        @{ Name = 'pen-hover'; Args = @('--afe', '--pen', 640, 1200, $quarter, $threeQuarters, '--pen-hover', '--pen-interleave') }
        # 真实笔数据走 StylusPipeline；没有笔的录制也跑，走的是「笔已连接但不在场」
        @{ Name = 'stylus';    Args = @('--afe', '--stylus-solver') }
    )
    # 默认只跑覆盖面互不重叠的三个：纯回放、静止后退出 idle、模拟笔书写
    $Full ? $list : @($list | Where-Object { $_.Name -in 'plain', 'idle', 'pen-write' })
}

# ── 计时 ────────────────────────────────────────────────────────────────

if ($Benchmark) {
    $benchDir = Join-Path $root 'build/tsa-bench-release'
    Write-Host "building benchmark ($BenchPreset, EGO_TSACORE_PROFILE)..."
    Push-Location $root
    try {
        Invoke-Native 'configure benchmark build' {
            # 发布构建的录制读取器不保留原始 SPI 帧（--afe 回放没有输入），整体打开诊断字段；TsaCore 算法不受它影响
            cmake --preset $BenchPreset -B $benchDir '-DEGO_TSACORE_PROFILE=ON' '-DCMAKE_CXX_FLAGS=-DEGOTOUCH_DIAGNOSTICS'
        }
        Invoke-Native 'benchmark build' { cmake --build $benchDir --target TsaRawReplay }
    } finally {
        Pop-Location
    }
    $exe = Join-Path $benchDir 'TsaRawReplay.exe'
    $recordings = @(Get-RecordingFiles)
    # 大小核机器上调度器会把回放放到小核、频率也随负载变化：钉在大核（-Affinity）、高优先级运行，返回 stderr。
    function Invoke-Pinned {
        param([string]$Exe, [string[]]$ArgList)
        $psi = [System.Diagnostics.ProcessStartInfo]::new($Exe, ($ArgList -join ' '))
        $psi.RedirectStandardError = $true
        $psi.RedirectStandardOutput = $true
        $psi.UseShellExecute = $false
        $p = [System.Diagnostics.Process]::Start($psi)
        $p.ProcessorAffinity = [IntPtr]$Affinity
        $p.PriorityClass = 'High'
        $null = $p.StandardOutput.ReadToEndAsync()
        $err = $p.StandardError.ReadToEnd()
        $p.WaitForExit()
        $err
    }
    # 逐个串行跑（并行会互相抢核心与缓存）；每轮把全部录制的耗时相加，各项取各轮中最小的一轮。
    # 整帧耗时用 --profile-frame（不计各步骤，没有分段计时自身的开销）；步骤明细另跑 --profile。
    function Measure-Rounds {
        param([string]$Flag, [int]$Count)
        $best = @{}
        $frames = 0
        for ($round = 0; $round -lt $Count; $round++) {
            $sum = @{}
            $roundFrames = 0
            foreach ($recording in $recordings) {
                $text = Invoke-Pinned $exe @("`"$recording`"", '--quiet', '--afe', $Flag)
                foreach ($line in ($text -split "`r?`n")) {
                    if ("$line" -match '^PROF\s+(\d+)\s+(\d+)\s+(.+)$') { $sum[$Matches[3]] = [double]$sum[$Matches[3]] + [double]$Matches[1] }
                    elseif ("$line" -match '^fed (\d+) frames') { $roundFrames += [int]$Matches[1] }
                }
            }
            if (-not $sum.ContainsKey('TSA_ProcessingAFEFrame')) { throw 'no profile data from the benchmark build' }
            $frames = $roundFrames
            foreach ($k in $sum.Keys) { if (-not $best.ContainsKey($k) -or $sum[$k] -lt $best[$k]) { $best[$k] = $sum[$k] } }
        }
        [pscustomobject]@{ Best = $best; Frames = $frames }
    }
    $frameOnly = Measure-Rounds '--profile-frame' $Rounds
    $stepped = Measure-Rounds '--profile' ([Math]::Max(2, [int]($Rounds / 2)))
    $frames = $stepped.Frames
    $total = $stepped.Best['TSA_ProcessingAFEFrame']
    $rows = $stepped.Best.GetEnumerator() | Sort-Object Value -Descending | ForEach-Object {
        [pscustomobject]@{ Step = $_.Key; UsPerFrame = [Math]::Round($_.Value / $frames / 1000, 2); Percent = [Math]::Round(100 * $_.Value / $total, 1) }
    }
    $rows | Format-Table -AutoSize | Out-String -Width 200 | Set-Content (Join-Path $benchDir 'profile.txt')
    $frameUs = $frameOnly.Best['TSA_ProcessingAFEFrame'] / $frameOnly.Frames / 1000
    Write-Host ("`n{0} recordings, {1} frames, best of {2} rounds: {3:F1} us/frame" -f $recordings.Count, $frameOnly.Frames, $Rounds, $frameUs)
    Write-Host ("steps (instrumented run, {0:F1} us/frame including the per-step timers):" -f ($total / $frames / 1000))
    $rows | Select-Object -First $Top | Format-Table -AutoSize | Out-String -Width 200 | Write-Host
    Write-Host "full table: $(Join-Path $benchDir 'profile.txt')"
    exit 0
}

# ── 构建 ────────────────────────────────────────────────────────────────

Write-Host "building current tree ($Preset)..."
Push-Location $root
try {
    if ($Coverage) {
        $covDir = Join-Path $root 'build/tsa-coverage'
        Invoke-Native 'configure coverage build' {
            cmake --preset $Preset -B $covDir '-DEGO_TSACORE_EXTRA_FLAGS=-fprofile-instr-generate -fcoverage-mapping'
        }
        Invoke-Native 'coverage build' { cmake --build $covDir --target TsaRawReplay }
        $newExe = Join-Path $covDir 'TsaRawReplay.exe'
    } else {
        Invoke-Native 'build' { cmake --build --preset $Preset --target TsaRawReplay }
        $newExe = Join-Path $root "build/$Preset/TsaRawReplay.exe"
    }
} finally {
    Pop-Location
}

$refExe = $null
if (-not $Coverage) {
    $sha = (git -C $root rev-parse --verify "$Reference^{commit}").Trim()
    if ($LASTEXITCODE -ne 0) { throw "reference '$Reference' does not resolve to a commit" }
    $refSrc = Join-Path $root "build/tsa-reference/$($sha.Substring(0, 12))"
    $refExe = Join-Path $refSrc "build/$Preset/TsaRawReplay.exe"
    if (-not (Test-Path $refExe)) {
        Write-Host "building reference $Reference ($($sha.Substring(0, 12)))..."
        if (-not (Test-Path (Join-Path $refSrc 'CMakeLists.txt'))) {
            Invoke-Native 'git worktree add' { git -C $root worktree add --detach $refSrc $sha }
        }
        Push-Location $refSrc
        try {
            # 工作树里没有子模块（imgui），GUI 工具不建
            Invoke-Native 'configure reference' { cmake --preset $Preset -DEGO_BUILD_TOOLS=OFF }
            Invoke-Native 'build reference' { cmake --build --preset $Preset --target TsaRawReplay }
        } finally {
            Pop-Location
        }
    }
}

# ── 运行 ────────────────────────────────────────────────────────────────

$outRoot = Join-Path $root ($Coverage ? 'build/tsa-coverage/runs' : 'build/tsa-compare')
if (Test-Path $outRoot) { Remove-Item $outRoot -Recurse -Force }
New-Item $outRoot -ItemType Directory | Out-Null

# 先把所有回放排上（参考版本命中缓存的不跑），全部结束后再逐项比较
$jobs = @()
foreach ($recording in Get-RecordingFiles) {
    $recName = [System.IO.Path]::GetFileNameWithoutExtension($recording)
    $info = Get-RecordingInfo $newExe $recording
    if ($info.Frames -eq 0) {
        Write-Warning "$recName has no frames, skipped"
        continue
    }
    $file = Get-Item $recording
    foreach ($scenario in Get-Scenarios $info) {
        $label = "$recName/$($scenario.Name)"
        if ($Only -and $label -notlike "*$Only*") { continue }
        $dir = Join-Path $outRoot $label
        New-Item $dir -ItemType Directory -Force | Out-Null
        $job = [pscustomobject]@{ Label = $label; Recording = $recording; Scenario = $scenario; Frames = $info.Frames; Dir = $dir
                                  New = $null; Ref = $null; RefBase = $null; RefExit = $null }

        if ($Coverage) {
            $job.New = Start-Queued $newExe $recording $scenario.Args "$dir/new" "$dir/run.profraw"
            $jobs += $job
            continue
        }

        $key = "$($file.FullName)|$($file.Length)|$($file.LastWriteTimeUtc.Ticks)|$($scenario.Args -join ' ')"
        $hash = [System.Convert]::ToHexString([System.Security.Cryptography.SHA256]::HashData([System.Text.Encoding]::UTF8.GetBytes($key))).Substring(0, 16)
        $job.RefBase = Join-Path $refSrc "runs/$hash"
        if (Test-Path "$($job.RefBase).exit") {
            $job.RefExit = [int](Get-Content "$($job.RefBase).exit")
        } else {
            New-Item (Split-Path $job.RefBase) -ItemType Directory -Force | Out-Null
            $job.Ref = Start-Queued $refExe $recording $scenario.Args $job.RefBase -Digest
        }
        $job.New = Start-Queued $newExe $recording $scenario.Args "$dir/new" -Digest:$State
        $jobs += $job
    }
}
$script:running | Wait-Process

$results = @()
foreach ($job in $jobs) {
    $label = $job.Label; $dir = $job.Dir; $recording = $job.Recording; $scenario = $job.Scenario
    if ($Coverage) {
        $results += [pscustomobject]@{ Run = $label; Frames = $job.Frames; Result = ($job.New.ExitCode -eq 0 ? 'ran' : "exit $($job.New.ExitCode)") }
        continue
    }
    if ($job.Ref) {
        $job.RefExit = $job.Ref.ExitCode
        Set-Content "$($job.RefBase).exit" $job.RefExit   # 写完退出码才算缓存完整
    }
    $refBase = $job.RefBase
    $failures = @()
    if ($job.RefExit -ne 0) { $failures += "reference exit $($job.RefExit)" }
    if ($job.New.ExitCode -ne 0) { $failures += "current exit $($job.New.ExitCode)" }
    if (-not $failures) {
        foreach ($kind in @('csv', 'err') + ($State ? @('dig') : @())) {
            if (-not (Test-SameFile "$refBase.$kind" "$dir/new.$kind")) { $failures += $kind }
        }
    }

    $detail = ''
    if ($failures -contains 'csv' -or $failures -contains 'err') {
        $kind = ($failures -contains 'csv') ? 'csv' : 'err'
        $d = Get-FirstDifference "$refBase.$kind" "$dir/new.$kind"
        $detail = "$kind line $($d.Line):`n    ref: $($d.A)`n    new: $($d.B)"
    }
    if ($failures -contains 'dig') {
        $d = Get-FirstDifference "$refBase.dig" "$dir/new.dig"
        $step = ($d.A -split ' ')[0]
        if ($step -match '^-?\d+$') {
            # 在第一处不同的那一帧把两边的完整状态导出来，列出不同的字段
            $r = Start-Process $refExe -ArgumentList (@("`"$recording`"") + $scenario.Args + @('--quiet', '--state-dump', $step, "`"$dir/ref.state`"")) `
                -NoNewWindow -PassThru -RedirectStandardOutput "$dir/dump-ref.out" -RedirectStandardError "$dir/dump-ref.err"
            $c = Start-Process $newExe -ArgumentList (@("`"$recording`"") + $scenario.Args + @('--quiet', '--state-dump', $step, "`"$dir/new.state`"")) `
                -NoNewWindow -PassThru -RedirectStandardOutput "$dir/dump-new.out" -RedirectStandardError "$dir/dump-new.err"
            $r, $c | Wait-Process
            $diff = & $newExe --state-diff "$dir/ref.state" "$dir/new.state" 2>&1
            $diff | Set-Content "$dir/state-diff.txt"
            $detail += ($detail ? "`n" : '') + "state first differs after step $step (A = reference, B = current):`n    " +
                (($diff | Select-Object -First 16) -join "`n    ") + "`n    ... full list: $dir/state-diff.txt"
        } else {
            $detail += ($detail ? "`n" : '') + "digest header differs:`n    ref: $($d.A)`n    new: $($d.B)"
        }
    }

    if ($SelfCheck -and $scenario.Name -eq 'plain') {
        $again = Start-Replay $refExe $recording ($scenario.Args + @('--heap-skew', 3000)) "$dir/ref-again" -Digest
        $again | Wait-Process
        if (-not (Test-SameFile "$refBase.dig" "$dir/ref-again.dig")) { $failures += 'self-check (reference digest is not reproducible)' }
    }

    $results += [pscustomobject]@{
        Run    = $label
        Frames = $job.Frames
        Result = ($failures ? ('DIFFERS: ' + ($failures -join ', ')) : 'identical')
        Detail = $detail
    }
    Write-Host ("  {0,-52} {1}" -f $label, $results[-1].Result)
}

# ── 汇总 ────────────────────────────────────────────────────────────────

if ($Coverage) {
    $covDir = Join-Path $root 'build/tsa-coverage'
    $profraw = Get-ChildItem $outRoot -Recurse -Filter *.profraw | ForEach-Object FullName
    if (-not $profraw) { throw 'no coverage profiles were written' }
    Invoke-Native 'llvm-profdata' { llvm-profdata merge -sparse @profraw -o "$covDir/merged.profdata" }
    llvm-cov report $newExe "-instr-profile=$covDir/merged.profdata" | Set-Content "$covDir/files.txt"
    llvm-cov report $newExe "-instr-profile=$covDir/merged.profdata" -show-functions (Join-Path $root 'EGoTouchService/Solvers/TouchSolver') |
        Set-Content "$covDir/functions.txt"
    Write-Host "`n$($results.Count) runs. Per-file coverage: $covDir/files.txt, per-function: $covDir/functions.txt"
    Get-Content "$covDir/files.txt" | Select-Object -Last 1
    exit 0
}

$failed = @($results | Where-Object { $_.Result -ne 'identical' })
Write-Host ''
foreach ($f in $failed) {
    Write-Host "$($f.Run): $($f.Result)" -ForegroundColor Red
    if ($f.Detail) { Write-Host "  $($f.Detail)" }
}
$frames = ($results | Measure-Object Frames -Sum).Sum
if ($failed.Count -eq 0) {
    Write-Host "$($results.Count) runs, $frames frames: identical to $Reference" -ForegroundColor Green
    exit 0
}
Write-Host "$($failed.Count) of $($results.Count) runs differ from $Reference" -ForegroundColor Red
exit 1
