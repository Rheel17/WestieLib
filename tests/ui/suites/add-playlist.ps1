# The Add playlist dialog: the ways to open it, how it reads links, and the Spotify and YouTube Music look-ups.
# Dot-sourced by ui-checks.ps1.

# What the Add playlist dialog says under the link once the look-up answered, then the Add button's text and whether it
# is enabled. Run 1 has no yt-dlp, so YouTube Music links wait for it.
$LinkChecks = @(
	@{ Link = ''; Line = ' | Add False' },
	@{ Link = $YouTubeLink; Line = "$WaitingForTools | Add False" },
	@{ Link = "  https://www.youtube.com/playlist?si=x&list=PLx9k2_-Ab`r`n"; Line = "$WaitingForTools | Add False" },
	@{ Link = 'MUSIC.youtube.com/browse/VLPLx9k2_-Ab'; Line = "$WaitingForTools | Add False" },
	@{ Link = 'https://music.youtube.com/playlist?list=LM'; Line = "$NotSupported | Add False" },
	@{ Link = 'https://music.youtube.com/watch?v=abc&list=PLx9k2_-Ab'; Line = "$NotSupported | Add False" },
	@{ Link = "$SpotifyLink`?si=4f1c2e7a9b3d4e5f"; Line = "$SpotifyFound | Add True" },
	@{ Link = "https://open.spotify.com/intl-nl/playlist/$SpotifyId"; Line = "$SpotifyFound | Add True" },
	@{ Link = "spotify:playlist:$SpotifyId"; Line = "$SpotifyFound | Add True" },
	@{ Link = "spotify:playlist:$MissingSpotifyId"; Line = "$Unavailable | Add False" },
	@{ Link = "spotify:playlist:$BrokenSpotifyId"; Line = 'Couldn''t look it up: HTTP 500 from 127.0.0.1 | Add False' },
	@{ Link = "https://open.spotify.com/album/$SpotifyId"; Line = "$NotSupported | Add False" },
	@{ Link = 'https://open.spotify.com/playlist/7mKq2pLx'; Line = "$NotSupported | Add False" },
	@{ Link = 'hello'; Line = "$NotSupported | Add False" }
)

# The toolbar, File > Add playlist, Ctrl+N, the left panel's button and the welcome screen's each open Add playlist,
# centered on the window. It starts empty, with Add disabled and Sync now checked; the line under the link follows the
# typing and the look-ups. Cancel writes no state.json.
function Check-AddPlaylistDialog($hwnd) {
	$strip = Get-ToolbarStrip $hwnd
	$toolbar = ([W]::Children($strip, 'ToolbarWindow32'))[0]
	$command = [IntPtr][int][W]::CommandOf([W]::GetMenu($hwnd), '&Add playlist...')
	$leftButton = ([W]::Children((Get-RowPanels $hwnd)[0], 'Button'))[0]
	$welcomeButton = (Get-DirectChildren (Get-Panel $hwnd 'Welcome') 'Button')[0]
	$opens = @(
		@{ Name = 'the toolbar button'; Open = { [void][W]::PostMessageW($strip, 0x111, $command, $toolbar) } },
		@{ Name = 'File > Add playlist'; Open = { Send-Command $hwnd '&Add playlist...' } },
		@{ Name = 'Ctrl+N'; Open = { [W]::PostCtrlKey($hwnd, 0x4E) } },
		@{ Name = 'the left panel button'; Open = { [void][W]::PostMessageW($leftButton, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero) } },
		@{ Name = 'the welcome button'; Open = { [void][W]::PostMessageW($welcomeButton, 0xF5, [IntPtr]::Zero, [IntPtr]::Zero) } }
	)
	foreach ($open in $opens) {
		& $open.Open
		$dialog = Wait-OwnedWindow $hwnd
		Check "$($open.Name) opens Add playlist" "$([W]::ClassOf($dialog)) $([W]::TextOf($dialog))" '#32770 Add playlist'
		[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
		Wait-Closed $dialog
	}

	Send-Command $hwnd '&Add playlist...'
	$dialog = Wait-OwnedWindow $hwnd
	$d = New-Object W+RECT
	$o = New-Object W+RECT
	[void][W]::GetWindowRect($dialog, [ref]$d)
	[void][W]::GetWindowRect($hwnd, [ref]$o)
	$offset = "$(($d.Left + $d.Right) - ($o.Left + $o.Right)) $(($d.Top + $d.Bottom) - ($o.Top + $o.Bottom))"
	Check-That '  centered on the window (offset x2)' $offset ($offset -match '^-?[01] -?[01]$') '-1 to 1 each way'
	$syncNow = [W]::SendMessageW([W]::GetDlgItem($dialog, 1102), 0xF0, [IntPtr]::Zero, [IntPtr]::Zero)
	Check '  focus on the link | line | Add | Sync now' "$([W]::FocusOf($dialog) -eq [W]::GetDlgItem($dialog, 1100)) | $(Get-LinkLine $dialog) | $syncNow" 'True |  | Add False | 1'
	foreach ($case in $LinkChecks) {
		Set-PlaylistLink $dialog $case.Link
		Check "  '$($case.Link.Trim())'" (Wait-LinkLine $dialog) $case.Line
	}
	Set-PlaylistLink $dialog "spotify:playlist:$SlowSpotifyId"
	Check '  while Spotify answers' (Get-LinkLine $dialog) "Looking up the Spotify playlist$Ellipsis | Add False"
	# The spinner takes a step every 100 ms, unless Windows' animations are off
	$line = [W]::GetDlgItem($dialog, 1101)
	$first = [W]::ChildPixels($dialog, $line, (S 16))
	Start-Sleep -Milliseconds 250
	$moved = if ([W]::ChildPixels($dialog, $line, (S 16)) -ne $first) { 'turns' } else { 'stands still' }
	Check '  its spinner' $moved $(if ([W]::ClientAreaAnimation()) { 'turns' } else { 'stands still' })
	if ($Screenshot) { Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-add-playlist-looking.png") }
	Check '  then' (Wait-LinkLine $dialog) 'Sunday Social Spotify playlist | Add True'
	$logo = [W]::ChildPixels($dialog, $line, (S 16))
	Start-Sleep -Milliseconds 250
	Check '  and its logo stands still' $(if ([W]::ChildPixels($dialog, $line, (S 16)) -eq $logo) { 'still' } else { 'moves' }) 'still'
	Set-PlaylistLink $dialog $YouTubeLink
	if ($Screenshot) {
		Start-Sleep -Milliseconds 300
		Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-add-playlist-waiting.png")
	}
	# Shared links are the longest: a Spotify link with its ?si= part, and a YouTube Music list id of 34 characters
	$edit = [W]::GetDlgItem($dialog, 1100)
	$c = New-Object W+RECT
	[void][W]::GetClientRect($edit, [ref]$c)
	$margins = [int64][W]::SendMessageW($edit, 0xD4, [IntPtr]::Zero, [IntPtr]::Zero)
	$room = $c.Right - ($margins -band 0xFFFF) - (($margins -shr 16) -band 0xFFFF)
	foreach ($link in "$SpotifyLink`?si=4f1c2e7a9b3d4e5f", 'https://music.youtube.com/playlist?list=PLWmMwQkx8mKVm2wQgkWMmwMWqWZqWmWZW') {
		$width = [W]::TextWidth($link, $script:dpi)
		Check-That "  fits without scrolling: $($link.Substring(8, 20))..." "$width of $room" ($width -le $room) "at most $room"
	}
	Set-PlaylistLink $dialog "$SpotifyLink`?si=4f1c2e7a9b3d4e5f"
	Check '  a new playlist again, for the screenshot' (Wait-LinkLine $dialog) "$SpotifyFound | Add True"
	if ($Screenshot) {
		Start-Sleep -Milliseconds 300
		Save-Screenshot $dialog ("$($Screenshot -replace '\.png$', '')-add-playlist.png")
	}
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
	Check '  Cancel writes no state.json' (Test-Path $StateFile) 'False'
}

# With yt-dlp installed, a YouTube Music link is looked up: the answer names the playlist, or says it is missing,
# private or out of reach. The look-up waits 300 ms after the last change, so quick changes run yt-dlp once, with the
# arguments the app gives it.
function Check-YouTubeLookups($hwnd) {
	Send-Command $hwnd '&Add playlist...'
	$dialog = Wait-OwnedWindow $hwnd
	Set-PlaylistLink $dialog $YouTubeLink
	Check 'yt-dlp installed: a YouTube Music link' (Wait-LinkLine $dialog) "$YouTubeFound | Add True"
	$cases = @(
		@{ Id = 'PLmissing1'; Line = "$Unavailable | Add False" },
		@{ Id = 'PLprivate1'; Line = "$Unavailable | Add False" },
		@{ Id = 'PLbroken1'; Line = 'Couldn''t look it up: Unable to download API page: HTTP Error 503: Service Unavailable | Add False' }
	)
	foreach ($case in $cases) {
		Set-PlaylistLink $dialog "https://music.youtube.com/playlist?list=$($case.Id)"
		Check "  $($case.Id)" (Wait-LinkLine $dialog) $case.Line
	}
	Set-PlaylistLink $dialog 'https://music.youtube.com/playlist?list=PLslow1'
	Check '  while yt-dlp looks it up' (Get-LinkLine $dialog) "Looking up the YouTube Music playlist$Ellipsis | Add False"
	Check '  then' (Wait-LinkLine $dialog) 'Checked PLslow1 YouTube Music playlist | Add True'
	$runs = @(Get-Content $FakeYtDlpRuns -Encoding UTF8).Count
	foreach ($id in 'PLquick1', 'PLquick2', 'PLquick3') { [W]::SetEditText([W]::GetDlgItem($dialog, 1100), "https://music.youtube.com/playlist?list=$id") }
	Check '  three quick changes' (Wait-LinkLine $dialog) 'Checked PLquick3 YouTube Music playlist | Add True'
	$new = @(Get-Content $FakeYtDlpRuns -Encoding UTF8 | Select-Object -Skip $runs)
	Check '  run yt-dlp once, with these arguments' ($new -join ' / ') '--ignore-config|--flat-playlist|--dump-single-json|--playlist-items|0|https://music.youtube.com/playlist?list=PLquick3'
	[void][W]::PostMessageW([W]::GetDlgItem($dialog, 2), 0xF5, [IntPtr]::Zero, [IntPtr]::Zero)
	Wait-Closed $dialog
}

"--- Run 1: no tools, so YouTube Music links wait for yt-dlp"
$app = Start-App
Check-AddPlaylistDialog $app.Hwnd
Stop-App $app

"--- Run 2: tools installed"
Install-FakeTools
Set-SavedAutoUpdate $false
$app = Start-App
Check-YouTubeLookups $app.Hwnd
Stop-App $app
