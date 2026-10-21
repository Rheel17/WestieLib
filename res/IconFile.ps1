# Writes .ico files. Dot-source it from the icon scripts:
#
#   . (Join-Path $PSScriptRoot 'IconFile.ps1')

# A 32-bit DIB frame from top-down BGRA pixels with straight alpha
function ConvertTo-DibBytes([byte[]]$pixels, [int]$size, [int]$stride) {
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

# $frames: objects with Size and Bytes (a DIB, or PNG at 256 px)
function Write-IconFile([string]$path, $frames) {
	$file = [System.IO.File]::Create($path)
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
}
