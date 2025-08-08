//Implementation file of CNeuralNet class
//
#include "CNeuralNet.h"
//
CNeuralNet::CNeuralNet(int i_InputNum, int i_OutputNum, int i_HiddenLayerNum, int i_NeuronEachHidden, double d_LearningRate)
{
	m_iNumInputs=i_InputNum; m_iNumOutputs=i_OutputNum; m_iNumHiddenLayers=i_HiddenLayerNum;
	m_iNumNeuronEachHiddenLayer=i_NeuronEachHidden; m_dLearningRate=d_LearningRate;

	m_dBias=NN_BIAS; m_dSigmoidP=NN_SIGMOID_P;

	m_dErrorSum=9999;	//used for error checking
	m_bNNTrainFlag=false;
	m_iNumEpoches=0;

	//create NN
	CreateNN();
}

void CNeuralNet::CreateNN()
{	
	//For no hidden layer network, create just the output layer
	if(m_iNumHiddenLayers==0){
		m_vectNeuronLayers.push_back(SNeuronLayer(m_iNumOutputs, m_iNumInputs));
		return;
	}
	
	//for there is at least one hidden layer network
	m_vectNeuronLayers.push_back(SNeuronLayer(m_iNumNeuronEachHiddenLayer, m_iNumInputs));
		//if there is more than one hidden layer
	for(int iH=1; iH<m_iNumHiddenLayers; iH++){
		m_vectNeuronLayers.push_back(SNeuronLayer(m_iNumNeuronEachHiddenLayer,m_iNumNeuronEachHiddenLayer));
	}

	//for the output layer
	m_vectNeuronLayers.push_back(SNeuronLayer(m_iNumOutputs, m_iNumNeuronEachHiddenLayer));

}

void CNeuralNet::InitNN()	//initialize all the neuron's weight and reset NN 
{
	m_dErrorSum=9999;
	m_iNumEpoches=0;
//	m_bNNTrainFlag=false;

	//initialize all the weights of neurons in all layers, including the weight of bias
	for(int iL=0; iL<m_iNumHiddenLayers+1; iL++){
		for(int iN=0; iN<m_vectNeuronLayers[iL].m_iNumNeurons; iN++){
			for(int iW=0; iW<m_vectNeuronLayers[iL].m_vectNeurons[iN].m_iNumNNInputs; iW++){
				m_vectNeuronLayers[iL].m_vectNeurons[iN].m_vectWeights[iW]=Rand_NN();
			}
		}
	}
}

double CNeuralNet::Sigmoid(double d_ActVal, double d_PVal)
{
	return (double)(1.0/(1+exp(-d_ActVal/d_PVal)));
}

vector<double> CNeuralNet::Update(vector<double> v_InputV)
{
	//update the NN output for the new input, and keep each neuron result seperately
	vector<double> vectTmp=v_InputV;
	vector<double> vectResult;

	int i_IP=0; double dTmp=0;
	
	//error check
	if(v_InputV.size()!=m_iNumInputs){
		return vectResult;	//return an empty vector if error happen
	}

	//add jitter(noise)...
	for(int i_J=0; i_J<v_InputV.size(); i_J++){
		v_InputV[i_J]+=Rand_NN() * NN_JITTER_RATE;
	}

	//for each layer
	for(int i=0; i<m_iNumHiddenLayers+1; i++){
		//for each neuron
		for(int j=0; j<m_vectNeuronLayers[i].m_iNumNeurons; j++){
			//for each weight, except bias weight
			for(int k=0; k<m_vectNeuronLayers[i].m_vectNeurons[j].m_iNumNNInputs-1; k++){
				dTmp+=vectTmp[i_IP]*m_vectNeuronLayers[i].m_vectNeurons[j].m_vectWeights[k];
				i_IP++;
			}
			//bias weight
			dTmp+=m_dBias*m_vectNeuronLayers[i].m_vectNeurons[j].m_vectWeights[k];

			//get the ouput and save
			double dAct=Sigmoid(dTmp, m_dSigmoidP);
			m_vectNeuronLayers[i].m_vectNeurons[j].m_dActivation=dAct;

			//make the temp new result layer
			vectResult.push_back(dAct);

			//reset
			i_IP=0;
			dTmp=0;
		}
		//transfer the new result vector to input for next round calc.
		vectTmp=vectResult;
		vectResult.clear();
		i_IP=0;
	}

	return vectTmp;
}

bool CNeuralNet::NNBackPropEpoch(vector<vector<double> > &v_InputSet, vector<vector<double> > &v_OutputSet)
{//Note: only calc. for 1 output layer and 1 hidden layer neural network
	//Input set and output set contains all input vectors and output result vecotrs for training
	vector<double>::iterator it_Weight;
	vector<SNeuron>::iterator it_HNeuron;
	vector<SNeuron>::iterator it_ONeuron;

	double d_Err, d_Weight; int i_MWP; //temp. variable for Momentum calc. 
	d_Err=0; d_Weight=0;

	m_dErrorSum=0;

	//Back-propagation calc., Data input & output set loop to adjust weights
	for(int iIS=0; iIS<v_InputSet.size(); iIS++){
		
		vector<double> v_TestOutput=Update(v_InputSet[iIS]);
		if(v_TestOutput.size()==0){
			return false;
		}
		
		//First stage: adjust output layer neuron's weights
		//for each neuron in output layer
		for(int iNO=0; iNO<m_iNumOutputs; iNO++){
			//Ek=(Tk-Ok)*Ok*(1-Ok)
			d_Err=(v_OutputSet[iIS][iNO]-v_TestOutput[iNO])*v_TestOutput[iNO]*(1-v_TestOutput[iNO]);

			//record each neuron's error for next layer's backprop calc.
			m_vectNeuronLayers[1].m_vectNeurons[iNO].m_dError=d_Err;
			
			//calc error sum square ---SSE+=(Tk-Ok)*(Tk-Ok)
			m_dErrorSum+=(v_OutputSet[iIS][iNO]-v_TestOutput[iNO])*(v_OutputSet[iIS][iNO]-v_TestOutput[iNO]);

			//-------------------------------------------------
			//reset current weight # pointer & current hidden neuron # pointer for Wjk
			it_Weight=m_vectNeuronLayers[1].m_vectNeurons[iNO].m_vectWeights.begin();
			it_HNeuron=m_vectNeuronLayers[0].m_vectNeurons.begin();

			i_MWP=0; //momentum weight pointer

			//for each weight adjusted calc in output layer
			for(; it_Weight<m_vectNeuronLayers[1].m_vectNeurons[iNO].m_vectWeights.end()-1; it_Weight++){
				// Wjk+=L*Ek*O+m*Prev_Wjk, in which with momentum calc.
				d_Weight=m_dLearningRate * d_Err * (it_HNeuron->m_dActivation);
				*it_Weight+=d_Weight+NN_MOMENTUM_RATE*(m_vectNeuronLayers[1].m_vectNeurons[iNO].m_vectPrevWeights[i_MWP]);
				
				//record the previous weight update
				m_vectNeuronLayers[1].m_vectNeurons[iNO].m_vectPrevWeights[i_MWP]=d_Weight;
				it_HNeuron++; 
				i_MWP++;
			}

			//and the bias weight adj.
			d_Weight=m_dLearningRate * d_Err * m_dBias;
			*it_Weight+=d_Weight+NN_MOMENTUM_RATE*(m_vectNeuronLayers[1].m_vectNeurons[iNO].m_vectPrevWeights[i_MWP]);

			//record the previous bias weight update
			m_vectNeuronLayers[1].m_vectNeurons[iNO].m_vectPrevWeights[i_MWP]=d_Weight;
		}

		//Second stage: adjust hidden layer neuron weights
		//for each neuron in hidden layer
		int i_HPos=0;
		for(it_HNeuron=m_vectNeuronLayers[0].m_vectNeurons.begin(); it_HNeuron<m_vectNeuronLayers[0].m_vectNeurons.end();it_HNeuron++){
			double d_Err=0;
			it_ONeuron=m_vectNeuronLayers[1].m_vectNeurons.begin();

			while(it_ONeuron<m_vectNeuronLayers[1].m_vectNeurons.end()){
				d_Err+=(it_ONeuron->m_dError)*(it_ONeuron->m_vectWeights[i_HPos]);
				it_ONeuron++;
			}
			
			//calc. the hidden neuron's error
			d_Err*=(it_HNeuron->m_dActivation)*(1-it_HNeuron->m_dActivation);

			//adj. the hidden neuron's weights
			//for each weight adjusted
			for(int i_IW=0; i_IW<m_iNumInputs; i_IW++){
				d_Weight=m_dLearningRate * d_Err * v_InputSet[iIS][i_IW];
				it_HNeuron->m_vectWeights[i_IW]+=d_Weight+NN_MOMENTUM_RATE*it_HNeuron->m_vectPrevWeights[i_IW];

				//record the previous weight update
				it_HNeuron->m_vectPrevWeights[i_IW]=d_Weight;
			}

			//and the bias weight adj.
			d_Weight=m_dLearningRate * d_Err * m_dBias;
			it_HNeuron->m_vectWeights[i_IW]+=d_Weight+NN_MOMENTUM_RATE*it_HNeuron->m_vectPrevWeights[i_IW];
			
			//reocrd the bias previous weight update
			it_HNeuron->m_vectPrevWeights[i_IW]=d_Weight;

			//increase output layer's pointer
			i_HPos++;
		}//next neuron in hidden layer
	}//next input & output vector calc. for weights adj.
	//
	return true;
}

bool CNeuralNet::TrainNN(CData* data_In, HWND hWnd)
{
	//get input data set & output data set
	vector<vector<double> > v_DataInSet=data_In->getDataSetIn();
	vector<vector<double> > v_DataOutSet=data_In->getDataSetOut();

	//error check
	if((v_DataInSet.size()!=v_DataOutSet.size())||(v_DataInSet[0].size()!=m_iNumInputs)||(v_DataOutSet[0].size()!=m_iNumOutputs)){

		MessageBox(NULL, "Data training set error!", "Error", MB_OK);
		return false;
	}
	
	//initializate NN
	InitNN();

	//Train the NN
	while(m_dErrorSum>NN_ERROR_THRESHOLD){
		if(!NNBackPropEpoch(v_DataInSet, v_DataOutSet)){
			return false;
		}

		//keep updating windows display during training to show the process
		InvalidateRect(hWnd, NULL, TRUE);
		UpdateWindow(hWnd);

		//increase the epoch number
		++m_iNumEpoches;

		//set flag
	}
	m_bNNTrainFlag=true;

	return true;
}
	
