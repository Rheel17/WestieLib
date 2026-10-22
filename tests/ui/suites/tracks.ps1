# The track panel, before it holds tracks: the filter bar, the status filter, the result line and the table's columns.
# Dot-sourced by ui-checks.ps1.

function Get-FilterBar($hwnd) { (Get-DirectChildren (Get-TrackPanel $hwnd) 'WestieLib.Panel')[0] }

function Get-SearchField($hwnd) { ([W]::Children((Get-FilterBar $hwnd), 'Edit'))[0] }

function Get-SourceBox($hwnd) { ([W]::Children((Get-FilterBar $hwnd), 'ComboBox'))[0] }

function Get-ClearButton($hwnd) { ([W]::Children((Get-FilterBar $hwnd), 'Button'))[0] }

function Get-SelectedTab($hwnd) { [int][W]::SendMessageW((Get-StatusTabs $hwnd), 0x130B, [IntPtr]::Zero, [IntPtr]::Zero) }

function Get-SelectedSource($hwnd) { [int][W]::SendMessageW((Get-SourceBox $hwnd), 0x147, [IntPtr]::Zero, [IntPtr]::Zero) }

# The search text, the source, the tab, and whether Clear is enabled
function Get-Filters($hwnd) {
	"'$([W]::EditText((Get-SearchField $hwnd)))' source $(Get-SelectedSource $hwnd) tab $(Get-SelectedTab $hwnd) | Clear $([W]::IsWindowEnabled((Get-ClearButton $hwnd)))"
}

function Click-Tab($hwnd, [int]$index) {
	$tabs = Get-StatusTabs $hwnd
	$r = [W]::SendForRect($tabs, 0x130A, [IntPtr]$index)
	Send-Click $tabs ([int](($r.Left + $r.Right) / 2)) ([int](($r.Top + $r.Bottom) / 2))
}

# Clicks a header by its place in the display order
function Click-Header($hwnd, [int]$position) {
	$header = [W]::SendMessageW((Get-TrackTable $hwnd), 0x101F, [IntPtr]::Zero, [IntPtr]::Zero)
	$index = [W]::SendMessageW($header, 0x120F, [IntPtr]$position, [IntPtr]::Zero)
	$r = [W]::SendForRect($header, 0x1207, $index)
	Send-Click $header ([int](($r.Left + $r.Right) / 2)) ([int](($r.Top + $r.Bottom) / 2))
}

# Each tab is as wide as its text and 8 DIPs at either side
function Get-TabPadding($hwnd) {
	$tabs = Get-StatusTabs $hwnd
	$texts = [W]::TabTexts($tabs) -split ' \| '
	$extras = for ($i = 0; $i -lt $texts.Count; $i++) {
		$r = [W]::SendForRect($tabs, 0x130A, [IntPtr]$i)
		($r.Right - $r.Left) - [W]::TextWidth($texts[$i], $script:dpi)
	}
	$extras -join ' '
}

# Two clicks quick enough for a double click: down, up, the double click's down, up
function Double-Click-Header($hwnd, [int]$position) {
	$header = [W]::SendMessageW((Get-TrackTable $hwnd), 0x101F, [IntPtr]::Zero, [IntPtr]::Zero)
	$index = [W]::SendMessageW($header, 0x120F, [IntPtr]$position, [IntPtr]::Zero)
	$r = [W]::SendForRect($header, 0x1207, $index)
	$point = LParam ([int](($r.Left + $r.Right) / 2)) ([int](($r.Top + $r.Bottom) / 2))
	foreach ($message in 0x201, 0x202, 0x203, 0x202) {
		[void][W]::SendMessageW($header, $message, [IntPtr]$(if ($message -eq 0x202) { 0 } else { 1 }), $point)
	}
}

function Get-Columns($hwnd) { [W]::ListViewColumns((Get-TrackTable $hwnd)) }

function Get-AllTracksColumns { "Status $(S 56) | Title $(S 204) ^ | Artists $(S 156) | Album $(S 132) | Playlists $(S 114) | Time $(S 48)" }

# From the top: the filter bar (search, Source, the drop-down, Clear), the status tabs, the result line, the rule and
# the table to the bottom
function Check-TrackLayout([string]$what, $hwnd) {
	$panel = Get-TrackPanel $hwnd
	$pw = Get-ClientWidth $panel
	$ph = Get-ClientHeight $panel
	$bar = Get-FilterBar $hwnd
	$barHeight = (S 8) + (S 23) + (S 4)
	Check "$what filter bar" (Format-Rect ([W]::RectIn($bar, $panel))) "0,0 $($pw)x$barHeight"

	$label = ([W]::Children($bar, 'WestieLib.Label'))[0]
	$labelWidth = [W]::TextWidth('Source', $script:dpi)
	$clearWidth = 2 * (S 9) + [W]::TextWidth('Clear', $script:dpi)
	$search = [math]::Min((S 300), $pw - 2 * (S 8) - 3 * (S 8) - $labelWidth - (S 128) - $clearWidth)
	$labelLeft = (S 8) + $search + (S 8)
	$sourceLeft = $labelLeft + $labelWidth + (S 8)
	$clearLeft = $sourceLeft + (S 128) + (S 8)
	$actual = @((Get-SearchField $hwnd), $label, (Get-SourceBox $hwnd), (Get-ClearButton $hwnd) | ForEach-Object { Format-Rect ([W]::RectIn($_, $bar)) }) -join ' | '
	# The label is one line tall, centered like the search field's text, so the two stand level
	$line = [W]::TextHeight($script:dpi)
	$expected = "$(S 8),$(S 8) $($search)x$(S 23) | $labelLeft,$((S 8) + [math]::Floor(((S 23) - $line) / 2)) $($labelWidth)x$line | " +
		"$sourceLeft,$(S 8) $(S 128)x$(S 23) | $clearLeft,$(S 8) $($clearWidth)x$(S 23)"
	Check "$what  search | Source | drop-down | Clear" $actual $expected

	$tabsTop = $barHeight + (S 4)
	$resultTop = $tabsTop + (S 22) + (S 2) + (S 3)
	$tableTop = $resultTop + (S 16) + (S 5) + 1
	$actual = @((Get-StatusTabs $hwnd), (Get-ResultLine $hwnd), (Get-TrackTable $hwnd) | ForEach-Object { Format-Rect ([W]::RectIn($_, $panel)) }) -join ' | '
	$expected = "$(S 8),$tabsTop $($pw - 2 * (S 8))x$(S 22) | $(S 9),$resultTop $($pw - 2 * (S 9))x$(S 16) | 0,$tableTop $($pw)x$($ph - $tableTop)"
	Check "$what  tabs | result line | table" $actual $expected
	Check "$what  rule over the table" (([W]::PixelsIn($hwnd, $panel, @((S 40), ($tableTop - 1), (S 40), ($tableTop - 2)))) -join ' ') 'e5e5e5 ffffff'
	# Under the titles, past the last column too, and nothing above it
	$header = [W]::SendMessageW((Get-TrackTable $hwnd), 0x101F, [IntPtr]::Zero, [IntPtr]::Zero)
	$hw = Get-ClientWidth $header
	$hh = Get-ClientHeight $header
	$points = @((S 30), ($hh - 1), [int]($hw / 2), ($hh - 1), ($hw - 2), ($hh - 1), (S 30), ($hh - 2))
	Check "$what  rule under the titles" (([W]::PixelsIn($hwnd, $header, [int[]]$points)) -join ' ') 'e5e5e5 e5e5e5 e5e5e5 ffffff'
}

# The search field's cue banner and the magnifier inside it, 16 DIPs, centered in its height; the drop-down's items,
# All chosen, and a list that shows them all; Clear disabled
function Check-FilterBar($hwnd) {
	$edit = Get-SearchField $hwnd
	$icon = ([W]::Children($edit, 'WestieLib.IconView'))[0]
	Check 'Search cue banner' ([W]::CueBanner($edit)) 'Search title, artist, album'
	# Centered in the field's height, 2 px of client edge and 4 DIPs in from its left
	$field = [W]::RectIn($edit, (Get-FilterBar $hwnd))
	$r = [W]::RectIn($icon, (Get-FilterBar $hwnd))
	$actual = "$($r.Left - $field.Left),$($r.Top - $field.Top) $($r.Right - $r.Left)x$($r.Bottom - $r.Top)"
	Check '  magnifier in the field' $actual "$(2 + (S 4)),$([math]::Floor(((S 23) - (S 16)) / 2)) $(S 16)x$(S 16)"
	# The text starts after the icon and its gap
	$margins = [int64][W]::SendMessageW($edit, 0xD4, [IntPtr]::Zero, [IntPtr]::Zero)
	Check '  text margins left | right' "$($margins -band 0xFFFF) | $($margins -shr 16)" "$((S 4) + (S 16) + (S 5)) | $(S 4)"
	# The text's line is centered in the field
	Check '  text line top in the field' ([W]::EditTextTop($edit)) ([math]::Floor(((S 23) - [W]::TextHeight($script:dpi)) / 2))

	$source = Get-SourceBox $hwnd
	Check 'Source items | chosen' "$([W]::ComboItems($source)) | $(Get-SelectedSource $hwnd)" 'All | YouTube Music | Spotify | 0'
	[void][W]::SendMessageW($source, 0x14F, [IntPtr]1, [IntPtr]::Zero)
	Start-Sleep -Milliseconds 300
	$drop = [W]::WindowOfProcess($app.Process.Id, 'ComboLBox')
	$itemHeight = [int][W]::SendMessageW($source, 0x154, [IntPtr]::Zero, [IntPtr]::Zero)
	$listHeight = if ($drop -ne [IntPtr]::Zero) { Get-ClientHeight $drop } else { 0 }
	[void][W]::SendMessageW($source, 0x14F, [IntPtr]0, [IntPtr]::Zero)
	Check-That '  dropped list height' $listHeight ($listHeight -ge 3 * $itemHeight) "at least 3 items of $itemHeight"
	Check 'Filters at the start' (Get-Filters $hwnd) "'' source 0 tab 0 | Clear False"
}

# Clear is enabled while the search, the source or the tab is off its default, and puts all three back
function Check-Clear($hwnd) {
	$clear = Get-ClearButton $hwnd
	[W]::SetEditText((Get-SearchField $hwnd), 'bruno')
	Check '  search typed' (Get-Filters $hwnd) "'bruno' source 0 tab 0 | Clear True"
	[void][W]::SendMessageW($clear, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Check '  Clear' (Get-Filters $hwnd) "'' source 0 tab 0 | Clear False"

	$source = Get-SourceBox $hwnd
	[void][W]::SendMessageW($source, 0x14E, [IntPtr]2, [IntPtr]::Zero)
	# CB_SETCURSEL doesn't notify; a choice by the user does
	[void][W]::SendMessageW((Get-FilterBar $hwnd), 0x111, [IntPtr](1 -shl 16), $source)
	Check '  Spotify chosen' (Get-Filters $hwnd) "'' source 2 tab 0 | Clear True"
	Click-Tab $hwnd 1
	Check '  and the Synced tab clicked' (Get-Filters $hwnd) "'' source 2 tab 1 | Clear True"
	[void][W]::SendMessageW($source, 0x14E, [IntPtr]0, [IntPtr]::Zero)
	[void][W]::SendMessageW((Get-FilterBar $hwnd), 0x111, [IntPtr](1 -shl 16), $source)
	Check '  source back to All' (Get-Filters $hwnd) "'' source 0 tab 1 | Clear True"
	[void][W]::SendMessageW($clear, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Check '  Clear' (Get-Filters $hwnd) "'' source 0 tab 0 | Clear False"
	Click-Tab $hwnd 3
	Click-Tab $hwnd 0
	Check '  Needs review clicked, then All' (Get-Filters $hwnd) "'' source 0 tab 0 | Clear False"
}

# All tracks has no # column and sorts by Title; a playlist shows # first and sorts by it. A header click sorts by its
# column, a second one turns the order round.
function Check-Columns($hwnd) {
	$AllTracksColumns = Get-AllTracksColumns
	Check 'Columns in All tracks' (Get-Columns $hwnd) $AllTracksColumns
	Click-Header $hwnd 2
	Check '  Artists clicked' (Get-Columns $hwnd) ($AllTracksColumns -replace ' \^', '' -replace "Artists $(S 156)", "Artists $(S 156) ^")
	Click-Header $hwnd 2
	Check '  again' (Get-Columns $hwnd) ($AllTracksColumns -replace ' \^', '' -replace "Artists $(S 156)", "Artists $(S 156) v")
	Click-Header $hwnd 0
	Check '  Status clicked' (Get-Columns $hwnd) ($AllTracksColumns -replace ' \^', '' -replace "Status $(S 56)", "Status $(S 56) ^")
	Double-Click-Header $hwnd 3
	Check '  Album double-clicked: sorted, then reversed' (Get-Columns $hwnd) ($AllTracksColumns -replace ' \^', '' -replace "Album $(S 132)", "Album $(S 132) v")
	Double-Click-Header $hwnd 3
	Check '  and again: reversed twice' (Get-Columns $hwnd) ($AllTracksColumns -replace ' \^', '' -replace "Album $(S 132)", "Album $(S 132) v")

	Click-PlaylistRow $hwnd 1
	$playlistColumns = "# $(S 34) ^ | $($AllTracksColumns -replace ' \^', '')"
	Check '  playlist 1 selected' (Get-Columns $hwnd) $playlistColumns
	Check '  result line' ([W]::TextOf((Get-ResultLine $hwnd))) 'Showing 0 of 0 in Spotify playlist'
	Click-Header $hwnd 3
	Click-PlaylistRow $hwnd 2
	Check '  playlist 2 starts at its default sort' (Get-Columns $hwnd) $playlistColumns
	Check '  result line' ([W]::TextOf((Get-ResultLine $hwnd))) 'Showing 0 of 0 in YouTube Music playlist'
	Click-AllTracks $hwnd
	Check '  back to All tracks' (Get-Columns $hwnd) $AllTracksColumns
	Check '  result line' ([W]::TextOf((Get-ResultLine $hwnd))) 'Showing 0 of 0'
}

# Rows are 22 DIPs tall; the check gives the table three empty rows to measure one
function Check-RowHeight($hwnd) {
	$list = Get-TrackTable $hwnd
	[void][W]::SendMessageW($list, 0x102F, [IntPtr]3, [IntPtr]::Zero)
	$r = [W]::SendForRect($list, 0x100E, [IntPtr]0)
	[void][W]::SendMessageW($list, 0x102F, [IntPtr]0, [IntPtr]::Zero)
	Check 'Row height' ($r.Bottom - $r.Top) (S 22)
}

"--- Run 1: two playlists, nothing synced"
Write-ManyPlaylists 2
Set-SavedWindow @{}
$app = Start-App
$h = $app.Hwnd
Check 'Track panel in the main column' ((Get-RowPanels $h)[1] -eq (Get-TrackPanel $h)) 'True'
Check-TrackLayout 'Layout:' $h
Check-FilterBar $h
Check 'Status tabs | selected' "$([W]::TabTexts((Get-StatusTabs $h))) | $(Get-SelectedTab $h)" 'All 0 | Synced 0 | Waiting 0 | Needs review 0 | Failed 0 | 0'
Check '  width beyond each text' (Get-TabPadding $h) ((1..5 | ForEach-Object { 2 * (S 8) }) -join ' ')
Check 'Result line' ([W]::TextOf((Get-ResultLine $h))) 'Showing 0 of 0'
Check-Clear $h
Check-Columns $h
Check-RowHeight $h
if ($Screenshot) { Save-Screenshot $h ("$($Screenshot -replace '\.png$', '')-tracks.png") }

# The search narrows to fit the panel at its narrowest
Set-Bounds $h 100 100 (S 1100) (S 700)
Start-Sleep -Milliseconds 300
Check-TrackLayout 'Narrow window:' $h
Stop-App $app
