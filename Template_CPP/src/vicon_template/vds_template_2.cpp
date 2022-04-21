/*
Written by:			Brandon Johns
Version created:	2021-12-06
Last edited:		2021-12-13

Version changes:
	NA

Purpose:
	Template for using Vicon DataStream (VDS)
	This version saves all data and prints it into a CSV

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

	// For exporting to CSV, it is important to always have the same number of objects captured
	//	=> Must use the filter & must print occluded objects
	VDS.EnableObjectFilter(AllowedObjectsList);
	VDS.DisableOccludedFilter();

	// The input estimate number of rows is not strict, you can go over, but performance will suffer
	// as memory will have to be reallocated.
	// VDS operates at 100 fps, so 10 second trail gives 10000 frames
	csv_exporter::ExportCSV ExportCSV(10000);

	// First row (header row) of the CSV
	csv_exporter::Export_CSV_RowBuilder<std::string> HeaderBuilder;
	std::vector<std::string> RP_string = {"R11","R12","R13", "R21","R22","R23", "R31","R32","R33", "P1","P2","P3"};
	for(auto& object : AllowedObjectsList)
	{
		auto RP_string_point = RP_string;
		for(auto& str : RP_string_point)
		{
			// Column name format: objectName_ElementOfMatrixName
			str = object + "_" + str;
		};
		HeaderBuilder.AddData(RP_string_point);
	}
	ExportCSV.AddHeadder(HeaderBuilder.Row);

	//************************************************************
	// Run
	//******************************
	for( uint64_t idx=0; idx<3; idx++ )
	{
		// Get new data frame from VDS
		VDS.UpdateFrame();

		// Re-encode data into Brandon's custom Points object
		auto points = VDS.FrameToPoints();

		// Encode the next row of the CSV
		//	The order will match the order in AllowedObjectsList
		csv_exporter::Export_CSV_RowBuilder<double> RowBuilder;
		for(auto& point : points.all)
		{
			RowBuilder.AddData(point->R_rowMajor_vec());
			RowBuilder.AddData(point->P_vec());
		}
		ExportCSV.AddRow(RowBuilder.Row);

	}

	// Print data into a file
	ExportCSV.printAll(*outData);

	VDS.Disconnect();
	return 0;
}

