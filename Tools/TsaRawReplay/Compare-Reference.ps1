#requires -Version 7
<#
.SYNOPSIS
  判断当前工作区的 TSACore 与参考版本在算法上是否等价。

.DESCRIPTION
  把同一批录制分别喂给两份 TsaRawReplay：一份从参考提交（默认是标签 tsa-reference）构建，一份从当前
  工作区构建。每份录制跑若干场景（纯回放、充电器、长时间静止、注入模拟笔、经 StylusPipeline 的真实笔），比较三样东西：

    触点 CSV        每帧上报的触点（数量、ID、事件、坐标）
    stderr          闭环请求 / AFE 动作、笔状态等逐帧诊断
    状态摘要        每喂一帧之后 TSACore 全部算法状态（镜像 + 堆缓冲）的摘要

  三样全部逐字节相同才算通过。状态摘要不同的时候，会在第一次出现差异的那一帧把两边的完整状态导出来，
  按符号名列出不同的字段。重构 TSACore 的每一步都应当跑它。

  参考版本检出在 build/tsa-reference/<提交> 下（git worktree），只在第一次用到时构建。

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

.EXAMPLE
  pwsh Tools/TsaRawReplay/Compare-Reference.ps1
  pwsh Tools/TsaRawReplay/Compare-Reference.ps1 -Recordings C:\ProgramData\EGoTouchRev\exports\dvr, D:\rec\pen.dvrbin -SelfCheck
  pwsh Tools/TsaRawReplay/Compare-Reference.ps1 -Coverage
#>
[CmdletBinding()]
param(
    [string]$Reference = 'tsa-reference',
    [string]$Preset = 'arm64-Debug-TsaCore',
    [string[]]$Recordings = @('C:\ProgramData\EGoTouchRev\exports\dvr'),
    [string]$Only = '',
    [switch]$SelfCheck,
    [switch]$Coverage
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
    param([string]$Exe, [string]$Recording, [object[]]$ReplayArgs, [string]$OutBase, [string]$ProfileFile = '')
    $argList = @("`"$Recording`"") + ($ReplayArgs | ForEach-Object { "$_" }) + @('--state-digest', "`"$OutBase.dig`"")
    if ($ProfileFile) { $env:LLVM_PROFILE_FILE = $ProfileFile }
    try {
        Start-Process -FilePath $Exe -ArgumentList $argList -NoNewWindow -PassThru `
            -RedirectStandardOutput "$OutBase.csv" -RedirectStandardError "$OutBase.err"
    } finally {
        if ($ProfileFile) { Remove-Item Env:LLVM_PROFILE_FILE }
    }
}

function Test-SameFile {
    param([string]$A, [string]$B)
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
    $list
}

# ── 构建 ────────────────────────────────────────────────────────────────

Write-Host "building current tree ($Preset)..."
Push-Location $root
try {
    if ($Coverage) {
        $covDir = Join-Path $root 'build/tsa-coverage'
        Invoke-Native 'configure coverage build' {
            cmake --preset $Preset -B $covDir '-DEGO_TSA_RAW_EXTRA_FLAGS=-fprofile-instr-generate -fcoverage-mapping'
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

$results = @()
foreach ($recording in Get-RecordingFiles) {
    $recName = [System.IO.Path]::GetFileNameWithoutExtension($recording)
    $info = Get-RecordingInfo $newExe $recording
    if ($info.Frames -eq 0) {
        Write-Warning "$recName has no frames, skipped"
        continue
    }
    foreach ($scenario in Get-Scenarios $info) {
        $label = "$recName/$($scenario.Name)"
        if ($Only -and $label -notlike "*$Only*") { continue }
        $dir = Join-Path $outRoot $label
        New-Item $dir -ItemType Directory -Force | Out-Null

        if ($Coverage) {
            $run = Start-Replay $newExe $recording $scenario.Args "$dir/new" "$dir/run.profraw"
            $run | Wait-Process
            $results += [pscustomobject]@{ Run = $label; Frames = $info.Frames; Result = ($run.ExitCode -eq 0 ? 'ran' : "exit $($run.ExitCode)") }
            continue
        }

        $refRun = Start-Replay $refExe $recording $scenario.Args "$dir/ref"
        $newRun = Start-Replay $newExe $recording $scenario.Args "$dir/new"
        $refRun, $newRun | Wait-Process
        $failures = @()
        if ($refRun.ExitCode -ne 0) { $failures += "reference exit $($refRun.ExitCode)" }
        if ($newRun.ExitCode -ne 0) { $failures += "current exit $($newRun.ExitCode)" }
        if (-not $failures) {
            foreach ($kind in 'csv', 'err', 'dig') {
                if (-not (Test-SameFile "$dir/ref.$kind" "$dir/new.$kind")) { $failures += $kind }
            }
        }

        $detail = ''
        if ($failures -contains 'csv' -or $failures -contains 'err') {
            $kind = ($failures -contains 'csv') ? 'csv' : 'err'
            $d = Get-FirstDifference "$dir/ref.$kind" "$dir/new.$kind"
            $detail = "$kind line $($d.Line):`n    ref: $($d.A)`n    new: $($d.B)"
        }
        if ($failures -contains 'dig') {
            $d = Get-FirstDifference "$dir/ref.dig" "$dir/new.dig"
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
            $again = Start-Replay $refExe $recording ($scenario.Args + @('--heap-skew', 3000)) "$dir/ref-again"
            $again | Wait-Process
            if (-not (Test-SameFile "$dir/ref.dig" "$dir/ref-again.dig")) { $failures += 'self-check (reference digest is not reproducible)' }
        }

        $results += [pscustomobject]@{
            Run    = $label
            Frames = $info.Frames
            Result = ($failures ? ('DIFFERS: ' + ($failures -join ', ')) : 'identical')
            Detail = $detail
        }
        Write-Host ("  {0,-52} {1}" -f $label, $results[-1].Result)
    }
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
