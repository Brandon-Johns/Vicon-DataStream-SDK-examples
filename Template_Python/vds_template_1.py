# NOT TESTED / Work in progress
# Minimal example based on the example that comes with the SDK
# TODO: https://docs.python.org/3/library/threading.html

from __future__ import print_function
from vicon_dssdk import ViconDataStream

# Program configuration
HostName = "localhost:801"

# Initialise
client = ViconDataStream.Client()
try:
    client.Connect( HostName )
except:
    raise Exception("ERROR_VDS: Failed to connect to " + HostName)
client.EnableSegmentData()
client.SetBufferSize( 1 )
client.SetStreamMode( ViconDataStream.Client.StreamMode.EServerPush )
client.SetAxisMapping( ViconDataStream.Client.AxisMapping.EForward, ViconDataStream.Client.AxisMapping.ELeft, ViconDataStream.Client.AxisMapping.EUp )

# Retrieve a frame from Vicon Tracker
client.GetFrame()

# Decode frame - misc info
print( 'Frame Number', client.GetFrameNumber() )
print( 'Frame Rate', client.GetFrameRate() )
print( 'Latency', client.GetLatencyTotal() )

# Decode frame - object pose
subjectNames = client.GetSubjectNames()
for subjectName in subjectNames:
    print( subjectName )

    segmentNames = client.GetSegmentNames( subjectName )
    segmentName = segmentNames[0]
    if length(segmentNames)!=1: raise Exception("ERROR_VDS: Invalid assumption that segment count is 1")

    print( 'Translation', client.GetSegmentGlobalTranslation( subjectName, segmentName ) )
    print( 'Rotation Matrix', client.GetSegmentGlobalRotationMatrix( subjectName, segmentName ) )


# SAMPLE: VDS exception handling
#try:
#    #
#except ViconDataStream.DataStreamException as e:
#    print( 'VDS error', e )
