//System main global parameter defination 
#ifndef DEFINE_H
#define DEFINE_H

//Data parameter
#define DATA_PATTERN_NUM 6	 //Up, down, left, right, clockwise, counter-clockwise
#define DATA_VECTOR_NUM 16	 //16 input vectors (32 points) must be even number. 
#define DATA_MATCH_THRESHOLD 0.80//0.95-0.98, for making sure about the recognition
 
//Neural network parameter
	//basic NN settings
#define NN_INPUT_NUM 32			// must = DATA_VECTOR_NUM * 2
#define NN_HIDDENLAYER_NUM 1	// must be 1
#define NN_NEURONPERHIDDEN_NUM 8// good = DATA_VECTOR_NUM / 2
#define NN_OUTPUT_NUM 6			// must = DATA_PATTERN_NUM

#define NN_SIGMOID_P 1			// good = 1
#define NN_BIAS	-1				// must = -1

#define NN_LEARNING_RATE 0.5	 //good=0.5, need to be tested, 0.2 to 0.8
#define NN_ERROR_THRESHOLD 0.003 //good=0.003, need to be tested, 0.001 to 0.05

	//NN improvement tricks (momentum , jitter...)
#define NN_MOMENTUM_RATE 0.9	//good=0.9, for momentum (in NNBackprop )
#define NN_JITTER_RATE 0.05		//good =0.01 to 0.2, for jitter (in Update)			

#endif