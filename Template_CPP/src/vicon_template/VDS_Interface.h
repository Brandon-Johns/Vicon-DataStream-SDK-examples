/*
Written by:			Brandon Johns
Version created:	2021-11-04
Last edited:		2022-04-28

Version changes:
	NA

Purpose:
	Interface to the Vicon DataStream SDK (Using Version 1.11.0)
	Manages the returned data

Class Summary:
	VDS_Interface
		Wrapper for the Vicon DataStream SDK

	Point
		Stores the position and rotation of vicon objects

	Points
		Stores collections Point objects.
		Interface allows retrieval of a Point by name

*/
#pragma once

// Vicon DataStream SDK
#include "DataStreamClient.h"
#include "DataStreamRetimingClient.h"
namespace vds = ViconDataStreamSDK::CPP;

// Standard library
#include <iostream>
#include <mutex>
#include <chrono>
#include <thread>
#include <vector>
#include <stdexcept>
#include <algorithm>


namespace vdsi
{
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Stores the position and rotaion of vicon objects
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// I suggest editing this to use the Armadillo C++ Maths library,
	// but to keep it simple for the template, I use std::vector
	class Point
	{
	public:
		// Object name, as appears in Vicon Tracker (VDS calls this the "SubjectName")
		// Global Transformation - Rotation matrix, stored in row major order
		// Global Transformation - Position vector
		// Set if the object was occluded
		std::string viconObjectName;
		std::vector<double> R_rowMajor;
		std::vector<double> P;
		bool IsOccluded;

		//********************************************************************************
		// Interface: Create
		//****************************************
		// INPUT: (see variable defs)
		Point(
			std::string name_in,
			std::vector<double> R_in = std::vector<double>(9, nan("")),
			std::vector<double> P_in = std::vector<double>(3, nan("")),
			bool occluded_in = true
		) :
			viconObjectName(name_in),
			R_rowMajor(R_in),
			P(P_in),
			IsOccluded(occluded_in)
		{
			// Validate input
			if(this->R_rowMajor.size() != 9) { throw std::runtime_error("ERROR_VDS: R_rowMajor is wrong size"); }
			if(this->P.size() != 3)        { throw std::runtime_error("ERROR_VDS: P is wrong size"); }
		}

		//********************************************************************************
		// Interface: Get
		//****************************************
		// OUTPUT: x,y,z coordinates
		double x() { return this->P.at(0); }
		double y() { return this->P.at(1); }
		double z() { return this->P.at(2); }

		// OUTPUT: the element of the rotation matrix, R(col,row)
		//	Counting starts at 1, because I said so! => valid range=[1:3]
		double R_at(uint8_t col, uint8_t row)
		{
			// Validate input
			if(1>row||row>3 || 1>col||col>3) {throw  std::runtime_error("ERROR_VDS: Out of bounds");}

			return this->R_rowMajor.at(3*(col-1) + (row-1));
		}
	};

	class Point_Marker : public Point
	{
	public:
		//********************************************************************************
		// Interface: Create
		//****************************************
		// INPUT: (see variable defs)
		Point_Marker(
			std::string name_in,
			std::vector<double> P_in = std::vector<double>(3, nan("")),
			bool occluded_in = true
		) :
			Point(
				name_in,
				std::vector<double>(9, nan("")),
				P_in,
				occluded_in)
		{
			// Nothing to do
		}
	};

	class Point_Object : public Point
	{
	public:
		// Child markers of this point
		std::vector<vdsi::Point_Marker> markers;

		//********************************************************************************
		// Interface: Create
		//****************************************
		// INPUT: (see variable defs)
		Point_Object(
			std::string name_in,
			std::vector<double> R_in = std::vector<double>(9, nan("")),
			std::vector<double> P_in = std::vector<double>(3, nan("")),
			bool occluded_in = true
		) :
			Point(
				name_in,
				R_in,
				P_in,
				occluded_in)
		{
			// Nothing to do
		}

		//********************************************************************************
		// Interface: Set
		//****************************************
		// INPUT: Point_Marker
		void AddMarker(vdsi::Point_Marker marker)
		{
			// Save point
			this->markers.push_back(marker);
		}

		//********************************************************************************
		// Interface: Get
		//****************************************
		// INPUT: Marker name
		// OUTPUT: Copy of the found Marker
		vdsi::Point_Marker Get(std::string name)
		{
			for (auto&& marker : this->markers)
			{
				// Return copy of point upon finding
				if (marker.viconObjectName == name) { return vdsi::Point_Marker(marker); }
			}

			// No point found => return occluded
			return vdsi::Point_Marker(name);
		}

	};

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Manages points of the Point class - storage, retrieval by name
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	class Points
	{
	public:
		// Vector of Point objects
		// VDS Frame number (incremental counter)
		std::vector<vdsi::Point_Object> all;
		unsigned int frameNumber = 0;

		//********************************************************************************
		// Interface: Set
		//****************************************
		// INPUT: Point
		void AddPoint(vdsi::Point_Object point)
		{
			// Save point
			this->all.push_back(point);
		}

		//********************************************************************************
		// Interface: Get
		//****************************************
		// INPUT: Point name
		// OUTPUT: Copy of the found point
		vdsi::Point_Object Get(std::string name)
		{
			for(auto&& point : this->all)
			{
				// Return copy of point upon finding
				if (point.viconObjectName == name) { return vdsi::Point_Object(point); }
			}

			// No point found => return occluded
			return vdsi::Point_Object(name);
		}
	};

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Interface to VDS
	//	Trust me, it's better than the raw interface
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	class VDS_Interface
	{
	private:
		vds::Client Client;

		// System data
		std::atomic<double> ViconFrameRate = nan("");

		// User settings: Filter enables and list
		std::atomic<bool> IsObjectFilterActive = false;
		std::atomic<bool> IsOccludedFilterActive = false;
		std::vector<std::string> filter_AllowedObjects;

		// Internal state control
		std::unique_ptr<std::thread> UpdateThread;
		std::atomic<bool> IsConnected = false;
		std::atomic<bool> IsKillReqest = false;
		std::atomic<bool> IsFrameReady = false;
		std::atomic<bool> HasLatestFrameBeenRead = false;
		vdsi::Points LatestFrame;
		std::mutex mtx_LatestFrame;

	public:
		//********************************************************************************
		// Interface: Constructor / Destructor
		//****************************************
		VDS_Interface() { }
		~VDS_Interface() { this->Disconnect(); }

		//********************************************************************************
		// Interface: Connect / Disconnect
		//****************************************
		// PURPOSE:
		//	Call this first to connect to VDS and initialise the client
		// INPUT:
		//	HOSTNAME = IP address of the vicon control computer (the computer running tracker 3)
		//	Flag_Lightweight:
		//		0 = Normal mode
		//		1 = Lightweight mode (Sacrifice precision to reduce the network bandwidth by ~75%)
		void Connect(std::string HostName = "localhost:801", bool EnableLightweight = false)
		{
			if(this->IsConnected) { return; } // Nothing to do

			// Connect to server
			if(this->Client.Connect(HostName).Result != vds::Result::Success)
			{
				throw std::runtime_error("ERROR_VDS: Failed to connect to " + HostName);
			}

			// Apply options
			bool lightweightResult = EnableLightweight ? this->Client.EnableLightweightSegmentData().Result != vds::Result::Success : true;
			bool streamModeResult = this->Client.SetStreamMode( vds::StreamMode::ServerPush ).Result == vds::Result::Success;
			bool segmentDataResult = this->Client.EnableSegmentData().Result == vds::Result::Success;
			bool markerDataResult = this->Client.EnableMarkerData().Result == vds::Result::Success;
			bool wasSuccessful =
				   lightweightResult
				&& streamModeResult
				&& segmentDataResult
				&& markerDataResult;
			if( !wasSuccessful ) { throw std::runtime_error("ERROR_VDS: Failed to initialise"); }

			// Start thread to listen for data
			this->IsKillReqest = false;
			this->IsFrameReady = false;
			this->HasLatestFrameBeenRead = false;
			this->UpdateThread = std::make_unique<std::thread>( [this] { this->UpdateFrameInBackground(); });
			this->IsConnected = true;

			// Get first frame to initialise
			this->GetFrame();

			std::cout << "INFO_VDS: Ready to capture data" << std::endl;
		}

		// PURPOSE:
		//	Close connection on the client side
		//	Has no effect on the vicon control computer (tracker 3 will keep broadcasting)
		//	Use to free up processing recources
		void Disconnect()
		{
			if( ! this->IsConnected) { return; } // Nothing to do

			// Set kill flag and wait for thread to finish it's last loop
			this->IsKillReqest = true;
			this->UpdateThread->join();

			this->Client.Disconnect();
			this->IsConnected = false;
		}

		//********************************************************************************
		// Interface: Settings
		//****************************************
		// NOTES:
		//	Upon changing settings, force the next read to get a new frame
		//	as the frame in the buffer applies the old settings, which could cause difficult to debug errors in user programs
		//	(difficult to debug because setting breakpoints allows enough time for the next frame to be read... removing the problem only while debugging)

		// PURPOSE:
		//	Apply filter to show only the objects in our allow list
		//	VDS actually already implements this.... but it just sets blocked objects to occluded
		// INPUT:
		//	Names of all the objects that you want to capture. Other captured objects will be discarded
		void EnableObjectFilter(std::vector<std::string> allowedObjects)
		{
			this->IsObjectFilterActive = true;
			this->filter_AllowedObjects = allowedObjects;
			this->IsFrameReady = false;
		}

		// PUTPOSE: Show all captured objects in the output
		// PUTPOSE: Do not show occluded objects in the output
		// PUTPOSE: Show all captured objects in the output
		void DisableObjectFilter()   { this->IsObjectFilterActive = false;   this->IsFrameReady = false; }
		void EnableOccludedFilter()  { this->IsOccludedFilterActive = true;  this->IsFrameReady = false; }
		void DisableOccludedFilter() { this->IsOccludedFilterActive = false; this->IsFrameReady = false;}

		//********************************************************************************
		// Interface: Get data frames
		//****************************************
		// PURPOSE:
		//	Get Frame rate of the vicon system [Hz]
		double GetFrameRate() { return this->ViconFrameRate; }

		// PURPOSE:
		//	Same as GetFrame() but blocks until the next frame arrives
		vdsi::Points GetFrame_WaitForNew()
		{
			this->IsFrameReady = false;
			return this->GetFrame();
		}

		// PURPOSE:
		//	Same as GetFrame() but
		//		If the latest frame has not yet been read, return the cached frame
		//		Otherwise, block until the next unread frame arrives
		vdsi::Points GetFrame_GetUnread()
		{
			if(this->HasLatestFrameBeenRead) {this->IsFrameReady = false;}
			return this->GetFrame();
		}

		// PURPOSE:
		//	Get next data frame
		// OUTPUT: Points object holding the captured data.
		vdsi::Points GetFrame()
		{
			if( ! this->IsConnected )
			{
				std::cout << "WARNING_VDS: (GetFrame) Not Connected" << std::endl;
				return vdsi::Points();
			}

			// Block until thread signals ready
			//	NOTE:
			//		The following does not work (On Windows at least, it sleeps way longer than it should)
			//		std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			//		==> Instead, use a no-op
			//		((void)0);
			while (!this->IsFrameReady) { ((void)0); }

			// This statement is written very specifically to invoke the copy contructor of vdsi::Points
			// See syntax differences to call copy constructor VS operator=
			this->mtx_LatestFrame.lock();
			auto LatestFrame_copy(this->LatestFrame);
			this->mtx_LatestFrame.unlock();

			this->HasLatestFrameBeenRead = true;
			return LatestFrame_copy;
		}

	private:
		//************************************************************
		// Frame update thread
		//******************************
		// Runs in background to update the frame data
		void UpdateFrameInBackground()
		{
			while( ! this->IsKillReqest )
			{
				// Wait for next frame
				auto UpdateResult = Client.GetFrame();

				// Retrieve system data
				this->ViconFrameRate = Client.GetFrameRate().FrameRateHz;

				// Create object holding frame data
				vdsi::Points LatestFrame_internal = this->DecodeFrame();

				// Replace public reference to the previous frame with the new frame
				this->mtx_LatestFrame.lock();
				this->LatestFrame = LatestFrame_internal;
				this->mtx_LatestFrame.unlock();

				// (Only first loop) Unblock GetFrame()
				this->HasLatestFrameBeenRead = false;
				this->IsFrameReady = true;
			}
		}

		// PURPOSE:
		//	Get next data frame from VDS (frame as in snapshot of system state at current time)
		//	Decode the data frame
		//	Apply filtering
		vdsi::Points DecodeFrame()
		{
			// Object to return
			vdsi::Points Points;
			Points.frameNumber = Client.GetFrameNumber().FrameNumber;

			// Loop over all subjects
			unsigned int numS = this->Client.GetSubjectCount().SubjectCount;
			for (unsigned int idxSubject = 0; idxSubject < numS; ++idxSubject)
			{
				std::string SubjectName = this->Client.GetSubjectName(idxSubject).SubjectName;

				// Number of segments should always be 1 when using Vicon Tracker3... as far as I can tell
				unsigned int SegmentCount = this->Client.GetSegmentCount(SubjectName).SegmentCount;
				if (SegmentCount!=1)
				{
					// If this happens, then you get to rewrite this interface to also loop over and store Segment data
					// Refer to the example that comes with the Vicon DataStream SDK "ViconDataStreamSDK_CPPRetimerTest.cpp"
					throw std::runtime_error("ERROR_VDS: Invalid assumption that segment count is 1");
				}

				// Segment name
				unsigned int idxSegment = 0;
				std::string SegmentName = this->Client.GetSegmentName(SubjectName, idxSegment).SegmentName;

				// Global translation
				vds::Output_GetSegmentGlobalTranslation ret_P = this->Client.GetSegmentGlobalTranslation(SubjectName, SegmentName);
				std::vector<double> P(std::begin(ret_P.Translation), std::end(ret_P.Translation));

				// Global rotation matrix
				//	Note: Vicon uses row major order
				vds::Output_GetSegmentGlobalRotationMatrix ret_R = this->Client.GetSegmentGlobalRotationMatrix(SubjectName, SegmentName);
				std::vector<double> R(std::begin(ret_R.Rotation), std::end(ret_R.Rotation));

				// The occluded return value is broken - Always gives 0 (meaning not occluded)
				// I'd like to do this:
				//		bool IsOccluded = ret_R.Occluded || ret_P.Occluded;
				// But oh well. Let's do it manually
				// If it's occluded, all values return exactly 0 (at least that works)
				// It's pretty unlikely that a real value will be exactly 0 in double precision
				// Hence:
				bool IsOccluded = (
						   ret_R.Rotation[0] == 0
						&& ret_R.Rotation[1] == 0
						&& ret_R.Rotation[2] == 0
						&& ret_R.Rotation[3] == 0
						&& ret_R.Rotation[4] == 0
						&& ret_R.Rotation[5] == 0
						&& ret_R.Rotation[6] == 0
						&& ret_R.Rotation[7] == 0
						&& ret_R.Rotation[8] == 0
					) || (
						   ret_P.Translation[0] == 0
						&& ret_P.Translation[1] == 0
						&& ret_P.Translation[2] == 0
					);

				// Save point to the return object if allowed by filters
				vdsi::Point_Object point(SubjectName, R, P, IsOccluded);
				if ( ! this->AllowedByFilters(point) ) { continue; }

				// Markers
				unsigned int numM = this->Client.GetMarkerCount(SubjectName).MarkerCount;
				for (unsigned int idxMarker = 0; idxMarker < numM; ++idxMarker)
				{
					// Maker name and global translation
					std::string MarkerName = this->Client.GetMarkerName(SubjectName, idxMarker).MarkerName;
					vds::Output_GetMarkerGlobalTranslation retM_P = this->Client.GetMarkerGlobalTranslation(SubjectName, MarkerName);
					std::vector<double> MarkerP(std::begin(retM_P.Translation), std::end(retM_P.Translation));
					bool marker_IsOccluded = retM_P.Occluded;

					// Save marker to object
					//	Marker arrays should stay same size for a given object, otherwise access would be painful
					//	Let's trust that VDS always specifies them in the same order...
					vdsi::Point_Marker marker(MarkerName, P, marker_IsOccluded);
					if (!marker_IsOccluded)
					{
						point.AddMarker(marker);
					}
					else
					{
						// Create occluded
						point.AddMarker(vdsi::Point_Marker(MarkerName));
					}
				}

				Points.AddPoint(point);

			}
			
			// Apply AllowedObjects filter if enabled
			return this->SortByObjectFilter(Points);
		}

		//********************************************************************************
		// Helper functions
		//****************************************
		// PURPOSE: Test if the point is alloed by the currently active filters
		bool AllowedByFilters(const vdsi::Point_Object& point)
		{
			// Occluded filter
			if(this->IsOccludedFilterActive && point.IsOccluded) {return false;}

			// Object filter
			if( ! this->IsObjectFilterActive ) {return true;}
			// Seach allowed objects list
			for(auto&& allowedObject_name : this->filter_AllowedObjects)
			{
				if(point.viconObjectName == allowedObject_name) {return true;}
			}
			// Not on allow list
			return false;
		}

		// PURPOSE: Sort Points by the ordering specified in the AllowedObjects filter
		// If the filter is not active, return the input
		vdsi::Points SortByObjectFilter(vdsi::Points& Points) {
			if( ! this->IsObjectFilterActive ) {return Points; }

			vdsi::Points Points_sorted;
			Points_sorted.frameNumber = Points.frameNumber;
			for(auto&& allowedObject_name : this->filter_AllowedObjects)
			{
				vdsi::Point_Object point = Points.Get(allowedObject_name);

				// Save point to the return object if allowed by filters
				//	i.e. apply occluded filter
				if(this->AllowedByFilters(point)) { Points_sorted.AddPoint(point); }
			}
			return Points_sorted;
		}
	};
}