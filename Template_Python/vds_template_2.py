# Written By: Brandon Johns
# Date Version Created: 2022-11-28
# Date Last Edited: 2022-11-28
# Status: functional

### PURPOSE ###
# Example for using Brandon's interface to the 'Vicon DataStream SDK'
# Print locations of objects to the screen

### NOTES ###
# Due to use of the 'multiprocessing' module,
# for some reason, any script that uses this module must be wrapped in
#   if __name__ == '__main__':
# Otherwise there will be some weird error about needing to call
#   multiprocessing.freeze_support()
# See: https://stackoverflow.com/a/24374798


import VDSInterface
import time
import numpy

if __name__ == "__main__":
    # Program configuration
    hostName = 'localhost:801' # IP address of the computer running Vicon Tracker
    lightWeightMode = False

    # Instance the interface
    vdsi = VDSInterface.Interface()
    vdsi.Connect(hostName, lightWeightMode)

    timeStart = time.time()
    duration = 3 # [s]
    # Collect data
    while time.time() - timeStart < duration:
        # Get next data frame
        frame = vdsi.GetFrame()

        print('')
        print('Frame Number [Count]:', frame.FrameNumber())
        print('Frame Age [s]:', frame.FrameAge_seconds())

        Jackal = frame.GetByName('Jackal')
        Pedestrian = frame.GetByName('Pedestrian')

        print('Jackal     (x,y,z) [mm]:', Jackal.P().transpose())
        print('Pedestrian (x,y,z) [mm]:', Pedestrian.P().transpose())

        if (not Jackal.IsOccluded()) and (not Pedestrian.IsOccluded()):
            Position_JackalRelPedestrian = Pedestrian.Inv().T() @ Jackal.T()
            print('Position of Jackal, relative to Pedestrian:', Position_JackalRelPedestrian.transpose())

    vdsi.Disconnect()
