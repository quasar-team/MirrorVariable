/* © Copyright CERN, 2021.  All rights not expressly granted are reserved.
 * MirrorVariableEngine.cpp
 *
 *  Created on: 16 Jun 2021
 *      Author: bfarnham
 *
 *  This file is part of Quasar.
 *
 *  Quasar is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public Licence as published by
 *  the Free Software Foundation, either version 3 of the Licence.
 *
 *  Quasar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public Licence for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with Quasar.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <ASNodeManager.h>
#include <MirrorVariableEngine.h>

namespace MirrorVariable
{

class OnChangeHandler
{
public:
	OnChangeHandler(AddressSpace::ChangeNotifyingVariable* mirroredVariable, WriteToMirroredVariable writeToMirroredVariable)
	  :m_mirroredVariable(mirroredVariable), m_writeToMirroredVariable(writeToMirroredVariable){};
	virtual ~OnChangeHandler(){};

	void operator()(AddressSpace::ChangeNotifyingVariable& fromWhere, const UaDataValue& newValue)
	{
		const auto mirrorValue = m_mirroredVariable->value(nullptr);
		if(mirrorValue != newValue)
		{
			LOG(Log::TRC) << "MirroredNodeUpdater: updating mirrored variable ["<<toString(m_mirroredVariable->browseName())<<"] from mirrored variable ["<<toString(fromWhere.browseName())<<"]";
			m_writeToMirroredVariable(nullptr, newValue, OpcUa_True);
		}
		else
		{
			LOG(Log::TRC) << "MirroredNodeUpdater: NOT updating mirrored variable, values are already same";
		}
	}
private:
	AddressSpace::ChangeNotifyingVariable* m_mirroredVariable;
	WriteToMirroredVariable m_writeToMirroredVariable;
};

bool verifyParameters(AddressSpace::ASNodeManager* nm,
	AddressSpace::ChangeNotifyingVariable* mirroredVariable,
    UaNode* mirrorParent,
	const std::string& mirrorVariableName)
{
	if(!nm)
	{
		LOG(Log::ERR) << "instantiateMirrorVariable called with null ASNodeManager; programming error - node manager is required, invalid";
		return false;
	}

	if(!mirroredVariable)
	{
		LOG(Log::ERR) << "instantiateMirrorVariable called with null mirrored variable; programming error - need to know which variable is mirrored, invalid";
		return false;
	}

	if(!mirrorParent)
	{
		LOG(Log::ERR) << "instantiateMirrorVariable called with null mirror parent; programming error - need to know where to attach mirror variable, invalid";
		return false;
	}

	if(mirrorVariableName.empty())
	{
		LOG(Log::ERR) << "instantiateMirrorVariable called with empty mirror variable name; programming error - mirror variable needs a name, invalid";
		return false;
	}

	return true;
}

bool instantiateMirrorVariable(
    AddressSpace::ASNodeManager* nm,
	AddressSpace::ChangeNotifyingVariable* mirroredVariable,
    UaNode* mirrorParent,
	const std::string& mirrorVariableName,
	WriteToMirroredVariable writeToMirroredVariable)
{
	LOG(Log::INF) << __FUNCTION__ << " called, to add mirror variable ["<<mirrorVariableName<<"] to address space parent ["<<std::hex<<mirrorParent<<"] mirroring ["<<std::hex<<mirroredVariable<<"]"
			<< " mirroring [id:"<<toString(mirroredVariable->nodeId())<<", name:"<<toString(mirroredVariable->browseName())<<"]"
			<< " parent node [id:"<<toString(mirrorParent->nodeId())<<", name "<<toString(mirrorParent->browseName())<<"]"
			<< " writeToMirroredVariable ["<<(writeToMirroredVariable? "OK" : "NULL!")<<"]";

	if(!verifyParameters(nm, mirroredVariable, mirrorParent, mirrorVariableName)) return false;

	const UaString mirrorVariableUaName(mirrorVariableName.c_str());
	UaNodeId mirrorVariableNodeId = nm->makeChildNodeId(mirrorParent->nodeId(), mirrorVariableUaName);

    LOG(Log::INF) << __FUNCTION__ << " mirrored node class ["<<mirroredVariable->nodeClass()<<"]";

    AddressSpace::ChangeNotifyingVariable* mirrorVariable = new AddressSpace::ChangeNotifyingVariable (
      mirrorVariableNodeId,
	  mirrorVariableUaName,
      nm->getNameSpaceIndex(),
	  UaVariant(),
      OpcUa_AccessLevels_CurrentReadOrWrite,
      nm);

    const auto initialValue = mirroredVariable->value(nullptr);
    mirrorVariable->setValue(nullptr, initialValue, OpcUa_False);

    mirrorVariable->addChangeListener(OnChangeHandler(mirroredVariable, writeToMirroredVariable));

    WriteToMirroredVariable writeToMirrorVariable = [mirrorVariable](Session* s, const UaDataValue& v, OpcUa_Boolean l) {return mirrorVariable->setValue(s, v, l);};
    mirroredVariable->addChangeListener(OnChangeHandler(mirrorVariable, writeToMirrorVariable));

    // add to address space
    UaStatus status = nm->addNodeAndReference( mirrorParent->nodeId(), mirrorVariable, OpcUaId_Organizes );
    if (!status.isGood())
    {
    	LOG(Log::ERR) << __FUNCTION__ << " failed to link mirror variable [" << mirrorVariableName << "]"
    			<< " to parent node [id:"<<toString(mirrorParent->nodeId())<<", name "<<toString(mirrorParent->browseName())<<"]"
				<< " status ["<<status.toString().toUtf8()<<"]";
    	return false;
    }
    else
    {
    	LOG(Log::INF) << __FUNCTION__ << " added new mirror variable [ptr:"<<std::hex<<mirrorVariable<<", addr:"<< toString(mirrorVariable->nodeId()) << "]"
    			<< " to mirror existing variable [ptr:"<<std::hex<<mirroredVariable<<", addr:"<< toString(mirroredVariable->nodeId()) <<"]";
    	return true;
    }
}

} /* namespace MirrorVariable */
