/*
Written by:			Brandon Johns
Version created:	2021-11-04
Last edited:		2022-04-22

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

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Manages points of the Point class - storage, retrieval by name
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	class Points
	{
	public:
		// Vector of Point objects
		std::vector<vdsi::Point> all;

		//********************************************************************************
		// Interface: Set
		//****************************************
		// INPUT: shared_prt to Point
		void AddPoint(vdsi::Point point)
		{
			// Save point
			this->all.push_back(point);
		}

		//********************************************************************************
		// Interface: Get
		//****************************************
		// INPUT: Point name
		// OUTPUT: Copy of the found point
		vdsi::Point Get(std::string name)
		{
			for(auto&& point : this->all)
			{
				// Return copy of point upon finding
				if (point.viconObjectName == name) { return vdsi::Point(point); }
			}

			// No point found => return occluded
			return vdsi::Point(name);
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
			bool wasSuccessful =
				   lightweightResult
				&& streamModeResult
				&& segmentDataResult;
			if( !wasSuccessful ) { throw std::runtime_error("ERROR_VDS: Failed to initialise"); }

			// Start thread to listen for data
			this->IsKillReqest = false;
			this->IsFrameReady = false;
			this->HasLatestFrameBeenRead = false;
			this->UpdateThread = std::make_unique<std::thread>( [this] { this->UpdateFrameInBackground(); });

			std::cout << "INFO_VDS: Ready to capture data" << std::endl;
			this->IsConnected = true;
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
		// Interace: Settings
		//****************************************
		// PURPOSE:
		//	Apply filter to show only the objects in our allow list
		//	VDS actually already implements this.... but it just sets blocked objects to occluded
		// INPUT:
		//	Names of all the objects that you want to capture. Other captured objects will be discarded
		void EnableObjectFilter(std::vector<std::string> allowedObjects)
		{
			this->IsObjectFilterActive = true;
			this->filter_AllowedObjects = allowedObjects;
		}

		// PUTPOSE: Show all captured objects in the output
		// PUTPOSE: Do not show occluded objects in the output
		// PUTPOSE: Show all captured objects in the output
		void DisableObjectFilter()   { this->IsObjectFilterActive = false; }
		void EnableOccludedFilter()  { this->IsOccludedFilterActive = true; }
		void DisableOccludedFilter() { this->IsOccludedFilterActive = false; }

		//********************************************************************************
		// Interface: Get data frames
		//****************************************
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
			while( ! this->IsFrameReady ) { std::this_thread::sleep_for(std::chrono::microseconds(1)); }

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
				std::vector<double> P;
				std::copy(std::begin(ret_P.Translation), std::end(ret_P.Translation), std::begin(P));

				// Global rotation matrix
				//	Note: Vicon uses row major order
				vds::Output_GetSegmentGlobalRotationMatrix ret_R = this->Client.GetSegmentGlobalRotationMatrix(SubjectName, SegmentName);
				std::vector<double> R;
				std::copy(std::begin(ret_R.Rotation), std::end(ret_R.Rotation), std::begin(R));

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
				vdsi::Point point(SubjectName, R, P, IsOccluded);
				if(this->AllowedByFilters(point)) { Points.AddPoint(point); }
			}
			
			// Apply AllowedObjects filter if enabled
			return this->SortByObjectFilter(Points);
		}

		//********************************************************************************
		// Helper functions
		//****************************************
		// PURPOSE: Test if the point is alloed by the currently active filters
		bool AllowedByFilters(const vdsi::Point& point)
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
			for(auto&& allowedObject_name : this->filter_AllowedObjects)
			{
				vdsi::Point point = Points.Get(allowedObject_name);

				// Save point to the return object if allowed by filters
				//	i.e. apply occluded filter
				if(this->AllowedByFilters(point)) { Points_sorted.AddPoint(point); }
			}
			return Points_sorted;
		}
	};
}