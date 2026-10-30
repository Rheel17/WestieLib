# The unit checks of the core's pure logic, so far the Matcher's rules against real cases: WestieLibTests.exe, which
# the build puts next to WestieLib.exe

$tests = Join-Path (Split-Path $Exe) 'WestieLibTests.exe'
if (-not (Test-Path $tests)) {
	Check-That 'WestieLibTests.exe next to the exe' $tests $false 'built with WestieLib.exe'
	return
}
$output = @(& $tests)
$output | Where-Object { $_ -like 'FAIL *' }
$summary = $output | Select-Object -Last 1
Check-That 'Unit checks: none fail' $summary ($LASTEXITCODE -eq 0 -and $summary -match '^\d+ checks, 0 failure\(s\)$') 'no failures'
