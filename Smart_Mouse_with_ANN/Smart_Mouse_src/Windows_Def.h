//GH Test defines....
#ifndef WINDOWS_DEF_H
#define WINDOWS_DEF_H

#include <windows.h>
#include "time.h"
//---------------------------------------------------
//define windows display size 
#define WINDOWS_WIDTH 350	
#define WINDOWS_HEIGHT 300

//define windows user message 
#define UM_NNTRAINING (WM_USER+0)

//----------------------------
//define windows global variables 
	//windows device handle
	HDC hdc;

	//windows screen update timing
	int iScrnUpdateTimer;
	BOOL bTimer1;

	//windows pen, brush and verties 
	HPEN OldPen; 
	HPEN RedPen;
	HBRUSH OldBrush;
	HBRUSH GreenBrush;

	//Windows back buffer
	HDC hdcBackBuf;
	HBITMAP hBitMap;
	HBITMAP hOldBitMap;

	//windows rectangle, circle, and polygon 
	RECT rect1;
	RECT circle1;
	RECT ellipse1;
		//text output rectangle ...
	RECT rectT1;

	//client window size
	int cxClient=0;
	int cyClient=0;

	//Windows color 
	COLORREF WHITE; 
	COLORREF RED;
	COLORREF GREEN;
	COLORREF BLUE; 
	COLORREF BLACK;

	//global # and variables

//----------------------------------------------------
//define functions...
	//define global functions
	void OnAppInit_GH();

	//define GH functions for message pump
	void OnWinInit_GH(HWND hWnd);
	void OnCreate_GH(HWND hWnd);
	void OnKeyUp_GH(HWND hWnd, WPARAM wParam, LPARAM lParam);
	void OnPaint_GH(HWND hWnd);
	void OnSizeChange_GH(HWND hWnd, LPARAM lParam);
	void OnDestroy_GH();

	//define windows functions
	LRESULT CALLBACK WinTest_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	VOID CALLBACK WinTest_TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);
	
	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);

	//user functions...

	//
//------------------------------------------------------
#endif