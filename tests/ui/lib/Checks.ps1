# Shared by the suites: the Win32 calls and the fakes, the app's surroundings, and the helpers more than one suite
# uses. ui-checks.ps1 dot-sources it once, because Add-Type loads a type only once per process.

# A stale Visual Studio path in LIB makes Add-Type's C# compiler fail on a warning
$env:LIB = $null
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -Path (Join-Path $PSScriptRoot 'W.cs')
Add-Type -Path (Join-Path $PSScriptRoot 'FakeWeb.cs')

if (Get-Process WestieLib -ErrorAction SilentlyContinue) { throw 'WestieLib is running; close it before running the checks' }

# Physical pixels, not coordinates virtualized for a DPI-unaware PowerShell
[void][W]::SetThreadDpiAwarenessContext([IntPtr](-4))

$KeyPath = 'HKCU:\Software\WestieLib\Window'
$StateNames = 'Left', 'Top', 'Right', 'Bottom', 'Maximized', 'PlaylistsWidthDips', 'InspectorWidthDips', 'LogHeightDips', 'LogVisible'
# The app keeps its files here instead of %LOCALAPPDATA%\WestieLib, so the user's settings and tools stay untouched
$DataDir = Join-Path ([IO.Path]::GetTempPath()) "WestieLib-checks-$([guid]::NewGuid().ToString('N').Substring(0, 8))"
$env:WESTIELIB_DATA_DIR = $DataDir
# Nothing listens there, so every release look-up fails at once, and never leaves the PC
$env:WESTIELIB_GITHUB_API = 'http://127.0.0.1:9'
$LookupFailure = 'Couldn''t reach 127\.0\.0\.1: .+'
$SettingsFile = Join-Path $DataDir 'settings.json'
$ToolsDir = Join-Path $DataDir 'tools'
$FakeYtDlp = Join-Path $DataDir 'fakes\yt-dlp.exe'
New-Item -ItemType Directory -Force (Split-Path $FakeYtDlp) | Out-Null
Add-Type -Path (Join-Path $PSScriptRoot 'FakeYtDlp.cs') -OutputAssembly $FakeYtDlp -OutputType ConsoleApplication
$FakeYtDlpRuns = Join-Path $ToolsDir 'yt-dlp-runs.txt'
$Web = New-Object FakeWeb
$env:WESTIELIB_SPOTIFY_URL = $Web.Url
$Ellipsis = [char]0x2026
$script:failures = 0
$script:dpi = 96
$app = $null

function Check([string]$what, $actual, $expected) {
	$ok = "$actual" -eq "$expected"
	if (-not $ok) { $script:failures++ }
	"{0,-56} {1,-26} {2}" -f $what, $actual, $(if ($ok) { 'ok' } else { "FAIL (expected $expected)" })
}

# For checks that allow a range: $ok says whether $actual passes
function Check-That([string]$what, $actual, [bool]$ok, [string]$expected) {
	if (-not $ok) { $script:failures++ }
	"{0,-56} {1,-26} {2}" -f $what, $actual, $(if ($ok) { 'ok' } else { "FAIL (expected $expected)" })
}

# DIPs to pixels, rounding like MulDiv
function S([int]$dips) { [int][math]::Floor($dips * $script:dpi / 96 + 0.5) }

# Waits for the app's first tool run to end, unless told not to
function Start-App([switch]$DuringToolRun) {
	$process = Start-Process -FilePath $Exe -PassThru
	for ($i = 0; $i -lt 50; $i++) {
		Start-Sleep -Milliseconds 100
		$hwnd = [W]::FindWindowW('WestieLib.MainWindow', [NullString]::Value)
		if ($hwnd -ne [IntPtr]::Zero) { break }
	}
	if ($hwnd -eq [IntPtr]::Zero) { throw 'Main window not found' }
	Start-Sleep -Milliseconds 400
	$script:dpi = [W]::GetDpiForWindow($hwnd)
	if (-not $DuringToolRun) { Wait-ToolsIdle $hwnd }
	return @{ Process = $process; Hwnd = $hwnd }
}

# A tool run grays Check for updates until it ends. Against the checks' GitHub a look-up fails within a few seconds.
function Wait-ToolsIdle($hwnd) {
	$menu = [W]::GetMenu($hwnd)
	$id = [W]::CommandOf($menu, 'Check for &updates')
	for ($i = 0; $i -lt 150; $i++) {
		Update-MenuStates $hwnd
		if (([W]::GetMenuState($menu, $id, 0) -band 0x1) -eq 0) { return }
		Start-Sleep -Milliseconds 100
	}
	throw 'The tool run did not end'
}

# Closes with WM_CLOSE, or with File > Exit
function Stop-App($app, [switch]$ExitCommand) {
	if ($ExitCommand) { Send-Command $app.Hwnd 'E&xit' } else { [void][W]::PostMessageW($app.Hwnd, 0x10, [IntPtr]::Zero, [IntPtr]::Zero) }
	if (-not $app.Process.WaitForExit(5000)) { throw 'Process did not exit' }
	Check '  exit code' $app.Process.ExitCode 0
}

# Posts WM_COMMAND for the menu item with this text (without its shortcut)
function Send-Command($hwnd, [string]$item) {
	$id = [W]::CommandOf([W]::GetMenu($hwnd), $item)
	[void][W]::PostMessageW($hwnd, 0x111, [IntPtr][int]$id, [IntPtr]::Zero)
}

# The app sets menu item states in WM_INITMENUPOPUP, so send it for every popup before reading them
function Update-MenuStates($hwnd) {
	$menu = [W]::GetMenu($hwnd)
	for ($i = 0; $i -lt [W]::GetMenuItemCount($menu); $i++) {
		[void][W]::SendMessageW($hwnd, 0x117, [W]::GetSubMenu($menu, $i), [IntPtr]$i)
	}
}

function Wait-OwnedWindow($owner) {
	for ($i = 0; $i -lt 50; $i++) {
		$window = [W]::OwnedWindow($owner)
		if ($window -ne [IntPtr]::Zero) { return $window }
		Start-Sleep -Milliseconds 100
	}
	[IntPtr]::Zero
}

function Get-ClientWidth($hwnd) {
	$c = New-Object W+RECT
	[void][W]::GetClientRect($hwnd, [ref]$c)
	$c.Right
}

function Get-ClientHeight($hwnd) {
	$c = New-Object W+RECT
	[void][W]::GetClientRect($hwnd, [ref]$c)
	$c.Bottom
}

function Format-Rect($r) { "$($r.Left),$($r.Top) $($r.Right - $r.Left)x$($r.Bottom - $r.Top)" }

function Get-ToolbarStrip($hwnd) { ([W]::Children($hwnd, 'WestieLib.MainToolbar'))[0] }

function Get-StatusBar($hwnd) { ([W]::Children($hwnd, 'msctls_statusbar32'))[0] }

function Get-RowTop($hwnd) { [W]::RectIn((Get-ToolbarStrip $hwnd), $hwnd).Bottom }

# The main window's own panels; EnumChildWindows also returns the panels inside them
function Get-MainPanels($hwnd) { @([W]::Children($hwnd, 'WestieLib.Panel') | Where-Object { [W]::GetAncestor($_, 1) -eq $hwnd }) }

# The playlists, the main column (tracks or welcome) and the inspector: the visible panels whose top is the row's, by
# their left edge
function Get-RowPanels($hwnd) {
	$top = Get-RowTop $hwnd
	@(Get-MainPanels $hwnd | Where-Object { [W]::IsWindowVisible($_) -and [W]::RectIn($_, $hwnd).Top -eq $top })
}

# A panel of the main window by its name (its window text), shown or not
function Get-Panel($hwnd, [string]$name) { @(Get-MainPanels $hwnd | Where-Object { [W]::TextOf($_) -eq $name })[0] }

function Get-LogPanel($hwnd) { Get-Panel $hwnd 'Log' }

function Test-Upright($hwnd, $child) { $r = [W]::RectIn($child, $hwnd); ($r.Right - $r.Left) -lt ($r.Bottom - $r.Top) }

# The visible splitters between the panels, by their left edge
function Get-ColumnSplitters($hwnd) {
	@([W]::Children($hwnd, 'WestieLib.Splitter') | Where-Object { [W]::IsWindowVisible($_) -and (Test-Upright $hwnd $_) })
}

function Get-LogSplitter($hwnd) {
	@([W]::Children($hwnd, 'WestieLib.Splitter') | Where-Object { -not [W]::IsWindowVisible($_) -or -not (Test-Upright $hwnd $_) })[0]
}

function Get-LogVisibility($hwnd) { "$([W]::IsWindowVisible((Get-LogPanel $hwnd))) $([W]::IsWindowVisible((Get-LogSplitter $hwnd)))" }

# Bottom of the panel row: the log splitter while the log shows, else the status bar
function Get-RowBottom($hwnd) {
	$splitter = Get-LogSplitter $hwnd
	if ([W]::IsWindowVisible($splitter)) { [W]::RectIn($splitter, $hwnd).Top } else { [W]::RectIn((Get-StatusBar $hwnd), $hwnd).Top }
}

# Checks playlists | tracks | inspector widths (within $tolerance px), that the splitters fill the gaps exactly, and that
# all of them fill the height between the toolbar and the log or status bar
function Check-Widths([string]$what, $hwnd, [int[]]$expected, [int]$tolerance = 0) {
	$panels = @(Get-RowPanels $hwnd | ForEach-Object { [W]::RectIn($_, $hwnd) })
	$splitters = @(Get-ColumnSplitters $hwnd | ForEach-Object { [W]::RectIn($_, $hwnd) })
	$top = Get-RowTop $hwnd
	$bottom = Get-RowBottom $hwnd
	$offRow = @($panels + $splitters | Where-Object { $_.Top -ne $top -or $_.Bottom -ne $bottom })
	$contiguous = $panels[0].Left -eq 0 -and $panels[0].Right -eq $splitters[0].Left -and $splitters[0].Right -eq $panels[1].Left -and
		$panels[1].Right -eq $splitters[1].Left -and $splitters[1].Right -eq $panels[2].Left -and $panels[2].Right -eq (Get-ClientWidth $hwnd) -and
		$offRow.Count -eq 0
	$widths = @($panels | ForEach-Object { $_.Right - $_.Left })
	$near = $contiguous
	for ($i = 0; $i -lt 3; $i++) { if ([math]::Abs($widths[$i] - $expected[$i]) -gt $tolerance) { $near = $false } }
	$actual = "$($widths -join ' | ')$(if (-not $contiguous) { ' (gaps!)' })"
	if (-not $near) { $script:failures++ }
	"{0,-56} {1,-26} {2}" -f $what, $actual, $(if ($near) { 'ok' } else { "FAIL (expected $($expected -join ' | '))" })
}

# The toolbar strip spans the top and the status bar the bottom, each at its height
function Check-Bars([string]$what, $hwnd) {
	$cw = Get-ClientWidth $hwnd
	$ch = Get-ClientHeight $hwnd
	$strip = Format-Rect ([W]::RectIn((Get-ToolbarStrip $hwnd), $hwnd))
	$status = Format-Rect ([W]::RectIn((Get-StatusBar $hwnd), $hwnd))
	Check $what "$strip | $status" "0,0 $($cw)x$(S 34) | 0,$($ch - (S 23)) $($cw)x$(S 23)"
}

# Toolbar buttons: their height and position, the spacing, and room for their text in the font at the window's DPI
function Check-Toolbar([string]$what, $hwnd) {
	$strip = Get-ToolbarStrip $hwnd
	$toolbar = ([W]::Children($strip, 'ToolbarWindow32'))[0]
	$height = ([int64][W]::SendMessageW($toolbar, 0x43A, [IntPtr]::Zero, [IntPtr]::Zero)) -shr 16
	$r = [W]::RectIn($toolbar, $strip)
	Check "$what button height, left, top (centered)" "$height $($r.Left) $($r.Top)" "$(S 26) $(S 4) $([math]::Floor(((S 34) - 1 - (S 26)) / 2))"
	Check "$what spacing" ([W]::ToolbarSpacing($toolbar)) "gaps $(S 1) $(S 1) $(S 1) $(S 1) | separator $(S 11)"
	# Padding, icon and gap, plus a few pixels the toolbar keeps around the text itself. A button whose text was measured
	# in an older, smaller font falls short of this.
	$least = 2 * (S 8) + (S 16) + (S 5)
	$actual = [W]::ToolbarExtraWidths($toolbar, $script:dpi)
	$extras = @([regex]::Matches($actual, '\+(-?\d+)') | ForEach-Object { [int]$_.Groups[1].Value })
	$ok = $extras.Count -eq 3 -and @($extras | Select-Object -Unique).Count -eq 1 -and $extras[0] -ge $least -and $extras[0] -le $least + (S 4)
	Check-That "$what width beyond the text" $actual $ok "the same for each, $least to $($least + (S 4))"

	# Disabled Cancel's text against the same text enabled, for a moment: same place, same antialiasing
	$offset = (S 8) + (S 16) + (S 5)
	$disabled = [W]::ToolbarTextInk($hwnd, $toolbar, 2, $offset)
	$cancel = [IntPtr][int][W]::CommandOf([W]::GetMenu($hwnd), '&Cancel')
	[void][W]::SendMessageW($toolbar, 0x401, $cancel, [IntPtr]1)
	Start-Sleep -Milliseconds 200
	$enabled = [W]::ToolbarTextInk($hwnd, $toolbar, 2, $offset)
	[void][W]::SendMessageW($toolbar, 0x401, $cancel, [IntPtr]0)
	Check "$what disabled text, against enabled" $disabled $enabled
}

# The menu bar runs into the toolbar: no gray from its last row down to the client area. Windows redraws the bar's
# items as the keyboard moves through the menus, and in another color for an inactive window, so with -Menus this
# checks after that too. Posted keys reach the app's menu loop only, never another window.
function Check-MenuBarEdge([string]$what, $hwnd, [switch]$Menus) {
	Check $what ([W]::MenuBarEdgeColors($hwnd)) '#FFFFFF'
	if (-not $Menus) { return }
	$keys = @(@(0x112, 0xF100, 0), @(0x100, 0x27, 0), @(0x100, 0x1B, 0), @(0x112, 0xF100, 0x76), @(0x100, 0x1B, 0), @(0x100, 0x1B, 0))
	foreach ($key in $keys) {
		[void][W]::PostMessageW($hwnd, $key[0], [IntPtr]$key[1], [IntPtr]$key[2])
		Start-Sleep -Milliseconds 250
	}
	Check '  after F10, Right, Esc, Alt+V, Esc, Esc' ([W]::MenuBarEdgeColors($hwnd)) '#FFFFFF'
	[void][W]::SendMessageW($hwnd, 0x86, [IntPtr]::Zero, [IntPtr]::Zero)
	Check '  while the window looks inactive' ([W]::MenuBarEdgeColors($hwnd)) '#FFFFFF'
	[void][W]::SendMessageW($hwnd, 0x86, [IntPtr]1, [IntPtr]::Zero)
}

# Status bar parts from the left; 3 is the gap
$ToolsPart = 4
$VersionPart = 5

# A part's share of the bar's width, or its minimum in DIPs
function Get-PartWidth([int]$barWidth, [int]$minimum, [int]$percent) { [math]::Max((S $minimum), [math]::Floor($barWidth * $percent / 100)) }

# The Tools part is as wide as the widest thing it can say: the word, the icon, and what the tools are doing, with the
# padding and the gap the bar keeps between a part's edge and its rect
$ToolsWorkTexts = 'checking', 'checking for updates', 'installing yt-dlp 100%', 'updating yt-dlp 100%', 'installing ffmpeg 100%', 'updating ffmpeg 100%'
function Get-ToolsWidth($status) {
	$result = 0L
	$borders = [W]::SendForBuffer($status, 0x407, [IntPtr]::Zero, 12, [ref]$result)
	$word = [W]::TextWidth('Tools', $script:dpi) + (S 7) + (S 12)
	$widest = ($ToolsWorkTexts | ForEach-Object { $word + (S 7) + [W]::TextWidth($_, $script:dpi) } | Measure-Object -Maximum).Maximum
	[BitConverter]::ToInt32($borders, 8) + 2 * (S 8) + $widest
}

# Run, Tracks and Library from the left, each at its share of the bar or its minimum, then the gap, Tools, and Version
# running to the right edge; Library gives way when they don't fit. Version's rect stops where the size grip starts, or
# at the edge when the window is maximized and shows no grip. Separators follow every part but Version.
function Check-StatusParts([string]$what, $hwnd) {
	$status = Get-StatusBar $hwnd
	$cw = Get-ClientWidth $hwnd
	$left = @((Get-PartWidth $cw 250 22), (Get-PartWidth $cw 100 10), (Get-PartWidth $cw 150 36))
	$tools = Get-ToolsWidth $status
	$missing = ($left | Measure-Object -Sum).Sum - ($cw - $tools - (S 80))
	if ($missing -gt 0) { $left[2] = [math]::Max(0, $left[2] - $missing) }
	$edges = @()
	$right = 0
	foreach ($width in $left) { $right += $width; $edges += $right }
	$edges += [math]::Max($right, $cw - $tools - (S 80))
	$edges += $edges[-1] + $tools
	$parts = $edges
	$grip = $cw - [W]::StatusPartRect($status, $VersionPart).Right
	$fits = if ([W]::IsZoomed($hwnd)) { $grip -eq 0 } else { $grip -gt 0 -and $grip -le (S 20) }
	Check $what "$([W]::StatusParts($status)) | stops at grip: $fits" "$($parts -join ' ') -1 | stops at grip: True"

	$lines = @([W]::StatusSeparators($hwnd, $status))
	$matching = $lines.Count -eq $edges.Count
	for ($i = 0; $matching -and $i -lt $edges.Count; $i++) { $matching = [math]::Abs($lines[$i] - $edges[$i]) -le 2 }
	Check-That "$what, separators" ($lines -join ' ') $matching "one within 2 px of each of $($edges -join ' '), none after"
}

# Ready, the track count and the library folder start after their part's padding (glyphs may start a pixel or two
# later); Version is centered in its part
function Check-StatusTexts([string]$what, $hwnd) {
	$status = Get-StatusBar $hwnd
	$gaps = @(0, 1, 2 | ForEach-Object { [W]::StatusTextGaps($hwnd, $status, $_)[0] })
	$padded = @($gaps | Where-Object { $_ -ge (S 8) -and $_ -le (S 8) + 2 }).Count -eq 3
	Check-That "$what Ready's | tracks' | folder's left gap" ($gaps -join ' | ') $padded "$(S 8) to $((S 8) + 2) each"
	$version = [W]::StatusTextGaps($hwnd, $status, $VersionPart)
	Check-That "$what Version's left | right gap" "$($version[0]) | $($version[1])" ([math]::Abs($version[0] - $version[1]) -le 1 -and $version[0] -ge (S 8)) 'equal within 1, at least the padding'
}

# The left panel from the top: the All tracks row, the rule, "No playlists yet." and the Add playlist button, which is
# as wide as its padding, icon, gap and text in the font at the window's DPI
function Check-PlaylistPanel([string]$what, $hwnd) {
	$panel = (Get-RowPanels $hwnd)[0]
	$pw = Get-ClientWidth $panel
	$row = ([W]::Children($panel, 'WestieLib.AllTracksRow'))[0]
	$rowHeight = 2 + (S 5) + (S 18) + (S 16) + (S 6)
	Check "$what All tracks row" "$([W]::TextOf($row)) $(Format-Rect ([W]::RectIn($row, $panel)))" "All tracks $(S 3),$(S 4) $($pw - 2 * (S 3))x$rowHeight"
	Check "$what  selected" (Get-Selection $hwnd 0) 'All tracks'
	# Where the row's name starts: its inset, border, padding, glyph and gap
	$text = (S 3) + 1 + (S 7) + (S 16) + (S 6)
	$empty = ([W]::Children($panel, 'WestieLib.Label'))[0]
	$emptyTop = (S 4) + $rowHeight + (S 5) + 1 + (S 5) + (S 6)
	Check "$what  empty list" "$([W]::TextOf($empty)) $(Format-Rect ([W]::RectIn($empty, $panel)))" "No playlists yet. $text,$emptyTop $($pw - $text - (S 12))x$(S 16)"
	$button = ([W]::Children($panel, 'Button'))[0]
	$width = 2 * (S 10) + (S 16) + (S 5) + [W]::TextWidth('Add playlist', $script:dpi)
	$buttonTop = $emptyTop + (S 16) + (S 2) + (S 8)
	Check "$what  button" "$([W]::TextOf($button)) $(Format-Rect ([W]::RectIn($button, $panel)))" "Add playlist $text,$buttonTop $($width)x$(S 23)"
}

$WelcomeTexts = 'Welcome to WestieLib / WestieLib keeps your YouTube Music and Spotify playlists in sync with a folder of tagged audio files. ' +
	'Choose where the files go, then add your first playlist. / Library folder'
$DefaultLibrary = Join-Path ([Environment]::GetFolderPath('MyMusic')) 'WestieLib'

function Get-DirectChildren($parent, [string]$class) { @([W]::Children($parent, $class) | Where-Object { [W]::GetAncestor($_, 1) -eq $parent }) }

function Get-FolderEdit($hwnd) { ([W]::Children((Get-DirectChildren (Get-Panel $hwnd 'Welcome') 'WestieLib.Panel')[0], 'Edit'))[0] }

# The welcome screen takes the main column while the track panel hides. Its column is 430 wide (less when the panel is
# narrow) and centered both ways; from the top: icon, heading, text, label, the field (edit and Browse across the
# column, the hint line under them) and the default button.
function Check-Welcome([string]$what, $hwnd) {
	$panel = Get-Panel $hwnd 'Welcome'
	Check "$what in the main column, tracks hidden" "$((Get-RowPanels $hwnd)[1] -eq $panel) $([W]::IsWindowVisible((Get-Panel $hwnd 'Tracks')))" 'True False'
	$labels = @(Get-DirectChildren $panel 'WestieLib.Label' | Sort-Object { [W]::RectIn($_, $panel).Top })
	Check "$what  texts" (($labels | ForEach-Object { [W]::TextOf($_) }) -join ' / ') $WelcomeTexts

	$pw = Get-ClientWidth $panel
	$width = [math]::Min((S 430), $pw - 2 * (S 16))
	$heading = [W]::RectIn($labels[0], $panel)
	$label = [W]::RectIn($labels[2], $panel)
	$field = (Get-DirectChildren $panel 'WestieLib.Panel')[0]
	$fieldRect = [W]::RectIn($field, $panel)
	$button = (Get-DirectChildren $panel 'Button')[0]
	$buttonRect = [W]::RectIn($button, $panel)
	$top = $heading.Top - (S 12) - (S 40)
	# Twice the offset from the panel's center, so it stays whole
	$offset = ($top + $buttonRect.Bottom) - (Get-ClientHeight $panel)
	$actual = "$($heading.Left) $($heading.Right - $heading.Left) | $offset"
	Check-That "$what  column left, width | center offset x2" $actual ($actual -match "^$([math]::Floor(($pw - $width) / 2)) $width \| -?[012]$") "$([math]::Floor(($pw - $width) / 2)) $width | -2 to 2"

	$edit = ([W]::Children($field, 'Edit'))[0]
	$browse = ([W]::Children($field, 'Button'))[0]
	$editRect = [W]::RectIn($edit, $field)
	$browseRect = [W]::RectIn($browse, $field)
	$actual = "$(Format-Rect $editRect) | $([W]::TextOf($browse)) $($browseRect.Left - $editRect.Right) $($fieldRect.Right - $fieldRect.Left - $browseRect.Right)"
	$expected = "0,0 $($browseRect.Left - (S 8))x$(S 23) | Browse$([char]0x2026) $(S 8) 0"
	Check "$what  edit | Browse, gap, right" $actual $expected
	# The hint line keeps room for its longest hint, a line or more
	$hint = [W]::RectIn(([W]::Children($field, 'WestieLib.Label'))[0], $field)
	$hintHeight = $hint.Bottom - $hint.Top
	$actual = "$(Format-Rect $fieldRect) | $(Format-Rect $hint)"
	$expected = "$($heading.Left),$($label.Bottom + (S 4)) $($width)x$((S 23) + (S 4) + $hintHeight) | 0,$((S 23) + (S 4)) $($width)x$hintHeight"
	Check-That "$what  field | its hint line" $actual ($actual -eq $expected -and $hintHeight -ge (S 16)) "$expected, the hint at least $(S 16) tall"
	$style = [W]::GetWindowLongW($button, -16) -band 0xF
	Check "$what  button, default" "$([W]::TextOf($button)) $($buttonRect.Left) $($buttonRect.Top) $($style -eq 1)" "Add your first playlist $($heading.Left) $($fieldRect.Bottom + (S 16)) True"
}

function Wait-Closed($window) {
	for ($i = 0; $i -lt 30 -and [W]::IsWindow($window); $i++) { Start-Sleep -Milliseconds 100 }
	Start-Sleep -Milliseconds 300
}

$Invariant = [Globalization.CultureInfo]::InvariantCulture
$CheckedAt = [DateTime]::UtcNow.AddDays(-10)

# What Settings says for a check at $CheckedAt: the local day and month, and the year when it isn't this one
function Get-LastCheckedText {
	$local = $CheckedAt.ToLocalTime()
	$year = if ($local.Year -ne [DateTime]::Now.Year) { " $($local.Year)" } else { '' }
	"Last checked $($local.Day) $($local.ToString('MMM', $Invariant))$year"
}

# Installed tools as the app would leave them: tools.json, checked at $CheckedAt, and the executables: the fake yt-dlp,
# and empty files for ffmpeg, which nothing runs yet
function Install-FakeTools {
	New-Item -ItemType Directory -Force $ToolsDir | Out-Null
	Copy-Item $FakeYtDlp $ToolsDir
	foreach ($file in 'ffmpeg.exe', 'ffprobe.exe') { [IO.File]::WriteAllBytes((Join-Path $ToolsDir $file), [byte[]]@()) }
	$json = '{"version": 1, "checkedAt": "' + $CheckedAt.ToString("yyyy-MM-dd'T'HH:mm:ss'Z'", $Invariant) + '", "tools": {' +
		'"yt-dlp": {"release": "2026.10.21", "digest": "sha256:0"}, "ffmpeg": {"release": "n9.0", "digest": "sha256:0"}}}'
	[IO.File]::WriteAllText((Join-Path $ToolsDir 'tools.json'), $json)
}

# Sets settings.json's autoUpdateTools, in a new file when there's none
function Set-SavedAutoUpdate([bool]$on) {
	$json = if (Test-Path $SettingsFile) { Get-Content $SettingsFile -Raw -Encoding UTF8 | ConvertFrom-Json } else { [pscustomobject]@{ version = 1 } }
	$json | Add-Member autoUpdateTools $on -Force
	[IO.File]::WriteAllText($SettingsFile, ($json | ConvertTo-Json), (New-Object Text.UTF8Encoding $false))
}

# Saves a window state for the next start, as the app would: a normal window at 100,100, 1280x800 at 96 DPI, the
# default panel widths and the log hidden, with $values (names of the values, to DWORDs) instead
function Set-SavedWindow([hashtable]$values) {
	$state = [ordered]@{ Left = 100; Top = 100; Right = 1380; Bottom = 900; Maximized = 0; PlaylistsWidthDips = 250; InspectorWidthDips = 290; LogHeightDips = 168; LogVisible = 0 }
	foreach ($name in $values.Keys) { $state[$name] = $values[$name] }
	if (-not (Test-Path $KeyPath)) { New-Item $KeyPath -Force | Out-Null }
	foreach ($name in $state.Keys) { New-ItemProperty $KeyPath -Name $name -PropertyType DWord -Value $state[$name] -Force | Out-Null }
}

# Every suite starts as on a new PC: no app running, nothing saved, no tools, and the fakes as they start
function Reset-Checks {
	if ($script:app -and -not $script:app.Process.HasExited) {
		$script:app.Process.Kill()
		$script:app.Process.WaitForExit()
	}
	$script:app = $null
	Remove-Item 'HKCU:\Software\WestieLib' -Recurse -Confirm:$false -ErrorAction SilentlyContinue
	Get-ChildItem $DataDir -Exclude 'fakes' | Remove-Item -Recurse -Force
	$env:WESTIELIB_GITHUB_API = 'http://127.0.0.1:9'
	$Web.YtDlp = $null
	[void]$Web.AssetGate.Reset()
	$script:dpi = 96
}

# The Settings dialog's Tools group: the list's rows, the check box, Last checked, and whether Check for updates is
# enabled
function Get-SettingsTools($dialog) {
	$list = [W]::GetDlgItem($dialog, 1003)
	$count = [int][W]::SendMessageW($list, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
	$rows = @(0..($count - 1) | ForEach-Object { [W]::ListViewRow($list, $_) }) -join ' / '
	$auto = [W]::SendMessageW([W]::GetDlgItem($dialog, 1004), 0xF0, [IntPtr]::Zero, [IntPtr]::Zero)
	"$rows | auto $auto | $([W]::TextOf([W]::GetDlgItem($dialog, 1006))) | check $([W]::IsWindowEnabled([W]::GetDlgItem($dialog, 1005)))"
}

$StateFile = Join-Path $DataDir 'state.json'
$SpotifyId = '7mKq2pLxV9nT4cR1sY8wZa'
$SpotifyLink = "https://open.spotify.com/playlist/$SpotifyId"
$SpotifyName = "Late Night Caf$([char]0xE9)"
$SlowSpotifyId = '3k9QeTqH2wVfXbN7RjL0aC'
$MissingSpotifyId = '0aaaaaaaaaaaaaaaaaaaaa'
$BrokenSpotifyId = '0bbbbbbbbbbbbbbbbbbbbb'
$Web.SpotifyNames[$SpotifyId] = $SpotifyName
$Web.SpotifyNames[$SlowSpotifyId] = 'Sunday Social'
[void]$Web.SlowIds.Add($SlowSpotifyId)
[void]$Web.BrokenIds.Add($BrokenSpotifyId)
$YouTubeLink = 'https://music.youtube.com/playlist?list=PLx9k2_-Ab'
$YouTubeName = "Caf$([char]0xE9) Check Mix"
$NotSupported = 'Not a supported playlist link'
$WaitingForTools = "Waiting for required tools to be installed$Ellipsis"
$Unavailable = 'This playlist doesn''t exist or is private.'
$SpotifyFound = "$SpotifyName Spotify playlist"
$YouTubeFound = "$YouTubeName YouTube Music playlist"

# The line under the link, then the Add button's text and whether it is enabled
function Get-LinkLine($dialog) {
	$add = [W]::GetDlgItem($dialog, 1)
	"$([W]::TextOf([W]::GetDlgItem($dialog, 1101))) | $([W]::TextOf($add)) $([W]::IsWindowEnabled($add))"
}

function Set-PlaylistLink($dialog, [string]$link) {
	[W]::SetEditText([W]::GetDlgItem($dialog, 1100), $link)
	Start-Sleep -Milliseconds 50
}

# The line once the look-up has answered; it starts 300 ms after the last change. The answer sets the line, then the
# button, so this reads them again after a moment.
function Wait-LinkLine($dialog) {
	for ($i = 0; $i -lt 100 -and (Get-LinkLine $dialog) -like 'Looking up*'; $i++) { Start-Sleep -Milliseconds 100 }
	Start-Sleep -Milliseconds 100
	Get-LinkLine $dialog
}

function Get-PlaylistList($hwnd) { ([W]::Children((Get-RowPanels $hwnd)[0], 'WestieLib.PlaylistList'))[0] }

# Border, padding, the name line, three lines under it, padding, border
function Get-PlaylistRowHeight { 2 + (S 5) + (S 18) + 3 * (S 16) + (S 6) }

# Where the Add playlist button starts in the list, scrolled to the top, under $count rows 1 apart
function Get-ListButtonTop([int]$count) { (S 1) + $count * ((Get-PlaylistRowHeight) + (S 1)) + (S 8) }

function Get-AllTracksRow($hwnd) { ([W]::Children((Get-RowPanels $hwnd)[0], 'WestieLib.AllTracksRow'))[0] }

# Where row $index (from 1) starts in the list, as it is scrolled now
function Get-PlaylistRowTop($hwnd, [int]$index) {
	(S 1) + ($index - 1) * ((Get-PlaylistRowHeight) + (S 1)) - [W]::GetScrollPos((Get-PlaylistList $hwnd), 1)
}

# What paints the selection box, sampled just inside each box's top-left corner: All tracks, and the rows from 1 of the
# $count in the list that are in view
function Get-Selection($hwnd, [int]$count) {
	$list = Get-PlaylistList $hwnd
	$height = Get-ClientHeight $list
	$selected = @()
	if (([W]::PixelsIn($hwnd, (Get-AllTracksRow $hwnd), @((S 4), (S 4))))[0] -eq 'cce8ff') { $selected += 'All tracks' }
	$rows = @(if ($count -gt 0) { 1..$count | Where-Object { $y = (Get-PlaylistRowTop $hwnd $_) + 3; $y -ge 0 -and $y -lt $height } })
	$points = @($rows | ForEach-Object { (S 3) + 3; (Get-PlaylistRowTop $hwnd $_) + 3 })
	if ($rows.Count -gt 0) {
		$colors = [W]::PixelsIn($hwnd, $list, [int[]]$points)
		for ($i = 0; $i -lt $rows.Count; $i++) { if ($colors[$i] -eq 'cce8ff') { $selected += "row $($rows[$i])" } }
	}
	if ($selected.Count -eq 0) { 'none' } else { $selected -join ', ' }
}

# A state.json with $count playlists (at most 31), added a day apart: Spotify and YouTube Music in turn, every third with a name
# too long for its row
function Write-ManyPlaylists([int]$count) {
	$entries = 1..$count | ForEach-Object {
		$key = if ($_ % 2) { "spotify:P$('{0:D21}' -f $_)" } else { "ytm:PLcheck$_" }
		$url = if ($_ % 2) { "https://open.spotify.com/playlist/$($key.Substring(8))" } else { "https://music.youtube.com/playlist?list=$($key.Substring(4))" }
		$name = if ($_ % 3 -eq 0) { "Playlist $_ with a name too long to fit in its row" } else { '' }
		$added = '2026-01-{0:D2}T12:00:00Z' -f $_
		"`"$key`": {`"url`": `"$url`", `"name`": `"$name`", `"addedAt`": `"$added`"}"
	}
	[IO.File]::WriteAllText($StateFile, '{"version": 1, "playlists": {' + ($entries -join ', ') + '}}')
}

# The inspector's own label; the cards in it have labels of their own
function Get-InspectorPlaceholder($hwnd) { (Get-DirectChildren (Get-Panel $hwnd 'Inspector') 'WestieLib.Label')[0] }

# The inspector's visible card
function Get-InspectorCard($hwnd) {
	@(Get-DirectChildren (Get-Panel $hwnd 'Inspector') 'WestieLib.Panel' | Where-Object { [W]::IsWindowVisible($_) })[0]
}

# The inspector's placeholder fills the panel's width inside the padding, its center within a pixel of the padded
# area's
function Check-InspectorPlaceholder([string]$what, $hwnd) {
	$panel = (Get-RowPanels $hwnd)[2]
	$label = Get-InspectorPlaceholder $hwnd
	$r = [W]::RectIn($label, $panel)
	$sides = "$($r.Left) $((Get-ClientWidth $panel) - $r.Right)"
	# Twice the offset, so it stays whole
	$offset = ($r.Top + $r.Bottom) - ((S 12) + (Get-ClientHeight $panel) - (S 10))
	$text = [W]::TextOf($label) -replace "`n", ' / '
	$ok = $text -eq 'Add a playlist to see / its tracks here.' -and $sides -eq "$(S 12) $(S 12)" -and [math]::Abs($offset) -le 2
	Check-That $what "$text | $sides | $offset" $ok "the text | $(S 12) $(S 12) | -2 to 2"
}

function LParam([int]$x, [int]$y) { [IntPtr]([int64]($y -band 0xFFFF) * 65536 + ($x -band 0xFFFF)) }

function Get-Splitter($hwnd, [int]$index) { (Get-ColumnSplitters $hwnd)[$index] }

function Set-Bounds($hwnd, $x, $y, $w, $h) {
	[void][W]::SetWindowPos($hwnd, [IntPtr]::Zero, $x, $y, $w, $h, 0x4 -bor 0x10)
}

function Get-Bounds($hwnd) {
	$r = New-Object W+RECT
	[void][W]::GetWindowRect($hwnd, [ref]$r)
	"$($r.Left),$($r.Top) $($r.Right - $r.Left)x$($r.Bottom - $r.Top)"
}

function Save-Screenshot($hwnd, $path) {
	$r = New-Object W+RECT
	[void][W]::GetWindowRect($hwnd, [ref]$r)
	$frame = New-Object W+RECT
	[void][W]::DwmGetWindowAttribute($hwnd, 9, [ref]$frame, 16)
	$bitmap = New-Object System.Drawing.Bitmap ($r.Right - $r.Left), ($r.Bottom - $r.Top)
	$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
	$hdc = $graphics.GetHdc()
	[void][W]::PrintWindow($hwnd, $hdc, 2)
	$graphics.ReleaseHdc($hdc)
	$graphics.Dispose()
	$crop = New-Object System.Drawing.Rectangle ($frame.Left - $r.Left), ($frame.Top - $r.Top), ($frame.Right - $frame.Left), ($frame.Bottom - $frame.Top)
	$cropped = $bitmap.Clone($crop, $bitmap.PixelFormat)
	$cropped.Save($path)
	$cropped.Dispose()
	$bitmap.Dispose()
	"Screenshot: $path"
}
