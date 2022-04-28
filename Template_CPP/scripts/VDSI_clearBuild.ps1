# Lang: Powershell
# Written by:		Brandon Johns
# Version created:	2022-04-28
# Last edited:		2022-04-28

# Purpose: clean cmake build

# Sample use:
#	./cearBuild.ps1 all

################################################################
# Script input
################################
param (
	[Parameter(Mandatory=$false,ValueFromPipeline=$false)]
	[ValidateSet('all', 'bin', 'build')]
	[string]$mode = 'build'
)
# INPUT:
#	mode = what directory to clear

################################################################
# Script Config
################################
$Project_Root = (Join-Path $PSScriptRoot "/..")


################################################################
# Automated
################################
# Locations
$Project_build = (Join-Path $Project_Root "/build") # Path to cache
$Project_bin   = (Join-Path $Project_Root "/bin") # Path to output generated exe

# Validate script is in the correct directory
if(-not (Test-Path (Join-Path $Project_Root "/src/CMakeLists.txt")) ) { throw "CMake file not found" }

# Empty /build
if( ('all','build') -contains $mode )
{
	if( Test-Path (Join-Path $Project_build "/CMakeCache.txt") ) { Get-ChildItem $Project_build | Remove-Item -Recurse }
}

# Empty only the files in the top level of /bin
if( ('all','bin')
{
	Get-ChildItem "$Project_bin/*" -File -Include ("*.exe","*.pdb","*.ilk","*.exp") | Remove-Item
}

