//neural network main horse
#ifndef CNEURALNET_H
#define CNEURALNET_H
//
#pragma warning (disable: 4786)
//
#include <vector>
#include <math.h>
#include <sstream>
#include "windows.h"

#include "Define.h"
#include "CData.h"
#include "Utility.h"
//
using namespace std;
//
struct SNeuron{
	int m_iNumNNInputs;

	vector<double> m_vectWeights;
	vector<double> m_vectPrevWeights; //for momentum vector
	
	double m_dActivation;
	double m_dError;
	//
	SNeuron(int iIn){
		m_iNumNNInputs=iIn+1;	//plus one bias 
		
		for(int i=0; i<m_iNumNNInputs; i++){
			m_vectWeights.push_back(Rand_NN()); //init un-trained NN

			m_vectPrevWeights.push_back(0);	//init previous weights record for momentum update
		}
		
		m_dActivation=0;
		m_dError=0; //for back-prop use
	}
};

struct SNeuronLayer{
	int m_iNumNeurons;
	vector<SNeuron> m_vectNeurons;

	SNeuronLayer(int iNeuronNum, int iInputs){
		m_iNumNeurons=iNeuronNum;
		for(int i=0; i<m_iNumNeurons; i++){
			m_vectNeurons.push_back(SNeuron(iInputs));
		}
	}
};

//
class CNeuralNet{
//protected: 
private:
	vector<SNeuronLayer> m_vectNeuronLayers;
	int m_iNumInputs;
	int m_iNumOutputs;
	int m_iNumHiddenLayers;
	int m_iNumNeuronEachHiddenLayer;
	
	double m_dLearningRate;
	double m_dErrorSum;
	double m_dBias;
	double m_dSigmoidP;

	bool m_bNNTrainFlag;
	int m_iNumEpoches;

	//protected member func.
	void CreateNN();
	void InitNN();
	bool NNBackPropEpoch(vector<vector<double> > &v_InputSet, vector<vector<double> > &v_OutputSet);
	double Sigmoid(double d_ActVal, double d_PVal);

public:
	CNeuralNet(int i_InputNum, int i_OutputNum, int i_HiddenLayerNum, int i_NeuronEachHidden, double d_LearningRate);

	vector<double> Update(vector<double> v_InputV);
	bool TrainNN(CData* data_In, HWND hWnd);
	
	//Access methods
	double getNNError(){return m_dErrorSum;}
	int getNumEpoch(){return m_iNumEpoches;}
	bool getTrainingFlag(){return m_bNNTrainFlag;}
};
//
#endif