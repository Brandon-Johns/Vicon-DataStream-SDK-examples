# Written By: Brandon Johns
# Date Version Created: 2022-11-28
# Date Last Edited: 2022-11-28
# Status: functional

### PURPOSE ###
# Example for using Brandon's interface to the 'Vicon DataStream SDK'
# Demonstrate use of all functions

### NOTES ###
# Due to use of the 'multiprocessing' module,
# for some reason, any script that uses this module must be wrapped in
#   if __name__ == '__main__':
# Otherwise there will be some weird error about needing to call
#   multiprocessing.freeze_support()
# See: https://stackoverflow.com/a/24374798


import VDSInterface


if __name__ == "__main__":
    # Program configuration
    hostName = 'localhost:801' # IP address of the computer running Vicon Tracker
    lightWeightMode = False

    # Instance the interface
    vdsi = VDSInterface.Interface()
    vdsi.Connect(hostName, lightWeightMode)

    # Options
    vdsi.EnableOccludedFilter()
    #vdsi.EnableObjectFilter(['Jackal', 'Pedestrian'])

    # Get next data frame
    frame = vdsi.GetFrame()

    print('Frame Rate [Hz]:     ', vdsi.GetFrameRate())
    print('Frame Number [Count]:', frame.FrameNumber())
    print('Frame Time (when received) [s]: ', frame.FrameTime_seconds())
    print('Frame Age (now - FrameTime) [s]:', frame.FrameAge_seconds())

    # Get every object (return type: list of points)
    print('Every Object:     ', end='')
    print([point.Name() for point in frame.All()], sep=', ')

    # Get every object that is visible in the current frame (return type: list of points)
    print('Visible Objects:  ', end='')
    print([point.Name() for point in frame.GetIfNotOccluded()], sep=', ')

    # Get specific objects (Use the names of the objects as specified in Vicon Tracker)
    print('Specific Objects: ', end='')
    print([point.Name() for point in frame.GetByNames(['Jackal', 'Pedestrian'])], sep=', ')

    # Get a specific object
    print()
    point = frame.GetByName('Jackal')

    # Name of object, as defined in Vicon Tracker
    print('Name:', point.Name())

    # Is the object occluded in this frame?
    print('Is Occluded in this frame?:', point.IsOccluded())

    # Translation [mm] (x, y, z, position vector, homogeneous position vector)
    print('x [mm]:', point.x())
    print('y [mm]:', point.y())
    print('z [mm]:', point.z())
    print('(x,y,z) [mm]:\n', point.P())
    print('(x,y,z,1) [mm]:\n', point.Ph())

    # Rotation (matrix, quaternions, axis angle, euler)
    print('Rotation Matrix:\n', point.R())
    print('Rotation quaternion (x,y,z,w):', point.quat_xyzw())
    print('Rotation quaternion (w,x,y,z):', point.quat_wxyz())

    # Transformation matrix [position in mm]
    print('Transformation Matrix:\n', point.T())
    print('Inverse of Transformation Matrix:\n', point.Inv().T())

    vdsi.Disconnect()
