/*
Written by:			Brandon Johns
Version created:	2021-11-04
Last edited:		2021-12-13

Version changes:
	NA

Purpose:
	Template for using Vicon DataStream (VDS)
	This version extracts data about objects in the stream
		e.g. for feedback into a controller

*/
#include "main.h"


namespace
{
	// Vicon object names
	// (add all your objects)
	constexpr auto NAME_MyViconObject1 = "bj_c1_bh";
	constexpr auto NAME_MyViconObject2 = "bj_c1_cwm";
	constexpr auto NAME_MyViconObject3 = "someOtherObject";
}


int main( int argc, char* argv[] )
{
	//************************************************************
	// User Settings - General
	//******************************
	// Output destination
	std::ostream* outData;
	outData = &std::cout; // Print to terminal
	//outData = new std::ofstream("out.txt"); // Print to File

	//************************************************************
	// User Settings - VDS
	//******************************
	// Network addresses of the computer running Vicon Tracker 3
	constexpr auto vds_HostName = "192.168.11.3";

	// Lightweight Segment Data Enable
	constexpr bool Flag_VDS_Lightweight = false;

	// List all the objects to be allowed through filtering
	// Prevents ghosts of other peoples objects from interfereing with the output
	std::vector<std::string> AllowedObjectsList;
	AllowedObjectsList.push_back(NAME_MyViconObject1);
	AllowedObjectsList.push_back(NAME_MyViconObject2);
	AllowedObjectsList.push_back(NAME_MyViconObject3);

	// Choose a Vicon DataStream client
	//vdsi::VDS_ServerPush_Interface VDS;
	//vdsi::VDS_ClientPull_Interface VDS;
	vdsi::VDS_ClientPullPreFetch_Interface VDS;
	//vdsi::VDS_Retimer_Interface VDS;

	//************************************************************
	// Initialise
	//******************************
	// Start to VDS
	std::cout << "BJ: Connecting to VDS" << std::endl;
	VDS.Initialise(vds_HostName, Flag_VDS_Lightweight);

	// Toggle options as desired
	VDS.EnableObjectFilter(AllowedObjectsList);
	// VDS.DisableObjectFilter();
	VDS.EnableOccludedFilter();

	//************************************************************
	// Run
	//******************************
	while( true )
	{
		// Get new data frame from VDS
		// Re-encode data into Brandon's custom Points object
		auto points = VDS.GetFrame();

		// Print everything
		points.Print(*outData);

		// Print just the object that I specify (specify with the same name you use in Tracker
		auto myPoint = points.Get(NAME_MyViconObject1);
		if(myPoint) // If found
		{
			*outData
				<< std::endl
				<< "The object with name " << myPoint->viconObjectName
				<< " was " << ( myPoint->IsOccluded ? "unfortunately" : "not" ) << " occluded."
				<< std::endl
				<< " It has position (x,y,z) is (" << myPoint->P_str() << ")."
				<< std::endl
				<< " The x coordinate is (" << myPoint->x() << ")."
				<< std::endl
				<< std::endl;
		}

		// Limit print rate (not required, just for the example)
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	VDS.Disconnect();
	return 0;
}

