//System data class 
#ifndef CDATA_H
#define CDATA_H
//
#pragma warning (disable: 4786)
//
#include "Define.h"
#include <vector>
#include <sstream>
#include "windows.h"

using namespace std;
//
class CData{
//protected:
private:
	vector<vector<double> > m_vectDataSetIn;	//include all pattern points info.
	vector<vector<double> > m_vectDataSetOut;//include all corresponding output, each pattern
											//has one ouput which found pattern set 1, others set 0.
	vector<vector<double> > m_vectPatternRec;
	vector<string> m_vectPatternName;

	int m_iNumPattern;			//system pattern number
	int m_iSizePatternVector;	//each pattern vector size (how many points/2D vectors)

public:
	//
	CData(int i_PatternNum, int i_PatternVectNum);
	
	void InitData();				//initialize the default pattern input and name
	void CreateTrainingDataSet();	//create vector data set input and output
	bool AddNewData(vector<double> &v_In, string &str_In);	//add new input & name into record

	string getPatternName(int i_Pos);
	vector<vector<double> > getDataSetIn(){return m_vectDataSetIn;}
	vector<vector<double> > getDataSetOut(){return m_vectDataSetOut;}
};

#endif