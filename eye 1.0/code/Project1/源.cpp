/*
* This is an example that demonstrates how to connect to the EyeX Engine and subscribe to the lightly filtered gaze data stream.
*
* Copyright 2013-2014 Tobii Technology AB. All rights reserved.
*/
#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <assert.h>
#include <Psapi.h>
#include <iostream>
#include <fstream>
#include "eyex/EyeX.h"
#include <cmath>
#pragma comment (lib, "Tobii.EyeX.Client.lib")



#define MAX 999999
#define ARRAYLENGTH 40

#define EDGESIZE 5

#define READSPEEDMAX 0.6
#define READSPEEDMIN 0.1
#define ISREADMIN 13
#define ISREADLAST 8
#define MISSCOUNTMAX 100
#define LINEGAP 80
#define MAXGAP 70

#define MAXSCROLL 100
#define SCROLLSPEEDMIN 0.3
#define SCROLLSPEEDMAX 1.5
#define ISSCROLLMIN 10
#define SCROLLMISSMAX 50



/* Basic Global Variables */
struct Point {
	double x;
	double y;
	double time;
};
POINT topoint(Point p){
	POINT point;
	point.x = p.x;
	point.y = p.y;
	return point;
}
Point points[ARRAYLENGTH];
double oldtime = -1;
short mode = 3;

/* Function Used Global Variables */
int beginpos = 0;
int recentpos = 0;
int pointcount = 0;
double leftEdge[EDGESIZE] = { 0 };
int leftCount = 0;
TCHAR lpClassName[MAX_PATH];

/* Reading Mode Global Variables */
int isreading = 0;
int readcount = 0;
int misscount = 0;
double tmpReadY = 0;
double lastLine = 0;
int changeline = 0;
double lineBegin = 0;
double k;
int move = 0;

/* Assistancy Mode Global Variables */
double ka;
unsigned assdowncount = 0;
unsigned assupcount = 0;
short isassscroll = 0;
unsigned scrollmisscount = 0;
int assmove = 0;

/* Debug Only Global Variables */
//std::ofstream out;


/* Original Part */

// ID of the global interactor that provides our data stream; must be unique within the application.
static const TX_STRING InteractorId = "Twilight Sparkle";

// global variables
static TX_HANDLE g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;

/*
* Initializes g_hGlobalInteractorSnapshot with an interactor that has the Gaze Point behavior.
*/
BOOL InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
{
	TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
	TX_GAZEPOINTDATAPARAMS params = { TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED };
	BOOL success;

	success = txCreateGlobalInteractorSnapshot(
		hContext,
		InteractorId,
		&g_hGlobalInteractorSnapshot,
		&hInteractor) == TX_RESULT_OK;
	success &= txCreateGazePointDataBehavior(hInteractor, &params) == TX_RESULT_OK;

	txReleaseObject(&hInteractor);
	return success;
}

/*
* Callback function invoked when a snapshot has been committed.
*/
void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
{
	// check the result code using an assertion.
	// this will catch validation errors and runtime errors in debug builds. in release builds it won't do anything.

	TX_RESULT result = TX_RESULT_UNKNOWN;
	txGetAsyncDataResultCode(hAsyncData, &result);
	assert(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}

/*
* Callback function invoked when the status of the connection to the EyeX Engine has changed.
*/
void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
	switch (connectionState) {
	case TX_CONNECTIONSTATE_CONNECTED: {
										   BOOL success;
										   printf("The connection state is now CONNECTED (We are connected to the EyeX Engine)\n");
										   // commit the snapshot with the global interactor as soon as the connection to the engine is established.
										   // (it cannot be done earlier because committing means "send to the engine".)
										   success = txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, OnSnapshotCommitted, NULL) == TX_RESULT_OK;
										   if (!success) {
											   printf("Failed to initialize the data stream.\n");
										   }
										   else {
											   printf("Waiting for gaze data to start streaming...\n");
										   }
	}
		break;

	case TX_CONNECTIONSTATE_DISCONNECTED:
		printf("The connection state is now DISCONNECTED (We are disconnected from the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
		printf("The connection state is now TRYINGTOCONNECT (We are trying to connect to the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
		printf("The connection state is now SERVER_VERSION_TOO_LOW: this application requires a more recent version of the EyeX Engine to run.\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		printf("The connection state is now SERVER_VERSION_TOO_HIGH: this application requires an older version of the EyeX Engine to run.\n");
		break;
	}
}

/* Our Implementation Part */

void addToLeftEdge(double newdata)
{
	if (leftCount < EDGESIZE)
	{
		leftEdge[leftCount] = newdata;
		leftCount++;
		return;
	}
	int index = 0;
	double value = leftEdge[index];
	for (int i = 1; i < EDGESIZE; i++)
	{
		if (leftEdge[i]>value)
		{
			index = i;
			value = leftEdge[index];
		}
	}
	if (newdata > value)
		return;
	leftEdge[index] = newdata;
	return;
}

double getLeftEdgeAver()
{
	double sum = 0;
	for (int i = 0; i < EDGESIZE; i++)
	{
		sum += leftEdge[i];
	}
	return sum / EDGESIZE;
	//return 0;
}
bool isLeftEdge(double pos)
{
	if (leftCount < EDGESIZE)
		return false;
	double aver = getLeftEdgeAver();
	//std::cout << aver << std::endl;
	double gap = 80;
	return (pos>(aver - gap)) && (pos < (aver + gap));
}
bool lineFit(Point points[], int beginpos, double &a, double &b, double &c, double &mean, short perm)
{
	int size = ARRAYLENGTH;
	double x_mean = 0;
	double y_mean = 0;
	double time_mean = 0;
	for (int i = 0; i < size; i++)
	{
		x_mean += points[beginpos].x;
		y_mean += points[beginpos].y;
		time_mean += points[beginpos].time;
		beginpos = (beginpos + 1) % size;
	}
	x_mean /= size;
	y_mean /= size;
	time_mean /= size; //至此，计算出了 x y 的均值
	double Dxx = 0, Dxy = 0, Dyy = 0;
	if (perm == 5){
		mean = y_mean;

		for (int i = 0; i < size; i++)
		{
			Dxx += (points[i].x - x_mean) * (points[i].x - x_mean);
			Dxy += (points[i].x - x_mean) * (points[i].time - time_mean);
			Dyy += (points[i].time - time_mean) * (points[i].time - time_mean);
		}
		c = -a * x_mean - b * time_mean;
	}
	else if (perm == 3){
		mean = x_mean;

		for (int i = 0; i < size; i++)
		{
			Dxx += (points[i].y - y_mean) * (points[i].y - y_mean);
			Dxy += (points[i].y - y_mean) * (points[i].time - time_mean);
			Dyy += (points[i].time - time_mean) * (points[i].time - time_mean);
		}
		c = -a * y_mean - b * time_mean;
	}
	double lambda = ((Dxx + Dyy) - sqrt((Dxx - Dyy) * (Dxx - Dyy) + 4 * Dxy * Dxy)) / 2.0;
	double den = sqrt(Dxy * Dxy + (lambda - Dxx) * (lambda - Dxx));
	a = Dxy / den;
	b = (lambda - Dxx) / den;
	return true;
}

void addPoint(Point points[], Point point) {
	points[recentpos] = point;
	recentpos = (recentpos + 1) % ARRAYLENGTH;
	if (pointcount < ARRAYLENGTH)
		pointcount++;
	if (pointcount >= ARRAYLENGTH)
		beginpos = (beginpos + 1) % ARRAYLENGTH;
}

double IsNaN(double x)
{
	return !(x == x);
}
/*
* Handles an event from the Gaze Point data stream.
*/

void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior)
{
	UINT count;
	Point point;
	TX_GAZEPOINTDATAEVENTPARAMS eventParams;
	RECT rect;
	typedef void (WINAPI *PSWITCHTOTHISWINDOW) (HWND, BOOL);
	PSWITCHTOTHISWINDOW SwitchToThisWindow;
	HMODULE hUser32 = GetModuleHandle(("user32"));
	SwitchToThisWindow = (PSWITCHTOTHISWINDOW)GetProcAddress(hUser32, "SwitchToThisWindow");
	if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) != TX_RESULT_OK) {
		//printf("Failed to interpret gaze data event packet.\n");
	}
	else {
		point.x = eventParams.X;
		point.y = eventParams.Y;

		if (oldtime < 0){
			oldtime = eventParams.Timestamp;
		}
		point.time = eventParams.Timestamp - oldtime;
		POINT curpoint;
		GetCursorPos(&curpoint);
		HWND curhwnd = WindowFromPoint(curpoint);
		HWND hwnd = WindowFromPoint(topoint(point));
		if (hwnd != NULL && hwnd != INVALID_HANDLE_VALUE) {
			count = GetWindowRect(hwnd, &rect);
			LONG h = rect.bottom - rect.top;
			addPoint(points, point);
			//if (beginpos == 0 || beginpos == ARRAYLENGTH / 2) 
			if (mode & 1)
			{
				double x, y, z, ymean;
				lineFit(points, beginpos, x, y, z, ymean, 5);
				if (x == 0)
					k = MAX;
				else {
					if (!IsNaN(-y / x)) {
						k = -y / x;
					}
					else {
						k = 0;
					}
				}
				if (k >= READSPEEDMIN && k <= READSPEEDMAX && point.y - tmpReadY <= MAXGAP && point.y - tmpReadY >= -MAXGAP)
				{
					readcount++;
					if (readcount == 1)
						lineBegin = point.x;
					if (readcount == 5)
						addToLeftEdge(lineBegin);
					if (readcount >= ISREADMIN)
					{
						lastLine = tmpReadY;
						changeline = 0;
						isreading = 1;
						misscount = 0;
					}
				}
				else
				{

					misscount++;
					readcount = 0;
					if (misscount > MISSCOUNTMAX){
						isreading = 0;
					}
					tmpReadY = point.y;
				}
				//printf("readcount:%d misscount:%d\n", readcount, misscount);


				if (isreading) {
					if (point.y >= 0.7*h + rect.top)
					{
						if (readcount >= ISREADLAST || (isLeftEdge(point.x) && k < 0.06  && k > 0)) {
							if (move == 0){
								if (hwnd != curhwnd){
									printf("change\n");
									double mx = point.x * 65535 / GetSystemMetrics(SM_CXSCREEN);
									double my = point.y * 65535 / GetSystemMetrics(SM_CYSCREEN);
									mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, mx, my, 0, 0);
									SwitchToThisWindow(hwnd, TRUE);
									leftCount = 0;
								}
								move = 10;
								isreading = 0;
								readcount = 0;
								misscount = 0;
								changeline = 1;


							}
						}
						else if (tmpReadY - lastLine < LINEGAP){
							if (changeline == 0){
								changeline = 1;
								if (move == 0){
									move = 10;
									isreading = 0;
									readcount = 0;
									misscount = 0;
								}
							}

						}
					}
				}
			}

			if (mode & 2){
				double x, y, z, xmean;
				lineFit(points, beginpos, x, y, z, xmean, 3);
				if (x == 0)
					ka = MAX;
				else {
					if (!IsNaN(-y / x)) {
						ka = -y / x;
					}
					else {
						ka = 0;
					}
				}
				if (ka >= SCROLLSPEEDMIN && ka <= SCROLLSPEEDMAX && abs(xmean - point.x) < MAXSCROLL)
				{
					assdowncount++;
					if (assdowncount >= ISSCROLLMIN)
					{
						isassscroll = 2;
						scrollmisscount = 0;
						assupcount = 0;
					}
				}
				else if (ka >= -SCROLLSPEEDMAX && ka <= -SCROLLSPEEDMIN && abs(xmean - point.x) < MAXSCROLL){
					assupcount++;
					if (assupcount >= ISSCROLLMIN)
					{
						isassscroll = 1;
						scrollmisscount = 0;
						assdowncount = 0;
					}
				}
				else
				{
					scrollmisscount++;
					assdowncount = assupcount = 0;
					if (scrollmisscount > SCROLLMISSMAX){
						isassscroll = 0;
					}
				}
				//printf("assdowncount:%d assupcount:%d scrollmisscount:%d\n", assdowncount, assupcount, scrollmisscount);

				if (isassscroll) {
					if (point.y >= rect.top && point.y <= 0.2*h + rect.top && isassscroll == 1 && move == 0)
					{
						if (hwnd != curhwnd){
							printf("change\n");
							double mx = point.x * 65535 / GetSystemMetrics(SM_CXSCREEN);
							double my = point.y * 65535 / GetSystemMetrics(SM_CYSCREEN);
							mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, mx, my, 0, 0);
							SwitchToThisWindow(hwnd, TRUE);
							leftCount = 0;
						}
						assmove = -8;
						isassscroll = 0;
						scrollmisscount = 0;
						assdowncount = 0;
						assupcount = 0;
					}

					/*else if (point.y <= rect.bottom && point.y >= 0.8*h + rect.top && isassscroll == 2 && move == 0)
					{
						if (hwnd != curhwnd){
							printf("change\n");
							double mx = point.x * 65535 / GetSystemMetrics(SM_CXSCREEN);
							double my = point.y * 65535 / GetSystemMetrics(SM_CYSCREEN);
							mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, mx, my, 0, 0);
							SwitchToThisWindow(hwnd, TRUE);
						}
						assmove = 8;
						isassscroll = 0;
						scrollmisscount = 0;
						assdowncount = 0;
						assupcount = 0;
					}*/
				}
			}


			if (move > 0)
			{
				if (hwnd == curhwnd){
					move--;
					mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -6, 0);
				}
				else{
					count = GetWindowText(hwnd, lpClassName, MAX_PATH);
					if (!strcmp(lpClassName, "运行中的应用程序") || !strcmp(lpClassName, "用户显示的通知区域")){
						move--;
						mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -6, 0);
					}
					else
					{
						double mx = point.x * 65535 / GetSystemMetrics(SM_CXSCREEN);
						double my = point.y * 65535 / GetSystemMetrics(SM_CYSCREEN);
						mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, mx, my, 0, 0);
						SwitchToThisWindow(hwnd, TRUE);
						leftCount = 0;
					}
				}
			}

			else if (assmove){
				int neg = assmove > 0 ? -1 : 1;
				if (hwnd == curhwnd){
					assmove += neg;
					mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 10 * neg, 0);
				}
				else{
					count = GetWindowText(hwnd, lpClassName, MAX_PATH);
					if (!strcmp(lpClassName, "运行中的应用程序") || !strcmp(lpClassName, "用户显示的通知区域")){
						assmove += neg;
						mouse_event(MOUSEEVENTF_WHEEL, 0, 0, 10 * neg, 0);
					}
					else
					{
						double mx = point.x * 65535 / GetSystemMetrics(SM_CXSCREEN);
						double my = point.y * 65535 / GetSystemMetrics(SM_CYSCREEN);
						mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, mx, my, 0, 0);
						SwitchToThisWindow(hwnd, TRUE);
						leftCount = 0;
					}
				}
			}
		}
	}
}

/* Original Part */

/*
* Callback function invoked when an event has been received from the EyeX Engine.
*/
void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

	txGetAsyncDataContent(hAsyncData, &hEvent);

	// NOTE. Uncomment the following line of code to view the event object. The same function can be used with any interaction object.
	//OutputDebugStringA(txDebugObject(hEvent));

	if (txGetEventBehavior(hEvent, &hBehavior, TX_BEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK) {
		OnGazeDataEvent(hBehavior);
		txReleaseObject(&hBehavior);
	}

	// NOTE since this is a very simple application with a single interactor and a single data stream, 
	// our event handling code can be very simple too. A more complex application would typically have to 
	// check for multiple behaviors and route events based on interactor IDs.

	txReleaseObject(&hEvent);
}



/*
* Application entry point.
*/
int main(int argc, char* argv[])
{
	//  out = std::ofstream("data.txt");
	//	out << "this is a test";
	if (argc == 2)mode = argv[1][0] - 48;
	printf("%d\n", mode);
	TX_CONTEXTHANDLE hContext = TX_EMPTY_HANDLE;
	TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
	TX_TICKET hEventHandlerTicket = TX_INVALID_TICKET;
	BOOL success;

	// initialize and enable the context that is our link to the EyeX Engine.
	success = txInitializeEyeX(TX_EYEXCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL, NULL) == TX_RESULT_OK;
	success &= txCreateContext(&hContext, TX_FALSE) == TX_RESULT_OK;
	success &= InitializeGlobalInteractorSnapshot(hContext);
	success &= txRegisterConnectionStateChangedHandler(hContext, &hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK;
	success &= txRegisterEventHandler(hContext, &hEventHandlerTicket, HandleEvent, NULL) == TX_RESULT_OK;
	success &= txEnableConnection(hContext) == TX_RESULT_OK;

	// let the events flow until a key is pressed.
	if (success) {
		printf("Initialization was successful.\n");
	}
	else {
		printf("Initialization failed.\n");
	}
	printf("Press any key to exit...\n");
	_getch();
	printf("Exiting.\n");

	// disable and delete the context.
	txDisableConnection(hContext);
	txReleaseObject(&g_hGlobalInteractorSnapshot);
	success = txShutdownContext(hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE) == TX_RESULT_OK;
	success &= txReleaseContext(&hContext) == TX_RESULT_OK;
	success &= txUninitializeEyeX() == TX_RESULT_OK;
	if (!success) {
		printf("EyeX could not be shut down cleanly. Did you remember to release all handles?\n");
	}

	return 0;
}
