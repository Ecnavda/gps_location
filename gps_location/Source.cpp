#include <Windows.h>
#include <atlbase.h>
#include <atlcom.h>
#include <locationapi.h>
#include <sensors.h> // For PROPERTYKEY definitions
#include <propvarutil.h>


// For parsing and generating INI files
#include <iostream>
#include <fstream>
#include <sstream>

// For sleeping
#include <thread>
#include <chrono>

// References
// https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/LocationSynchronousAccess
// https://github.com/microsoft/Windows-classic-samples/blob/master/Samples/LocationSetDefault
// https://www.winwaed.com/blog/2018/12/07/using-the-windows-7-location-api/

// Due to depecration of Location API
#pragma warning(disable : 4995)

// TODO
// Not make these global
CComPtr<ILocation> spLoc;
double lfLat, lfLong;

void createLocationObject();
void printStatus(LOCATION_REPORT_STATUS*);
void parseIni(double, double);

int main(int argc, char* argv[]) {
	if (argc == 1) {
		CoInitialize(nullptr);
		createLocationObject();
	}
	else if (argc == 2) {
		if (strcmp(argv[1], "-v") == 0) {
			wprintf_s(L"Starting program\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
			CoInitialize(nullptr);
			// TODO: turn print statements on inside createLocationObject() to show what's happening
			createLocationObject();
			wprintf_s(L"Ending program\n");
		}
	}
}

void createLocationObject() {
	// Create Location Object
	LOCATION_REPORT_STATUS status = REPORT_NOT_SUPPORTED;

	HRESULT hr = S_OK;
	hr = spLoc.CoCreateInstance(CLSID_Location);

	if (SUCCEEDED(hr)) {
		// Array of report types of interest
		IID REPORT_TYPES[] = { IID_ILatLongReport };

		// Request permissions for this user account to receive location
		// data for all types defined in REPORT_TYPES
		// The final parameter (TRUE) indicates a synchronous request
		// use FALSE to request asynchronous calls
		hr = spLoc->RequestPermissions(NULL, REPORT_TYPES, ARRAYSIZE(REPORT_TYPES), TRUE);

		if (FAILED(hr)) {
			// wprintf_s(L"Unable to get permissions\n");
		}

		bool loop = TRUE;

		while (loop) {
			hr = spLoc->GetReportStatus(IID_ILatLongReport, &status);

			if (SUCCEEDED(hr)) {
				// Display Status
				// Refactor to take pointer instead of using global variable?
				// printStatus(&status);

				// Defining report objects
				// These store reports requested from spLoc
				CComPtr<ILocationReport> spLocationReport;
				CComPtr<ILatLongReport> spLatLongReport;

				// Get the current location report (ILocationReport) and
				// then get a ILatLongReport from it
				// Check that they are reported okay and not null
				hr = spLoc->GetReport(IID_ILatLongReport, &spLocationReport);

				if (SUCCEEDED(hr)) {
					hr = spLocationReport->QueryInterface(&spLatLongReport);

					if (SUCCEEDED(hr)) {
						lfLat = 0;
						lfLong = 0;

						// Fetch Latitude and Longitude
						spLatLongReport->GetLatitude(&lfLat);
						spLatLongReport->GetLongitude(&lfLong);

						// wprintf_s(L"Lat: %.6f, Long: %.6f\n\n", lfLat, lfLong);
						// wprintf_s(L"Attempting to write to INI file\n");
						parseIni(lfLat, lfLong);
						// wprintf_s(L"Check INI file\n");
					}
				}

				// Sleeping for 10 seconds
				// https://stackoverflow.com/questions/1658386/sleep-function-in-c
				std::chrono::milliseconds timespan(10000);
				std::this_thread::sleep_for(timespan);
			}
		}
	}
}

void printStatus(LOCATION_REPORT_STATUS* status) {
	switch (*status) {
	case REPORT_RUNNING:
		wprintf_s(L"Status: Report Running\n");
		break;
	case REPORT_NOT_SUPPORTED:
		wprintf_s(L"Status: Report Not Supported\n");
		break;
	case REPORT_ERROR:
		wprintf_s(L"Status: Report Error\n");
		break;
	case REPORT_ACCESS_DENIED:
		wprintf_s(L"Status: Report Access Denied\n");
		break;
	case REPORT_INITIALIZING:
		wprintf_s(L"Status: Report Initializing\n");
		break;
	}
}

void parseIni(double latitude, double longitude) {
	BOOL result = FALSE;
	const char fileLocation[] = "C:\\ProgramData\\TraCS\\Settings\\LocationToolFL.ini";
	
	// https://stackoverflow.com/questions/13294067/how-to-convert-string-to-char-array-in-c
	// For converting the doubles into string/char
	std::ostringstream temp;
	temp << latitude;
	std::string strLat = temp.str();
	
	// https://stackoverflow.com/questions/5288036/how-to-clear-ostringstream
	// Clearing temp variable
	temp.str("");
	temp.clear();
	
	temp << longitude;
	std::string strLong = temp.str();

	result = WritePrivateProfileStringA("GPS", "Lat", strLat.c_str(), fileLocation);
	WritePrivateProfileStringA("GPS", "Long", strLong.c_str(), fileLocation);
	if (result) {
		// TODO: Do something with TRUE/FALSE result in parseINI
	}
	else if (!result) {
	
	}
}

// Function required to make this executable a service
// https://docs.microsoft.com/en-us/windows/win32/api/winsvc/nc-winsvc-lpservice_main_functiona?redirectedfrom=MSDN
// TODO: Make a service function natively instead of using NSSM