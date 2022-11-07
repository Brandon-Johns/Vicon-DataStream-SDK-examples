# Written by:      Brandon Johns
# Version created: 2022-11-06
# Last edited:     2022-11-06
# Status: NOT TESTED

# Version changes:
#   NA

# Purpose:
#   Template for using Vicon DataStream (VDS)
#   Minimal example based on the example that comes with the SDK


from vicon_dssdk import ViconDataStream


# ************************************************************
# Config
# ******************************
hostName = "localhost:801" # IP address of the computer running Vicon Tracker

# ************************************************************
# Initialise
# ******************************
client = ViconDataStream.Client()
client.Connect(hostName)

client.EnableSegmentData()
client.SetBufferSize(1) # Always return most recent frame
client.SetStreamMode(ViconDataStream.Client.StreamMode.EServerPush)
client.SetAxisMapping(
    ViconDataStream.Client.AxisMapping.EForward,
    ViconDataStream.Client.AxisMapping.ELeft,
    ViconDataStream.Client.AxisMapping.EUp) # Set the global up axis Z-UP

# ************************************************************
# Main
# ******************************
# Get 3 frames
for idx in range(0, 3):
    # Retrieve a frame from Vicon Tracker
    client.GetFrame()

    # Decode frame - miscellaneous
    print('Frame Number: ', client.GetFrameNumber())
    print('Frame Rate:   ', client.GetFrameRate())
    print('Latency:      ', client.GetLatencyTotal())

    # Decode frame - object pose
    subjectNames = client.GetSubjectNames()
    for subjectName in subjectNames:
        segmentNames = client.GetSegmentNames(subjectName)
        segmentName = segmentNames[0]

        ret_P = client.GetSegmentGlobalTranslation(subjectName, segmentName)
        ret_R = client.GetSegmentGlobalRotationMatrix(subjectName, segmentName)

        P = ret_P[0]
        R = ret_R[0]
        isOccluded = ret_P[1] or ret_R[1]

        print(subjectName)
        print('Is Occluded:     ', isOccluded)
        print('Translation:     ', P)
        print('Rotation Matrix: ', R)

# ************************************************************
# End
# ******************************
client.Disconnect()
