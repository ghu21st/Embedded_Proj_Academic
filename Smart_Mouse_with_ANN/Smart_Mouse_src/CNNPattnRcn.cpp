//Implementation file for CNNPattnRcn class
#include "CNNPattnRcn.h"

#include "resource.h" //for dialog window or other windows display resource
//
string CNNPattnRcn::m_strInputData=""; //Note: all static member variables need to be declared globally
//
//construction & destruction
CNNPattnRcn::CNNPattnRcn(HWND hwnd)
{
	m_iNumDefPoints=DATA_VECTOR_NUM+1;
	m_iNumPatterns=DATA_PATTERN_NUM;
	m_bActed=false; m_dHighestOutput=0;
	m_iBestFit=-1; m_iMatchedPattern=-1;
	m_hWnd=hwnd; m_PRMode=UNREADY;
	m_iNewInputNum=0;

	//
	m_pData=new CData(m_iNumPatterns, DATA_VECTOR_NUM);
	m_pNN=new CNeuralNet(DATA_VECTOR_NUM*2, m_iNumPatterns, NN_HIDDENLAYER_NUM, NN_NEURONPERHIDDEN_NUM,	NN_LEARNING_RATE);

}

CNNPattnRcn::~CNNPattnRcn()
{
	delete m_pData;
	delete m_pNN;
}

void CNNPattnRcn::Clear()	//clear data vectors
{
	m_vPathPoints.clear();
	m_vScaleDownPoints.clear();
	m_vectData.clear();

}

void CNNPattnRcn::GenerateInputData()	//after scale down, normalize input to suit for NN input need
{
	double d_TmpX, d_TmpY; 
	m_vectData.clear();

	//
	for(int i=1; i<m_vScaleDownPoints.size(); i++){
		d_TmpX=m_vScaleDownPoints[i].x-m_vScaleDownPoints[i-1].x;
		d_TmpY=m_vScaleDownPoints[i].y-m_vScaleDownPoints[i-1].y;

		SVector2D sv(d_TmpX, d_TmpY);
		Vector2DNorm(sv);
		m_vectData.push_back(sv.x);
		m_vectData.push_back(sv.y);
	}
}

bool CNNPattnRcn::PointsScaleDown()	//scale down points by inserting/deleting points have shortest distance
{
	//error check
	if(m_vPathPoints.size()<m_iNumDefPoints){
		return false;
	}
	//
	m_vScaleDownPoints=m_vPathPoints;
	//
	while(m_vScaleDownPoints.size()>m_iNumDefPoints){
		int iPointCnt=0;
		double dShortest=99999;
		
		//find shortest distance among all points.
		for(int iPN=2; iPN<m_vScaleDownPoints.size()-1; iPN++){ //keep the first and last points
			double d_Dist=sqrt((m_vScaleDownPoints[iPN].x-m_vScaleDownPoints[iPN-1].x)*
								(m_vScaleDownPoints[iPN].x-m_vScaleDownPoints[iPN-1].x)+
								(m_vScaleDownPoints[iPN].y-m_vScaleDownPoints[iPN-1].y)*
								(m_vScaleDownPoints[iPN].y-m_vScaleDownPoints[iPN-1].y));
			if(d_Dist<dShortest){
				dShortest=d_Dist;
				iPointCnt=iPN;
			}
		}

		//insert the new point and delete old two boundary points
		POINTS newPoint;
		newPoint.x=(m_vScaleDownPoints[iPointCnt-1].x+m_vScaleDownPoints[iPointCnt].x)/2;
		newPoint.y=(m_vScaleDownPoints[iPointCnt-1].y+m_vScaleDownPoints[iPointCnt].y)/2;
		m_vScaleDownPoints[iPointCnt-1]=newPoint;
		m_vScaleDownPoints.erase(m_vScaleDownPoints.begin()+iPointCnt);
	}
	return true;

}

bool CNNPattnRcn::MatchTest()	//test for a match from output vector
{
	//error check
	if(m_vectData.size()==0){
		MessageBox(NULL, "Error with the input vector!", "Error", MB_OK);
		return false;
	}
	//get the output vector
	vector<double> v_OutputTmp=m_pNN->Update(m_vectData);
	if(v_OutputTmp.size()==0){
		MessageBox(NULL, "Neural Network output error!", "Error", MB_OK);
		return false;
	}
	
	//find the matched pattern
	m_dHighestOutput=0;
	m_iBestFit=0;
	m_iMatchedPattern=0;
	
	for(int op=0; op<v_OutputTmp.size(); op++){
		if(v_OutputTmp[op]>m_dHighestOutput){
			m_dHighestOutput=v_OutputTmp[op];
			m_iBestFit=op;
			if(v_OutputTmp[op] > DATA_MATCH_THRESHOLD){
				m_iMatchedPattern=op;
			}
		}
	}
	
	return true;

}

//func. for NN oper.
bool CNNPattnRcn::TrainNN()		//Train NN
{
	m_PRMode=TRAINING;
	if(!m_pNN->TrainNN(m_pData, m_hWnd)){
		return false;
	}else{
		m_PRMode=READY;
		return true;
	}
}

void CNNPattnRcn::LearningMode()	//Learning mode
{
	m_PRMode=LEARNING;
	Clear();
	//
	InvalidateRect(m_hWnd, NULL, TRUE);
	UpdateWindow(m_hWnd);
}

bool CNNPattnRcn::WIN_Action(bool bIn, HINSTANCE hInst)	//drawing process, hInst for dialogbox
{
	//mouse pressed, bIn = true, adding points; otherwise, sample &analyze data
	if(bIn){
		Clear();
		m_bActed=true;
		return true;
	}

	//mouse released, analyze data
	if(PointsScaleDown()){
		//create data input
		GenerateInputData();
		
		//check mode and acting
		if(m_PRMode==READY){
			if(!MatchTest()){
				return false;
			}
		}else if(m_PRMode==LEARNING){
			if(MessageBox(NULL, "Do you want to accept what you input?", "Confirmation", MB_YESNO)==IDYES){
				//show the data input dialog window
				DialogBox(hInst, MAKEINTRESOURCE(IDD_Input_DLG), m_hWnd, DataInputDlgProc);
				
				//add new data into training set
				++m_iNewInputNum;
				ostringstream ostrTmp;
				ostrTmp<<m_iNewInputNum;
				string s1="User P"+ostrTmp.str();
				string s=s1+"- "+ m_strInputData;

				m_pData->AddNewData(m_vectData, s);

				//re-construct the NN
				delete m_pNN;
				++m_iNumPatterns;
				m_pNN=new CNeuralNet(DATA_VECTOR_NUM*2, m_iNumPatterns, NN_HIDDENLAYER_NUM, NN_NEURONPERHIDDEN_NUM, NN_LEARNING_RATE);
				
				//Train the new data set with the new customer inputed vector
				if(!TrainNN()){
					return false;
				}
				m_PRMode=READY;
			}else{
				m_vectData.clear();
			}
		}else{
			MessageBox(NULL, "Please train the ANN first. Wrong running mode!", "Error", MB_OK);
			return false;
		}
	}
	m_bActed=bIn;

	return true;
}

void CNNPattnRcn::Render(HDC &hdc, int cxClient, int cyClient) //Render the window display
{
	//
	if(m_PRMode==TRAINING){
		ostringstream ostrTmp1; 
		ostrTmp1<<m_pNN->getNumEpoch();
		string s1="Epoch: "+ostrTmp1.str();
		TextOut(hdc, 5, 5, s1.c_str(), s1.size());
		
		ostringstream ostrTmp2;
		ostrTmp2<<m_pNN->getNNError();
		string s2="Error: "+ostrTmp2.str();
		TextOut(hdc, cxClient/2, 5, s2.c_str(), s2.size());
		
		string s3="Training...";
		TextOut(hdc, 5, cyClient-25, s3.c_str(), s3.size());
	}else if(m_PRMode==READY){
		string s4="Ready! Hold right mouse button and draw...";
		TextOut(hdc, 5, cyClient-25, s4.c_str(), s4.size());
	}else if(m_PRMode==LEARNING){
		string s4="Input new gesture...";
		TextOut(hdc, 5, cyClient-25, s4.c_str(), s4.size());
	}else{
		string s4="'T'-Training, 'L'-Learning, 'ESC'-Quit";
		TextOut(hdc, 5, cyClient-25, s4.c_str(), s4.size());
	}

	//recognition result showing and mouse drawing 
	if(!m_bActed){
		if(m_dHighestOutput>0){
			if((m_vScaleDownPoints.size()>1)&&(m_PRMode!=LEARNING)){
				if(m_dHighestOutput<DATA_MATCH_THRESHOLD){ //guessing the pattern
					ostringstream ostrTmp1;
					double d_Out=m_dHighestOutput*100;
					ostrTmp1<<d_Out;
					string s="Guessing ("+ostrTmp1.str()+"%): ";
					s+=m_pData->getPatternName(m_iBestFit);
					TextOut(hdc, 5, 5, s.c_str(), s.size());
				}else{	//give the exact pattern
					string s="Matched :"+m_pData->getPatternName(m_iMatchedPattern);
					TextOut(hdc, 5, 5, s.c_str(), s.size());
				}
			}else if(m_PRMode!=LEARNING){
				SetTextColor(hdc, RGB(255, 0, 0));
				string s="Please try again! Not enough points inputed.";
				TextOut(hdc, 5, cyClient-25, s.c_str(), s.size());
				SetTextColor(hdc, RGB(0, 0, 0));
			}
		}
	}

}

//user input dialog for windows
BOOL CALLBACK CNNPattnRcn::DataInputDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	HWND hwndEditBox=GetDlgItem(hwnd, IDC_Input_EDIT);

	switch(msg){
	case WM_INITDIALOG:
		//initialize dialog...

		//
		return TRUE;
		break;

	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			//get input data from edit box in the dialog
			SetFocus(hwndEditBox);
			char chTmp[70];
			GetWindowText(hwndEditBox, chTmp, 70);
			//
//			if(chTmp==""){
//				strcpy(chTmp, "Empty Input");
//			}
			
			m_strInputData=chTmp;

			//end the dialog
			EndDialog(hwnd, 0);
			
			return TRUE;
			break;

		case IDCANCEL:
			EndDialog(hwnd, 0);

			return TRUE;
			break;
		}
		break;
	}
	//
	return FALSE;
}




