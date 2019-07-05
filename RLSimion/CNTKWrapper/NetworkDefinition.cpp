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

#include "NetworkDefinition.h"
#include "xmltags.h"
#include "InputData.h"
#include "NetworkArchitecture.h"
#include "Link.h"
#include "Chain.h"
#include "ParameterValues.h"
#include "Network.h"
#include "OptimizerSetting.h"
#include "Exceptions.h"
#include "Minibatch.h"
#include "CNTKWrapperInternals.h"

#include "../Common/named-var-set.h"

#include <stdexcept>

NetworkDefinition::NetworkDefinition(tinyxml2::XMLElement * pParentNode)
{
	//load architecture
	tinyxml2::XMLElement *pNode = pParentNode->FirstChildElement(XML_TAG_NETWORK_ARCHITECTURE);
	if (pNode == nullptr)
		throw ProblemParserElementNotFound(XML_TAG_NETWORK_ARCHITECTURE);

	m_pNetworkArchitecture = NetworkArchitecture::getInstance(pNode);
	m_pNetworkArchitecture->setParentProblem(this);

	//load output
	pNode = pParentNode->FirstChildElement(XML_TAG_Output);
	if (pNode == nullptr)
		throw ProblemParserElementNotFound(XML_TAG_Output);
	pNode = pNode->FirstChildElement(XML_TAG_LinkConnection);
	if (pNode == nullptr)
		throw ProblemParserElementNotFound(XML_TAG_LinkConnection);
	m_pOutput = LinkConnection::getInstance(pNode);

	//load optimizer
	pNode = pParentNode->FirstChildElement(XML_TAG_OptimizerSetting)->FirstChildElement(XML_TAG_Optimizer);
	if (pNode == nullptr)
		throw ProblemParserElementNotFound(XML_TAG_Optimizer);
	m_pOptimizerSetting = OptimizerSettings::getInstance(pNode);
}

NetworkDefinition::NetworkDefinition()
{
}

NetworkDefinition::~NetworkDefinition()
{
}

void NetworkDefinition::destroy()
{
	delete this;
}

NetworkDefinition * NetworkDefinition::getInstance(tinyxml2::XMLElement * pNode)
{
	if (!strcmp(pNode->Name(), XML_TAG_Problem))
		return new NetworkDefinition(pNode);

	return nullptr;
}

NetworkDefinition * NetworkDefinition::loadFromFile(std::string fileName)
{
	tinyxml2::XMLDocument doc;
	doc.LoadFile(fileName.c_str());

	if (doc.Error() == tinyxml2::XML_SUCCESS)
	{
		tinyxml2::XMLElement *pRoot = doc.RootElement();
		if (pRoot)
		{
			return NetworkDefinition::getInstance(pRoot);
		}
	}

	return nullptr;
}

NetworkDefinition * NetworkDefinition::loadFromString(std::string content)
{
	tinyxml2::XMLDocument doc;

	if (doc.Parse(content.c_str()) == tinyxml2::XML_SUCCESS)
	{
		tinyxml2::XMLElement *pRoot = doc.RootElement();
		if (pRoot)
		{
			return NetworkDefinition::getInstance(pRoot);
		}
	}

	return nullptr;
}

INetwork * NetworkDefinition::createNetwork(double learningRate, bool inputsNeedGradient)
{
	m_bInputsNeedGradient = inputsNeedGradient;
	Network* result = new Network(this);

	/*
	basic idea of this algorithm: we traverse all chains to create the real model
	to achieve this we follow this pseudo-code:

	while output_node has not been reached:
	for each chain  in chains
	traverse chain as far as possible

	stop the processing when no changes are possible anymore -> the structure is faulty!

	*/

	//output node
	CNTK::FunctionPtr pOutputFunction = nullptr;

	//stores the progress of each chain
	std::map<Chain*, Link*> progressMap;

	//the currently processed link
	Link* pCurrentLink;

	//the dependencies of the current link
	vector<const Link*> dependencies;

	//termination condition
	bool architectureDeadEndReached = false;

	//traverse the architecture until either the output has been created
	//or no more changes are applied
	while (pOutputFunction == nullptr && !architectureDeadEndReached)
	{
		architectureDeadEndReached = true;

		for (Chain* pChain : m_pNetworkArchitecture->getChains())
		{
			//get current link of the chain
			if (progressMap.find(pChain) == progressMap.end())
			{
				//start at the beginning
				pCurrentLink = pChain->getLinks().at(0);
			}
			else
			{
				//resume the progress
				pCurrentLink = progressMap.at(pChain);
			}

			//go down in this chain until we can't go any further
			while (pCurrentLink != nullptr)
			{
				//get dependencies
				dependencies = pCurrentLink->getDependencies();

				//check if dependencies have already been traversed
				bool dependencies_satisfied = true;
				for (const Link* item : dependencies)
				{
					if (item->getFunctionPtr() == nullptr)
					{
						dependencies_satisfied = false;
						break;
					}
				}

				//dependencies not satisfied -> try to process another chain
				if (!dependencies_satisfied)
					break;

				//all dependencies are satisfied and already traversed
				//create the FunctionPtr now and save it!

				//create CNTK node
				pCurrentLink->createCNTKFunctionPtr(dependencies);

				//check if output has been reached already
				if (m_pOutput->getTargetId() == pCurrentLink->getId())
				{
					pOutputFunction = pCurrentLink->getFunctionPtr()->Output();
					result->setOutputLayer(pOutputFunction);
					setOutputLayer(pOutputFunction->Name());
					break;
				}

				//update current link
				pCurrentLink = pCurrentLink->getNextLink();

				//keep the loop alive
				architectureDeadEndReached = false;
			}

			//the output node has already been created
			//the network is created
			//all other links are not used to calculate the output
			if (pOutputFunction != nullptr)
				break;

			//store last processed item of the current chain
			progressMap[pChain] = pCurrentLink;
		}
	}

	result->buildNetwork(learningRate);
	return result;
}


void NetworkDefinition::addInputStateVar(string name)
{
	m_inputStateVariables.push_back(name);
}

const vector<string>& NetworkDefinition::getInputStateVariables()
{
	return m_inputStateVariables;
}

void NetworkDefinition::addInputActionVar(string name)
{
	m_inputActionVariables.push_back(name);
}

const vector<string>& NetworkDefinition::getInputActionVariables()
{
	return m_inputActionVariables;
}

void NetworkDefinition::inputStateVariablesToVector(const State* s, vector<double>& outState, size_t offset)
{
	const vector<string>& stateVars = getInputStateVariables();
	size_t stateInputSize = stateVars.size();
	for (size_t i = 0; i < stateInputSize; i++)
		outState[offset*stateInputSize + i] = s->getNormalized(stateVars[i].c_str());
}

void NetworkDefinition::inputActionVariablesToVector(const Action* a, vector<double>& outAction, size_t offset)
{
	const vector<string>& actionVars = getInputActionVariables();
	size_t actionInputSize = actionVars.size();
	for (size_t i = 0; i < actionInputSize; i++)
		outAction[offset*actionInputSize + i] = a->getNormalized(actionVars[i].c_str());
}

size_t NetworkDefinition::getInputSize()
{
	return m_inputStateVariables.size() + m_inputActionVariables.size();
}


void NetworkDefinition::setDiscretizedActionVectorOutput(size_t numSteps)
{
	m_outputType = DiscretizedActionVector;
	m_outputSize = numSteps;
}


size_t NetworkDefinition::getOutputSize()
{
	return m_outputSize;
}

void NetworkDefinition::setScalarOutput()
{
	m_outputType = Scalar;
	m_outputSize = 1;
}

void NetworkDefinition::setVectorOutput(size_t dimension)
{
	m_outputType = Vector;
	m_outputSize = dimension;
}

IMinibatch* NetworkDefinition::createMinibatch(size_t sampleSize, size_t outputSize, size_t numActionVariables)
{
	return new Minibatch(sampleSize, this, outputSize, numActionVariables);
}

void NetworkDefinition::setStateInputLayer(wstring name)
{
	m_stateInputLayer= name;
}

void NetworkDefinition::setActionInputLayer(wstring name)
{
	m_actionInputLayer = name;
}

wstring NetworkDefinition::getStateInputLayer()
{
	return m_stateInputLayer;
}

wstring NetworkDefinition::getActionInputLayer()
{
	return m_actionInputLayer;
}


void NetworkDefinition::setOutputLayer(wstring name)
{
	m_outputLayerName = name;
}
wstring NetworkDefinition::getOutputLayer()
{
	return m_outputLayerName;
}

string NetworkDefinition::getDeviceName()
{
	return CNTKWrapper::Internal::wstring2string(CNTK::DeviceDescriptor::UseDefaultDevice().AsString());
}