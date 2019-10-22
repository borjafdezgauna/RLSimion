#pragma once
#include "parameters.h"
class NamedVarSet;
typedef NamedVarSet State;
typedef NamedVarSet Action;
typedef NamedVarSet Reward;
class ConfigNode;
class Timer;

#define MAX_PROGRESS_MSG_LEN 1024

//utility class to getSample a experiment time reference
//for example, we can use this reference to know if some feature vector has already been calculated in this time-step
class ExperimentTime
{
public:
	unsigned int m_episodeIndex; //[1..g_numEpisodes]
	unsigned int m_step; //]1..g_numStepsPerEpisode]

	ExperimentTime(unsigned int episodeIndex, unsigned int step){ m_episodeIndex = episodeIndex; m_step = step; }

	ExperimentTime& operator=(ExperimentTime& exp);
	bool operator==(ExperimentTime& exp);
};



class Experiment
{
	unsigned int m_episodeIndex; //[1..g_numEpisodes]
	unsigned int m_step; //]1..g_numStepsPerEpisode]
	unsigned int m_trainingEpisodeIndex; //[1..m_numTrainingEpisodes]
	unsigned int m_evalEpisodeIndex; //[1..1+m_numTrainingEpisodes/ evalFreq]

	INT_PARAM m_randomSeed;

	//episode stuff
	DOUBLE_PARAM m_episodeLength;			//length in seconds of an episode
	unsigned int m_totalNumEpisodes= 0;		//total number of episodes (training + evaluation)
	INT_PARAM m_numTrainingEpisodes;		//total number of training episodes
	INT_PARAM m_numOfflineTrainingEpisodes; //how many of the training episodes should be offline? m_numOfflineTrainingEpisodes < m_numTrainingEpisodes
	unsigned int m_numEvaluations= 0;	//total number of evaluation episodes
	unsigned int m_numEpisodesPerEvaluation= 1;//number of episodes in each evaluation
	INT_PARAM m_evalFreq;					//frequeny (in episodes) at which an evaluation episode will be done
	//steps
	unsigned int m_numSteps= 0;
	unsigned int m_experimentStep= 0;
	bool m_bTerminalState;

	unsigned int m_numUpdates = 0;

	DOUBLE_PARAM m_progUpdateFreq; //in seconds: time between progress updates
	Timer* m_pProgressTimer;

	char m_progressMsg[MAX_PROGRESS_MSG_LEN];

public:
	Experiment(ConfigNode* pParameters);
	Experiment() = default;
	virtual ~Experiment();

	void getExperimentTime(ExperimentTime& ref) { ref.m_episodeIndex = m_episodeIndex; ref.m_step = m_step; }
	void reset();

	//STEP
	bool isValidStep();
	unsigned int getStep(){ return m_step; }
	bool isFirstStep(){ return m_step == 1; }
	bool isLastStep(){ return (m_bTerminalState || m_step == m_numSteps); }
	void setTerminalState(){ m_bTerminalState = true; }
	void nextStep();

	//EPISODES
	
	//Returns the expected episode length in seconds (simulation time)
	double getEpisodeLength(){ return m_episodeLength.get(); }//seconds
	void setEpisodeLength(double length);
	unsigned int getEpisodeIndex(){ return m_episodeIndex; }
	unsigned int getRelativeEpisodeIndex();
	unsigned int getTrainingEpisodeIndex(){ return m_trainingEpisodeIndex; }
	unsigned int getEvaluationIndex();
	unsigned int getEpisodeInEvaluationIndex();
	void nextEpisode();
	//true if is the first evaluation episode or the first training episode
	bool isFirstEpisode();
	//true if is the last evaluation episode or the last training episode
	bool isLastEpisode();
	bool isValidEpisode();
	bool isEvaluationEpisode();
	bool isOnlineTrainingEpisode();
	bool isOfflineTrainingEpisode();

	unsigned int getNumEvaluations(){ return m_numEvaluations; }
	void setEvaluationFreq(int evalFreq);
	void setNumEpisodesPerEvaluation(int numEpisodes);
	unsigned int getNumEpisodesPerEvaluation() { return m_numEpisodesPerEvaluation; }
	unsigned int getNumTrainingEpisodes(){ return m_numTrainingEpisodes.get(); }
	void setNumTrainingEpisodes(int numEpisodes);
	unsigned int getTotalNumEpisodes(){ return m_totalNumEpisodes; }
	unsigned int getNumSteps(){ return m_numSteps; }
	void setNumSteps(int numSteps);
	unsigned int getExperimentStep() { return m_experimentStep; } //returns the current step since the experiment began
	
	void incNumUpdateSteps();
	unsigned int getNumUpdateSteps();

	//Progress is normalized
	double getExperimentProgress(); //[0,1]
	double getTrainingProgress(); //[0,1]
	double getEpisodeProgress(); //[0,1]

	const char* getProgressString();

	void timestep(State *s, Action *a,State *s_p, Reward* pReward);
};