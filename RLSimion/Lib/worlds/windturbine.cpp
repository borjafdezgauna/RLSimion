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

#include "world.h"
#include "windturbine.h"
#include "setpoint.h"
#include "../../Common/named-var-set.h"
#include "../experiment.h"
#include "../config.h"
#include "../logger.h"
#include "../app.h"
#include "../reward.h"

#include <math.h>

#define NUM_BETA_SAMPLES 100
#define NUM_TSR_SAMPLES 100


double WindTurbine::C_p(double lambda, double beta) 
{
	//Using values from the table given in: https://wind.nrel.gov/forum/wind/viewtopic.php?t=582
	double betaInDegrees= beta*360.0/(2*3.14159265);

	return m_Cp.getInterpolatedValue(betaInDegrees, lambda);
}

double WindTurbine::C_q(double lambda, double beta)
{
	if (lambda==0.0)
		return 0.0;

	return C_p(lambda,beta)/lambda;
}

double WindTurbine::aerodynamicTorque(double tip_speed_ratio,double beta, double wind_speed)
{
	double cq= C_q(tip_speed_ratio,beta);

	//Ta= 0.5 * rho * pi * R^3 * C_q(lambda,beta) * v^2
	double torque= 0.5*getConstant("AirDensity")*3.14159265
		*pow(getConstant("RotorDiameter")*0.5,3.0)*cq*wind_speed*wind_speed;
	return torque;
}

double WindTurbine::aerodynamicPower(double tip_speed_ratio,double beta, double wind_speed)
{
	double cp= C_p(tip_speed_ratio,beta);

	//Pa= 0.5 * rho * pi * R^2 * C_p(lambda,beta) * v^3
	double power= 0.5*getConstant("AirDensity")*3.14159265
		*(getConstant("RotorDiameter")*0.5)
		*(getConstant("RotorDiameter")*0.5)*cp*pow(wind_speed,3.0);
	return power;
}

double WindTurbine::aerodynamicPower(double cp, double wind_speed)
{
	//Pa= 0.5 * rho * pi * R^2 * C_p(lambda,beta) * v^3
	double power= 0.5*getConstant("AirDensity")*3.14159265
		*(getConstant("RotorDiameter")*0.5)
		*(getConstant("RotorDiameter")*0.5)*cp*pow(wind_speed,3.0);
	return power;
}



void WindTurbine::findSuitableParameters(double initial_wind_speed,double& initial_rotor_speed
							,double &initial_blade_angle)
{
	double beta, tsr;
	double omega_r;
	initial_rotor_speed = 0.0;
	initial_blade_angle= 0.0;

	double betaRange = m_Cp.getMaxCol() - m_Cp.getMinCol();
	double tsrRange= m_Cp.getMaxRow()- m_Cp.getMinRow();

	for (int i = 0; i < NUM_TSR_SAMPLES; i++)
	{
		tsr = m_Cp.getMinRow() + (double)i * tsrRange/(double)NUM_TSR_SAMPLES;
		for (int j = 0; j < NUM_BETA_SAMPLES; j++)
		{
			beta = m_Cp.getMinCol() + (double)j * (betaRange / (double)NUM_BETA_SAMPLES);

			omega_r= tsr * initial_wind_speed/ (getConstant("RotorDiameter")*0.5) ;

			if (fabs(getConstant("RatedRotorSpeed") - omega_r) 
				< fabs(getConstant("RatedRotorSpeed") - initial_rotor_speed))
			{
				initial_blade_angle = beta;
				initial_rotor_speed = omega_r;
			}
		}
	}
}


WindTurbine::WindTurbine(ConfigNode* pConfigNode)
{
	METADATA("World", "Wind-turbine");
	//load all the wind data files
	m_currentDataFile = 0;

	//evaluation file
	FILE_PATH_PARAM evalFile= FILE_PATH_PARAM(pConfigNode, "Evaluation-Wind-Data"
		, "The wind file used for evaluation", "../config/world/wind-turbine/TurbSim-10.25.hh");
	m_pEvaluationWindData = new HHFileSetPoint(evalFile.get());

	//training files
	MULTI_VALUE_SIMPLE_PARAM<FILE_PATH_PARAM, const char*> trainingFiles 
		= MULTI_VALUE_SIMPLE_PARAM<FILE_PATH_PARAM,const char*>(pConfigNode
		, "Training-Wind-Data", "The wind files used for training","../config/world/wind-turbine/TurbSim-10.5.hh");
	m_numDataFiles = trainingFiles.size();
	m_pTrainingWindData = new SetPoint*[m_numDataFiles];
	for (unsigned int i= 0; i<trainingFiles.size(); i++)
		m_pTrainingWindData[i] = new HHFileSetPoint(trainingFiles[i]->get());

	FILE_PATH_PARAM powerSetpoint= FILE_PATH_PARAM(pConfigNode, "Power-Set-Point", "The power setpoint file", "../config/world/wind-turbine/power-setpoint.txt");
	m_pPowerSetpoint = new FileSetPoint(powerSetpoint.get());

	char cp_table_file[] = "../config/world/wind-turbine/cp-table.txt";
	SimionApp::get()->registerInputFile(cp_table_file);
	m_Cp.readFromFile(cp_table_file);

	//model constants
	addConstant("RatedPower", 5e6);				//W
	addConstant("HubHeight", 90);				//m
	addConstant("CutInWindSpeed", 3.0);			//m/s
	addConstant("RatedWindSpeed", 11.4);		//m/s
	addConstant("CutOutWindSpeed", 25.0);		//m/s
	addConstant("CutInRotorSpeed", 0.72256);	//6.9 rpm
	addConstant("CutOutRotorSpeed", 1.26711);	//12.1 rpm
	addConstant("RatedRotorSpeed", 1.26711);	//12.1 rpm
	addConstant("RatedTipSpeed", 8.377);		//80 rpm
	addConstant("RatedGeneratorSpeed", 122.91); //1173.7 rpm
	addConstant("RatedGeneratorTorque", 43093.55);
	addConstant("GearBoxRatio", 97.0);
	addConstant("ElectricalGeneratorEfficiency", 0.944); //%94.4
	addConstant("TotalTurbineInertia", 43784725); //J_t= J_r + n_g^2*J_g= 38759228 + 5025497 
	addConstant("GeneratorInertia", 534.116);			//kg*m^2
	addConstant("HubInertia", 115.926);				//kg*m^2
	addConstant("TotalTurbineTorsionalDamping", 3470794.95); //N*m/(rad/s)
	addConstant("RotorDiameter", 128.0); //m
	addConstant("AirDensity", 1.225);	//kg/m^3

	addStateVariable("T_a", "N/m", 0.0, 10000000.0);
	addStateVariable("P_a", "W", 0.0, 16000000.0);
	addStateVariable("P_s", "W", 0.0, 6e6);
	addStateVariable("P_e", "W", 0.0, 10e6);
	addStateVariable("E_p", "W", -10e6, 10e6);
	addStateVariable("v", "m/s", 1.0, 50.0);
	addStateVariable("omega_r", "rad/s", 0.0, 6.0);
	addStateVariable("d_omega_r", "rad/s^2", -10.0, 10.0);
	addStateVariable("E_omega_r", "rad/s", -4.0, 4.0);
	addStateVariable("omega_g", "rad/s", 0.0, 200.0);
	addStateVariable("d_omega_g", "rad/s^2", -50.0, 50.0);
	addStateVariable("E_omega_g", "rad/s", -122.0, 122.0);
	addStateVariable("beta", "rad", 0.0, 1.570796);
	addStateVariable("d_beta", "rad/s", -0.1396263, 0.1396263);
	addStateVariable("T_g", "N/m", 0.0, 47402.91);
	addStateVariable("d_T_g", "N/m/s", -15000, 15000);
	addStateVariable("E_int_omega_r", "rad/s", -1.0e6, 1.0e6);
	addStateVariable("E_int_omega_g", "rad/s", -1.0e6, 1.0e6);
	addStateVariable("theta", "rad", -3.1415, 3.1415, true); //roll angle of the blades in the rotor

	addActionVariable("beta", "rad", 0.0, 1.570796);
	addActionVariable("T_g", "N/m", 0.0, 47402.91);
	
	ToleranceRegionReward* pToleranceReward = new ToleranceRegionReward("E_p", 500000.0, 1.0);
	//pToleranceReward->setMin(-1000.0);
	m_pRewardFunction->addRewardComponent(pToleranceReward);
	m_pRewardFunction->initialize();
}

WindTurbine::~WindTurbine()
{
	for (size_t i = 0; i < m_numDataFiles; i++)
	{
		delete m_pTrainingWindData[i];
	}
	delete[] m_pTrainingWindData;
	delete m_pEvaluationWindData;
	delete m_pPowerSetpoint;
}

void WindTurbine::reset(State *s)
{
	if (SimionApp::get()->pExperiment->isEvaluationEpisode())
		m_pCurrentWindData = m_pEvaluationWindData;
	else
		m_pCurrentWindData = m_pTrainingWindData[rand() % m_numDataFiles];

	double initial_wind_speed = getConstant("RatedWindSpeed");
	double initial_rotor_speed= getConstant("RatedRotorSpeed");
	double initial_blade_angle= 0.0;

	double tsr= initial_rotor_speed*getConstant("RotorDiameter")*0.5/initial_wind_speed;

	s->set("T_a",aerodynamicTorque(tsr,initial_blade_angle,initial_wind_speed));
	s->set("P_a", s->get("T_a")*initial_rotor_speed);
	s->set("P_s",m_pPowerSetpoint->getSetPoint(0.0));

	s->set("P_e", getConstant("RatedPower"));
	s->set("E_p", s->get("P_e") - s->get("P_s"));
	s->set("v",initial_wind_speed);

	s->set("omega_r",initial_rotor_speed);
	s->set("E_omega_r",initial_rotor_speed-getConstant("RatedRotorSpeed"));
	s->set("d_omega_r",0.0);
	s->set("omega_g", initial_rotor_speed*getConstant("GearBoxRatio"));
	s->set("E_omega_g", s->get("omega_g") - getConstant("RatedGeneratorSpeed"));
	s->set("d_omega_g", 0.0);
	s->set("beta",initial_blade_angle);
	s->set("d_beta",0.0);
	s->set("T_g", getConstant("RatedGeneratorTorque"));
	s->set("d_T_g",0.0);
	s->set("E_int_omega_r", 0.0);
	s->set("E_int_omega_g", 0.0);
	s->set("theta", 0.0);
}


void WindTurbine::executeAction(State *s, const Action *a, double dt)
{
	s->set("P_s", m_pPowerSetpoint->getSetPoint(SimionApp::get()->pWorld->getEpisodeSimTime()));
	s->set("v",m_pCurrentWindData->getSetPoint(SimionApp::get()->pWorld->getEpisodeSimTime()));

	double lastBeta = s->get("beta");
	double lastTorque = s->get("T_g");

	if (SimionApp::get()->pWorld->bIsFirstIntegrationStep())
	{
		//calculate action variables' derivatives to clamp them
		s->set("d_T_g", (a->get("T_g") - lastTorque) / dt);
		s->set("d_beta", (a->get("beta") - lastBeta) / dt);
	}

	s->set("beta", lastBeta + s->get("d_beta")*dt);
	s->set("T_g", lastTorque + s->get("d_T_g")*dt);

	//P_e= T_g*omega_g
	double omega_r = s->get("omega_r");
	double omega_g = s->get("omega_g");

	s->set("P_e",a->get("T_g")*omega_g*getConstant("ElectricalGeneratorEfficiency"));
	s->set("E_p", s->get("P_e") - s->get("P_s"));

	double tip_speed_ratio = (s->get("omega_r")*getConstant("RotorDiameter")*0.5) / s->get("v");
	
	//C_p(tip_speed_ratio,blade_angle)
	//double power_coef=C_p(tip_speed_ratio,beta);
	//P_a= 0.5*rho*pi*R^2*C_p(lambda,beta)v^3
	double P_a = aerodynamicPower(tip_speed_ratio, s->get("beta"), s->get("v"));
	s->set("P_a",P_a);
	//T_a= P_a/omega_r
	double T_a= 0.0;
	if (omega_r>0.0)
		T_a= P_a / omega_r;
	s->set("T_a",T_a);


	//d(omega_r)= (T_a - DriveTrainTorsionalDamping*omega_r - T_g) / GeneratorInertia
	double d_omega_r = (T_a - getConstant("TotalTurbineTorsionalDamping")*omega_r - a->get("T_g"))
		/ getConstant("TotalTurbineInertia");//437847250;//

	s->set("d_omega_r",d_omega_r);
	s->set("d_omega_g", d_omega_r*getConstant("GearBoxRatio"));

	s->set("omega_r", omega_r + d_omega_r*dt);
	s->set("omega_g", s->get("omega_r")*getConstant("GearBoxRatio"));
	s->set("E_omega_r", s->get("omega_r") - getConstant("RatedRotorSpeed"));
	s->set("E_omega_g", s->get("omega_g") - getConstant("RatedGeneratorSpeed"));
	s->set("E_int_omega_r", s->get("E_int_omega_r") + s->get("E_omega_r")*dt);

	s->set("theta", s->get("theta") + omega_r * dt);
}