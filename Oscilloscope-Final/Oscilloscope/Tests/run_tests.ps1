$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$testsRoot = $PSScriptRoot
$compiler = Get-Command gcc -ErrorAction Stop

function Invoke-HostTest {
    param(
        [string]$Name,
        [string[]]$Sources
    )

    $executable = Join-Path $testsRoot "$Name.exe"
    try {
        & $compiler.Source '-std=c99' '-Wall' '-Wextra' '-Werror' '-I' (Join-Path $projectRoot 'Middle') @Sources '-o' $executable
        if ($LASTEXITCODE -ne 0) {
            throw "FAIL: $Name did not compile"
        }
        & $executable
        if ($LASTEXITCODE -ne 0) {
            throw "FAIL: $Name failed"
        }
    }
    finally {
        Remove-Item -LiteralPath $executable -Force -ErrorAction SilentlyContinue
    }
}

Invoke-HostTest 'test_scope_math' @(
    (Join-Path $testsRoot 'test_scope_math.c'),
    (Join-Path $projectRoot 'Middle\scope_math.c')
)
Invoke-HostTest 'test_timer_math' @(
    (Join-Path $testsRoot 'test_timer_math.c'),
    (Join-Path $projectRoot 'Middle\timer_math.c')
)
& (Join-Path $testsRoot 'test_firmware_contract.ps1')

'all tests passed'
