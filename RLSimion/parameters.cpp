#include "parameters.h"
#include "worlds/world.h"
#include "named-var-set.h"

STATE_VARIABLE::STATE_VARIABLE(ConfigNode* pConfigNode, const char* name, const char* comment)
{
	m_variableName = pConfigNode->getConstString(name);
	m_hVariable = World::getDynamicModel()->getStateDescriptor().getVarIndex(pConfigNode->getConstString(name));
	m_pProperties = &World::getDynamicModel()->getStateDescriptor()[m_hVariable];
	m_name = name;
	m_comment = comment;
}

STATE_VARIABLE::STATE_VARIABLE(Descriptor& stateDescriptor, size_t varId)
{
	m_variableName = stateDescriptor[varId].getName();
	m_name = m_variableName;
	m_hVariable = varId;
	m_pProperties = &stateDescriptor[varId];
	m_comment = "Object created from code, not a data file";
}

ACTION_VARIABLE::ACTION_VARIABLE(ConfigNode* pConfigNode, const char* name, const char* comment)
{
	m_variableName = pConfigNode->getConstString(name);
	m_hVariable = World::getDynamicModel()->getActionDescriptor().getVarIndex(pConfigNode->getConstString(name));
	m_pProperties = &World::getDynamicModel()->getActionDescriptor()[m_hVariable];
	m_name = name;
	m_comment = comment;
}

ACTION_VARIABLE::ACTION_VARIABLE(Descriptor& actionDescriptor, size_t varId)
{
	m_variableName = actionDescriptor[varId].getName();
	m_name = m_variableName;
	m_hVariable = varId;
	m_pProperties = &actionDescriptor[varId];
	m_comment = "Object created from code, not a data file";
}

#ifdef _WIN64
#include "../tools/CNTKWrapper/CNTKWrapper.h"

//Overriden Copy-assignment to avoid destroying copied buffer
//Expected use: problem= NN_DEFINITION(...)
NN_DEFINITION&  NN_DEFINITION::operator=(NN_DEFINITION& copied)
{
	//we move pointers to the copy
	m_pDefinition = copied.m_pDefinition;

	copied.m_pDefinition = nullptr;

	m_name = copied.m_name;
	m_comment = copied.m_comment;

	return *this;
}


NN_DEFINITION::~NN_DEFINITION()
{
}

NN_DEFINITION::NN_DEFINITION(ConfigNode* pConfigNode, const char* name, const char* comment)
{
	m_name = name;
	m_comment = comment;
	m_pDefinition = CNTKWrapperLoader::getNetworkDefinition(pConfigNode->FirstChildElement(m_name)->FirstChildElement("Problem"));
}

void NN_DEFINITION::destroy()
{
	if (m_pDefinition)
	{
		m_pDefinition->destroy();
		m_pDefinition = nullptr;
	}
}

#endif