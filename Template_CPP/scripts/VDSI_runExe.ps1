# Lang: Powershell
# Written by:		Brandon Johns
# Version created:	2022-04-28
# Last edited:		2022-07-26

# Purpose: Find DLLs and run binary

# Sample use:
#	./runExe.ps1 "vds_template_1"

################################################################
# Script input
################################
param (
	[Parameter(Mandatory=$true,  ValueFromPipeline=$false, Position=0)][string]$exeName,
	[Parameter(Mandatory=$false, ValueFromPipeline=$false, Position=1, ValueFromRemainingArguments)][string[]]$exeArgs
)
# INPUT:
#	exeName = path to / name of exe
#		absoulte path
#		relative path
#		name (if in "../bin")

################################################################
# Script Config
################################
# Path to Vicon DataStream SDK
$VDS_LibRoot = 'C:\Program Files\Vicon\DataStream SDK\Win64\CPP'

# Relative path from this script to the binary
$VDS_RelS2C = "../bin"

################################################################
# Automated
################################
# Find exe
if ( -not $exeName.EndsWith(".exe") ) { $exeName += ".exe" }

$tmpPath = (Join-Path $PSScriptRoot $VDS_RelS2C | Join-Path -ChildPath $exeName)

if    ( Test-Path $exeName -PathType "Leaf" ) { $exePath = $exeName }
elseif( Test-Path $tmpPath -PathType "Leaf" ) { $exePath = $tmpPath }
else { throw "input file not found" }
$exePath = Resolve-Path($exePath)


# DLLs: Temporaraly add to path
#	https://docs.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_environment_variables?view=powershell-5.1
#		When you change environment variables in PowerShell, the change affects only the current session
$DLL_Directories = (Get-ChildItem $VDS_LibRoot *.dll -Recurse).DirectoryName | Get-Unique
$DLL_Directories | ForEach-Object {
	# Don't add if already on path (from multiple runs of this script)
	if(-not ( $env:PATH.Contains($_) )) { $env:PATH  += ";" + $_ }
}

# Run exe
#	NOTE: "Start-Process $exePath -NoNewWindow -Wait" is no good because the process is detached => hard to capture output
& $exePath $exeArgs


