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

#include "featuremap.h"
#include "../Common/named-var-set.h"
#include "features.h"
#include "single-dimension-grid.h"
#include <math.h>

#define ACTIVATION_THRESHOLD 0.0001

GaussianRBFGridFeatureMap::GaussianRBFGridFeatureMap()
{
	m_pVarFeatures = new FeatureList("RBFGrid/var");
}

GaussianRBFGridFeatureMap::GaussianRBFGridFeatureMap(ConfigNode* pConfigNode)
{
	m_pVarFeatures = new FeatureList("RBFGrid/var");
}

GaussianRBFGridFeatureMap::~GaussianRBFGridFeatureMap()
{
	if (m_pVarFeatures)
		delete m_pVarFeatures;
}

void GaussianRBFGridFeatureMap::init(vector<SingleDimensionGrid*>& grids)
{
	//calculate the number of features
	m_totalNumFeatures = 1;
	m_maxNumActiveFeatures = 1;

	for (unsigned int i = 0; i < grids.size(); i++)
	{
		m_totalNumFeatures *= grids[i]->getValues().size();
		m_maxNumActiveFeatures *= m_maxNumActiveFeaturesPerDimension;
	}
}

/// <summary>
/// Implements a Gaussian Radial-Basis feature mapping function that maps state-actions to feature.
/// </summary>
/// <param name="grids">Input grids for every state-variable used</param>
/// <param name="values">The values of every state-variable used</param>
/// <param name="outFeatures">The output list of features</param>
void GaussianRBFGridFeatureMap::map(vector<SingleDimensionGrid*>& grids, const vector<double>& values, FeatureList* outFeatures)
{
	size_t offset = 1;

	outFeatures->clear();
	if (grids.size() == 0) return;
	//features of the 0th variable in outFeatures
	getDimensionFeatures(grids[0], values[0], outFeatures);

	for (unsigned int i = 1; i < grids.size(); i++)
	{
		offset *= grids[i-1]->getValues().size();

		//we calculate the features of i-th variable in m_pVarFeatures
		getDimensionFeatures(grids[i], values[i], m_pVarFeatures);
		//spawn features in buffer with the i-th variable's features
		outFeatures->spawn(m_pVarFeatures, offset);
	}
	//unnecessary (it will be done by each grid) if there is only one variable
	if (grids.size() > 1)
	{
		outFeatures->applyThreshold(ACTIVATION_THRESHOLD);
		outFeatures->normalize();
	}
}

/// <summary>
/// Inverse of the feature mapping operation. Given a feature it returns the state-action to which it corresponds.
/// </summary>
/// <param name="feature">The index of the feature</param>
/// <param name="grids">The set of grids used to discretize each variable</param>
/// <param name="outValues">The set of output values for every state-action variable</param>
void GaussianRBFGridFeatureMap::unmap(size_t feature, vector<SingleDimensionGrid*>& grids, vector<double>& outValues)
{
	size_t dimFeature;

	for (size_t i = 0; i < grids.size(); i++)
	{
		dimFeature = feature % grids[i]->getValues().size();

		outValues[i] = grids[i]->getFeatureValue(dimFeature);

		feature = feature / grids[i]->getValues().size();
	}
}

void GaussianRBFGridFeatureMap::getDimensionFeatures(SingleDimensionGrid* pGrid, double value, FeatureList* outFeatures)
{
	double u;
	size_t i;

	outFeatures->clear();

	size_t numCenters = pGrid->getValues().size();

	if (numCenters <= 2) return;

	if (value <= pGrid->getValues()[1])
	{
		if (!pGrid->isCircular())
		{
			outFeatures->add(0, getFeatureFactor(pGrid, 0, value));
			outFeatures->add(1, getFeatureFactor(pGrid, 1, value));
			outFeatures->add(2, getFeatureFactor(pGrid, 2, value));
		}
		else
		{
			outFeatures->add(0, getFeatureFactor(pGrid, 0, value));
			outFeatures->add(1, getFeatureFactor(pGrid, 1, value));
			outFeatures->add(numCenters - 1, getFeatureFactor(pGrid, numCenters - 1, value + pGrid->getRangeWidth()));
		}
	}
	else if (value >= pGrid->getValues()[numCenters - 2])
	{
		if (!pGrid->isCircular())
		{
			outFeatures->add(numCenters - 3, getFeatureFactor(pGrid, numCenters - 3, value));
			outFeatures->add(numCenters - 2, getFeatureFactor(pGrid, numCenters - 2, value));
			outFeatures->add(numCenters - 1, getFeatureFactor(pGrid, numCenters - 1, value));
		}
		else
		{
			outFeatures->add(numCenters - 2, getFeatureFactor(pGrid, numCenters - 2, value));
			outFeatures->add(numCenters - 1, getFeatureFactor(pGrid, numCenters - 1, value));
			outFeatures->add(0, getFeatureFactor(pGrid, 0, value - pGrid->getRangeWidth()));
		}
	}
	else
	{
		i = 1;

		while (value > pGrid->getValues()[i + 1]) i++;

		u = (value - pGrid->getValues()[i]) / (pGrid->getValues()[i + 1] - pGrid->getValues()[i]);

		if (u < 0.5)
		{
			outFeatures->add(i, getFeatureFactor(pGrid, i, value));
			outFeatures->add(i + 1, getFeatureFactor(pGrid, i + 1, value));
		}
		else
		{
			outFeatures->add(i + 1, getFeatureFactor(pGrid, i + 1, value));
			outFeatures->add(i, getFeatureFactor(pGrid, i, value));
		}

		if (value - pGrid->getValues()[i - 1] < pGrid->getValues()[i + 2] - value)
			outFeatures->add(i - 1, getFeatureFactor(pGrid, i - 1, value));
		else
			outFeatures->add(i + 2, getFeatureFactor(pGrid, i + 2, value));
	}
	outFeatures->applyThreshold(ACTIVATION_THRESHOLD);
	outFeatures->normalize();
}

double GaussianRBFGridFeatureMap::getFeatureFactor(SingleDimensionGrid* pGrid, size_t feature, double value) const
{
	double range, dist;

	if (value > pGrid->getValues()[feature])
	{
		dist = value - pGrid->getValues()[feature];
		if (feature != pGrid->getValues().size() - 1)
			range = pGrid->getValues()[feature + 1] - pGrid->getValues()[feature];
		else
			range = pGrid->getValues()[feature] - pGrid->getValues()[feature - 1];
	}
	else
	{
		dist = pGrid->getValues()[feature] - value;
		if (feature != 0)
			range = pGrid->getValues()[feature] - pGrid->getValues()[feature - 1];
		else
			range = pGrid->getValues()[1] - pGrid->getValues()[0];
	}

	//f_gauss(x)= a*exp(-(x-b)^2 / 2c^2 )
	//instead of 2c^2, we use the distance to the next feature
	double f = 2 * dist / range;
	double factor = exp(-(f*f));
	return factor;
}