# Amalgamate qb-linq into single_header/linq.h (same API as #include <qb/linq.h>)
# Run from repo root: pwsh -File scripts/amalgamate_single_header.ps1

$ErrorActionPreference = "Stop"
$Repo = Resolve-Path (Join-Path $PSScriptRoot "..")

$Linq = Join-Path $Repo "include/qb/linq"
$Order = @(
    "detail/type_traits.h",
    "detail/order.h",
    "detail/group_by.h",
    "detail/lazy_copy_once.h",
    "detail/query_range.h",
    "iterators.h",
    "reverse_iterator.h",
    "detail/extra_views.h",
    "query.h",
    "enumerable.h"
)

$cm = Get-Content (Join-Path $Repo "CMakeLists.txt") -Raw -Encoding utf8
if ($cm -notmatch 'project\s*\(\s*qb-linq\s+VERSION\s+(\d+)\.(\d+)\.(\d+)') {
    throw "Could not parse project(qb-linq VERSION x.y.z)"
}
$major = [int]$Matches[1]; $minor = [int]$Matches[2]; $patch = [int]$Matches[3]
$ver = "$major.$minor.$patch"

$template = Get-Content (Join-Path $Repo "cmake/version.h.in") -Raw -Encoding utf8
$versionSrc = $template `
    -replace '@PROJECT_VERSION_MAJOR@', $major `
    -replace '@PROJECT_VERSION_MINOR@', $minor `
    -replace '@PROJECT_VERSION_PATCH@', $patch `
    -replace '@PROJECT_VERSION@', $ver

function Split-AmalgamSource([string]$source) {
    $sys = [System.Collections.Generic.List[string]]::new()
    $seen = @{}
    $sb = [System.Text.StringBuilder]::new()
    foreach ($line in $source -split "`r?`n", -1) {
        $t = $line.Trim()
        if ($t.StartsWith("#include")) {
            $rest = $t.Substring(8).Trim()
            if ($rest.StartsWith('"qb/linq')) { continue }
            if ($rest.StartsWith("<") -and $rest.EndsWith(">")) {
                if (-not $seen.ContainsKey($rest)) {
                    $seen[$rest] = $true
                    [void]$sys.Add("#include $rest")
                }
                continue
            }
            if ($rest.StartsWith('"')) { continue }
        }
        if ($t -eq "#pragma once") { continue }
        [void]$sb.AppendLine($line)
    }
    return @{ Includes = $sys; Body = $sb.ToString().TrimEnd() + "`n" }
}

$v = Split-AmalgamSource $versionSrc
$allInc = [System.Collections.Generic.List[string]]::new()
foreach ($x in $v.Includes) { [void]$allInc.Add($x) }

$out = [System.Collections.Generic.List[string]]::new()
[void]$out.Add("/**")
[void]$out.Add(" * @file linq.h")
[void]$out.Add(" * @brief Single-header qb-linq (same public API as ``#include <qb/linq.h>``).")
[void]$out.Add(" *")
[void]$out.Add(" * @note Do not edit by hand. Regenerate with:")
[void]$out.Add(" * ``python scripts/amalgamate_single_header.py`` or ``pwsh -File scripts/amalgamate_single_header.ps1`` or CMake target **qb_linq_single_header**.")
[void]$out.Add(" * @note Embedded version **$ver** from ``CMakeLists.txt`` ``project(VERSION ...)``.")
[void]$out.Add(" */")
[void]$out.Add("")
[void]$out.Add("#pragma once")
[void]$out.Add("")

$merged = @{}
foreach ($x in $allInc) { $merged[$x.ToLowerInvariant()] = $x }
foreach ($rel in $Order) {
    $raw = Get-Content (Join-Path $Linq $rel) -Raw -Encoding utf8
    $p = Split-AmalgamSource $raw
    foreach ($i in $p.Includes) { $merged[$i.ToLowerInvariant()] = $i }
}
foreach ($k in ($merged.Keys | Sort-Object)) { [void]$out.Add($merged[$k]) }
[void]$out.Add("")
[void]$out.Add("// ---- qb/linq/version.h (embedded) ----")
[void]$out.Add($v.Body.TrimEnd())
[void]$out.Add("")

foreach ($rel in $Order) {
    $raw = Get-Content (Join-Path $Linq $rel) -Raw -Encoding utf8
    $p = Split-AmalgamSource $raw
    [void]$out.Add("// ---- qb/linq/$rel ----")
    [void]$out.Add($p.Body.TrimEnd())
    [void]$out.Add("")
}

$destDir = Join-Path $Repo "single_header"
New-Item -ItemType Directory -Force -Path $destDir | Out-Null
$dest = Join-Path $destDir "linq.h"
$text = ($out -join "`n").TrimEnd() + "`n"
[System.IO.File]::WriteAllText($dest, $text, [System.Text.UTF8Encoding]::new($false))
Write-Host "Wrote $dest ($((Get-Item $dest).Length) bytes)"
