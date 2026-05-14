# Setup script - downloads and extracts SDL3 libraries into 3rdparty/
# Run once after cloning: .\setup.ps1

$ErrorActionPreference = "Stop"

$libs = @(
    @{
        Name    = "SDL3"
        Url     = "https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.8.zip"
        Archive = "SDL-release-3.4.8.zip"
        ExtractedName = "SDL-release-3.4.8"
    },
    @{
        Name    = "SDL3_ttf"
        Url     = "https://github.com/libsdl-org/SDL_ttf/archive/refs/tags/release-3.2.2.zip"
        Archive = "SDL_ttf-release-3.2.2.zip"
        ExtractedName = "SDL_ttf-release-3.2.2"
    },
    @{
        Name    = "SDL3_image"
        Url     = "https://github.com/libsdl-org/SDL_image/archive/refs/tags/release-3.4.4.zip"
        Archive = "SDL_image-release-3.4.4.zip"
        ExtractedName = "SDL_image-release-3.4.4"
    }
)

$thirdParty = "$PSScriptRoot\3rdparty"
New-Item -ItemType Directory -Force $thirdParty | Out-Null

foreach ($lib in $libs) {
    $dest = "$thirdParty\$($lib.Name)"
    if (Test-Path $dest) {
        Write-Host "$($lib.Name) already exists, skipping." -ForegroundColor Yellow
        continue
    }

    $archive = "$thirdParty\$($lib.Archive)"
    Write-Host "Downloading $($lib.Name)..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $lib.Url -OutFile $archive

    Write-Host "Extracting $($lib.Name)..." -ForegroundColor Cyan
    Expand-Archive -Path $archive -DestinationPath $thirdParty -Force

    Rename-Item "$thirdParty\$($lib.ExtractedName)" $lib.Name
    Remove-Item $archive

    Write-Host "$($lib.Name) ready." -ForegroundColor Green
}

Write-Host "`nAll libraries set up. You can now build the project." -ForegroundColor Green
