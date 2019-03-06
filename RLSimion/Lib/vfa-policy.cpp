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

#include "../Common/named-var-set.h"
#include "noise.h"
#include "vfa.h"
#include "policy-learner.h"
#include "config.h"
#include "worlds/world.h"
#include "features.h"
#include "policy.h"
#include "app.h"
#include "simgod.h"
#include "featuremap.h"
#include <iostream>
#include <typeinfo>
#include "../Common/named-var-set.h"
#include "logger.h"
#include "experiment.h"

#include <math.h>
#include <algorithm>

#define PROBABILITY_INTEGRATION_WIDTH 0.05
//0.05

std::shared_ptr<Policy> Policy::getInstance(ConfigNode* pConfigNode)
{
	return CHOICE<Policy>(pConfigNode, "Policy", "The policy type",
	{
		{"Deterministic-Policy-Gaussian-Noise",CHOICE_ELEMENT_NEW<DeterministicPolicyGaussianNoise>},
		{"Stochastic-Policy-Gaussian-Noise",CHOICE_ELEMENT_NEW<StochasticGaussianPolicy>}
	});
}

Policy::Policy(ConfigNode* pConfigNode)
{
	m_outputAction = ACTION_VARIABLE(pConfigNode, "Output-Action", "The output action variable");
}

Policy::~Policy()
{
}

DeterministicPolicy::DeterministicPolicy(ConfigNode* pConfigNode) : Policy(pConfigNode) {}

StochasticPolicy::StochasticPolicy(ConfigNode* pConfigNode) : Policy(pConfigNode) {}



//CDetPolicyGaussianNoise////////////////////////////////

DeterministicPolicyGaussianNoise::DeterministicPolicyGaussianNoise(ConfigNode* pConfigNode)
	: DeterministicPolicy(pConfigNode)
{
	m_pDeterministicVFA = CHILD_OBJECT<LinearStateVFA>(pConfigNode, "Deterministic-Policy-VFA"
		, "The parameterized VFA that approximates the function");

	SimionApp::get()->registerStateActionFunction(string("Policy"), m_pDeterministicVFA.ptr());

	m_pExpNoise = CHILD_OBJECT_FACTORY<Noise>(pConfigNode, "Exploration-Noise"
		, "Parameters of the noise used as exploration");

	NamedVarProperties* pProperties = World::getDynamicModel()->getActionDescriptor().getProperties(m_outputAction.get());
	m_pDeterministicVFA->saturateOutput(pProperties->getMin(), pProperties->getMax());

	m_lastNoise = 0.0;
	SimionApp::get()->pLogger->addVarToStats<double>("Policy", "Noise", m_lastNoise);
}

DeterministicPolicyGaussianNoise::~DeterministicPolicyGaussianNoise()
{
}

void DeterministicPolicyGaussianNoise::getParameterGradient(const State* s, const Action* a, FeatureList* pOutGradient)
{
	//0. Grad_u pi(a|s)/pi(a|s) = (a - pi(s)) * phi(s) / sigma*2
	m_pDeterministicVFA->getFeatures(s, pOutGradient);

	double sigma = std::max(0.0000001, m_pExpNoise->getVariance());

	double noise = a->get(m_outputAction.get())
		- m_pDeterministicVFA->get((const FeatureList*)pOutGradient);

	double unscaled_noise = m_pExpNoise->unscale(noise);
	double factor = unscaled_noise / (sigma*sigma);

	pOutGradient->mult(factor);
}


/// <summary>
/// A DeterministicPolicyGaussianNoise policy uses a single function representing the deterministic output. In training episodes, the
/// noise signal used for exploration is sampled and added to the deterministic output
/// </summary>
/// <param name="s">Initial state</param>
/// <param name="a">Output state</param>
double DeterministicPolicyGaussianNoise::selectAction(const State *s, Action *a)
{
	if (!SimionApp::get()->pExperiment->isEvaluationEpisode())
		m_lastNoise = m_pExpNoise->getSample();
	else m_lastNoise = 0.0;

	double output = m_pDeterministicVFA->get(s);

	a->set(m_outputAction.get(), output + m_lastNoise);

	if (!SimionApp::get()->pExperiment->isEvaluationEpisode())
		return m_pExpNoise->getSampleProbability(m_lastNoise);
	return 1.0;
}

double DeterministicPolicyGaussianNoise::getProbability(const State* s, const Action* a, bool bStochastic)
{
	double noise;
	if (SimionApp::get()->pSimGod->useSampleImportanceWeights())
	{
		noise = a->get(m_outputAction.get()) - m_pDeterministicVFA->get(s);
		return m_pExpNoise->getSampleProbability(noise, !bStochastic);
	}
	return 1.0;
}


/// <summary>
/// Uses the policy's feature map to return the features representing the state 
/// </summary>
/// <param name="state">State</param>
/// <param name="outFeatureList">Output feature list</param>
void DeterministicPolicyGaussianNoise::getFeatures(const State* state, FeatureList* outFeatureList)
{
	m_pDeterministicVFA->getFeatures(state, outFeatureList);
}

void DeterministicPolicyGaussianNoise::addFeatures(const FeatureList* pFeatureList, double factor)
{
	m_pDeterministicVFA->add(pFeatureList, factor);
}

double DeterministicPolicyGaussianNoise::getDeterministicOutput(const FeatureList* pFeatureList)
{
	return m_pDeterministicVFA->get(pFeatureList);
}


//CStoPolicyGaussianNoise//////////////////////////

StochasticGaussianPolicy::StochasticGaussianPolicy(ConfigNode* pConfigNode)
	: StochasticPolicy(pConfigNode)
{
	m_pMeanVFA = CHILD_OBJECT<LinearStateVFA>(pConfigNode, "Mean-VFA", "The parameterized VFA that approximates the function");
	SimionApp::get()->registerStateActionFunction(string("Policy"), m_pMeanVFA.ptr());

	NamedVarProperties* pProperties= World::getDynamicModel()->getActionDescriptor().getProperties(m_outputAction.get());
	m_pMeanVFA->saturateOutput(pProperties->getMin(), pProperties->getMax());

	m_pSigmaVFA = CHILD_OBJECT<LinearStateVFA>(pConfigNode, "Sigma-VFA", "The parameterized VFA that approximates variance(s)");
	SimionApp::get()->registerStateActionFunction(string("Policy"), m_pSigmaVFA.ptr());
	m_pSigmaVFA->saturateOutput(0.0, 1.0);
	m_pSigmaVFA->setIndexOffset((unsigned int) m_pMeanVFA->getNumWeights());
	m_pMeanFeatures = new FeatureList("Sto-Policy/mean-features");
	m_pSigmaFeatures = new FeatureList("Sto-Policy/sigma-features");

	m_lastNoise = 0.0;
	SimionApp::get()->pLogger->addVarToStats<double>("Stoch.Policy", "Noise", m_lastNoise);
}

StochasticGaussianPolicy::~StochasticGaussianPolicy()
{
	delete m_pMeanFeatures;
	delete m_pSigmaFeatures;
}

double clip(double n, double lower, double upper)
{
	return std::max(lower, std::min(n, upper));
}

/// <summary>
/// A DeterministicPolicyGaussianNoise policy uses a function to represent the mean value of the function at each state
/// and a second function to represent the variance of the output at each state. To calculate the policy's output,
/// the two functions are evaluated for a state, and then the values used to sample a normal distribution, which is
/// the actual output of the policy
/// </summary>
/// <param name="s">Initial state</param>
/// <param name="a">Output state</param>
double StochasticGaussianPolicy::selectAction(const State *s, Action *a)
{
	double mean = m_pMeanVFA->get(s);
	double sigma = exp(m_pSigmaVFA->get(s));
	double output = mean;
	double probability;

	if (!SimionApp::get()->pExperiment->isEvaluationEpisode())
	{
		//Training: add noise
		sigma = exp(m_pSigmaVFA->get(s));
		output = GaussianNoise::getNormalDistributionSample(mean, sigma);
	}

	output = clip(output, a->getProperties(m_outputAction.get())->getMin(), a->getProperties(m_outputAction.get())->getMax());

	probability = GaussianNoise::getSampleProbability(mean, sigma, output);

	m_lastNoise = output - mean;

	a->set(m_outputAction.get(), output);


	//this is only an approximation as the PDF now looks differently because of the clipping
	//this means, that the values at the borders will be higher as predicted by this function call
	//maybe this is causing problems, but I do not see a way to solve this
	return probability;
}

double StochasticGaussianPolicy::getProbability(const State *s, const State *a, bool bStochastic)
{
	double mean = m_pMeanVFA->get(s);

	if (bStochastic && SimionApp::get()->pSimGod->useSampleImportanceWeights())
	{
		double value = a->get(m_outputAction.get());

		return GaussianNoise::getSampleProbability(mean, exp(m_pSigmaVFA->get(s)), value);
	}
	return 1.0;
}

//according to https://hal.inria.fr/hal-00764281/document
void StochasticGaussianPolicy::getParameterGradient(const State* s, const Action* a, FeatureList* pOutGradient)
{
	m_pMeanFeatures->clear();
	m_pSigmaFeatures->clear();

	m_pMeanVFA->getFeatures(s, m_pMeanFeatures);
	m_pSigmaVFA->getFeatures(s, m_pSigmaFeatures);

	//mu(s) = u_mu * x_mu(s)
	//sigma(s) = exp(u_sigma * x_sigma(s))
	double mean = m_pMeanVFA->get(m_pMeanFeatures);
	double sigma = exp(m_pSigmaVFA->get(m_pSigmaFeatures));

	//a. Grad_u_mu pi(a|s)/pi(a|s) = (a - mu(s)) / sigma(s)^2 * x_mu(s)
	double noise = a->get(m_outputAction.get()) - mean;

	double factor = noise / (sigma*sigma);
	pOutGradient->addFeatureList(m_pMeanFeatures, factor);

	//b. Grad_u_sigma pi(a|s)/pi(a|s) = ((a - mu(s))^2 / (sigma(s)^2) - 1) * x_sigma(s)
	factor = (noise*noise) / (sigma*sigma) - 1.0;
	pOutGradient->addFeatureList(m_pSigmaFeatures, factor);
}

/// <summary>
/// Uses the policy's feature map to return the features representing the state 
/// </summary>
/// <param name="state">State</param>
/// <param name="outFeatureList">Output feature list</param>
void StochasticGaussianPolicy::getFeatures(const State* state, FeatureList* outFeatureList)
{
	m_pMeanVFA->getFeatures(state, outFeatureList);
	m_pSigmaVFA->getFeatures(state, m_pSigmaFeatures);
	outFeatureList->addFeatureList(m_pSigmaFeatures);
}
void StochasticGaussianPolicy::addFeatures(const FeatureList* pFeatureList, double factor)
{
	m_pMeanVFA->add(pFeatureList);
	m_pSigmaVFA->add(pFeatureList);
}
double StochasticGaussianPolicy::getDeterministicOutput(const FeatureList* pFeatureList)
{
	return m_pMeanVFA->get(pFeatureList);
}
