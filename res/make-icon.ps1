# Draws the 'app' glyph from plan/ui-mockup.html into WestieLib.ico.
#
#   powershell -ExecutionPolicy Bypass -File res/make-icon.ps1

param([string]$OutFile = (Join-Path $PSScriptRoot 'WestieLib.ico'))

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
. (Join-Path $PSScriptRoot 'IconFile.ps1')

$Sizes = 16, 20, 24, 32, 40, 48, 64, 256
$PngFromSize = 256

function New-IconBitmap([int]$size) {
	$bitmap = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
	try {
		$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
		# Puts integer coordinates on pixel edges, so the square stays crisp
		$graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
		$graphics.Clear([System.Drawing.Color]::Transparent)

		$margin = [math]::Floor($size / 32)
		$blue = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(0x00, 0x78, 0xD7))
		$graphics.FillRectangle($blue, [int]$margin, [int]$margin, [int]($size - 2 * $margin), [int]($size - 2 * $margin))
		$blue.Dispose()

		# The mockup's SVG path on its 16-unit grid: M6.4 11.6 V4.4 l5.2-1.3 v6.8
		$graphics.ScaleTransform($size / 16, $size / 16)
		$pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::White), 1.4
		$stem = [System.Drawing.PointF[]]@(
			(New-Object System.Drawing.PointF 6.4, 11.6),
			(New-Object System.Drawing.PointF 6.4, 4.4),
			(New-Object System.Drawing.PointF 11.6, 3.1),
			(New-Object System.Drawing.PointF 11.6, 9.9))
		$graphics.DrawLines($pen, $stem)
		$pen.Dispose()
		$graphics.FillEllipse([System.Drawing.Brushes]::White, 3.3, 9.9, 3.4, 3.4)
		$graphics.FillEllipse([System.Drawing.Brushes]::White, 8.5, 8.2, 3.4, 3.4)
	} finally {
		$graphics.Dispose()
	}
	return $bitmap
}

function Get-DibBytes([System.Drawing.Bitmap]$bitmap) {
	$size = $bitmap.Width
	$rect = New-Object System.Drawing.Rectangle 0, 0, $size, $size
	$data = $bitmap.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
		[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
	$stride = $data.Stride
	$pixels = New-Object byte[] ($stride * $size)
	[System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $pixels, 0, $pixels.Length)
	$bitmap.UnlockBits($data)
	return , (ConvertTo-DibBytes $pixels $size $stride)
}

function Get-PngBytes([System.Drawing.Bitmap]$bitmap) {
	$stream = New-Object System.IO.MemoryStream
	$bitmap.Save($stream, [System.Drawing.Imaging.ImageFormat]::Png)
	return , $stream.ToArray()
}

$frames = foreach ($size in $Sizes) {
	$bitmap = New-IconBitmap $size
	try {
		$bytes = if ($size -ge $PngFromSize) { Get-PngBytes $bitmap } else { Get-DibBytes $bitmap }
	} finally {
		$bitmap.Dispose()
	}
	[pscustomobject]@{ Size = $size; Bytes = [byte[]]$bytes }
}

Write-IconFile $OutFile $frames

Write-Host "Wrote $OutFile ($($Sizes -join ', ') px)"
