//Neural Network mouse pattern recognition 1.0
#ifndef CNNPATTNRCN_H
#define CNNPATTNRCN_H
//
#pragma warning (disable: 4786)
//
#include "Define.h"
#include "Utility.h"
#include "CData.h"
#include "CNeuralNet.h"
#include "SVector2D.h"

#include "windows.h"
#include <vector>
#include <sstream>
#include "math.h"

using namespace std;
//
enum PR_RunMode{UNREADY, LEARNING, TRAINING, READY}; //ANN standard procedure defination

//CNNPattnRcn class defination
class CNNPattnRcn{
//protected:	//member variables
private:

	//define NN and data
	CNeuralNet *m_pNN;
	CData *m_pData;

	//mouse input data
	vector<POINTS> m_vPathPoints;	//original input points vector <POINTS>
	vector<POINTS> m_vScaleDownPoints;	//scaled down points vector
	vector<double> m_vectData;	//reday data for NN

	int m_iNumDefPoints;
	bool m_bActed;
	
	int m_iNewInputNum;	//new input number 

	//Pattern matched variables
	double m_dHighestOutput;
	int m_iBestFit;
	int m_iMatchedPattern;

	int m_iNumPatterns;

	//control flag of mode
	PR_RunMode m_PRMode;

	//window handle
	HWND m_hWnd;

	//user input dialog for windows
	static BOOL CALLBACK DataInputDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	
	//input dialog data
	static string m_strInputData;

protected:	//member functions
	void Clear();	//clear data vectors
	void GenerateInputData();	//after scale down, normalize input to suit for NN input need
	bool PointsScaleDown();	//scale down points
	bool MatchTest();	//test for a match from output vector

public:
	//construction & destruction
	CNNPattnRcn(HWND hwnd);
	~CNNPattnRcn();

	//func. for NN oper.
	bool TrainNN();		//Train NN
	void Render(HDC &hdc, int cxClient, int cyClient); //Render the window
	bool WIN_Action(bool bIn, HINSTANCE hInst);	//drawing process
	void LearningMode();	//Learning mode
	void AddPoints(POINTS pIn){ m_vPathPoints.push_back(pIn);}	//Points adding

	//access methods
	bool getActionFlag(){return m_bActed;}
	PR_RunMode getPRMode(){ return m_PRMode;}

};

#endif