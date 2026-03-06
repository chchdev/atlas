$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $repoRoot "build\atlas.exe"
if (-not (Test-Path $exe)) {
    throw "atlas executable not found at $exe. Build first."
}

$smoke = Join-Path $PSScriptRoot "smoke.atlas"
$output = & $exe $smoke

$expected = @(
    "15"
    "5"
    "50"
    "2"
    "3"
    "2"
    "1"
    "4"
    "true"
    "20"
    "object"
    "1"
    "text"
    "dictionary"
    "true"
    "9001"
    "true"
    "false"
    "77"
    "9"
    "2"
)

if ($output.Count -ne $expected.Count) {
    throw "Unexpected output count. Got $($output.Count), expected $($expected.Count)."
}

for ($i = 0; $i -lt $expected.Count; $i++) {
    if ($output[$i] -ne $expected[$i]) {
        throw "Mismatch at line $($i + 1): got '$($output[$i])', expected '$($expected[$i])'."
    }
}

"All Atlas tests passed."
