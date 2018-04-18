// CNTK-Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "CNTKLibrary.h"
#include <unordered_map>
using namespace std;

int main()
{
	const size_t input1Dim = 1;
	const size_t input2Dim = 1;
	const size_t outputDim = input1Dim * input2Dim;
	double input1Scale = 2.0;
	double input1Offset = -3.0;

	wcout << L"Test Function: f(x,y) = (x*" << input1Scale << L"+" << input1Offset << L") * y\n";

	CNTK::Variable input1 = CNTK::InputVariable({ input1Dim }, CNTK::DataType::Double, true, L"x");
	CNTK::Variable input2 = CNTK::InputVariable({ input2Dim }, CNTK::DataType::Double, true, L"y");
	CNTK::FunctionPtr scale1 = CNTK::Constant::Scalar(CNTK::DataType::Double, input1Scale, CNTK::DeviceDescriptor::UseDefaultDevice());
	CNTK::FunctionPtr offset1 = CNTK::Constant::Scalar(CNTK::DataType::Double, input1Offset, CNTK::DeviceDescriptor::UseDefaultDevice());
	CNTK::FunctionPtr scaledInput1 = CNTK::ElementTimes(scale1, input1, L"scaled-state");
	CNTK::FunctionPtr transInput1 = CNTK::Plus(scaledInput1, offset1, L"transformed-state");
	//CNTK::Lin(input1, input2, L"input1*input2");
	CNTK::FunctionPtr output = CNTK::ElementTimes(transInput1,input2,L"(input1*scale1 + offset1)*input2");
	vector<double> outputVector = vector<double>(outputDim);

	//test evaluating the output function
	unordered_map<CNTK::Variable, CNTK::ValuePtr> args = unordered_map<CNTK::Variable, CNTK::ValuePtr>();
	vector<double> input1Data = vector<double>(input1Dim);
	input1Data[0] = 0.75;
	//input1Data[1] = 1.52;
	vector<double> input2Data = vector<double>(input2Dim);
	input2Data[0] = 0.5;
	//input2Data[1] = 2;
	CNTK::ValuePtr outputValuePtr;
	unordered_map<CNTK::Variable, CNTK::ValuePtr> out = unordered_map<CNTK::Variable, CNTK::ValuePtr>();
	args[input1] = CNTK::Value::CreateBatch({ input1Dim }, input1Data, CNTK::DeviceDescriptor::UseDefaultDevice());
	args[input2] = CNTK::Value::CreateBatch({ input2Dim }, input2Data, CNTK::DeviceDescriptor::UseDefaultDevice());
	out[output] = nullptr;

	output->Evaluate(args, out);
	outputValuePtr = out[output];
	//size_t totalSize = output->Output().Shape().TotalSize();
	//wcout << L"Function Output Shape: " << output->Output().Shape().AsString() << L"\n";
	//wcout << L"Evaluation Output Shape: " << outputValuePtr->Shape().AsString() << L"\n";

	CNTK::NDArrayViewPtr cpuArrayOutput = CNTK::MakeSharedObject<CNTK::NDArrayView>(outputValuePtr->Shape() , outputVector, false);
	outputValuePtr = out[output];
	cpuArrayOutput->CopyFrom(*outputValuePtr->Data());
	wcout << L"Function evaluation : f(" << input1Data[0] << L"," << input2Data[0] << L") = [";
	for (size_t i = 0; i<outputVector.size(); i++)
		wcout << outputVector[i] << L" ";
	wcout << L"]\n";
	//test gradient calculation
	unordered_map<CNTK::Variable, CNTK::ValuePtr> arguments= unordered_map<CNTK::Variable, CNTK::ValuePtr>();
	unordered_map<CNTK::Variable, CNTK::ValuePtr> gradients = unordered_map<CNTK::Variable, CNTK::ValuePtr>();

	arguments[input1] = CNTK::Value::CreateBatch({ (size_t)input1Dim }, input1Data, CNTK::DeviceDescriptor::UseDefaultDevice());
	arguments[input2] = CNTK::Value::CreateBatch({ (size_t)input2Dim }, input2Data, CNTK::DeviceDescriptor::UseDefaultDevice());

	gradients[input2] = nullptr;
	output->Gradients(arguments, gradients);
	
	vector<double> gradientVector = vector<double>(input2Dim);
	CNTK::ValuePtr input2Gradient = gradients[input2];
	CNTK::NDArrayViewPtr cpuArrayOutput2 =
		CNTK::MakeSharedObject<CNTK::NDArrayView>(input2Gradient->Shape(), gradientVector, false);
	cpuArrayOutput2->CopyFrom(*input2Gradient->Data());

	wcout << L"Gradient wrt y= [ ";
	for (size_t i = 0; i<gradientVector.size(); i++)
		wcout << gradientVector[i] << L" ";
	wcout << L"]\n";

	char c= getchar();
    return 0;
}

