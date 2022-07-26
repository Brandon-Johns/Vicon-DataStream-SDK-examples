/*
Written by:			Brandon Johns
Version created:	2022-07-26
Last edited:		2022-07-26

Version changes:
	NA

Purpose:
	Template for using Vicon DataStream (VDS)
	This version saves all data and prints it into a CSV

	This version also stores marker data

*/
// Program output
#include <iostream>
#include <fstream> // read/write to files

// Other
#include <chrono> // Time keeping
#include <thread> // For sleep
#include <vector>

// Brandon's VDS Interface
#include "VDS_Interface.h"
#include "CSV_Exporter.h"


namespace
{
	// Vicon object names
	// (add all your objects)
	constexpr auto NAME_MyViconObject1 = "Jackal";
	constexpr auto NAME_MyViconObject2 = "bj_ctrl";
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

	// List all the objects to be allowed through filtering
	// Prevents ghosts of other peoples objects from interfereing with the output
	std::vector<std::string> AllowedObjectsList;
	AllowedObjectsList.push_back(NAME_MyViconObject1);
	AllowedObjectsList.push_back(NAME_MyViconObject2);
	AllowedObjectsList.push_back(NAME_MyViconObject3);

	//************************************************************
	// Initialise
	//******************************
	// Start to VDS
	std::cout << "BJ: Connecting to VDS" << std::endl;
	vdsi::VDS_Interface VDS;
	VDS.Connect(vds_HostName);

	// For exporting to CSV, it is important to always have the same number of objects captured
	//	=> Must use the filter & must print occluded objects
	VDS.EnableObjectFilter(AllowedObjectsList);
	VDS.DisableOccludedFilter();
	auto points = VDS.GetFrame();

	// The input estimate number of rows is not strict, you can go over, but performance will suffer
	// as memory will have to be reallocated.
	// Assuming VDS operates at 100 fps, a 10 second trail gives 10000 frames
	csv_exporter::ExportCSV ExportCSV(10);

	// First row (header row) of the CSV
	csv_exporter::Export_CSV_RowBuilder<std::string> HeaderBuilder;
	HeaderBuilder.AddData("FrameNumber");

	std::vector<std::string> P_string = { "P1","P2","P3" };
	std::vector<std::string> RP_string = { "R11","R12","R13", "R21","R22","R23", "R31","R32","R33", "P1","P2","P3" };
	for(auto& object : AllowedObjectsList)
	{
		// Copy string vector
		// Format columns as: objectName_elementOfMatrixName, objectName_markerName_elementOfMatrixName
		// Add object to header
		auto RP_string_point(RP_string);
		for(auto& str : RP_string_point) { str = object + "_" + str; };
		HeaderBuilder.AddData(RP_string_point);

		// Add markers to header
		for (auto& marker : points.Get(object).markers)
		{
			auto P_string_marker(P_string);
			for (auto& str : P_string_marker) { str = object + "_" + marker.viconObjectName + "" + str; };
			HeaderBuilder.AddData(P_string_marker);
		}
	}
	ExportCSV.AddHeadder(HeaderBuilder.Row);

	//************************************************************
	// Run
	//******************************
	unsigned int frameNumberStart = 0;
	bool IsFirstLoop = true;

	for( uint64_t idx=0; idx<10; idx++ )
	{
		// Get new data frame from VDS
		// Re-encode data into Brandon's custom Points object
		auto points = VDS.GetFrame_GetUnread();

		// Encode the next row of the CSV
		//	The order will match the order in AllowedObjectsList
		csv_exporter::Export_CSV_RowBuilder<double> RowBuilder;

		// Offset frameNumber to start at 1
		if (IsFirstLoop) { IsFirstLoop=false; frameNumberStart = points.frameNumber; }
		RowBuilder.AddData( double(points.frameNumber - frameNumberStart + 1) );

		// Write data
		for(auto& point : points.all)
		{
			RowBuilder.AddData(point.R_rowMajor);
			RowBuilder.AddData(point.P);
			for (auto& marker : point.markers)
			{
				RowBuilder.AddData(marker.P);
			}
		}
		ExportCSV.AddRow(RowBuilder.Row);

		// Print data into a file
		// Print incrementally, during the loop
		ExportCSV.PrintAll_clear(*outData);
	}

	VDS.Disconnect();
	return 0;
}

