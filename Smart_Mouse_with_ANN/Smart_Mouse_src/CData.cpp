//Implementation file for CData.h
//
#include "CData.h"
//
const double InputVectorDef[DATA_PATTERN_NUM][DATA_VECTOR_NUM*2]={//6 pattern,16 input vect,32 points
	-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0, //left
	 1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,	//right
	0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,	//Down (windows)
	0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1,0,-1, //Up (windows)
	1,0,1,0,1,0,1,0,0,-1,0,-1,0,-1,0,-1,-1,0,-1,0,-1,0,-1,0,0,1,0,1,0,1,0,1, //counter-clockwise square
	1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,-1,0,-1,0,-1,0,-1,0,0,-1,0,-1,0,-1,0,-1	//clockwise square
};
const string PatternNameDef[DATA_PATTERN_NUM]={
	"Left", "Right", "Down", "Up", "Counter-clockwise square", "Clockwise square"
};

//
CData::CData(int i_PatternNum, int i_PatternVectNum)
{
	m_iNumPattern=i_PatternNum;
	m_iSizePatternVector=i_PatternVectNum;
	//
	InitData();
	CreateTrainingDataSet();
}

void CData::InitData()
{
	for(int i=0; i<m_iNumPattern; i++){
		vector<double> vectTmp;
		for(int j=0; j<m_iSizePatternVector*2; j++){
			vectTmp.push_back(InputVectorDef[i][j]);
		}
		m_vectPatternRec.push_back(vectTmp);
		m_vectPatternName.push_back(PatternNameDef[i]);
	}
}

void CData::CreateTrainingDataSet()	
{
	m_vectDataSetIn.clear();
	m_vectDataSetOut.clear();
	//
	for(int i=0; i<m_iNumPattern; i++){
		m_vectDataSetIn.push_back(m_vectPatternRec[i]);
		//
		vector<double> vectTmp;
		for(int j=0; j<m_iNumPattern; j++){
			if(j==i){
				vectTmp.push_back(1);
			}else{
				vectTmp.push_back(0);
			}
		}
		m_vectDataSetOut.push_back(vectTmp);
	}

}

bool CData::AddNewData(vector<double> &v_In, string &str_In)
{
	//error check
	if(v_In.size()!=m_iSizePatternVector*2){
		MessageBox(NULL, "New pattern data has incorrect vector size (points number)!", 
							"Error", MB_OK);
		return false;
	}
	
	//Add new data to pattern record.
	m_vectPatternRec.push_back(v_In);
	m_vectPatternName.push_back(str_In);

	++m_iNumPattern;

	CreateTrainingDataSet();
	return true;

}

string CData::getPatternName(int i_Pos)
{

	if((i_Pos<m_vectPatternName.size())&&(m_vectPatternName.size()>0)){
		return m_vectPatternName[i_Pos];
	}else{
		return "";
	}
}

