# The Settings dialog, the library folder (the welcome screen's field, Browse, and Settings' Change...), and
# settings.json: what is saved when, and a file that doesn't read. Dot-sourced by ui-checks.ps1.

$HintNotFullPath = "Enter a full folder path, such as $DefaultLibrary."
$HintNoSuchDrive = "That drive isn't on this PC."
$HintNotAFolder = "That's a file, not a folder."
$HintWillBeCreated = "This folder doesn't exist yet. WestieLib creates it when it first syncs."
# Folders no test leaves behind: they're only typed, never created
$Unique = [guid]::NewGuid().ToString('N').Substring(0, 8)
$MissingFolder = "C:\WestieLib-check-$Unique"
$ClosedFolder = "C:\WestieLib-$([char]0x00FC)-closed-$Unique"

function Get-FolderHint($hwnd) { ([W]::Children([W]::GetAncestor((Get-FolderEdit $hwnd), 1), 'WestieLib.Label'))[0] }

# What the hint line says right after typing $text, before the field loses focus; whether it's red; and whether the
# line moved
function Get-HintFor($hwnd, [string]$text) {
	$hint = Get-FolderHint $hwnd
	$before = Format-Rect ([W]::RectIn($hint, $hwnd))
	[W]::SetEditText((Get-FolderEdit $hwnd), $text)
	Start-Sleep -Milliseconds 100
	"$([W]::TextOf($hint)) | red $([W]::RedInk($hwnd, $hint)) | moved $((Format-Rect ([W]::RectIn($hint, $hwnd))) -ne $before)"
}

# The field reads $field, and the status bar's library text is as wide as $folder in the message font (within the
# glyphs' overhang)
function Check-FolderInEffect([string]$what, $hwnd, [string]$field, [string]$folder) {
	$status = Get-StatusBar $hwnd
	$part = [W]::StatusPartRect($status, 2)
	$gaps = [W]::StatusTextGaps($hwnd, $status, 2)
	$width = $part.Right - $part.Left - $gaps[0] - $gaps[1]
	$expected = [W]::TextWidth($folder, $script:dpi)
	$text = [W]::EditText((Get-FolderEdit $hwnd))
	Check-That $what "$text | $width" ($text -eq $field -and [math]::Abs($width - $expected) -le 3) "$field | $expected within 3"
}

# Picks $folder in an open folder picker: its only visible edit is its Folder box, and its button for picking is IDOK.
# Returns how many Folder boxes it found; with any other count than 1 it cancels the picker instead.
function Select-InPicker($picker, [string]$folder) {
	Start-Sleep -Milliseconds 500
	$boxes = @([W]::Children($picker, 'Edit') | Where-Object { [W]::IsWindowVisible($_) })
	if ($boxes.Count -eq 1) {
		[W]::SetEditText($boxes[0], $folder)
		[void][W]::PostMessageW([W]::GetDlgItem($picker, 1), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	} else {
		[void][W]::PostMessageW($picker, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	}
	Wait-Closed $picker
	$boxes.Count
}

# The field starts at the default folder. The hint line follows the typing. Leaving the field takes a folder into
# effect, unless its hint is red. Browse works whatever the field says, and picking a folder takes effect at once.
function Check-LibraryFolder($hwnd) {
	$edit = Get-FolderEdit $hwnd
	$browse = ([W]::Children([W]::GetAncestor($edit, 1), 'Button'))[0]
	$music = [Environment]::GetFolderPath('MyMusic')
	Check-FolderInEffect 'Library folder: field | status bar text width' $hwnd $DefaultLibrary $DefaultLibrary
	$expected = if (Test-Path $DefaultLibrary -PathType Container) { '' } else { $HintWillBeCreated }
	Check '  hint for the default folder' ([W]::TextOf((Get-FolderHint $hwnd))) $expected

	$drive = @([char[]]'QRSTUVWXYZ' | Where-Object { -not (Test-Path "$($_):\") })[0]
	Check '  typing a relative path' (Get-HintFor $hwnd 'Music\WestieLib') "$HintNotFullPath | red True | moved False"
	Check '  typing a path on a missing drive' (Get-HintFor $hwnd "$($drive):\Music") "$HintNoSuchDrive | red True | moved False"
	Check '  typing the path of a file' (Get-HintFor $hwnd (Resolve-Path $Exe).Path) "$HintNotAFolder | red True | moved False"
	Check '  typing a folder that does not exist' (Get-HintFor $hwnd $MissingFolder) "$HintWillBeCreated | red False | moved False"
	Check '  typing a folder that exists' (Get-HintFor $hwnd $music) ' | red False | moved False'

	[void](Get-HintFor $hwnd 'Music\WestieLib')
	[void][W]::SendMessageW($edit, 0x8, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 200
	Check-FolderInEffect '  leaving it with a red hint keeps the folder' $hwnd 'Music\WestieLib' $DefaultLibrary

	# The click moves the focus out of the field, which commits it, so start with the focus in it
	[W]::Focus($edit)
	[void][W]::PostMessageW($browse, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	$picker = Wait-OwnedWindow $hwnd
	Check '  Browse with that text opens the picker' "$([W]::ClassOf($picker)) $([W]::TextOf($picker))" '#32770 Library folder'
	[void][W]::PostMessageW($picker, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 500
	Check-FolderInEffect '  and Cancel keeps the text and the folder' $hwnd 'Music\WestieLib' $DefaultLibrary

	[void][W]::PostMessageW($browse, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	$boxes = Select-InPicker (Wait-OwnedWindow $hwnd) $music
	Check-FolderInEffect "  picking a folder in the picker (Folder boxes: $boxes)" $hwnd $music $music
	Check '  and its hint' ([W]::TextOf((Get-FolderHint $hwnd))) ''

	Check '  typing a quoted folder that does not exist' (Get-HintFor $hwnd "  `"$MissingFolder\`"  ") "$HintWillBeCreated | red False | moved False"
	[void][W]::SendMessageW($edit, 0x8, [IntPtr]::Zero, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 200
	Check-FolderInEffect '  leaving it: the folder takes effect, cleaned up' $hwnd $MissingFolder $MissingFolder
}

# Tools > Settings, the toolbar's Settings button and Ctrl+Alt+S each open the Settings dialog, centered on the window,
# and Esc, Cancel and its close box each shut it. It shows the folder in effect, read-only, and the version.
function Check-SettingsDialog($hwnd, [string]$folder) {
	$strip = Get-ToolbarStrip $hwnd
	$toolbar = ([W]::Children($strip, 'ToolbarWindow32'))[0]
	$command = [IntPtr][int][W]::CommandOf([W]::GetMenu($hwnd), '&Settings...')
	$opens = @(
		@{ Name = 'Tools > Settings'; Open = { Send-Command $hwnd '&Settings...' } },
		@{ Name = 'the toolbar button'; Open = { [void][W]::PostMessageW($strip, 0x111, $command, $toolbar) } },
		@{ Name = 'Ctrl+Alt+S'; Open = { [W]::PostCtrlKey($hwnd, 0x53, $true) } }
	)
	$closes = @(
		@{ Name = 'Esc'; Close = { param($dialog) [void][W]::PostMessageW($dialog, 0x100, [IntPtr]0x1B, [IntPtr]1) } },
		@{ Name = 'Cancel'; Close = { param($dialog) [void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero) } },
		@{ Name = 'its close box'; Close = { param($dialog) [void][W]::PostMessageW($dialog, 0x10, [IntPtr]::Zero, [IntPtr]::Zero) } }
	)
	for ($i = 0; $i -lt $opens.Count; $i++) {
		& $opens[$i].Open
		$dialog = Wait-OwnedWindow $hwnd
		Check "$($opens[$i].Name) opens Settings" "$([W]::ClassOf($dialog)) $([W]::TextOf($dialog))" '#32770 Settings'
		if ($i -eq 0) {
			$d = New-Object W+RECT
			$o = New-Object W+RECT
			[void][W]::GetWindowRect($dialog, [ref]$d)
			[void][W]::GetWindowRect($hwnd, [ref]$o)
			$offset = "$(($d.Left + $d.Right) - ($o.Left + $o.Right)) $(($d.Top + $d.Bottom) - ($o.Top + $o.Bottom))"
			Check-That '  centered on the window (offset x2)' $offset ($offset -match '^-?[01] -?[01]$') '-1 to 1 each way'
			$edit = ([W]::Children($dialog, 'Edit'))[0]
			$readOnly = ([W]::GetWindowLongW($edit, -16) -band 0x800) -ne 0
			Check '  the folder in effect, read-only' "$([W]::EditText($edit)) $readOnly" "$folder True"
			$version = @([W]::Children($dialog, 'Static') | ForEach-Object { [W]::TextOf($_) } | Where-Object { $_ -like 'WestieLib *' })
			Check '  the version' ($version -join ', ') "WestieLib $((Get-Item $Exe).VersionInfo.ProductVersion)"
			Check '  starts on Change, the path not selected' "$([W]::TextOf([W]::FocusOf($dialog))) $([W]::SendMessageW($edit, 0xB0, [IntPtr]::Zero, [IntPtr]::Zero))" "&Change$([char]0x2026) 0"
			$pattern = "yt-dlp \|  \| Failed: $LookupFailure / ffmpeg \|  \| Failed: $LookupFailure \| auto 1 \| Never checked \| check True"
			$actual = Get-SettingsTools $dialog
			Check-That '  tools, none installed' $actual ($actual -match "^$pattern$") $pattern
			if ($Screenshot) { Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-settings.png") }
		}
		& $closes[$i].Close $dialog
		Wait-Closed $dialog
		Check "  $($closes[$i].Name) closes it" ([W]::IsWindow($dialog)) 'False'
	}
}

# Settings starts from a folder typed into the welcome field but not yet in effect. Change... opens the picker over the
# dialog, and a picked folder shows in the dialog at once but takes effect only on OK.
function Check-SettingsLibraryFolder($hwnd) {
	$music = [Environment]::GetFolderPath('MyMusic')
	$typed = "$MissingFolder-typed"
	$field = Get-FolderEdit $hwnd
	[W]::Focus($field)
	[W]::SetEditText($field, $typed)
	Send-Command $hwnd '&Settings...'
	$dialog = Wait-OwnedWindow $hwnd
	$edit = ([W]::Children($dialog, 'Edit'))[0]
	$change = [W]::GetDlgItem($dialog, 1001)
	Check 'Settings, with a folder typed into the field' ([W]::EditText($edit)) $typed

	[void][W]::PostMessageW($change, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	$picker = Wait-OwnedWindow $dialog
	Check '  Change opens the picker over the dialog' "$([W]::ClassOf($picker)) $([W]::TextOf($picker))" '#32770 Library folder'
	[void][W]::PostMessageW($picker, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $picker
	Check '  closing the picker keeps the folder' ([W]::EditText($edit)) $typed

	[void][W]::PostMessageW($change, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	$boxes = Select-InPicker (Wait-OwnedWindow $dialog) $music
	Check "  a picked folder shows (Folder boxes: $boxes)" ([W]::EditText($edit)) $music
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check-FolderInEffect '  Cancel: the typed folder stays in effect' $hwnd $typed $typed
	Check '  and in settings.json' (Get-SavedSettings) "version 1, $typed"

	Send-Command $hwnd '&Settings...'
	$dialog = Wait-OwnedWindow $hwnd
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1001), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	[void](Select-InPicker (Wait-OwnedWindow $dialog) $music)
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 1), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check-FolderInEffect '  OK: the picked folder takes effect' $hwnd $music $music
	Check '  and is saved' (Get-SavedSettings) "version 1, $music"
}

# settings.json's version and library folder, read as UTF-8
function Get-SavedSettings {
	if (-not (Test-Path $SettingsFile)) { return 'no file' }
	$json = Get-Content $SettingsFile -Raw -Encoding UTF8 | ConvertFrom-Json
	"version $($json.version), $($json.libraryPath)"
}

# Starts the app with a settings.json that isn't JSON: it shows the error instead of its window, exits with 1, and
# leaves the file as it was
function Check-BrokenSettings {
	$broken = '{"version": 1, "libraryPath": '
	[IO.File]::WriteAllText($SettingsFile, $broken)
	$process = Start-Process -FilePath $Exe -PassThru
	$box = [IntPtr]::Zero
	for ($i = 0; $i -lt 50 -and $box -eq [IntPtr]::Zero; $i++) {
		Start-Sleep -Milliseconds 100
		$box = [W]::WindowOfProcess($process.Id, '#32770')
	}
	$message = @([W]::Children($box, 'Static') | ForEach-Object { [W]::TextOf($_) } | Where-Object { $_ -like '*settings.json*' })
	$window = [W]::FindWindowW('WestieLib.MainWindow', [NullString]::Value) -ne [IntPtr]::Zero
	Check 'A broken settings.json: error, names the file, no window' "$([W]::TextOf($box)) $($message.Count) $window" 'WestieLib 1 False'
	[void][W]::PostMessageW($box, 0x10, [IntPtr]::Zero, [IntPtr]::Zero)
	if (-not $process.WaitForExit(5000)) { $process.Kill(); throw 'Process did not exit' }
	Check '  exit code, file kept' "$($process.ExitCode) $([IO.File]::ReadAllText($SettingsFile) -eq $broken)" '1 True'
}

"--- Run 1: no settings.json"
$app = Start-App
$h = $app.Hwnd
Check 'A start writes no settings.json' (Get-SavedSettings) 'no file'
Check-SettingsDialog $h $DefaultLibrary
Check '  and writes no settings.json' (Get-SavedSettings) 'no file'
Check-LibraryFolder $h
Check '  saved in settings.json' (Get-SavedSettings) "version 1, $MissingFolder"
Check-SettingsLibraryFolder $h
[W]::SetEditText((Get-FolderEdit $h), $ClosedFolder)
"File > Exit, with a folder typed but the field never left"
Stop-App $app -ExitCommand
Check '  the folder is saved, in UTF-8' (Get-SavedSettings) "version 1, $ClosedFolder"

"--- Run 2: the saved library folder"
$app = Start-App
$h = $app.Hwnd
Check-FolderInEffect 'Restored library folder' $h $ClosedFolder $ClosedFolder
[W]::SetEditText((Get-FolderEdit $h), 'Music\WestieLib')
"Closed with a relative path typed"
Stop-App $app
Check '  the relative path is not saved' (Get-SavedSettings) "version 1, $ClosedFolder"

"--- Run 3: started and closed without a change"
$written = (Get-Item $SettingsFile).LastWriteTimeUtc
$app = Start-App
$h = $app.Hwnd
Check 'Library folder still the saved one' ([W]::EditText((Get-FolderEdit $h))) $ClosedFolder
Stop-App $app
Check '  settings.json untouched without a change' ((Get-Item $SettingsFile).LastWriteTimeUtc -eq $written) 'True'

"--- Run 4: settings.json doesn't read"
Check-BrokenSettings
