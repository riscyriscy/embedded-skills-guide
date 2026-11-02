# Reports flash and RAM use of an ELF file against the limits of the part.
# Usage: tools/size_report.ps1 <elf> [flash_bytes] [ram_bytes]
# Exit code 1 when a limit is exceeded, 2 on a usage error.
param(
    [Parameter(Mandatory = $true)][string]$Elf,
    [int]$FlashLimit = 262144,
    [int]$RamLimit = 65536
)
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $Elf)) {
    [Console]::Error.WriteLine("size_report.ps1: no such file: $Elf")
    exit 2
}

# Berkeley format: the second line holds text, data and bss.
$line = (& arm-none-eabi-size $Elf)[1].Trim() -split '\s+'
$text, $data, $bss = [int]$line[0], [int]$line[1], [int]$line[2]

$status = 0
function Show([string]$name, [int]$used, [int]$limit) {
    $verdict = if ($used -gt $limit) { $script:status = 1; 'OVER' } else { 'ok' }
    '{0,-6} {1,7} of {2,7} bytes {3,5:N1}%  {4}' -f $name, $used, $limit, (100 * $used / $limit), $verdict
}
Show 'flash' ($text + $data) $FlashLimit
Show 'ram' ($data + $bss) $RamLimit
exit $status
