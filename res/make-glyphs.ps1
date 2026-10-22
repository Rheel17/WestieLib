# Renders glyphs from the icon table in plan/ui-mockup.html (the `name: SVG('...')` entries) into
# res/glyphs/<name>.ico. Glyphs marked Disabled below also get <name>-disabled.ico: grayscale at 45% opacity, like
# the mockup's disabled toolbar buttons. Glyphs marked Small also get the sizes of a 12 px glyph, as in the status bar.
# The progress ring, which the mockup draws with ring(percent), becomes ring-00.ico to ring-24.ico, one per step.
# The brand logos come from the official SVG files in res/ instead of the mockup's stand-ins, and are also drawn at
# 14 px, as in the playlist rows.
#
#   powershell -ExecutionPolicy Bypass -File res/make-glyphs.ps1
#
# WPF draws the markup: Geometry.Parse reads SVG path data. The .ico files are committed; rerun this when a glyph
# changes or the app needs a new one.

param(
	[string]$Mockup = (Join-Path $PSScriptRoot '..\plan\ui-mockup.html'),
	[string]$OutDir = (Join-Path $PSScriptRoot 'glyphs')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName PresentationCore, WindowsBase
. (Join-Path $PSScriptRoot 'IconFile.ps1')

# Glyph name in the mockup, whether it needs a disabled copy, and whether it is also drawn at 12 px (Small) or at
# 14 px, as in the playlist rows (Row)
$Glyphs = [ordered]@{
	plus = @{ Disabled = $true }
	syncing = @{ Disabled = $true; Small = $true }
	stop = @{ Disabled = $true }
	gear = @{ Disabled = $true }
	lib = @{}
	synced = @{ Small = $true }
	warn = @{ Small = $true }
	waiting = @{ Row = $true }
	syncgray = @{ Row = $true }
}

# Brand logo name, its SVG file in res/, and its options as above
$BrandGlyphs = [ordered]@{
	ytm = @{ File = 'Youtube_Music_icon.svg'; Row = $true }
	spotify = @{ File = 'Spotify_icon.svg'; Row = $true }
}

# 100% to 300% display scaling of a 16 px glyph, of a 12 px one, and of a 14 px one
$Sizes = 16, 20, 24, 28, 32, 40, 48
$SmallSizes = 12, 15, 18, 21, 24, 30, 36
$RowSizes = 14, 18, 21, 25, 28, 35, 42

# The mockup's ring(): a gray track and a blue arc from 12 o'clock, clockwise
$RingSteps = 24
$RingRadius = 5.6
$RingWidth = 2.4
$RingTrack = '#d6d6d6'
$RingArc = '#0078d7'
$Invariant = [Globalization.CultureInfo]::InvariantCulture

function Get-RingMarkup([int]$step) {
	$circle = "<circle cx=`"8`" cy=`"8`" r=`"$RingRadius`" fill=`"none`" stroke-width=`"$RingWidth`""
	$body = "$circle stroke=`"$RingTrack`"/>"
	if ($step -eq $RingSteps) {
		$body += "$circle stroke=`"$RingArc`"/>"
	} elseif ($step -gt 0) {
		$angle = 2 * [math]::PI * $step / $RingSteps
		$x = (8 + $RingRadius * [math]::Sin($angle)).ToString('0.####', $Invariant)
		$y = (8 - $RingRadius * [math]::Cos($angle)).ToString('0.####', $Invariant)
		$large = if ($step * 2 -gt $RingSteps) { 1 } else { 0 }
		$top = (8 - $RingRadius).ToString('0.####', $Invariant)
		$body += "<path d=`"M8 $top A$RingRadius $RingRadius 0 $large 1 $x $y`" fill=`"none`" stroke=`"$RingArc`" stroke-width=`"$RingWidth`"/>"
	}
	[pscustomobject]@{ Root = (ConvertTo-SvgRoot $body); ViewBox = [double[]](0, 0, 16, 16) }
}

function ConvertTo-SvgRoot([string]$body) { ([xml]"<svg>$body</svg>").DocumentElement }
$DisabledOpacity = 0.45
$InheritedAttributes = 'fill', 'stroke', 'stroke-width', 'stroke-linecap', 'stroke-linejoin'

function Get-GlyphMarkup([string]$html, [string]$name) {
	$match = [regex]::Match($html, "(?m)^\s*$name\s*:\s*SVG\('([^']*)'(?:,\s*'([^']*)')?\)")
	if (-not $match.Success) { throw "Glyph '$name' isn't in $Mockup" }
	$viewBox = if ($match.Groups[2].Success) { $match.Groups[2].Value } else { '0 0 16 16' }
	[pscustomobject]@{ Root = (ConvertTo-SvgRoot $match.Groups[1].Value); ViewBox = [double[]]($viewBox -split ' ') }
}

function Get-SvgFileGlyph([string]$path) {
	$settings = New-Object System.Xml.XmlReaderSettings
	# Illustrator declares its namespaces as entities in an internal DTD; nothing is fetched from outside
	$settings.DtdProcessing = [System.Xml.DtdProcessing]::Parse
	$settings.XmlResolver = $null
	$reader = [System.Xml.XmlReader]::Create($path, $settings)
	try {
		$document = New-Object System.Xml.XmlDocument
		$document.Load($reader)
	} finally {
		$reader.Dispose()
	}
	$root = $document.DocumentElement
	[pscustomobject]@{ Root = $root; ViewBox = [double[]]($root.GetAttribute('viewBox').Trim() -split '[\s,]+') }
}

function Get-Number($element, [string]$name) {
	$value = $element.GetAttribute($name)
	if ($value) { [double]$value } else { 0.0 }
}

# SVG fills black when no fill is given
function Get-Brush([string]$color) {
	if ($color -eq 'none') { return $null }
	if (-not $color) { $color = '#000' }
	if ($color.Length -eq 4) { $color = '#' + (($color.Substring(1).ToCharArray() | ForEach-Object { "$_$_" }) -join '') }
	New-Object System.Windows.Media.SolidColorBrush ([System.Windows.Media.ColorConverter]::ConvertFromString($color))
}

function Get-Pen([hashtable]$style) {
	if (-not $style['stroke'] -or $style['stroke'] -eq 'none') { return $null }
	$width = if ($style['stroke-width']) { [double]$style['stroke-width'] } else { 1.0 }
	$pen = New-Object System.Windows.Media.Pen (Get-Brush $style['stroke']), $width
	$cap = switch ($style['stroke-linecap']) { 'round' { 'Round' } 'square' { 'Square' } default { 'Flat' } }
	$pen.StartLineCap = $cap
	$pen.EndLineCap = $cap
	$pen.LineJoin = switch ($style['stroke-linejoin']) { 'round' { 'Round' } 'bevel' { 'Bevel' } default { 'Miter' } }
	# SVG's default; WPF's is 10
	$pen.MiterLimit = 4
	$pen
}

$NonDrawingElements = 'metadata', 'title', 'desc', 'defs'

function Add-Shape($context, [System.Xml.XmlNode]$element, [hashtable]$inherited) {
	if ($element -isnot [System.Xml.XmlElement] -or $NonDrawingElements -contains $element.LocalName) { return }
	$style = $inherited.Clone()
	foreach ($name in $InheritedAttributes) {
		$value = $element.GetAttribute($name)
		if ($value) { $style[$name] = $value }
	}

	switch ($element.LocalName) {
		'g' {
			foreach ($child in $element.ChildNodes) { Add-Shape $context $child $style }
			return
		}
		'path' {
			$geometry = [System.Windows.Media.Geometry]::Parse($element.GetAttribute('d'))
		}
		'polygon' {
			$numbers = $element.GetAttribute('points').Trim() -split '[\s,]+'
			$pairs = for ($i = 0; $i -lt $numbers.Count; $i += 2) { "$($numbers[$i]),$($numbers[$i + 1])" }
			$geometry = [System.Windows.Media.Geometry]::Parse("M$($pairs -join ' L')Z")
		}
		'circle' {
			$center = New-Object System.Windows.Point (Get-Number $element 'cx'), (Get-Number $element 'cy')
			$radius = Get-Number $element 'r'
			$geometry = New-Object System.Windows.Media.EllipseGeometry $center, $radius, $radius
		}
		'rect' {
			$bounds = New-Object System.Windows.Rect (Get-Number $element 'x'), (Get-Number $element 'y'), (Get-Number $element 'width'), (Get-Number $element 'height')
			$corner = Get-Number $element 'rx'
			$geometry = New-Object System.Windows.Media.RectangleGeometry $bounds, $corner, $corner
		}
		default { throw "Unsupported SVG element <$($element.LocalName)>" }
	}
	$context.DrawGeometry((Get-Brush $style['fill']), (Get-Pen $style), $geometry)
}

# Top-down BGRA with straight alpha, as the icon format wants
function Get-GlyphPixels($glyph, [int]$size) {
	$visual = New-Object System.Windows.Media.DrawingVisual
	$context = $visual.RenderOpen()
	# A view box that isn't square is centered in the square glyph
	$width = $glyph.ViewBox[2]
	$height = $glyph.ViewBox[3]
	$side = [math]::Max($width, $height)
	$scale = $size / $side
	$x = (($side - $width) / 2 - $glyph.ViewBox[0]) * $scale
	$y = (($side - $height) / 2 - $glyph.ViewBox[1]) * $scale
	$context.PushTransform((New-Object System.Windows.Media.MatrixTransform $scale, 0, 0, $scale, $x, $y))
	foreach ($element in $glyph.Root.ChildNodes) { Add-Shape $context $element @{} }
	$context.Pop()
	$context.Close()

	$bitmap = New-Object System.Windows.Media.Imaging.RenderTargetBitmap $size, $size, 96, 96, ([System.Windows.Media.PixelFormats]::Pbgra32)
	$bitmap.Render($visual)
	$straight = New-Object System.Windows.Media.Imaging.FormatConvertedBitmap $bitmap, ([System.Windows.Media.PixelFormats]::Bgra32), $null, 0
	$pixels = New-Object byte[] ($size * $size * 4)
	$straight.CopyPixels($pixels, $size * 4, 0)
	return , $pixels
}

# CSS grayscale(1) weights, then the opacity
function ConvertTo-Disabled([byte[]]$pixels) {
	$gray = [byte[]]$pixels.Clone()
	for ($i = 0; $i -lt $gray.Length; $i += 4) {
		$value = [byte][math]::Round(0.0722 * $gray[$i] + 0.7152 * $gray[$i + 1] + 0.2126 * $gray[$i + 2])
		$gray[$i] = $value
		$gray[$i + 1] = $value
		$gray[$i + 2] = $value
		$gray[$i + 3] = [byte][math]::Round($gray[$i + 3] * $DisabledOpacity)
	}
	return , $gray
}

$html = [System.IO.File]::ReadAllText((Resolve-Path $Mockup))
New-Item -ItemType Directory -Force $OutDir | Out-Null

$work = @($Glyphs.Keys | ForEach-Object { [pscustomobject]@{ Name = $_; Glyph = (Get-GlyphMarkup $html $_); Options = $Glyphs[$_] } })
# The track table shows the ring at 16 px, and the status bar at 12 px
$work += @(0..$RingSteps | ForEach-Object { [pscustomobject]@{ Name = 'ring-{0:D2}' -f $_; Glyph = (Get-RingMarkup $_); Options = @{ Small = $true } } })
$work += @($BrandGlyphs.Keys | ForEach-Object {
	[pscustomobject]@{ Name = $_; Glyph = (Get-SvgFileGlyph (Join-Path $PSScriptRoot $BrandGlyphs[$_].File)); Options = $BrandGlyphs[$_] }
})

foreach ($item in $work) {
	$name = $item.Name
	$options = $item.Options
	$glyph = $item.Glyph
	$glyphSizes = $Sizes
	if ($options.Small) { $glyphSizes += $SmallSizes }
	if ($options.Row) { $glyphSizes += $RowSizes }
	$glyphSizes = @($glyphSizes | Sort-Object -Unique)
	$frames = @()
	$disabledFrames = @()
	foreach ($size in $glyphSizes) {
		$pixels = Get-GlyphPixels $glyph $size
		$frames += [pscustomobject]@{ Size = $size; Bytes = [byte[]](ConvertTo-DibBytes $pixels $size ($size * 4)) }
		if ($options.Disabled) {
			$disabledFrames += [pscustomobject]@{ Size = $size; Bytes = [byte[]](ConvertTo-DibBytes (ConvertTo-Disabled $pixels) $size ($size * 4)) }
		}
	}

	Write-IconFile (Join-Path $OutDir "$name.ico") $frames
	if ($options.Disabled) { Write-IconFile (Join-Path $OutDir "$name-disabled.ico") $disabledFrames }
	Write-Host "Wrote $name ($($glyphSizes -join ', ') px)$(if ($options.Disabled) { ' and its disabled copy' })"
}
