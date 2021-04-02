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

#include "setpoint.h"
#include "../config.h"
#include "../logger.h"
#include "../app.h"
#include "../../../tools/System/CrossPlatform.h"
#include <stdexcept>

//FileSetPoint//////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

int countlines(const char *filename)
{
  // count the number of lines in the file called filename                                    
  FILE *fp;
  
  CrossPlatform::Fopen_s(&fp,filename,"r");

  if (!fp)
	  throw std::runtime_error((std::string("Couldn't open setpoint file: ") + std::string(filename)).c_str());

  int ch=0;
  int lines=1;

  while (!feof(fp))
    {
	ch= getc(fp);
      if (ch == '\n')
    lines++;
    }
  fclose(fp);
  return lines;
}

FileSetPoint::FileSetPoint()
{
	m_numSteps= 0;
	m_pSetPoints= 0;
	m_pTimes= 0;
	m_totalTime= 0.0;
}

FileSetPoint::FileSetPoint(const char* filename)
{
	SimionApp::get()->registerInputFile(filename);
	//char fullFilename[1024];
	m_numSteps= 0;

	FILE *pFile;

	int numLines = countlines(filename);
	if (numLines==0) return;

	char buffer [1024];
	CrossPlatform::Fopen_s(&pFile,filename,"r");
	
	if (pFile!=0)
	{
		m_pSetPoints= new double [numLines];
		m_pTimes= new double [numLines];

		while (!feof(pFile))
		{
			fgets(buffer,1024,pFile);
			if (CrossPlatform::Sscanf_s(buffer,"%lf %lf\n",&m_pTimes[m_numSteps],&m_pSetPoints[m_numSteps])==2)
				m_numSteps++;
		}
		fclose(pFile);
	}
	else
	{
		throw std::runtime_error((std::string("Couldn't open setpoint file: ") + std::string(filename)).c_str());
	}

	m_totalTime= m_pTimes[m_numSteps-1];
}


FileSetPoint::~FileSetPoint()
{
	if (m_pSetPoints)
	{
		delete[] m_pSetPoints;
		m_pSetPoints = 0;
	}
	if (m_pTimes)
	{
		delete[] m_pTimes;
		m_pTimes = 0;
	}
}

double FileSetPoint::getPointSet(double time)
{
	int i= 0;

	if (m_totalTime==0) return 0.0;

	while (time>m_totalTime)
		time-= m_totalTime;

	//first approach: assume equal-lenght time points
	i= (int) (time/(m_totalTime/m_numSteps));

	//forward?
	while (i< m_numSteps-1 && time>m_pTimes[i+1])
		i++;
	//backward?
	while (i>=0 && time<m_pTimes[i])
		i--;

	if (i<m_numSteps-1)
	{
		double u;
		u= ((time-m_pTimes[i])/(m_pTimes[i+1]-m_pTimes[i]));
		return m_pSetPoints[i] + u* (m_pSetPoints[i+1]-m_pSetPoints[i]);
	}

	return m_pSetPoints[m_numSteps-1];//step % m_numSteps];
}

//HHFileSetPoint//////////////////////////////////////////////
///////////////////////////////////////////////////////////////

HHFileSetPoint::HHFileSetPoint(const char* filename) : FileSetPoint()
{
	FILE* pHHFile;
	char buffer[1024];
	char* pNext;

	SimionApp::get()->registerInputFile(filename);

	int numLines = countlines(filename);
	if (numLines == 0) return;

	CrossPlatform::Fopen_s(&pHHFile, filename, "r");
	if (pHHFile)
	{
		m_pSetPoints = new double[numLines];
		m_pTimes = new double[numLines];

		while (!feof(pHHFile))
		{
			fgets(buffer, 1024, pHHFile);
			if (buffer[0] != '!') //skip comments
			{
				m_pTimes[m_numSteps] = strtod(buffer, &pNext);		//first value is the time
				m_pSetPoints[m_numSteps] = strtod(pNext, 0);		//second value is the horizontal wind speed

				m_numSteps++;
			}
		}
		m_totalTime = m_pTimes[m_numSteps - 1];
		fclose(pHHFile);
	}
	else
	{
		throw std::runtime_error((std::string("Couldn't open setpoint file: ") + std::string(filename)).c_str());
	}
}


//FixedStepSizeSetPoint////////////////////////////////////////
///////////////////////////////////////////////////////////////

FixedStepSizeSetPoint::FixedStepSizeSetPoint(double stepTime, double min, double max)
{
	m_stepTime= stepTime;
	m_min= min;
	m_max= max;
	m_lastStepTime= 0.0;
	m_lastSetPoint= min;
}

FixedStepSizeSetPoint::~FixedStepSizeSetPoint(){}

double FixedStepSizeSetPoint::getPointSet(double time)
{
	if (time==0.0 || (time-m_lastStepTime>m_stepTime))
	{
		m_lastSetPoint= ((double)(rand()%10000))*(m_max-m_min) + m_min;
		m_lastStepTime= time;
	}
	return m_lastSetPoint;

}