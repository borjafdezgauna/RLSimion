#pragma once
#if defined(__linux__) || defined(_WIN64)

#include <vector>
#include "simion.h"
#include "parameters.h"
#include "deferred-load.h"

class INetwork;
class DiscreteDeepPolicy;
class IMinibatch;
class ConfigNode;

class DQN : public Simion, DeferredLoad
{
	/*
	Deep Reinforcement Learning with Double Deep Q-Learning
	Hado van Hasselt and Arthur Guez and David Silver
	Proceedings of the Thirtieth AAAI Conference on Artificial Intelligence (AAAI-16)
	*/
protected:
	MULTI_VALUE_VARIABLE<STATE_VARIABLE> m_inputState;
	INT_PARAM m_numActionSteps;
	MULTI_VALUE_VARIABLE<ACTION_VARIABLE> m_outputAction;
	DOUBLE_PARAM m_learningRate;

	INT_PARAM m_minibatchSize;

	NN_DEFINITION m_pNNDefinition;
	INetwork* m_pTargetQNetwork= nullptr;
	INetwork* m_pOnlineQNetwork= nullptr;
	IMinibatch* m_pMinibatch = nullptr;

	int m_totalNumActionSteps = 0;
	vector<double> m_Q_s_p;
	vector<double> m_target;
	vector<double> m_maxQValue;
	vector<int> m_argMaxIndex;

	CHILD_OBJECT_FACTORY<DiscreteDeepPolicy> m_policy;

	virtual INetwork* getTargetNetwork();
	
public:
	~DQN();
	DQN(ConfigNode *pParameters);

	virtual void deferredLoadStep();

	//selects an according to the learned policy pi(a|s)
	virtual double selectAction(const State *s, Action *a);

	//updates the critic and the actor
	virtual double update(const State *s, const Action *a, const State *s_p, double r, double behaviorProb);
};

class DoubleDQN : public DQN
{
public:
	DoubleDQN(ConfigNode* pParameters);
	
	virtual INetwork* getTargetNetwork();
};

#endif