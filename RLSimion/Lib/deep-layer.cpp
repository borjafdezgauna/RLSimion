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
#include "deep-layer.h"
#include "parameters.h"
#include "deep-functions.h"

DeepLayer::DeepLayer(ConfigNode* pConfigNode)
{
	m_activationFunction = ENUM_PARAM<Activation>(pConfigNode, "Activation", "Activation function used by the layer", Activation::Sigmoid).get();
	m_numUnits = INT_PARAM(pConfigNode, "Num-Units", "Number of neural units in the layer", 100).get();
}

int DeepLayer::getNumUnits()
{
	return m_numUnits;
}

Activation DeepLayer::getActivationFunction()
{
	return m_activationFunction;
}

string DeepLayer::to_string()
{
	return activationFunctionName(m_activationFunction) + DeepNetworkDefinition::layerParameterDelimiter + std::to_string(m_numUnits) ;
}