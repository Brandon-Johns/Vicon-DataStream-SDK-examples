Vicon DataStream SDK - Template Projects (Not Official)
# Python
These instructions assume that you are running on Windows

## Setup
1. Install Python with pip
	- e.g. Install [Anaconda Distribution](https://www.anaconda.com/products/distribution)

2. Install the Vicon DataStream SDK
	- Download the Vicon DataStream SDK
	- Run the Win64 installer
	- The install path should place a script at `C:\Program Files\Vicon\DataStream SDK\Win64\Python\install_vicon_dssdk.bat`
	- Open a python cmd.exe prompt run this file (it will execute a `pip install` command)

2. If the computer you are working on is not the Vicon Control Computer, then
	- In the each `vds_template_#.m` file, change the line `hostName = 'localhost:801'` so that `hostName` is equal to the IP address of the Vicon Control Computer (specified as a string)

## Run
In a Python IDE, run any of the files
- `vds_template_#.py`

or in powershell/cmd, run
-	`python vds_template_#.py`




