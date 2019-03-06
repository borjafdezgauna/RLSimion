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

#include "critic.h"
#include "vfa.h"
#include "features.h"
#include "etraces.h"
#include "experiment.h"
#include "vfa-critic.h"
#include "parameters-numeric.h"
#include "config.h"
#include "app.h"
#include "simgod.h"

TrueOnlineTDLambdaCritic::TrueOnlineTDLambdaCritic(ConfigNode* pConfigNode): VLearnerCritic(pConfigNode)
{
	m_e = CHILD_OBJECT<ETraces>(pConfigNode, "E-Traces", "Eligibility traces of the critic", true);
	m_e->setName("Critic/E-Traces" );
	m_aux= new FeatureList("Critic/aux");
	m_v_s= 0.0;
	m_pAlpha= CHILD_OBJECT_FACTORY<NumericValue>(pConfigNode, "Alpha", "Learning gain of the critic");
}

TrueOnlineTDLambdaCritic::~TrueOnlineTDLambdaCritic()
{
	delete m_aux;
}

/// <summary>
/// Updates the value function using the True-Online TD(Lambda) update rule in "True Online TD(lambda)"
/// (Harm van Seijen, Richard Sutton), Proceedings of the 31st International Conference on Machine learning
/// </summary>
/// <param name="s">Initial state</param>
/// <param name="a">Action</param>
/// <param name="s_p">Resultant state</param>
/// <param name="r">Reward</param>
/// <param name="rho">Importance sampling</param>
/// <returns>The Temporal-Difference error</returns>
double TrueOnlineTDLambdaCritic::update(const State *s,const  Action *a,const State *s_p, double r, double rho)
{
	double v_s_p;

	if (m_pAlpha->get()==0.0) return 0.0;
	
	if (SimionApp::get()->pExperiment->isFirstStep())
	{
		//vs= theta^T * phi(s)
		m_pVFunction->getFeatures(s,m_aux);
		m_v_s= m_pVFunction->get(m_aux);
	}
		
	//vs_p= theta^T * phi(s_p)
	m_pVFunction->getFeatures(s_p,m_aux);
	v_s_p= m_pVFunction->get(m_aux);

	double gamma = SimionApp::get()->pSimGod->getGamma();
	double alpha = m_pAlpha->get();
	//delta= R + gamma* v_s_p - v_s
	double td = r + gamma*v_s_p - m_v_s;

	//e= gamma*lambda*e + alpha*[1-gamma*lambda* e^T*phi(s)]* phi(s)
	m_pVFunction->getFeatures(s,m_aux);										//m_aux <- phi(s)
	double e_T_phi_s= m_e->innerProduct(m_aux);


	m_e->update(gamma);
	double lambda = m_e->getLambda();
	m_e->addFeatureList(m_aux,alpha *(1-gamma*lambda*e_T_phi_s));

	//theta= theta + delta*e + alpha[v_s - theta^T*phi(s)]* phi(s)
	m_pVFunction->add(m_e.ptr(),td);
	double theta_T_phi_s= m_pVFunction->get(m_aux);
	m_pVFunction->add(m_aux,alpha *(m_v_s - theta_T_phi_s));
	//v_s= v_s_p
	m_v_s= v_s_p;

	return td;
}

