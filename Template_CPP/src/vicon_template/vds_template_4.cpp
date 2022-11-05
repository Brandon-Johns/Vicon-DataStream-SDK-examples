/*
Written by:			Brandon Johns
Version created:	2022-07-26
Last edited:		2022-07-26

Version changes:
	NA

Purpose:
	Template for using Vicon DataStream (VDS)
	This version is intended to be run from command line with arguments to specify run conditions

	Features:
		prints data to command line or CSV
		stores marker data
		variable duration & can be terminated at will

Inputs:
	Run with command line argument --Help

Sample call:
	.\vds_template_4 --Objects Jackal bj_ctrl
	.\vds_template_4 --Objects Jackal bj_ctrl --SaveMarkerLocations
	.\vds_template_4 --Objects Jackal bj_ctrl --DurationSeconds 10
	.\vds_template_4 --FileName tmp --Objects Jackal bj_ctrl --DurationSeconds 10  --SaveMarkerLocations

	Using arithmetic in powershell to specify time in min
		.\vds_template_4 --Objects Jackal bj_ctrl --DurationSeconds $(10*60)

*/
// Program output
#include <iostream>
#include <fstream> // read/write to files

// Other
#include <chrono> // Time keeping
#include <thread> // For sleep
#include <vector>

// Interrupt handling for program termination
#include <cstdlib>
#include <csignal>

// Brandon's VDS Interface
#include "VDS_Interface.h"
#include "CSV_Exporter.h"


namespace Kill
{
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Program termination routine
	// Called on abnormal program termination
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// This flag should be frequently checked in the main program
	// 1 = The program should be safely stopped ASAP
	bool Flag_TerminateProgramCalled = false;

	// This may be called at ANY point during execution
	// => it is unsafe to do anything in here other than set a flag or force terminate
	void ProgramTerminationHandler(int signal)
	{
		if (Flag_TerminateProgramCalled)
		{
			// 2nd call to terminate => force terminate now
			exit(signal);
		}
		else
		{
			// First call to terminate
			// Set flag to tell main program to safely exit
			Flag_TerminateProgramCalled = true;
		}
	}

	// Call this in main to enable
	void ProgramTerminationEnable()
	{
		// Register program termination routine for Event: CTRL+C
		signal(SIGINT, Kill::ProgramTerminationHandler);
	}

}

namespace
{
	bool IsFlag(std::vector<std::string>& argsOfFlag, std::string flagName)
	{
		return(argsOfFlag.back() == flagName);
	};

	template<typename Function>
	std::vector<std::string> ParseArgsOfFlag(std::vector<std::string> argsOfFlag, Function ValidCondition)
	{
		auto flagName = argsOfFlag.back();

		// Pop the flag itself
		argsOfFlag.pop_back();

		// Flag detected
		//	Test the input Lambda that the flag's args are valid
		if (!ValidCondition(argsOfFlag.size()))
		{
			std::cout << "ERROR: (Bad Input) " + flagName << std::endl;
			throw(std::invalid_argument("ERROR: (Bad Input) " + flagName));
		}

		// Correct reversal of args list
		std::reverse(argsOfFlag.begin(), argsOfFlag.end());

		return argsOfFlag;
	};
}


int main( int argc, char* argv[] )
{
	// Time to run for
	double durationSeconds = double(10)/double(200);

	// Output destination
	std::ostream* outData;
	outData = &std::cout; // (Default) Print to terminal

	// Network addresses of the computer running Vicon Tracker 3
	std::string vds_HostName = "192.168.11.3";

	// List all the objects to be allowed through filtering
	// Prevents ghosts of other peoples objects from interfering with the output
	std::vector<std::string> AllowedObjectsList;

	// (See description in arguments)
	bool saveMarkerLocations = false;

	//************************************************************
	// Parse Command Line Arguments
	//******************************
	// Copy arguments into vector of strings
	// Then reverse parse the args list
	std::vector<std::string> argList(argv + 1, argv + argc);
	std::reverse(argList.begin(), argList.end());

	std::vector<std::string> argsOfFlag;
	for (auto& arg : argList)
	{
		argsOfFlag.push_back(arg);
		if (IsFlag(argsOfFlag, "--Help"))
		{
			std::cout <<
				"--FileName\n"
				"    Filename to direct to output to. Without extension or path\n"
				"    Default: Print to terminal\n"
				"--HostName\n"
				"    IP address or hostname of computer running Vicon Tracker\n"
				"    Default: "+vds_HostName+"\n"
				"--Objects\n"
				"    Space separated list of vicon objects\n"
				"    Default: (empty)\n"
				"--SaveMarkerLocations\n"
				"    Marker positions are exported in addition to object pose\n"
				"    Default: (does no save marker locations)\n"
				"--DurationSeconds\n"
				"    Time program will fun for before it exits\n"
				"    It is safe to end the program before then with CTRL+C\n"
				"    In case the program hangs, pressing CTRL+C a 2nd time should force exit\n"
				"    Default: "+ std::to_string(durationSeconds)+"\n"
				<< std::endl;
			return 0;
		}
		else if ( IsFlag(argsOfFlag, "--FileName") )
		{
			auto parsedArgsOfFlag = ParseArgsOfFlag(argsOfFlag, [&](size_t numArgs) {return numArgs == 1; });
			argsOfFlag.clear();

			// Print to this File
			outData = new std::ofstream(parsedArgsOfFlag.front() + ".csv" );
		}
		else if ( IsFlag(argsOfFlag, "--HostName") )
		{
			auto parsedArgsOfFlag = ParseArgsOfFlag(argsOfFlag, [&](size_t numArgs) {return numArgs == 1; });
			argsOfFlag.clear();

			vds_HostName = parsedArgsOfFlag.front();
		}
		else if (IsFlag(argsOfFlag, "--Objects"))
		{
			auto parsedArgsOfFlag = ParseArgsOfFlag(argsOfFlag, [&](size_t numArgs) {return numArgs > 0; });
			argsOfFlag.clear();

			AllowedObjectsList = parsedArgsOfFlag;
		}
		else if (IsFlag(argsOfFlag, "--SaveMarkerLocations"))
		{
			auto parsedArgsOfFlag = ParseArgsOfFlag(argsOfFlag, [&](size_t numArgs) {return numArgs == 0; });
			argsOfFlag.clear();

			saveMarkerLocations = true;
		}
		else if (IsFlag(argsOfFlag, "--DurationSeconds"))
		{
			auto parsedArgsOfFlag = ParseArgsOfFlag(argsOfFlag, [&](size_t numArgs) {return numArgs > 0; });
			argsOfFlag.clear();

			// Convert string to double: std::stod()
			durationSeconds = std::stod( parsedArgsOfFlag.front() );
			if (durationSeconds <= 0)
			{
				std::cout << "ERROR: (Bad Input) DurationSeconds" << std::endl;
				throw(std::invalid_argument("ERROR: (Bad Input) DurationSeconds"));
			}
		}
		else if (argsOfFlag.back().substr(0, 2) == "--")
		{
			std::cout << "ERROR: (Bad Input) Invalid Flag" << std::endl;
			throw(std::invalid_argument("ERROR: (Bad Input) Invalid Flag"));
		}
	}

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
		if (saveMarkerLocations) {
			for (auto& marker : points.Get(object).markers)
			{
				auto P_string_marker(P_string);
				for (auto& str : P_string_marker) { str = object + "_" + marker.viconObjectName + "" + str; };
				HeaderBuilder.AddData(P_string_marker);
			}
		}
	}
	ExportCSV.AddHeader(HeaderBuilder.Row);

	// Duration of trial, in frame count
	uint32_t durationFrames = uint32_t( durationSeconds * VDS.GetFrameRate() );

	//************************************************************
	// Run
	//******************************
	unsigned int frameNumberStart = 0;
	bool IsFirstLoop = true;

	Kill::ProgramTerminationEnable();
	for( uint32_t idx=0; idx<durationFrames; idx++ )
	{
		// Safely terminate program if CTRL+C
		if (Kill::Flag_TerminateProgramCalled) { break; }

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

			if (saveMarkerLocations) {
				for (auto& marker : point.markers)
				{
					RowBuilder.AddData(marker.P);
				}
			}
		}
		ExportCSV.AddRow(RowBuilder.Row);

		// Print data into a file
		// Print incrementally, during the loop
		ExportCSV.PrintAll_clear(*outData);
	}

	std::cout << "Finished" << std::endl;

	VDS.Disconnect();
	return 0;
}

