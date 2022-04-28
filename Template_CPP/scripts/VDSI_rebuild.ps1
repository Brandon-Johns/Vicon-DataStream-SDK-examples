# Lang: Powershell
# Written by:		Brandon Johns
# Version created:	2022-04-28
# Last edited:		2022-04-28

# Purpose: Build CMake project on Windows with Visual Studio 2022

# Sample use:
#	./rebuild.ps1

################################################################
# Script Config
################################
# Path to this CMake project
$Project_Root = Resolve-Path( Join-Path $PSScriptRoot ".." )

# Path to Vicon DataStream SDK
$VDS_LibRoot = 'C:\Program Files\Vicon\DataStream SDK\Win64\CPP'


################################################################
# Automated
################################
# Locations
$Project_src   = (Join-Path $Project_Root "/src") # Path to my source
$Project_build = (Join-Path $Project_Root "/build") # Path to cache
$Project_bin   = (Join-Path $Project_Root "/bin") # Path to output generated exe

# Validate script is in the correct directory
if(-not (Test-Path (Join-Path $Project_src "/CMakeLists.txt")) ) { throw "CMake file not found" }

# Create build directories
if(-not (Test-Path $Project_build) ) { New-Item -ItemType "directory" -Path $Project_build }
if(-not (Test-Path $Project_bin) )   { New-Item -ItemType "directory" -Path $Project_bin }

# DLLs: Temporaraly add to path
#	https://docs.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_environment_variables?view=powershell-5.1
#		When you change environment variables in PowerShell, the change affects only the current session
$DLL_Directories = (Get-ChildItem $VDS_LibRoot *.dll -Recurse).DirectoryName | Get-Unique
$DLL_Directories | ForEach-Object {
	# Don't add if already on path (from multiple runs of this script)
	if(-not ( $env:PATH.Contains($_) )) { $env:PATH  += ";" + $_ }
}

# Empty /build
# Empty only the files in the top level of /bin
if( Test-Path (Join-Path $Project_build "/CMakeCache.txt") ) { Get-ChildItem $Project_build | Remove-Item -Recurse }
Get-ChildItem "$Project_bin/*" -File -Include ("*.exe","*.pdb","*.ilk","*.exp") | Remove-Item


# Initiate build tools
#	https://docs.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell
#	Have fun getting it to use x64
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1" -SkipAutomaticLocation

## Run cmake
Push-Location $Project_build
cmake -G "Visual Studio 17 2022" -A x64 $Project_src
cmake --build . --target ALL_BUILD
Pop-Location

