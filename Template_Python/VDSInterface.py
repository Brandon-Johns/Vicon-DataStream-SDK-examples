# Version:
#   Multithreading through "multiprocessing" module
#   This module implements true multithreading: concurrency + parallelism
#   Performance is great

# Usage Note:
#   Due to use of the "multiprocessing" module,
#   for some reason, any script that uses this module must be wrapped in
#       if __name__ == "__main__":
#   Otherwise there will be some weird error about needing to call
#       multiprocessing.freeze_support()
#   See: https://stackoverflow.com/a/24374798


from vicon_dssdk import ViconDataStream
import multiprocessing
import copy
import numpy
from numpy import nan
import queue
import scipy
import time


############################################################################################
# Data Structures
##############################################
class Point:
    def __init__(self,
                 Name:str,
                 R=numpy.array([nan,nan,nan,nan,nan,nan,nan,nan,nan]).reshape(3,3),
                 P=numpy.array([[nan,nan,nan]]).T):
        # Input validation
        if P.shape == (3,1):
            # 2D column
            pass
        elif P.shape == (1,3):
            # 2D row
            P = P.transpose()
        elif P.shape == (3,):
            # 1D row
            P = P.reshape(-1,1)
        else:
            raise Exception("ERROR_VDS: Invalid input: P")
        if R.shape != (3,3):
            raise Exception("ERROR_VDS: Invalid input: R")

        # The Name of the object in Vicon Tracker
        # Flag for if the object was not detected for this frame
        self._name = Name
        self._isOccluded = numpy.all(numpy.isnan(R)) and numpy.all(numpy.isnan(P))

        # Rotation matrix in Vicon global frame
        # Position in Vicon global frame
        self._R = R
        self._P = P

    def Name(self): return self._name
    def IsOccluded(self): return self._isOccluded

    def x(self): return self._P[0]
    def y(self): return self._P[1]
    def z(self): return self._P[2]

    def P(self): return self._P
    def Ph(self): return numpy.append(self._P, [[1]], axis=0)

    def R(self): return self._R
    def quat_xyzw(self): return scipy.spatial.transform.Rotation.from_matrix(self._R).as_quat()
    def quat_wxyz(self): return self.quat_xyzw().take([3,0,1,2])

    def T(self): return numpy.append( numpy.append(self._R, [[0, 0, 0]], axis=0), self.Ph(), axis=1)

    def Inv(self):
        R_transpose = self._R.T
        return Point(
            Name=self._name + '_inv',
            R=R_transpose,
            P=-R_transpose @ self._P
        )

    # Overloads
    # Based on https://stackoverflow.com/a/3188723
    # Matrix multiplication: result = this @ other
    def __matmul__(self, other):
        if isinstance(other, self.__class__):
            result = self.T() @ other.T()
            return Point(
                Name=self.Name()+'_'+other.Name(),
                R=result[0:3,0:3],
                P=result[0:3,3]
            )
        else:
            raise TypeError(f'ERROR_VDS: Invalid operation for operand @ between: {self.__class__} and {type(other)}')

    # Matrix multiplication: this @= other
    def __imatmul__(self, other):
        if isinstance(other, self.__class__):
            result = self.T() @ other.T()
            return Point(
                Name=self.Name()+'_'+other.Name(),
                R=result[0:3,0:3],
                P=result[0:3,3]
            )
        else:
            raise TypeError(f'ERROR_VDS: Invalid operation for operand @ between: {self.__class__} and {type(other)}')

    # Matrix multiplication: result = other @ this
    def __rmatmul__(self, other):
        if isinstance(other, self.__class__):
            result = other.T() @ self.T()
            return Point(
                Name=self.Name()+'_'+other.Name(),
                R=result[0:3,0:3],
                P=result[0:3,3]
            )
        else:
            raise TypeError(f'ERROR_VDS: Invalid operation for operand @ between: {self.__class__} and {type(other)}')

class Frame:
    def __init__(self, frameNumber=nan, frameTime_seconds=nan):
        self._frameNumber = frameNumber
        self._frameTime_seconds = frameTime_seconds
        self._all = {}

    def FrameNumber(self): return self._frameNumber
    def FrameTime_seconds(self): return self._frameTime_seconds
    def FrameAge_seconds(self): return time.time() - self._frameTime_seconds

    def All(self): return list(self._all.values())
    def All_dict(self): return self._all

    def AddPoint(self, point):
        self._all[point.Name()] = point

    def GetByName(self, name:str):
        if name in self._all:
            # Get from frame data
            return self._all[name]
        else:
            # Create new as occluded
            return Point(name)

    def GetByNames(self, names):
        out = []
        for name in names:
            out.append( self.GetByName(name) )
        return out

    def GetIfNotOccluded(self):
        out = []
        for point in self.All():
            if not point.IsOccluded():
                out.append(point)
        return out


############################################################################################
# Interface
##############################################
# Class is automatically instanced by the interface
# User should not call this
class BackgroundThread:
    def __init__(self,
                 Lock,                   # multiprocessing.Lock
                 IsKillRequest,          # multiprocessing.Event
                 IsFrameReady,           # multiprocessing.Event
                 HasLatestFrameBeenRead, # multiprocessing.Event
                 IsSettingChanged,       # multiprocessing.Event
                 IsObjectFilterActive,   # multiprocessing.Event
                 IsOccludedFilterActive, # multiprocessing.Event
                 ViconFrameRate,         # multiprocessing.Value
                 FrameQueue,             # multiprocessing.Queue
                 AllowedObjectsQueue     # multiprocessing.Queue
                 ):
        self._filter_AllowedObjects = []

        # Variables shared between threads
        self._lock = Lock
        self._IsKillRequest = IsKillRequest
        self._IsFrameReady = IsFrameReady
        self._HasLatestFrameBeenRead = HasLatestFrameBeenRead
        self._IsSettingChanged = IsSettingChanged
        self._IsObjectFilterActive = IsObjectFilterActive
        self._IsOccludedFilterActive = IsOccludedFilterActive
        self._ViconFrameRate = ViconFrameRate
        self._FrameQueue = FrameQueue
        self._AllowedObjectsQueue = AllowedObjectsQueue

    # ************************************************************
    # Frame update thread
    # ******************************
    # Runs in background to update the frame data
    def UpdateFrameInBackground(self, HostName, EnableLightweight):
        # Weird errors happen if ViconDataStream.Client() is in the constructor
        #   Something about how the threads communicate conflicting with how the C++ dlls are called
        #   Therefore, we get one big method because whatever
        client = ViconDataStream.Client()

        # Connect to server
        try:
            client.Connect(HostName)
        except:
            raise Exception("ERROR_VDS: Failed to connect to " + HostName)

        # Apply options
        if EnableLightweight: client.EnableLightweightSegmentData()
        client.SetStreamMode(ViconDataStream.Client.StreamMode.EServerPush)
        client.EnableSegmentData()
        # self._Client.EnableMarkerData()
        client.SetBufferSize(1)
        client.SetAxisMapping(
            ViconDataStream.Client.AxisMapping.EForward,
            ViconDataStream.Client.AxisMapping.ELeft,
            ViconDataStream.Client.AxisMapping.EUp
        )

        # Loop until kill is called
        while not self._IsKillRequest.is_set():
            # All settings change methods should lock while changing and set IsSettingChanged
            #   This prevents delivery of
            #       Corrupt frames from settings changed midway through
            #       Frames using old settings
            #   This should be sufficient, as GetFrame() can not be called simultaneously to a settings change method
            # Wait for in-progress settings changes
            # Then flag that the changes have been processed
            with self._lock:
                self._IsSettingChanged.clear()
            # Any settings change to occur after this point will cause the current frame to be discarded

            # Check for new filter list
            if not self._AllowedObjectsQueue.empty():
                # New list incoming
                with self._lock:
                    self._filter_AllowedObjects.clear()
                    while not self._AllowedObjectsQueue.empty():
                        self._filter_AllowedObjects.append( self._AllowedObjectsQueue.get() )

            # Wait for next frame
            # Vicon does not have timestamp, so make it here
            client.GetFrame()
            timestamp = time.time()

            # Retrieve system data
            self._ViconFrameRate.value = client.GetFrameRate()

            # Create object holding frame data
            frame = Frame(client.GetFrameNumber(), timestamp)

            # Loop over all subjects
            # Decode frame - object pose
            for SubjectName in client.GetSubjectNames():

                # Number of segments should always be 1 when using Vicon Tracker3... as far as I can tell
                SegmentNames = client.GetSegmentNames(SubjectName)
                if len(SegmentNames) != 1:
                    # If this happens, then you get to rewrite this interface to also loop over and store Segment data
                    raise Exception("ERROR_VDS: Invalid assumption that segment count is 1")

                SegmentName = SegmentNames[0]

                # Global translation
                ret_P = client.GetSegmentGlobalTranslation(SubjectName, SegmentName)
                P = numpy.array([ret_P[0]]).T

                # Global rotation matrix
                #	Note: Vicon uses row major order
                ret_R = client.GetSegmentGlobalRotationMatrix(SubjectName, SegmentName)
                R = numpy.array(ret_R[0])

                IsOccluded = ret_P[1] or ret_R[1]

                # Save point to the return object if allowed by filters
                if IsOccluded:
                    point = Point(SubjectName)
                else:
                    point = Point(SubjectName, R, P)

                if not self._AllowedByFilters(point):
                    # Skip adding point
                    continue
                frame.AddPoint(point)

            # Apply AllowedObjects filter if enabled
            LatestFrame_internal = self._SortByObjectFilter(frame)

            with self._lock:
                # Replace public reference to the previous frame with the new frame
                while not self._FrameQueue.empty():
                    self._FrameQueue.get()
                self._FrameQueue.put( LatestFrame_internal )

                # Unblock GetFrame() if no settings have been changed
                # Otherwise get the next frame, as the current frame is corrupted
                if not self._IsSettingChanged.is_set():
                    # Unblock GetFrame()
                    self._HasLatestFrameBeenRead.clear()
                    self._IsFrameReady.set()

        # Kill called
        client.Disconnect()

    #********************************************************************************
    # Helper functions
    #****************************************
    # PURPOSE: Test if the point is allowed by the currently active filters
    def _AllowedByFilters(self, point):
        # Occluded filter
        if self._IsOccludedFilterActive.is_set() and point.IsOccluded(): return False

        # Object filter
        if not self._IsObjectFilterActive.is_set(): return True
        # Search allowed objects list
        for allowedObject_name in self._filter_AllowedObjects:
            if point.Name() == allowedObject_name: return True
        # Not on allow list
        return False

    # PURPOSE: Sort Points by the ordering specified in the AllowedObjects filter
    # If the filter is not active, return the input
    def _SortByObjectFilter(self, frame):
        if not self._IsObjectFilterActive.is_set(): return frame

        frame_sorted = Frame(frame.FrameNumber(), frame.FrameTime_seconds())
        for allowedObject_name in self._filter_AllowedObjects:
            point = frame.GetByName(allowedObject_name)

            # Save point to the return object if allowed by filters
            #	i.e. apply occluded filter
            if self._AllowedByFilters(point): frame_sorted.AddPoint(point)
        return frame_sorted


class Interface:
    def __init__(self):
        # System data
        self._ViconFrameRate = multiprocessing.Value('d', -1)

        # User settings: Filter enables and list
        self._IsSettingChanged = multiprocessing.Event()
        self._IsObjectFilterActive = multiprocessing.Event(); self._IsObjectFilterActive.clear()
        self._IsOccludedFilterActive = multiprocessing.Event(); self._IsOccludedFilterActive.clear()
        self._AllowedObjectsQueue = multiprocessing.Queue()

        # Internal state control
        self._UpdateThread = None
        self._IsConnected = False
        self._lock = multiprocessing.Lock()
        self._IsKillRequest = multiprocessing.Event()
        self._IsFrameReady = multiprocessing.Event()
        self._HasLatestFrameBeenRead = multiprocessing.Event()
        self._LatestFrame = Frame()
        self._FrameQueue = multiprocessing.Queue()

        self._backgroundThread = BackgroundThread(
            Lock=self._lock,
            IsKillRequest=self._IsKillRequest,
            IsFrameReady=self._IsFrameReady,
            HasLatestFrameBeenRead=self._HasLatestFrameBeenRead,
            IsSettingChanged=self._IsSettingChanged,
            IsObjectFilterActive=self._IsObjectFilterActive,
            IsOccludedFilterActive=self._IsOccludedFilterActive,
            ViconFrameRate=self._ViconFrameRate,
            FrameQueue=self._FrameQueue,
            AllowedObjectsQueue=self._AllowedObjectsQueue
        )

    def __del__(self):
        self.Disconnect()

    #********************************************************************************
    # Interface: Connect / Disconnect
    #****************************************
    # PURPOSE:
    #	Call this first to connect to VDS and initialise the client
    # INPUT:
    #	HOSTNAME = IP address of the vicon control computer (the computer running tracker 3)
    #	Flag_Lightweight:
    #		0 = Normal mode
    #		1 = Lightweight mode (Sacrifice precision to reduce the network bandwidth by ~75%)
    def Connect(self, HostName="localhost:801", EnableLightweight=False):
        if self._IsConnected: return # Nothing to do

        # Start thread to listen for data
        self._IsKillRequest.clear()
        self._IsFrameReady.clear()
        self._HasLatestFrameBeenRead.clear()
        self._UpdateThread = multiprocessing.Process(
            target=self._backgroundThread.UpdateFrameInBackground,
            args=(HostName, EnableLightweight)
        )
        self._UpdateThread.start()
        self._IsConnected = True

        # Get first frame to initialise
        self.GetFrame()

        print("INFO_VDS: Ready to capture data")

    # PURPOSE:
    #	Close connection on the client side
    #	Has no effect on the vicon control computer (tracker 3 will keep broadcasting)
    #	Use to free up processing resources
    def Disconnect(self):
        if not self._IsConnected: return # Nothing to do

        # Set kill flag and wait for thread to finish its last loop
        self._IsKillRequest.set()
        self._UpdateThread.join()

        self._IsConnected = False
        print("INFO_VDS: Client is now Disconnected")

    #********************************************************************************
    # Interface: Settings
    #****************************************
    # NOTES:
    #	Upon changing settings, force the next read to get a new frame
    #	as the frame in the buffer applies the old settings, which could cause difficult to debug errors in user programs

    # PURPOSE:
    #	Apply filter to show only the objects in our allow list
    #	VDS actually already implements this.... but it just sets blocked objects to occluded
    # INPUT:
    #	Names of all the objects that you want to capture. Other captured objects will be discarded
    def EnableObjectFilter(self, allowedObjects):
        with self._lock:
            # Empty old values from queue, then fill with new values
            while not self._AllowedObjectsQueue.empty(): self._AllowedObjectsQueue.get()
            for allowedObject in allowedObjects: self._AllowedObjectsQueue.put(allowedObject)

            self._IsObjectFilterActive.set()
            self._IsFrameReady.clear()
            self._IsSettingChanged.set()

    # PURPOSE: Show all captured objects in the output
    # PURPOSE: Do not show occluded objects in the output
    # PURPOSE: Show all captured objects in the output
    def DisableObjectFilter(self):
        with self._lock:
            self._IsObjectFilterActive.clear()
            self._IsFrameReady.clear()
            self._IsSettingChanged.set()

    def EnableOccludedFilter(self):
        with self._lock:
            self._IsOccludedFilterActive.set()
            self._IsFrameReady.clear()
            self._IsSettingChanged.set()

    def DisableOccludedFilter(self):
        with self._lock:
            self._IsOccludedFilterActive.clear()
            self._IsFrameReady.clear()
            self._IsSettingChanged.set()

    #********************************************************************************
    # Interface: Get data frames
    #****************************************
    # PURPOSE:
    #	Get Frame rate of the vicon system [Hz]
    def GetFrameRate(self): return self._ViconFrameRate.value

    # PURPOSE:
    #	Same as GetFrame() but blocks until the next frame arrives
    def GetFrame_WaitForNew(self):
        self._IsFrameReady.clear()
        return self.GetFrame()

    # PURPOSE:
    #	Same as GetFrame() but
    #		If the latest frame has not yet been read, return the cached frame
    #		Otherwise, block until the next unread frame arrives
    def GetFrame_GetUnread(self):
        if self._HasLatestFrameBeenRead.is_set(): self._IsFrameReady.clear()
        return self.GetFrame()

    # PURPOSE:
    #	Get next data frame
    # OUTPUT: Frame object holding the captured data.
    def GetFrame(self):
        if not self._IsConnected:
            print("WARNING_VDS: (GetFrame) Not Connected")
            return Frame()

        # Block until thread signals ready
        self._IsFrameReady.wait()

        # Get and cache new frame if available, otherwise leave old frame in cache
        with self._lock:
            try:
                self._LatestFrame = self._FrameQueue.get_nowait()
            except queue.Empty:
                pass

        # Return frame from cache
        self._HasLatestFrameBeenRead.set()
        return copy.deepcopy(self._LatestFrame)


############################################################################################
# Test
##############################################
if __name__ == "__main__":
    # Basic test
    # Get and print a frame
    vdsi = Interface()
    vdsi.Connect()

    frame = vdsi.GetFrame()
    print('Frame Rate [Hz]:     ', vdsi.GetFrameRate())
    print('Frame Number [Count]:', frame.FrameNumber())

    for point in frame.All():
        print(point.Name(), point.P().transpose())

    vdsi.Disconnect()
