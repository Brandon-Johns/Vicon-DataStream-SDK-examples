/*
Written by:			Brandon Johns
Version created:	2021-11-04
Last edited:		2021-12-13

Version changes:
	NA

Purpose:
	Interface to the Vicon DataStream SDK (Using Version 1.11.0)
	Manages the returned data

Class Summary:
	(ABSTRACT) VDS_Interface
	(ABSTRACT) VDS_DirectClient_Interface
	VDS_ServerPush_Interface
	VDS_ClientPull_Interface
	VDS_ClientPullPreFetch_Interface
	VDS_Retimer_Interface
		Interfaces to the Vicon DataStream SDK Retiming Client
		Returns data as objects of type 'Point'
		Choice of interface changes the behaviour for UpdateFrame() method
			VDS_ServerPush_Interface         = low latency, blocking
			VDS_ClientPull_Interface         = low bandwidth, bocking
			VDS_ClientPullPreFetch_Interface = med bandwidth, not bocking - returns last received frame
			VDS_Retimer_Interface            = low latency, not blocking - returns forward predicted frame (but loss of accuracy)

	Point
		Stores the position and rotation of vicon objects

	Points
		Stores collections Point objects.
		Interface allows retrieval of a Point by name

*/
#pragma once
#include "main.h"

// Vicon DataStream SDK
#include "DataStreamClient.h"
#include "DataStreamRetimingClient.h"
namespace vds = ViconDataStreamSDK::CPP;


namespace vdsi
{
	// Forward Declaration required for typedef
	class Point;
	typedef std::shared_ptr<vdsi::Point> point_ptr;


	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Stores the position and rotaion of vicon objects
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// I suggest editing this to use the Armadillo C++ Maths library,
	// but to keep it simple for the template, I use std::array
	class Point
	{
	public:
		// Object name, as appears in Vicon Tracker (VDS calls this the "SubjectName")
		std::string viconObjectName;

		// Global Transformation - Rotation matrix, stored in row major order
		std::array<double,9> R_rowMajor;

		// Global Transformation - Position vector
		std::array<double,3> P;

		// This is set if the object was occluded
		bool IsOccluded;

		//********************************************************************************
		// Interface: Create
		//****************************************
		// INPUT: (see variable defs)
		Point(std::string name_in, std::array<double,9> R_in, std::array<double,3> P_in, bool occluded_in)
		{
			// Save all the input
			this->viconObjectName = name_in;
			this->R_rowMajor = R_in;
			this->P = P_in;
			this->IsOccluded = occluded_in;
		}

		//********************************************************************************
		// Interface: Get
		//****************************************
		// OUTPUT: Matrices encoded as a std::vector
		std::vector<double> R_rowMajor_vec() { return std::vector<double>(this->R_rowMajor.begin(), this->R_rowMajor.end()); }
		std::vector<double> P_vec() { return std::vector<double>(this->P.begin(), this->P.end()); }

		// OUTPUT: the element of the rotation matrix, R(col,row)
		//	Counting starts at 1, because I said so! => valid range=[1:3]
		double R_at(uint8_t col, uint8_t row)
		{
			// Input validation
			if(1>row||row>3 || 1>col||col>3) {throw "BJ_ERROR: Out of bounds";}

			return this->R_rowMajor.at(3*(col-1) + (row-1));
		}

		// OUTPUT: x,y,z coordinates
		double x() { return this->P.at(0); }
		double y() { return this->P.at(1); }
		double z() { return this->P.at(2); }

		// OUTPUT: position as a string (x,y,z)
		std::string P_str()
		{
			return
				std::to_string(this->x()) + ", "
				+ std::to_string(this->y()) + ", "
				+ std::to_string(this->z());
		}
		// OUTPUT: rotation matrix as a string (in row major format)
		std::string R_str_rowMajor()
		{
			return
				std::to_string(this->R_rowMajor.at(0)) + ", "
				+ std::to_string(this->R_rowMajor.at(1)) + ", "
				+ std::to_string(this->R_rowMajor.at(2)) + ", "
				+ std::to_string(this->R_rowMajor.at(3)) + ", "
				+ std::to_string(this->R_rowMajor.at(4)) + ", "
				+ std::to_string(this->R_rowMajor.at(5)) + ", "
				+ std::to_string(this->R_rowMajor.at(6)) + ", "
				+ std::to_string(this->R_rowMajor.at(7)) + ", "
				+ std::to_string(this->R_rowMajor.at(8));
		}
	};

	
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Manages points of the Point class - storage, retrieval by name
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	class Points
	{
	public:
		// Vector of pointers to the Point object
		std::vector< vdsi::point_ptr > all;

		//********************************************************************************
		// Interface: Set
		//****************************************
		// INPUT: shared_prt to Point
		void AddPoint(point_ptr point)
		{
			// Save point
			this->all.push_back(point);
		}

		//********************************************************************************
		// Interface: Get
		//****************************************
		// INPUT: Point name
		// OUTPUT: Pointer to the found point or a nullptr if not found
		vdsi::point_ptr Get(std::string name)
		{
			// Search for point by name
			for(auto point : this->all)
			{
				if (point->viconObjectName == name)
				{
					return point;
				}
			}

			// no point found => return nullptr
			return nullptr;
		}

		// Print all points
		// INPUT: pointer to stream to print to
		//		To Terminal: out = &std::cout;
		//		To File:     out = new std::ofstream("out.txt");
		void Print(std::ostream& out)
		{
			out << "----------------------" << std::endl;
			for(auto point : this->all)
			{
				if(!point->IsOccluded)
				{
					out << point->viconObjectName << std::endl;
					out << "Global Position: " << point->P_str() << std::endl;
					out << "Global Translation: " << point->R_str_rowMajor() << std::endl;
				}
			}
		}
	};

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// (ABSTRACT) Interface to VDS
	//	Trust me, it's better than the raw interface
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	template<class vds_Client>
	class VDS_Interface
	{
	private:
		// Switch filters on or off
		bool IsObjectFilterActive = false;
		bool IsOccludedFilterActive = false;

		// Vector of vicon object names that are allowed by our filter
		std::vector<std::string> filter_AllowedObjects;

	public:
		vds_Client Client;

		//********************************************************************************
		// Interface: Start / Stop
		//****************************************
		// PURPOSE:
		//	Call this first to connect to VDS and initialise the client
		// INPUT:
		//	HOSTNAME = IP address of the vicon control computer (the computer running tracker 3)
		//	Flag_Lightweight:
		//		0 = Normal mode
		//		1 = Lightweight mode (Sacrifice precision to reduce the network bandwidth by ~75%)
		void Initialise(std::string HostName, bool Flag_Lightweight)
		{
			// Make client and connect to server
			if (this->Client.Connect(HostName).Result != vds::Result::Success)
			{
				throw std::runtime_error("BJ_ERROR: (VDS) Could not connect to " + HostName);
			}

			// Apply options
			if (Flag_Lightweight)
			{
				std::cout << "BJ_INFO: (VDS) Using lightweight segment data" << std::endl;
				if( this->Client.EnableLightweightSegmentData().Result != vds::Result::Success )
				{
					std::cout << "BJ_WARNING: (VDS) Lightweight segment data did not enable. Continuing in normal mode" << std::endl;
				}
			}

			// Client specific setings
			if(! Initialise_Specifics() )
			{
				std::cout << "BJ_WARNING: (VDS) Client did not properly initialise" << std::endl;
			}

			// Mostly for the retimer interface, but a good check for the others
			//	Wait for the VDS retimer buffer to fill (requirement for it to produce output)
			std::cout << "BJ_INFO: (VDS) Filling buffer - Requres the cameras to see some objects" << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			while( ! this->UpdateFrame_NoWarn() )
			{
				// No specific reason for this time length
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			std::cout << "BJ_INFO: (VDS) Ready to capture data" << std::endl;
		}

		// PURPOSE:
		//	Close connection on the client side
		//	Has no effect on the vicon control computer (tracker 3 will keep broadcasting)
		//	Use to free up processing recources
		void Disconnect()
		{
			this->Client.Disconnect();
		}

		//********************************************************************************
		// Interace: Settings
		//****************************************
		// PURPOSE:
		//	Apply filter to show only the object in our allow list
		//	VDS actually already implements this.... but it just sets blocked objects to occluded
		// INPUT:
		//	Names of all the objects that you want to capture. Other captured objects will be discarded
		void EnableObjectFilter(std::vector<std::string> allowedObjects)
		{
			this->IsObjectFilterActive = true;
			this->filter_AllowedObjects = allowedObjects;
		}

		// PUTPOSE: Show all captured objects in the output
		void DisableObjectFilter() { this->IsObjectFilterActive = false; }

		// PUTPOSE: Do not show occluded objects in the output
		void EnableOccludedFilter() { this->IsOccludedFilterActive = true; }

		// PUTPOSE: Show all captured objects in the output
		void DisableOccludedFilter() { this->IsOccludedFilterActive = false; }

		//********************************************************************************
		// Interface: Control during operation
		//****************************************
		// PURPOSE:
		//	Get next data frame from VDS (frame as in snapshot of system state at current time)
		// INFO:
		//	See specifics under UpdataFrame_NoWarn()
		void UpdateFrame()
		{
			auto UpdateResult = this->UpdateFrame_NoWarn();
			if (! UpdateResult )
			{
				// This should only happen if the client is disconnected
				std::cout << "BJ_WARNING: (VDS) Frame update failed for this frame" << std::endl;
			}
		}

		// PURPOSE:
		//	Decode the data frame that was captured when UpdateFrame was last called
		//	Filtering is applied here
		// OUTPUT: Points object holding the captured data.
		vdsi::Points FrameToPoints()
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
					throw std::runtime_error("BJ_ERROR: (VDS) Invalid assumption that segment count is 1");
				}

				// Segment name
				unsigned int idxSegment = 0;
				std::string SegmentName = this->Client.GetSegmentName(SubjectName, idxSegment).SegmentName;

				// Global translation
				vds::Output_GetSegmentGlobalTranslation ret_P = this->Client.GetSegmentGlobalTranslation(SubjectName, SegmentName);
				std::array<double,3> P;
				std::copy(std::begin(ret_P.Translation), std::end(ret_P.Translation), std::begin(P));

				// Global rotation matrix
				//	Note: Vicon uses row major order
				vds::Output_GetSegmentGlobalRotationMatrix ret_R = this->Client.GetSegmentGlobalRotationMatrix(SubjectName, SegmentName);
				std::array<double,9> R;
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

				// Make point
				auto point = std::make_shared<vdsi::Point>(SubjectName, R, P, IsOccluded);

				// Save point to the return object if allowed by filters
				if(this->AllowedByFilters(point))
				{
					Points.AddPoint(point);
				}
			}
			
			// If Object filter enabled & show occluded, then output in same order as filter_AllowedObjects
			if( !this->IsOccludedFilterActive && this->IsObjectFilterActive)
			{
				vdsi::Points Points_sorted;
				for(auto& allowedObject_name : this->filter_AllowedObjects)
				{
					vdsi::point_ptr point = Points.Get(allowedObject_name);
					if(!point)
					{
						// Point not found => create as occluded
						std::array<double,3> P = {};
						std::array<double,9> R = {};
						bool IsOccluded = true;
						point = std::make_shared<vdsi::Point>(allowedObject_name, R, P, IsOccluded);
					}
					Points_sorted.AddPoint(point);
				}
				Points = Points_sorted;
			}

			return Points;
		}

	private:
		//********************************************************************************
		// Virtual functions
		//****************************************
		// Client specific setings
		// OUTPUT: true = success, false = error
		virtual bool Initialise_Specifics() = 0;

		// UpdateFrame, but returns warning as an output instead of printing the warning
		// OUTPUT: true = success, false = error
		virtual bool UpdateFrame_NoWarn() = 0;

		//********************************************************************************
		// Helper functions
		//****************************************
		// PURPOSE: Test if the point is alloed by the currently active filters
		// INPUT: shared_prt to Point
		bool AllowedByFilters(point_ptr point)
		{
			// Apply filters
			if(this->IsOccludedFilterActive && point->IsOccluded)
			{
				return false;
			}

			bool PassesObjectFilter = true;
			if(this->IsObjectFilterActive)
			{
				PassesObjectFilter = false;

				// Seach allowed objects list
				for(auto allowedObject_name : this->filter_AllowedObjects)
				{
					if(point->viconObjectName == allowedObject_name)
					{
						// Found on allow list
						PassesObjectFilter = true;
						break;
					}
				}
			}

			return PassesObjectFilter;
		}

	};

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// (ABSTRACT) Interface to VDS
	//	Direct Client
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	class VDS_DirectClient_Interface : public VDS_Interface<vds::Client>
	{
	private:
		//********************************************************************************
		// Virtual functions
		//****************************************
		// Client specific setings
		// OUTPUT: true = success, false = error
		virtual bool Initialise_Specifics_SteamMode() = 0;

		//********************************************************************************
		// Instance virtual functions from superclass
		//****************************************
		bool Initialise_Specifics()
		{
			bool streamModeResult_Result = Initialise_Specifics_SteamMode();

			// For the Direct Client, data types need to be explicitly enabled
			vds::Output_EnableSegmentData segmentDataResult = this->Client.EnableSegmentData();

			bool WasSuccessful =
				   streamModeResult_Result
				&& segmentDataResult.Result == vds::Result::Success;
			
			return WasSuccessful;
		}

		bool UpdateFrame_NoWarn()
		{
			auto UpdateResult = this->Client.GetFrame();
			bool WasSuccessful = UpdateResult.Result == vds::Result::Success;
			return WasSuccessful;
		}

	};

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Interface to VDS
	//	Direct Client > ClientPullPreFetch
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// UpdateFrame()
	//	Returns latest cached frame (up to a few miliseconds out of date e.g. up to 10ms lag with Vicon running at 100Hz)
	//	Is non-blocking
	// See description in:
	//	VDS documentation > Client Class Reference > "SetStreamMode" method
	class VDS_ClientPullPreFetch_Interface : public VDS_DirectClient_Interface
	{
	private:
		//********************************************************************************
		// Instance virtual functions from superclass
		//****************************************
		bool Initialise_Specifics_SteamMode()
		{
			vds::Output_SetStreamMode streamModeResult = this->Client.SetStreamMode ( vds::StreamMode::ClientPullPreFetch );
			return streamModeResult.Result == vds::Result::Success;
		}
	};

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Interface to VDS
	//	Direct Client > ClientPull
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// UpdateFrame()
	//	Requests a frame on call
	//	Blocks until next frame is received
	// See description in:
	//	VDS documentation > Client Class Reference > "SetStreamMode" method
	class VDS_ClientPull_Interface : public VDS_DirectClient_Interface
	{
	private:
		//********************************************************************************
		// Instance virtual functions from superclass
		//****************************************
		bool Initialise_Specifics_SteamMode()
		{
			vds::Output_SetStreamMode streamModeResult = this->Client.SetStreamMode ( vds::StreamMode::ClientPull );
			return streamModeResult.Result == vds::Result::Success;
		}
	};

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Interface to VDS
	//	Direct Client > ServerPush
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// UpdateFrame()
	//	Waits for the server to send the next frame
	//	Blocks until next frame is received
	// See description in:
	//	VDS documentation > Client Class Reference > "SetStreamMode" method
	class VDS_ServerPush_Interface : public VDS_DirectClient_Interface
	{
	private:
		//********************************************************************************
		// Instance virtual functions from superclass
		//****************************************
		bool Initialise_Specifics_SteamMode()
		{
			vds::Output_SetStreamMode streamModeResult = this->Client.SetStreamMode ( vds::StreamMode::ServerPush );
			return streamModeResult.Result == vds::Result::Success;
		}
	};

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Interface to VDS
	//	Retimer Client
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// UpdateFrame()
	//	Uses forward extrapolation to predict the current state of the system from the previous received frames
	//	Is more susceptible to noise
	//	Is non-blocking
	// See description in:
	//	VDS documentation > RetimingClient Class Reference > Detailed Description
	//	VDS documentation > RetimingClient Class Reference > "UpdateFrame" method
	class VDS_Retimer_Interface : public VDS_Interface<vds::RetimingClient>
	{
	private:
		bool Initialise_Specifics()
		{
			return true;
		}

		bool UpdateFrame_NoWarn()
		{
			auto UpdateResult = this->Client.UpdateFrame();
			bool WasSuccessful = UpdateResult.Result == vds::Result::Success;
			return WasSuccessful;
		}

	};

}