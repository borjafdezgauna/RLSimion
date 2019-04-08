#pragma once

#include <vector>
using namespace std;

#include "parameters.h"
#include "../../tools/System/NamedPipe.h"
#include "stats.h"

class NamedVarSet;
typedef NamedVarSet State;
typedef NamedVarSet Action;
typedef NamedVarSet Reward;
class ConfigNode;
class Descriptor;
class Timer;
class FunctionSampler;

enum MessageType {Progress,Evaluation,Info,Warning, Error};
enum MessageOutputMode {Console,NamedPipe};

class Logger
{
	static const int MAX_FILENAME_LENGTH = 1024;
	static const int BUFFER_SIZE = 10000;

	//Functions log file: drawable downsampled 2d or 1d versions of the functions learned by the agents
	string m_outputFunctionLogBinary;
	FILE *m_functionLogFile = nullptr;

	BOOL_PARAM m_bLogFunctions;
	INT_PARAM m_numFunctionLogPoints;

	void openFunctionLogFile(const char* filename);
	void closeFunctionLogFile();

	void writeFunctionLogSample();

	//Log file
	string m_outputLogDescriptor;
	string m_outputLogBinary;
	static FILE *m_logFile;

	BOOL_PARAM m_bLogEvaluationEpisodes;
	BOOL_PARAM m_bLogTrainingEpisodes;
	DOUBLE_PARAM m_logFreq; //in seconds: time between file logs

	Timer *m_pEpisodeTimer = nullptr;
	Timer *m_pExperimentTimer = nullptr;

	double m_episodeRewardSum;
	double m_lastLogSimulationT;

	void openLogFile(const char* fullLogFilename);
	void closeLogFile();

private:
	static void writeLogBuffer(const char* pBuffer, int numBytes);
	void writeLogFileXMLDescriptor(const char* filename);

	void writeNamedVarSetDescriptorToBuffer(char* buffer, const char* id, const Descriptor* pNamedVarSet);
	void writeStatDescriptorToBuffer(char* buffer);

	void writeExperimentHeader();
	void writeEpisodeHeader();
	void writeEpisodeEndHeader();
	int writeStepHeaderToBuffer(char* buffer, int offset);
	int writeAvgLogData(char* buffer, int offset);
	int writeStatsToBuffer(char* buffer, int offset);

	void writeLogData();
	void addLogDataSample(State* s, Action* a, Reward* r);
	void resetAvgLogData();

	//stats
	vector<IStats *> m_stats;
	//Variables used to log averaged data (state/action/reward)
	size_t m_numSamples = 0;
	size_t m_numLoggedVars = 0;
	vector<double> m_avgLogData;
public:
	static const unsigned int BIN_FILE_VERSION = 2;

	Logger(ConfigNode* pParameters);
	Logger() = default;
	virtual ~Logger();

	//returns whether a specific type of episode is going to be logged
	bool isEpisodeTypeLogged(bool evaluation);

	//returns whether we are logging functions
	bool areFunctionsLogged() { return m_bLogFunctions.get(); }

	//METHODS CALLED FROM ANY CLASS
	template <typename T>
	void addVarToStats(string key, string subkey, T& variable)
	{
		m_stats.push_back(new Stats<T>(key, subkey, variable));
	}
	void addVarSetToStats(const char* key, NamedVarSet* varset);

	size_t getNumStats();
	IStats* getStats(unsigned int i);

	void setOutputFilenames();

	static MessageOutputMode m_messageOutputMode;
	static NamedPipeClient m_outputPipe;
	static bool m_bLogMessagesEnabled;

	//Function called to report progress and error messages
	//static so that it can be called right from the beginning
	static void logMessage(MessageType type, const char* message);
	//Function called to enable/disable output messages. Used when RLSimion outputs its requirements
	static void enableLogMessages(bool enable);

protected:
	friend class Experiment;
	//METHODS CALLED FROM Experiment
	//called to log episodes
	void firstEpisode();
	void lastEpisode();
	//called to log steps
	void firstStep();
	void lastStep();
	void timestep(State* s, Action* a, State* s_p,Reward* r);
};