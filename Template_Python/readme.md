Vicon DataStream SDK - Template Projects (Not Official)
# Python
These instructions assume that you are running Python 3.10 on Windows

## Setup
1. Install Python with pip
	- e.g. Install [Anaconda Distribution](https://www.anaconda.com/products/distribution)

2. Install the Vicon DataStream SDK
    - Download the Vicon DataStream SDK
    - Run the Win64 installer
    - Try
        - The install path should place a script at `C:\Program Files\Vicon\DataStream SDK\Win64\Python\install_vicon_dssdk.bat`
        - Open a command prompt run this file (it will execute a `pip install` command)
    - Or if working in a virtual environment
		- Open your python environment's cmd / PowerShell prompt (e.g. anaconda prompt, pycharm prompt, ect.)
		- In this, run `pip install "C:\Program Files\Vicon\DataStream SDK\Win64\Python\vicon_dssdk"`
    - If this gives permissions errors
        - Right click the folder `C:\Program Files\Vicon\DataStream SDK\Win64\Python` and select `Properties`
        - In the `Security` tab, click `Edit` to change permissions, and grant `Full control` to your user

2. If the computer you are working on is not the Vicon Control Computer, then
	- In the each `vds_template_#.m` file, change the line `hostName = 'localhost:801'` so that `hostName` is equal to the IP address of the Vicon Control Computer (specified as a string)

## Run
In a Python IDE, run any of the files
- `vds_template_#.py`

or in powershell/cmd, run
-	`python vds_template_#.py`

## About this template
Refer to the main readme for general information on why this wrapper class was written.

To use the interface, look at the 3 example files:
1) Demonstrates the use of all functions provided by the interface
2) Loop and operate on the captured data
3) Loop and write the captured data into Excel

