# yt-dlp and ffmpeg: the status bar's Tools part, the Settings dialog's Tools group and Check for updates, and yt-dlp
# installing while Add playlist waits for it. Dot-sourced by ui-checks.ps1.

function Get-SavedAutoUpdate {
	if (-not (Test-Path $SettingsFile)) { return 'no file' }
	(Get-Content $SettingsFile -Raw -Encoding UTF8 | ConvertFrom-Json).autoUpdateTools
}

# The Tools part's icon color and tooltip lines, matched against a regex
function Check-ToolsPart([string]$what, $hwnd, [string]$pattern) {
	$status = Get-StatusBar $hwnd
	$tip = [W]::TooltipText($status, $ToolsPart) -replace "`r`n", ' / '
	$actual = "$([W]::StatusIconColor($hwnd, $status, $ToolsPart)) | $tip"
	Check-That $what $actual ($actual -match "^$pattern$") $pattern
}

# Clicks Check for updates in the dialog and waits for the button to come back: it grays while the run goes
function Invoke-CheckForUpdates($dialog) {
	$button = [W]::GetDlgItem($dialog, 1005)
	[void][W]::PostMessageW($button, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	for ($i = 0; $i -lt 150; $i++) {
		Start-Sleep -Milliseconds 100
		if ($i -ge 2 -and [W]::IsWindowEnabled($button)) { return }
	}
	throw 'Check for updates did not end'
}

# The Tools group with the tools installed and Update tools automatically off, as the run starts them. Check for
# updates runs at once, even with the check box off; its look-ups fail, which leaves the tools' files as they were.
# The check box takes effect on OK, and is saved.
function Check-SettingsTools($hwnd) {
	$checked = Get-LastCheckedText
	Send-Command $hwnd '&Settings...'
	$dialog = Wait-OwnedWindow $hwnd
	Check 'Settings, tools installed' (Get-SettingsTools $dialog) "yt-dlp | 2026.10.21 | Installed / ffmpeg | n9.0 | Installed | auto 0 | $checked | check True"
	Invoke-CheckForUpdates $dialog
	$pattern = "yt-dlp \| 2026\.10\.21 \| Failed: $LookupFailure / ffmpeg \| n9\.0 \| Failed: $LookupFailure \| auto 0 \| $checked \| check True"
	$actual = Get-SettingsTools $dialog
	Check-That '  Check for updates, GitHub unreachable' $actual ($actual -match "^$pattern$") $pattern
	Check-ToolsPart '  Tools icon | tooltip' $hwnd "amber \| yt-dlp: $LookupFailure / ffmpeg: $LookupFailure"
	$kept = @('yt-dlp.exe', 'ffmpeg.exe', 'ffprobe.exe' | ForEach-Object { (Get-Item (Join-Path $ToolsDir $_)).Length }) -join ' '
	Check '  tools kept, no staging' "$kept $(Test-Path (Join-Path $ToolsDir '.staging'))" "$((Get-Item $FakeYtDlp).Length) 0 0 False"
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1004), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 200
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check '  checking Update tools automatically, then Cancel' (Get-SavedAutoUpdate) 'False'

	Send-Command $hwnd '&Settings...'
	$dialog = Wait-OwnedWindow $hwnd
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1004), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 200
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check '  then OK saves it' (Get-SavedAutoUpdate) 'True'
	Send-Command $hwnd '&Settings...'
	$dialog = Wait-OwnedWindow $hwnd
	Check '  and the dialog shows it' ((Get-SettingsTools $dialog) -replace '^.* \| (auto \d) \|.*$', '$1') 'auto 1'
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
}

"--- Run 1: no tools, GitHub unreachable"
$app = Start-App
$h = $app.Hwnd
Check-ToolsPart 'No tools, GitHub unreachable: Tools icon | tooltip' $h "amber \| yt-dlp: $LookupFailure / ffmpeg: $LookupFailure"
Stop-App $app

"--- Run 2: tools installed, not updated automatically"
Install-FakeTools
Set-SavedAutoUpdate $false
$app = Start-App
$h = $app.Hwnd
Check-ToolsPart 'Tools installed: Tools icon | tooltip' $h 'green \| yt-dlp 2026\.10\.21 / ffmpeg n9\.0'
$toolbar = ([W]::Children((Get-ToolbarStrip $h), 'ToolbarWindow32'))[0]
Check '  toolbar buttons: Sync all still waits for a playlist' ([W]::ToolbarButtons($toolbar)) 'Add playlist, Sync all v (disabled), Cancel (disabled), -, Settings'
Check-SettingsTools $h
Stop-App $app

"--- Run 3: yt-dlp installs while Add playlist waits for it"
Remove-Item $ToolsDir -Recurse -Force
$Web.YtDlp = [IO.File]::ReadAllBytes($FakeYtDlp)
$env:WESTIELIB_GITHUB_API = $Web.Url
$app = Start-App -DuringToolRun
$h = $app.Hwnd
Send-Command $h '&Add playlist...'
$dialog = Wait-OwnedWindow $h
Set-PlaylistLink $dialog $YouTubeLink
Check 'yt-dlp downloading: the line waits' (Get-LinkLine $dialog) "$WaitingForTools | Add False"
[void]$Web.AssetGate.Set()
for ($i = 0; $i -lt 100 -and (Get-LinkLine $dialog) -match '^(Waiting|Looking up)'; $i++) { Start-Sleep -Milliseconds 100 }
Start-Sleep -Milliseconds 100
Check '  installed: the look-up starts by itself' (Get-LinkLine $dialog) "$YouTubeFound | Add True"
[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
Wait-Closed $dialog
Wait-ToolsIdle $h
Check '  yt-dlp installed' (Get-Item (Join-Path $ToolsDir 'yt-dlp.exe')).Length (Get-Item $FakeYtDlp).Length
Stop-App $app
