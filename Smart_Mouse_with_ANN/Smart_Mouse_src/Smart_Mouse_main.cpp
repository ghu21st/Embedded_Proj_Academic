//Smart_Mouse1 main program
//Artificial Neural Network Mouse Pattern Recognition 1.0
//Author: Gang Hu; Date: Dec, 2006
//---------------------------------------------------------------
//
#pragma warning (disable: 4786)
//
#include "Windows_Def.h"
#include "define.h"
#include <time.h>
//
#include "CData.h"
#include "CNeuralNet.h"
#include "CNNPattnRcn.h"

#include "resource.h"
//--------------global variables-------------------------
char*	g_szWin_TestClassName = "Smart_Mouse_Win_Class";
char*	g_szWin_TestAppWindowName="Smart Mouse V1.0";

CNNPattnRcn *mprNN;
static HINSTANCE m_hInstance;
//
using namespace std;
//--------------------------------------------------------------
//Implementation for Global and user functions
void OnAppInit_GH()
{
	//brush and pen init
	RedPen=CreatePen(PS_DOT, 1, RGB(255, 0, 0));
	GreenBrush=CreateSolidBrush(RGB(0, 255, 0));
	OldPen=NULL;
	OldBrush=NULL;

	//set the initial text position...
	rectT1.bottom=100;
	rectT1.left=10;
	rectT1.right=200;
	rectT1.top=10;

	//init color settings
	BLACK=RGB(0, 0, 0);
	WHITE=RGB(255,255,255);
	RED=RGB(255, 0, 0);
	GREEN=RGB(0, 255, 0);
	BLUE=RGB(0, 0, 255);

	//windows screen update timing. 40 ms (25 fps)
	iScrnUpdateTimer=40;	

}

//--------------------------------------------------------------
//Implementation for message pump
void OnWinInit_GH(HWND hWnd)
{

}

void OnCreate_GH(HWND hWnd)
{
	//actived by WM_CREATE message when windows created first time
	PlaySound("window_open.wav", NULL, SND_FILENAME|SND_ASYNC);

	//get the window's cx and cy
	RECT rect;
	GetClientRect(hWnd, &rect);
	cxClient=rect.right;
	cyClient=rect.bottom;

	//make time-based random() 
	srand((unsigned)time(NULL));

	//Back buffer created ...
	hdcBackBuf=CreateCompatibleDC(NULL);
	HDC hdc=GetDC(hWnd);
	hBitMap=CreateCompatibleBitmap(hdc, cxClient, cyClient);
	hOldBitMap=(HBITMAP)SelectObject(hdcBackBuf, hBitMap);
	ReleaseDC(hWnd, hdc);

	//init timer
	bTimer1=FALSE;
	SetTimer(hWnd, 1, iScrnUpdateTimer, WinTest_TimerProc); //Timer 1 (ID = 1) set start

	//init NN pattern recognition main pointer
	mprNN=new CNNPattnRcn(hWnd);
}

void OnKeyUp_GH(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	//Keys press detection...
	switch(wParam)
	{	//judge which key is pressed...
		case VK_ESCAPE:
			SendMessage(hWnd, WM_DESTROY, NULL, NULL);
//			PostQuitMessage(0);
			return;

		case 'L':	//enter learning mode 
			mprNN->LearningMode();
			break;

		case 'T':	//enter training mode
			if(!mprNN->TrainNN()){
//				SendMessage(hWnd, WM_DESTROY, NULL, NULL);
				MessageBox(hWnd, "Error happen! Try again or restart.", "Error", MB_OK);
			}
			break;

		case VK_RETURN:
			break;

		case VK_SPACE:
			break;
	
		default:
			return;
	}
}

void OnPaint_GH(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc;
	hdc=BeginPaint(hWnd, &ps);
	
	//Apply back buffer...
	BitBlt(hdcBackBuf, 0, 0, cxClient, cyClient, NULL, NULL, NULL, WHITE);
	
	//--------user graphic functions...----------

	mprNN->Render(hdcBackBuf, cxClient, cyClient);
	
	//------- end user funciton......------------

	//Back buffer drawn to front
	BitBlt(ps.hdc, 0, 0, cxClient, cyClient, hdcBackBuf, 0, 0, SRCCOPY);

//end drawing
	ReleaseDC(hWnd, hdc);

	EndPaint(hWnd, &ps);
//	Sleep(10);

	return;
}

void OnSizeChange_GH(HWND hWnd, LPARAM lParam)
{
	cxClient=LOWORD(lParam);
	cyClient=HIWORD(lParam);

	//for back buffer...
	SelectObject(hdcBackBuf, hOldBitMap);
	DeleteObject(hBitMap);
	hdc=GetDC(hWnd);
	hBitMap=CreateCompatibleBitmap(hdc, cxClient, cyClient);
	ReleaseDC(hWnd, hdc);
	SelectObject(hdcBackBuf, hBitMap);
}

void OnDestroy_GH()
{
	//clean windows mem...
	SelectObject(hdcBackBuf, hOldBitMap);
	DeleteDC(hdcBackBuf);
	DeleteObject(hBitMap);


	//kill the application, send the WM_QUIT message
	PostQuitMessage(0);

	//clean user define...
	delete mprNN;
}

//----------------------------------------------------------------
//Implementation for windows procedure
	//call back function for message pump
LRESULT CALLBACK WinTest_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//user variable define which related to HWND or initialize from here
		
	//end user define
	
	//window message processing start from here...
	OnWinInit_GH(hWnd);
	//
	switch(uMsg)
	{
		case WM_CREATE:
			OnCreate_GH(hWnd);
			break;

		case WM_KEYUP:
			OnKeyUp_GH(hWnd, wParam, lParam);
			break;

		case WM_PAINT:
			OnPaint_GH(hWnd);
			break;

		case WM_SIZE:
			OnSizeChange_GH(hWnd, lParam);
			break;

		case WM_DESTROY:
			OnDestroy_GH();
			break;
		
		case WM_RBUTTONDOWN:	//record mouse track action
			if(!mprNN->WIN_Action(true, m_hInstance)){
				MessageBox(hWnd, "Error happen! Try again or restart.", "Error", MB_OK);
//				SendMessage(hWnd, WM_DESTROY, NULL, NULL);
			}
			break;
		
		case WM_RBUTTONUP:	//stop mouse track action
			if(!mprNN->WIN_Action(false, m_hInstance)){
				MessageBox(hWnd, "Error happen! Please try again or restart.", "Error", MB_OK);
//				SendMessage(hWnd, WM_DESTROY, NULL, NULL);
			}
			break;
		
		case WM_MOUSEMOVE:	//add new data while mouse right button pressed
			if(mprNN->getActionFlag()){
				mprNN->AddPoints(MAKEPOINTS(lParam));
			}
			
			break;

		case UM_NNTRAINING:	//entern training
			mprNN->TrainNN();
			break;
	}
	//
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//callback fun. for Timer
VOID CALLBACK WinTest_TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	if(uMsg!=WM_TIMER){
		return;
	}
	switch(idEvent){
	case 1:
		bTimer1=TRUE;
		break;
	}
}

//-----------------------------------------
//Windows main function...
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//This application's windows handle
	HWND hWin_Wnd;

	//This application windows class structure
	WNDCLASSEX Win_TestWinClass;

	Win_TestWinClass.cbSize=sizeof(WNDCLASSEX);
	Win_TestWinClass.style=CS_HREDRAW|CS_VREDRAW;
	Win_TestWinClass.lpfnWndProc=WinTest_WindowProc;
	Win_TestWinClass.cbClsExtra=0;
	Win_TestWinClass.cbWndExtra=0;
	Win_TestWinClass.hInstance=hInstance;
	Win_TestWinClass.hIcon=LoadIcon(NULL, IDI_APPLICATION);
	Win_TestWinClass.hCursor=LoadCursor(NULL, IDC_ARROW);
//	Win_TestWinClass.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH); //not for backbuffer 
	Win_TestWinClass.hbrBackground=NULL;	// for backbuffer 
	Win_TestWinClass.lpszMenuName=NULL;
	Win_TestWinClass.lpszClassName=g_szWin_TestClassName;
	Win_TestWinClass.hIconSm=LoadIcon(NULL,IDI_APPLICATION);

	//register the windows class
	if(!RegisterClassEx(&Win_TestWinClass)){
		MessageBox(NULL, "Can not create the application's window class!", "Error", 0);
		return 0;
	}

	//Initializations before windows created
	OnAppInit_GH();
	
	m_hInstance=hInstance;	//init the global instance 

	//create the windows 
	hWin_Wnd=CreateWindowEx(NULL,		//extended window style
							  g_szWin_TestClassName, //registered window class name 	
							  g_szWin_TestAppWindowName,//application window name
//							  WS_OVERLAPPEDWINDOW|WS_THICKFRAME,//size changable and thick frame
							  WS_OVERLAPPED|WS_VISIBLE|WS_CAPTION|WS_SYSMENU,//size fixed hangable
							  0,		//horizontal position
							  0,		//vertical position
							  WINDOWS_WIDTH,	//window width
							  WINDOWS_HEIGHT,	//windows height
							  NULL,		//handle to parent/owner window
							  NULL,		//handle to menu
							  hInstance, //handle to application instance
							  NULL);		//window creation parameter

	if(!hWin_Wnd){
		MessageBox(NULL, "Can not create window!", "Error", 0);
	}
	//
	ShowWindow(hWin_Wnd, nShowCmd);
	UpdateWindow(hWin_Wnd);
	
	//Message pump & hold, analyze windows message
	MSG Win_AppMsg;
	//...............User procedure ..................
		//when windows start, send message to start training automatically
//	SendMessage(hWin_Wnd, UM_NNTRAINING, NULL, NULL);
	//

	while (GetMessage (&Win_AppMsg, NULL, 0, 0))
	{
		TranslateMessage (&Win_AppMsg);
		DispatchMessage (&Win_AppMsg);

	 //******User winmain() codes...****************
		//Execute User Proc by timer(time slice of task)...
		if(bTimer1){
		//---Start user main func ---------------------------------------------


		//---end user main func except windows update set by user timer--------
			
			//Update window display for user function
			InvalidateRect(hWin_Wnd, NULL, TRUE);
			UpdateWindow(hWin_Wnd);

			//reset timer flag
			bTimer1=false;	
			
		}
	//******end of user winmain() codes.***********
	} 

	//Unregister window class
	UnregisterClass(g_szWin_TestClassName, Win_TestWinClass.hInstance);
	//
	return Win_AppMsg.wParam;
}

/*
	while (GetMessage (&Win_AppMsg, NULL, 0, 0))
	{
     TranslateMessage (&Win_AppMsg);
     DispatchMessage (&Win_AppMsg);

     //update window
     InvalidateRect(hWin_Wnd, NULL, TRUE);
		 UpdateWindow(hWin_Wnd); 
	} 
*/							
