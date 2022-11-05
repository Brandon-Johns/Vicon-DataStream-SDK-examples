/*
Written by:			Brandon Johns
Version created:	2021-12-13
Last edited:		2022-04-28

Version changes:
	NA

Purpose:
	Helper to print data into a CSV
		Enforces row length
		Prints data at end to improve performance while collecting data

Summary:
	class ExportCSV
		Holds the CSV data
		Prints the data to the file when printAll is called
	class Export_CSV_RowBuilder
		Helper for ExportCSV. Use to build a row of the CSV, then pass Row to ExportCSV.AddRow()

*/
#pragma once

// Standard library
#include <stdexcept>
#include <iostream>
#include <fstream> // read/write to files
#include <vector>


namespace csv_exporter
{
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Print data into a csv
	// Stores the data before user calls print to output all at once
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	class ExportCSV
	{
	private:
		// Strict requirement that each row of the CSV must be exactly this length
		// This is calculated from the first row
		uint64_t rowLength = 0;
		bool rowLength_IsSet = false;

		// CSV header & data rows
		std::vector< std::vector<std::string> > headerRows;
		std::vector< std::vector<double> > dataRows;

		// Enforces the strict requirement that each row of the CSV must be exactly this length
		void ValidateRowLength(uint64_t rowLength_in)
		{
			// Enforce all rows are the same length
			if(this->rowLength_IsSet)
			{
				if(rowLength_in != this->rowLength) { throw std::runtime_error("csv_exporter_ERROR: Row lengths do not match"); }
			}
			else // First row => set as row length
			{
				this->rowLength = rowLength_in;
				this->rowLength_IsSet = true;
			}
		}

	public:
		//********************************************************************************
		// Interface: create
		//****************************************
		// INPUT:
		//	numRows_estimate = predicted required number of rows. Not an absolute max, just for preallocation
		ExportCSV(uint64_t numRows_estimate)
		{
			// preallocate space to store data
			this->dataRows.reserve(numRows_estimate);
		}

		//********************************************************************************
		// Interface: add data
		//****************************************
		// Use the helper class Export_CSV_RowBuilder to generate the input to these methods

		// Header: the first line of the CSV
		void AddHeader(std::vector<std::string> row)
		{
			this->ValidateRowLength(row.size());
			this->headerRows.push_back( row );
		}

		// Append a data row to the CSV
		void AddRow(std::vector<double> row)
		{
			this->ValidateRowLength(row.size());
			this->dataRows.push_back( row );
		}

		//********************************************************************************
		// Interface: output
		//****************************************
		// Print all currently held data in CSV format
		void PrintAll(std::ostream& outStream)
		{
			// Print head
			for (auto& row : this->headerRows)
			{
				for (auto& value : row)
				{
					outStream << value << ",";
				}
				outStream << std::endl;
			}

			// Print rows
			for (auto& row : this->dataRows)
			{
				for (auto& value : row)
				{
					outStream << value << ",";
				}
				outStream << std::endl;
			}
		}

		// Print currently held data, then clear held data
		// Allows incremental printing of data to a csv
		void PrintAll_clear(std::ostream& outStream)
		{
			PrintAll(outStream);
			this->headerRows.clear();
			this->dataRows.clear();
		}

	};

	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Helper for ExportCSV
	// Use to build a row, then pass Row to ExportCSV.AddRow()
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	template<class DataType>
	class Export_CSV_RowBuilder
	{
	public:
		//********************************************************************************
		// Interface: create
		//****************************************
		// TEMPLATE INPUT:
		//	DataType = data type contained by the Row vector

		//********************************************************************************
		// Interface: output
		//****************************************
		std::vector<DataType> Row;

		//********************************************************************************
		// Interface: add data
		//****************************************
		// Append data given in different forms
		void AddData(DataType value)
		{
			this->Row.push_back(value);
		}

		void AddData(std::vector<DataType> values)
		{
			this->Row.insert( this->Row.end(), values.begin(), values.end() );
		}
	};
}

