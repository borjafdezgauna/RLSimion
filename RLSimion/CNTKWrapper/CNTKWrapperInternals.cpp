/*
	SimionZoo: A framework for online model-free Reinforcement Learning on continuous
	control problems

	Copyright (c) 2016 SimionSoft. https://github.com/simionsoft

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#include "CNTKWrapperInternals.h"
#include "Parameter.h"
#include "Link.h"
#include "Chain.h"
#include "NetworkArchitecture.h"
#include "NetworkDefinition.h"
#include "InputData.h"
#include "OptimizerSetting.h"
#include "Chain.h"
#include "Exceptions.h"
#include "Network.h"


CNTK::FunctionPtr CNTKWrapper::InputLayer(Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{
	//MAKE SURE THAT THE STATE/ACTION SPACES ARE DEFINED BEFORE CALLING THIS
	//determine the linked input data
	wstring inputID = CNTKWrapper::Internal::string2wstring(
		pLink->getParameterByName<InputDataParameter>("Input Data")->getValue());

	NetworkDefinition* pNetworkDefinition =
		(pLink->getParentChain()->getNetworkArchitecture()->getNetworkDefinition());

	bool inputNeedsGradient = pNetworkDefinition->inputsNeedGradient();

	size_t numInputs;
	if (inputID == L"state-input")
	{
		numInputs = pNetworkDefinition->getInputStateVariables().size() + pNetworkDefinition->getInputActionVariables().size();
		pNetworkDefinition->setStateInputLayer(inputID);
	}
	else
	{
		numInputs = pNetworkDefinition->getInputActionVariables().size();
		pNetworkDefinition->setActionInputLayer(inputID);
	}

	
	return CNTK::InputVariable({ numInputs }, CNTK::DataType::Double, inputNeedsGradient, inputID);
}

CNTK::FunctionPtr CNTKWrapper::ActivationLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{
	auto activationFunction = pLink->getParameterByName<ActivationFunctionParameter>("Activation")->getValue();
	return CNTKWrapper::Internal::applyActivationFunction(dependencies.at(0)->getFunctionPtr(), activationFunction);
}

CNTK::FunctionPtr CNTKWrapper::Convolution1DLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{

	FunctionPtr input = dependencies.at(0)->getFunctionPtr();

	wstring name = pLink->getName();

	size_t filters = pLink->getParameterByName<IntParameter>("Filters")->getValue();
	CIntTuple1D kernelShape = pLink->getParameterByName<IntTuple1DParameter>("Kernel Size")->getValue();
	CIntTuple1D strideShape = pLink->getParameterByName<IntTuple1DParameter>("Stride")->getValue();
	ActivationFunction activationFunction = pLink->getParameterByName<ActivationFunctionParameter>("Activation")->getValue();

	size_t kernel = (size_t)kernelShape.getX1();

	size_t stride = (size_t)strideShape.getX1();

	return CNTKWrapper::Internal::Convolution1D(input, filters, kernel, stride, activationFunction, 1.0, device, name);
}

CNTK::FunctionPtr CNTKWrapper::Convolution2DLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{

	FunctionPtr input = dependencies.at(0)->getFunctionPtr();

	wstring name = pLink->getName();

	size_t filters = pLink->getParameterByName<IntParameter>("Filters")->getValue();
	CIntTuple2D kernelShape = pLink->getParameterByName<IntTuple2DParameter>("Kernel Size")->getValue();
	CIntTuple2D strideShape = pLink->getParameterByName<IntTuple2DParameter>("Stride")->getValue();
	ActivationFunction activationFunction = pLink->getParameterByName<ActivationFunctionParameter>("Activation")->getValue();

	size_t kernelWidth = (size_t)kernelShape.getX1();
	size_t kernelHeight = (size_t)kernelShape.getX2();

	size_t strideWidth = (size_t)strideShape.getX1();
	size_t strideHeight = (size_t)strideShape.getX2();

	return CNTKWrapper::Internal::Convolution2D(input, filters, kernelWidth, kernelHeight, strideWidth, strideHeight, activationFunction, 1.0, device, name);
}

CNTK::FunctionPtr CNTKWrapper::Convolution3DLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{

	FunctionPtr input = dependencies.at(0)->getFunctionPtr();

	wstring name = pLink->getName();

	size_t filters = pLink->getParameterByName<IntParameter>("Filters")->getValue();
	CIntTuple3D kernelShape = pLink->getParameterByName<IntTuple3DParameter>("Kernel Size")->getValue();
	CIntTuple3D strideShape = pLink->getParameterByName<IntTuple3DParameter>("Stride")->getValue();
	ActivationFunction activationFunction = pLink->getParameterByName<ActivationFunctionParameter>("Activation")->getValue();

	size_t kernelWidth = (size_t)kernelShape.getX1();
	size_t kernelHeight = (size_t)kernelShape.getX2();
	size_t kernelDepth = (size_t)kernelShape.getX3();

	size_t strideWidth = (size_t)strideShape.getX1();
	size_t strideHeight = (size_t)strideShape.getX2();
	size_t strideDepth = (size_t)strideShape.getX3();

	return CNTKWrapper::Internal::Convolution3D(input, filters, kernelWidth, kernelHeight, kernelDepth, strideWidth, strideHeight, strideDepth, activationFunction, 1.0, device, name);
}

CNTK::FunctionPtr CNTKWrapper::DenseLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor& device)
{

	int output_nodes = pLink->getParameterByName<IntParameter>("Units")->getValue();
	wstring name = pLink->getName();
	auto activationFunction = pLink->getParameterByName<ActivationFunctionParameter>("Activation")->getValue();

	CNTK::FunctionPtr layer= CNTKWrapper::Internal::FullyConnectedDNNLayer(
		dependencies.at(0)->getFunctionPtr(), output_nodes, device, activationFunction, name);

	return layer;
}

CNTK::FunctionPtr CNTKWrapper::DropoutLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{

	return CNTK::FunctionPtr();
}

CNTK::FunctionPtr CNTKWrapper::FlattenLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{

	wstring name = pLink->getName();
	NDShape inputShape = dependencies.at(0)->getFunctionPtr()->Output().Shape();

	return Reshape(dependencies.at(0)->getFunctionPtr(), { inputShape.TotalSize() }, name);
}

CNTK::FunctionPtr CNTKWrapper::ReshapeLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{

	wstring name = pLink->getName();
	CIntTuple4D shapeTuple = pLink->getParameterByName<IntTuple4DParameter>("4D Shape")->getValue();
	NDShape shape = {};
	if (shapeTuple.getX1() != 0)
		shape = shape.AppendShape({ (size_t)shapeTuple.getX1() });
	if (shapeTuple.getX2() != 0)
		shape = shape.AppendShape({ (size_t)shapeTuple.getX2() });
	if (shapeTuple.getX3() != 0)
		shape = shape.AppendShape({ (size_t)shapeTuple.getX3() });
	if (shapeTuple.getX4() != 0)
		shape = shape.AppendShape({ (size_t)shapeTuple.getX4() });

	return Reshape(dependencies.at(0)->getFunctionPtr(), shape, name);

}

CNTK::FunctionPtr CNTKWrapper::MergeLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{

	vector<Variable> inputs = vector<Variable>();
	if (dependencies.size() > 1)
	{
		for (auto pItem : dependencies)
			inputs.push_back((CNTK::Variable)pItem->getFunctionPtr());

		auto axisIndex = pLink->getParameterByName<IntParameter>("Axis")->getValue();
		CNTK::Axis axis(axisIndex);
		return Splice(inputs, axis);
	}

	return dependencies.at(0)->getFunctionPtr();
}

CNTK::FunctionPtr CNTKWrapper::BatchNormalizationLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{

	wstring name = pLink->getName();

	auto biasParams = Parameter({ NDShape::InferredDimension }, (float)0.0, device);

	auto scaleParams = Parameter({ NDShape::InferredDimension }, (float)0.0, device);

	auto runningMean = Parameter({ NDShape::InferredDimension }, (float)0.0, device);

	auto runningInvStd = Parameter({ NDShape::InferredDimension }, (float)0.0, device);
	
	auto runningCount = Constant::Scalar(0.0f, device);

	//TODO: check if spatial=false and 5000 are good values.
	return BatchNormalization(dependencies[0]->getFunctionPtr(), scaleParams, biasParams, runningMean, runningInvStd, runningCount, false);

}


CNTK::FunctionPtr CNTKWrapper::LinearTransformationLayer(const Link * pLink, vector<const Link*> dependencies, CNTK::DeviceDescriptor & device)
{
	wstring name = pLink->getName();
	double offset = pLink->getParameterByName<DoubleParameter>("Offset")->getValue();
	double scale = pLink->getParameterByName<DoubleParameter>("Scale")->getValue();

	auto timesParam = Constant::Scalar(DataType::Double, scale, device);

	auto timesFunction = ElementTimes(timesParam, dependencies.at(0)->getFunctionPtr(),L"Times");

	Constant plusParam(dependencies.at(0)->getFunctionPtr()->Output().Shape(), offset, device);
	
	return Plus(plusParam, timesFunction, name);

}

CNTK::TrainerPtr CNTKWrapper::CreateOptimizer(const OptimizerSettings * pOptimizerSetting, CNTK::FunctionPtr& output
	, CNTK::FunctionPtr& lossFunction, double learningRate)
{

	CNTK::LearnerPtr pLearner;
	switch (pOptimizerSetting->getOptimizerType())
	{
	case OptimizerType::SGD:
		pLearner = CNTK::SGDLearner(output->Parameters(),TrainingParameterPerSampleSchedule(learningRate));
		break;

	case OptimizerType::MomentumSGD:
		pLearner = CNTK::MomentumSGDLearner(output->Parameters(),
			TrainingParameterPerSampleSchedule(learningRate),
			TrainingParameterPerSampleSchedule(pOptimizerSetting->getParameterByKey("Momentum")->getValue())
		);
		break;

	case OptimizerType::Nesterov:
		pLearner = CNTK::NesterovLearner(output->Parameters(),
			TrainingParameterPerSampleSchedule(learningRate),
			TrainingParameterPerSampleSchedule(pOptimizerSetting->getParameterByKey("Momentum")->getValue())
		); break;

	case OptimizerType::FSAdaGrad:
		pLearner = CNTK::FSAdaGradLearner(output->Parameters(),
			TrainingParameterPerSampleSchedule(learningRate),
			TrainingParameterPerSampleSchedule(pOptimizerSetting->getParameterByKey("Momentum")->getValue()),
			DefaultUnitGainValue(),
			TrainingParameterPerSampleSchedule(pOptimizerSetting->getParameterByKey("Variance momentum")->getValue())
		); break;

	case OptimizerType::Adam:
		pLearner = CNTK::AdamLearner(output->Parameters(),
			TrainingParameterPerSampleSchedule(learningRate),
			TrainingParameterPerSampleSchedule(pOptimizerSetting->getParameterByKey("Momentum")->getValue()),
			DefaultUnitGainValue(),
			TrainingParameterPerSampleSchedule(pOptimizerSetting->getParameterByKey("Variance momentum")->getValue()),
			pOptimizerSetting->getParameterByKey("Epsilon")->getValue()
		); break;

	case OptimizerType::AdaGrad:
		pLearner = CNTK::AdaGradLearner(output->Parameters(),
			TrainingParameterPerSampleSchedule(learningRate)
		); break;

	case OptimizerType::RMSProp:
		pLearner = CNTK::RMSPropLearner(output->Parameters(),
			TrainingParameterPerSampleSchedule(learningRate),
			pOptimizerSetting->getParameterByKey("Gamma")->getValue(),
			pOptimizerSetting->getParameterByKey("Inc")->getValue(),
			pOptimizerSetting->getParameterByKey("Dec")->getValue(),
			pOptimizerSetting->getParameterByKey("Max")->getValue(),
			pOptimizerSetting->getParameterByKey("Min")->getValue()
		); break;

	case OptimizerType::AdaDelta:
		pLearner = CNTK::AdaDeltaLearner(output->Parameters(),
			TrainingParameterPerSampleSchedule(learningRate),
			pOptimizerSetting->getParameterByKey("Rho")->getValue(),
			pOptimizerSetting->getParameterByKey("Epsilon")->getValue()
		); break;
	}

	return CreateTrainer(output, lossFunction, lossFunction, { pLearner });
}


FunctionPtr CNTKWrapper::Internal::applyActivationFunction(FunctionPtr pInput, ActivationFunction activationFunction)
{
	switch (activationFunction)
	{
	case ActivationFunction::elu:
		return CNTK::ELU(pInput,L"ELU");
	case ActivationFunction::hard_sigmoid:
		//TODO: is this the correct function?
		return CNTK::Hardmax(pInput,L"HardMax");
	case ActivationFunction::relu:
		return CNTK::ReLU(pInput,L"ReLu");
	case ActivationFunction::selu:
		return CNTK::SELU(pInput, 1.0507009873554804934193349852946,1.6732632423543772848170429916717,L"SELU");
	case ActivationFunction::sigmoid:
		return CNTK::Sigmoid(pInput,L"Sigmoid");
	case ActivationFunction::softmax:
		return CNTK::Softmax(pInput,L"Softmax");
		//TODO: solve this problem
		//return std::bind<FunctionPtr(const Variable&, const std::wstring&)>(Softmax, std::placeholders::_1, L"");
	case ActivationFunction::softplus:
		return CNTK::Softplus(pInput,L"Softplus");
	case ActivationFunction::softsign:
		throw "not supported at the moment";
	case ActivationFunction::tanh:
		return CNTK::Tanh(pInput,L"Tanh");
	case ActivationFunction::linear:
		return pInput;
	}
	return FunctionPtr(nullptr);
}

std::wstring CNTKWrapper::Internal::string2wstring(std::string value)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(value);
	return wide;
}

string CNTKWrapper::Internal::wstring2string(const std::wstring& value)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::string wide = converter.to_bytes(value);

	return wide;
}

CNTK::FunctionPtr CNTKWrapper::Internal::FullyConnectedLinearLayer(CNTK::Variable input, size_t outputDim, const DeviceDescriptor& device, const wstring& outputName)
{
	assert(input.Shape().Rank() == 1);
	size_t inputDim = input.Shape()[0];

	auto timesParam = Parameter({ outputDim, inputDim }
								, DataType::Double, GlorotUniformInitializer(DefaultParamInitScale
								, SentinelValueForInferParamInitRank, SentinelValueForInferParamInitRank, 1)
								, device, L"timesParam");
	auto timesFunction = Times(timesParam, input, L"times");

	auto plusParam = Parameter({ outputDim }, 0.0, device, L"plusParam");
	return Plus(plusParam, timesFunction, outputName);
}

FunctionPtr CNTKWrapper::Internal::Convolution1D(Variable input, size_t kernelCount, size_t kernel, size_t stride, ActivationFunction activationFunction, double wScale
	, const DeviceDescriptor& device, const wstring& outputName)
{
	assert(input.Shape().Rank() == 2);
	size_t numInputChannels = input.Shape()[input.Shape().Rank() - 1];

	auto convParams = Parameter({ kernel, numInputChannels, kernelCount }, DataType::Double
		, GlorotUniformInitializer(wScale, -1, 2), device,L"ConvParams");
	return applyActivationFunction(
		Convolution(convParams, input, { stride, numInputChannels }, { true }, { true }, {1}, 1, 1, 0, outputName),
		activationFunction
	);
}

FunctionPtr CNTKWrapper::Internal::Convolution2D(Variable input, size_t kernelCount, size_t kernelWidth, size_t kernelHeight, size_t strideWidth, size_t strideHeight, ActivationFunction activationFunction
	, double wScale, const DeviceDescriptor& device, const wstring& outputName)
{
	assert(input.Shape().Rank() == 3);
	size_t numInputChannels = input.Shape()[input.Shape().Rank() - 1];

	auto convParams = Parameter({ kernelWidth, kernelHeight, numInputChannels, kernelCount }, DataType::Double, GlorotUniformInitializer(wScale, -1, 2), device);
	return applyActivationFunction(
		Convolution(convParams, input, { strideWidth, strideHeight, numInputChannels }, { true }, { true }, { 1 }, 1, 1, 0, outputName),
		activationFunction
	);
}

FunctionPtr CNTKWrapper::Internal::Convolution3D(Variable input, size_t kernelCount, size_t kernelWidth, size_t kernelHeight, size_t kernelDepth, size_t strideWidth, size_t strideHeight, size_t strideDepth, ActivationFunction activationFunction
	, double wScale, const DeviceDescriptor& device, const wstring& outputName)
{
	assert(input.Shape().Rank() == 4);
	size_t numInputChannels = input.Shape()[input.Shape().Rank() - 1];

	auto convParams = Parameter({ kernelWidth, kernelHeight, kernelDepth, numInputChannels, kernelCount }, DataType::Double, GlorotUniformInitializer(wScale, -1, 2), device);
	return applyActivationFunction(
		Convolution(convParams, input, { strideWidth, strideHeight, strideDepth, numInputChannels }, { true }, { true }, { 1 }, 1, 1, 0, outputName),
		activationFunction
	);
}

FunctionPtr CNTKWrapper::Internal::FullyConnectedDNNLayer(Variable input, size_t outputDim, const DeviceDescriptor& device
	, const std::function<FunctionPtr(const FunctionPtr&)>& nonLinearity, const wstring& outputName)
{
	return nonLinearity(FullyConnectedLinearLayer(input, outputDim, device, outputName));
}

FunctionPtr CNTKWrapper::Internal::FullyConnectedDNNLayer(Variable input, size_t outputDim, const DeviceDescriptor& device, ActivationFunction activationFunction
	, const wstring& outputName)
{
	return applyActivationFunction(FullyConnectedLinearLayer(input, outputDim, device, outputName)
		, activationFunction);
}
