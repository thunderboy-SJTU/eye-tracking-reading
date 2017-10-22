#include <Windows.h>
#include <Psapi.h>
#include <iostream>
#include <math.h>

#define SCROLL_MAX 10
#define MAX 999999
#define ARRAYLENGTH 500
//#define OUT_RANGE_MAX 2500

//unsigned down_scroll_count = 0;
//unsigned scroll_out_range = 0;
POINT recent;
int scroll = 0;
int isreading = 0;
int readcount = 0;
int readbegin = 0;
int misscount = 0;
double k;
double b;
POINT points[ARRAYLENGTH];
int beginpos = 0;
int recentpos = 0;
int pointcount = 0;

bool lineFit(POINT points[], int beginpos, double &a, double &b, double &c)
{
	int size = ARRAYLENGTH;
	double x_mean = 0;
	double y_mean = 0;
	for (int i = 0; i <  size; i++)
	{
		x_mean += points[beginpos].x;
		y_mean += points[beginpos].y;
		beginpos = (beginpos + 1) % size;
	}
	x_mean /= size;
	y_mean /= size; //至此，计算出了 x y 的均值

	double Dxx = 0, Dxy = 0, Dyy = 0;

	for (int i = 0; i < size; i++)
	{
		Dxx += (points[i].x - x_mean) * (points[i].x - x_mean);
		Dxy += (points[i].x - x_mean) * (points[i].y - y_mean);
		Dyy += (points[i].y - y_mean) * (points[i].y - y_mean);
	}
	double lambda = ((Dxx + Dyy) - sqrt((Dxx - Dyy) * (Dxx - Dyy) + 4 * Dxy * Dxy)) / 2.0;
	double den = sqrt(Dxy * Dxy + (lambda - Dxx) * (lambda - Dxx));
	a = Dxy / den;
	b = (lambda - Dxx) / den;
	c = -a * x_mean - b * y_mean;
	return true;
}

void addPoint(POINT points[], POINT point) {    
	points[recentpos] = point;
	recentpos = (recentpos + 1) % ARRAYLENGTH;
	if (pointcount < ARRAYLENGTH)
		pointcount++;
	if( pointcount >= ARRAYLENGTH)
		beginpos = (beginpos + 1) % ARRAYLENGTH;
}

double IsNaN(double x)
{
	return !(x == x);
}

int main()
{
	BOOL ret;
	UINT count;
	POINT point;
	TCHAR pszFileName[MAX_PATH];
	TCHAR lpClassName[MAX_PATH];
	TCHAR windowText[MAX_PATH];
	RECT rect;
	while (true) {
		ret = GetCursorPos(&point);
		if (!ret) {
			//printf("GetCursorPos -> fail(%ld)\n", GetLastError());
		}
		else {
			//printf("GetCursorPos -> (%ld, %ld)\n", point.x, point.y);
			HWND hwnd = WindowFromPoint(point);
			if (hwnd == NULL || hwnd == INVALID_HANDLE_VALUE) {
				//printf("WindowFromPoint point=(%ld, %ld) -> hwnd=%p -> fail(%ld)\n", point.x, point.y, hwnd, GetLastError());
			}
			else {
				//printf("WindowFromPoint -> hwnd=%p\n", hwnd);
				DWORD dwProcessID = 0;
				GetWindowThreadProcessId(hwnd, &dwProcessID);

				HANDLE hProcess = NULL;
				hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID);

				count = GetModuleFileNameEx(
					hProcess,
					NULL,
					pszFileName,
					MAX_PATH
				);
				//printf(("GetWindowModuleFileName hwnd=%p -> count=%d, fileName=%s\n"), hwnd, count, pszFileName);
				CloseHandle(hProcess);
				count = GetClassName(hwnd, lpClassName, MAX_PATH);
				//printf(("GetClassName hwnd=%p -> count=%d, lpClassName=%s\n"), hwnd, count, lpClassName);


				count = GetWindowRect(hwnd, &rect);
				LONG h = rect.bottom - rect.top, w = rect.right - rect.left;
				addPoint(points, point);
				/*double kx = point.x - recent.x;
				double ky = point.y - recent.y;
				if (kx == 0 && ky == 0)
					k = 0;
				else if (kx == 0)
					k = MAX;
				else
					k = ky / kx;*/
				if (beginpos == 0 || beginpos == ARRAYLENGTH / 2) {
					double x, y, z;
					lineFit(points, beginpos, x, y, z);
					//printf("x,y,z: %f,%f,%f", x, y, z);
					if (y == 0)
						k = MAX;
					else {
						if (!IsNaN(-x / y) && !IsNaN(-z / y)) {
							k = -x / y;
							b = -z / y;
						}
					}
					printf("k,b:%f,%f\n", k, b);
					if (k <= 0.1 && k >= -0.1) {
						if (!readbegin) {
							readbegin = 1;
							readcount++;
							b = (point.y + recent.y) / 2;
						}
						else {
							b = (b*readcount + (point.y + recent.y) / 2) / (readcount + 1);
							readcount++;
						}
					}
					else {
						readbegin = 0;
						readcount = 0;
						misscount++;
					}
					if (readcount >= 500) {
						isreading = 1;
						misscount = 0;
					}
					if (misscount >= 100)
						isreading = 0;
					printf("readcount:%d\n", readcount);

				}
				
				//printf("isreading:%d\n", isreading);
				//if (!(point.x - recent.x))scroll = 1;
				/*else {
					double ky = point.y - recent.y, kx = point.x - recent.x;
					double k = ky / kx;
					if (k > 0 && (point.y - recent.y) < 0)scroll = 1;
					else if (scroll) {*/
				if (isreading) {
					if (point.y >= 0.8*h + rect.top) {
						mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -200, 0);
						printf("success\n");
					}
					//scroll = 0;
				}
					/*}
					if (point.x - recent.x)printf("%f\n", k);
				}*/
				recent = point;
				/*printf(("GetWindowText scroll=%d -> fault=%d, width=%d\n"), down_scroll_count, scroll_out_range, w);
				if (point.y >= 0.95*h + rect.top) {
					if (down_scroll_count < SCROLL_MAX)down_scroll_count++;
					else {
						mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -200, 0);
						down_scroll_count = 0;
					}
				}
				else if (down_scroll_count) {
					if (scroll_out_range < OUT_RANGE_MAX)scroll_out_range++;
					else
					{
						scroll_out_range = 0;
						down_scroll_count = 0;
					}
				}*/
			}
		}
		//Sleep(5000);
	}
	return 0;
}