# Drives WestieLib.exe through Win32 messages and checks it, in suites that each start the app a few times:
#
#   window        DPI awareness, icons, menus, the toolbar and status bar, the side panels' first-run content, sizes,
#                 splitter drags and limits, the log pane, and saving and restoring the window state
#   settings      the Settings dialog, the library folder, and settings.json
#   tools         the Tools part, the Settings dialog's Tools group, and installing yt-dlp
#   add-playlist  the Add playlist dialog and its look-ups
#   playlists     adding, keeping, selecting, showing, removing and scrolling playlists, and state.json
#   tracks        the track panel: filter bar, status tabs, result line and the table's columns
#   sync          syncing YouTube Music and Spotify playlists into state.json and the track table, and matching
#   choose-video  the Choose a video dialog: its candidates, searches and links, and the video it sets
#   painting      painting and blinking during splitter drags
#   dpi           moving to a monitor with another DPI, and restoring there
#
# A fake yt-dlp and a local stand-in for Spotify, YouTube Music's search and GitHub answer the look-ups, so nothing
# reaches the network.
#
#   powershell -NoProfile -ExecutionPolicy Bypass -File tests/ui/ui-checks.ps1 -Exe <WestieLib.exe> [-Suite <name>,...] [-Screenshot <png>]
#
# Without -Suite it runs them all; with it, the ones named, in that order. Each suite starts from nothing saved and sets
# up what it needs, so any of them runs alone. The exit code is the number of failed checks. The user's saved window
# state is set aside and put back afterwards. The app's files go to a scratch folder (WESTIELIB_DATA_DIR), which is
# deleted at the end.

param(
	[Parameter(Mandatory)] [string]$Exe,
	[string[]]$Suite,
	[string]$Screenshot
)

$ErrorActionPreference = 'Stop'
$Suites = 'window', 'settings', 'tools', 'add-playlist', 'playlists', 'tracks', 'sync', 'choose-video', 'painting', 'dpi'
# powershell -File passes "a,b" as one string
$chosen = if ($Suite) { @($Suite -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ }) } else { $Suites }
$unknown = @($chosen | Where-Object { $_ -notin $Suites })
if ($unknown.Count -gt 0) { throw "No suite $($unknown -join ', '); the suites are $($Suites -join ', ')" }

. (Join-Path $PSScriptRoot 'lib\Checks.ps1')

$userState = if (Test-Path $KeyPath) { Get-ItemProperty $KeyPath } else { $null }
Remove-Item 'HKCU:\Software\WestieLib' -Recurse -Confirm:$false -ErrorAction SilentlyContinue

$results = @()
try {
	foreach ($name in $chosen) {
		"=== Suite $name"
		$before = $script:failures
		# A suite that stops counts as a failure; the next one starts afresh anyway
		try {
			Reset-Checks
			. (Join-Path $PSScriptRoot "suites\$name.ps1")
		} catch {
			$script:failures++
			"FAIL: the suite stopped at $([IO.Path]::GetFileName($_.InvocationInfo.ScriptName)):$($_.InvocationInfo.ScriptLineNumber): $_"
		}
		$results += '{0,-14} {1} failure(s)' -f $name, ($script:failures - $before)
	}
} finally {
	if ($app -and -not $app.Process.HasExited) { $app.Process.Kill() }
	$Web.Stop()
	Remove-Item 'HKCU:\Software\WestieLib' -Recurse -Confirm:$false -ErrorAction SilentlyContinue
	if ($userState) {
		New-Item $KeyPath -Force | Out-Null
		foreach ($name in $StateNames) {
			if ($null -ne $userState.$name) { New-ItemProperty $KeyPath -Name $name -PropertyType DWord -Value $userState.$name | Out-Null }
		}
		"Put the user's saved window state back"
	}
	Remove-Item $DataDir -Recurse -Force -ErrorAction SilentlyContinue
}

$results
"--- $($script:failures) failure(s)"
exit $script:failures
