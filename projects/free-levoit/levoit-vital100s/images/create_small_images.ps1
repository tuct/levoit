# Resize JPG images to max 800px width if _small version doesn't exist
# Run from the images folder: .\create_small_images.ps1

[System.Reflection.Assembly]::LoadWithPartialName('System.Drawing') | Out-Null

Get-ChildItem -Path ".\*.jpg" -Exclude "*_small.jpg" | ForEach-Object {
    $newFile = $_.FullName -replace '\.jpg$', '_small.jpg'
    
    if (-not (Test-Path $newFile)) {
        $pic = New-Object System.Drawing.Bitmap($_.FullName)
        
        if ($pic.Width -gt 800) {
            $ratio = 800 / $pic.Width
            $newH = [int]($pic.Height * $ratio)
            $nb = New-Object System.Drawing.Bitmap(800, $newH)
            $g = [System.Drawing.Graphics]::FromImage($nb)
            $g.DrawImage($pic, 0, 0, 800, $newH)
            $nb.Save($newFile)
            $g.Dispose()
            $nb.Dispose()
            Write-Host "Created $(Split-Path -Leaf $newFile) - 800x$newH"
        } else {
            $pic.Save($newFile)
            Write-Host "Created $(Split-Path -Leaf $newFile) - $($pic.Width)x$($pic.Height)"
        }
        
        $pic.Dispose()
    } else {
        Write-Host "$(Split-Path -Leaf $newFile) already exists - skipped"
    }
}

Write-Host "Done!"
