# Renders glyphs from the icon table in plan/ui-mockup.html (the `name: SVG('...')` entries) into
# res/glyphs/<name>.ico. Glyphs marked below also get <name>-disabled.ico: grayscale at 45% opacity, like the
# mockup's disabled toolbar buttons.
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

# Glyph name in the mockup, and whether it needs a disabled copy
$Glyphs = [ordered]@{
	plus = $true
	syncing = $true
	stop = $true
	gear = $true
	lib = $false
}

# 100% to 300% display scaling of a 16 px glyph
$Sizes = 16, 20, 24, 28, 32, 40, 48
$DisabledOpacity = 0.45
$InheritedAttributes = 'fill', 'stroke', 'stroke-width', 'stroke-linecap', 'stroke-linejoin'

function Get-GlyphMarkup([string]$html, [string]$name) {
	$match = [regex]::Match($html, "(?m)^\s*$name\s*:\s*SVG\('([^']*)'(?:,\s*'([^']*)')?\)")
	if (-not $match.Success) { throw "Glyph '$name' isn't in $Mockup" }
	$viewBox = if ($match.Groups[2].Success) { $match.Groups[2].Value } else { '0 0 16 16' }
	[pscustomobject]@{ Body = $match.Groups[1].Value; ViewBox = [double[]]($viewBox -split ' ') }
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

function Add-Shape($context, [System.Xml.XmlElement]$element, [hashtable]$inherited) {
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
	$scale = $size / $glyph.ViewBox[2]
	$context.PushTransform((New-Object System.Windows.Media.MatrixTransform $scale, 0, 0, $scale, (-$glyph.ViewBox[0] * $scale), (-$glyph.ViewBox[1] * $scale)))
	$svg = [xml]"<svg>$($glyph.Body)</svg>"
	foreach ($element in $svg.DocumentElement.ChildNodes) { Add-Shape $context $element @{} }
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

foreach ($name in $Glyphs.Keys) {
	$glyph = Get-GlyphMarkup $html $name
	$frames = @()
	$disabledFrames = @()
	foreach ($size in $Sizes) {
		$pixels = Get-GlyphPixels $glyph $size
		$frames += [pscustomobject]@{ Size = $size; Bytes = [byte[]](ConvertTo-DibBytes $pixels $size ($size * 4)) }
		if ($Glyphs[$name]) {
			$disabledFrames += [pscustomobject]@{ Size = $size; Bytes = [byte[]](ConvertTo-DibBytes (ConvertTo-Disabled $pixels) $size ($size * 4)) }
		}
	}

	Write-IconFile (Join-Path $OutDir "$name.ico") $frames
	if ($Glyphs[$name]) { Write-IconFile (Join-Path $OutDir "$name-disabled.ico") $disabledFrames }
	Write-Host "Wrote $name ($($Sizes -join ', ') px)$(if ($Glyphs[$name]) { ' and its disabled copy' })"
}
