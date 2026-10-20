# Draws the 'app' glyph from plan/ui-mockup.html into WestieLib.ico.
#
#   powershell -ExecutionPolicy Bypass -File res/make-icon.ps1

param([string]$OutFile = (Join-Path $PSScriptRoot 'WestieLib.ico'))

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

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

	$maskStride = [int]([math]::Floor(($size + 31) / 32) * 4)
	$stream = New-Object System.IO.MemoryStream
	$writer = New-Object System.IO.BinaryWriter $stream

	# BITMAPINFOHEADER. In an icon, the height counts the color rows and the AND-mask rows.
	$writer.Write([int32]40)
	$writer.Write([int32]$size)
	$writer.Write([int32]($size * 2))
	$writer.Write([int16]1)
	$writer.Write([int16]32)
	$writer.Write([int32]0)
	$writer.Write([int32]($size * $size * 4 + $maskStride * $size))
	$writer.Write([int32]0)
	$writer.Write([int32]0)
	$writer.Write([int32]0)
	$writer.Write([int32]0)

	for ($y = $size - 1; $y -ge 0; $y--) {
		$writer.Write($pixels, $y * $stride, $size * 4)
	}

	# AND mask: a set bit marks a fully transparent pixel
	for ($y = $size - 1; $y -ge 0; $y--) {
		$row = New-Object byte[] $maskStride
		for ($x = 0; $x -lt $size; $x++) {
			if ($pixels[$y * $stride + $x * 4 + 3] -eq 0) {
				$byte = [int][math]::Floor($x / 8)
				$row[$byte] = $row[$byte] -bor (0x80 -shr ($x % 8))
			}
		}
		$writer.Write($row)
	}

	$writer.Flush()
	return , $stream.ToArray()
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

$file = [System.IO.File]::Create($OutFile)
$writer = New-Object System.IO.BinaryWriter $file
try {
	# ICONDIR, then one ICONDIRENTRY per frame
	$writer.Write([int16]0)
	$writer.Write([int16]1)
	$writer.Write([int16]$frames.Count)

	$offset = 6 + 16 * $frames.Count
	foreach ($frame in $frames) {
		# A width and height of 0 mean 256
		$dimension = if ($frame.Size -ge 256) { 0 } else { $frame.Size }
		$writer.Write([byte]$dimension)
		$writer.Write([byte]$dimension)
		$writer.Write([byte]0)
		$writer.Write([byte]0)
		$writer.Write([int16]1)
		$writer.Write([int16]32)
		$writer.Write([int32]$frame.Bytes.Length)
		$writer.Write([int32]$offset)
		$offset += $frame.Bytes.Length
	}

	foreach ($frame in $frames) {
		$writer.Write($frame.Bytes)
	}
} finally {
	$writer.Dispose()
}

Write-Host "Wrote $OutFile ($($Sizes -join ', ') px)"
