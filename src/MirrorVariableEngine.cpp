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
			auto result = m_writeToMirroredVariable(nullptr, newValue, OpcUa_True);
			LOG(Log::TRC) << "MirroredNodeUpdater: updated mirrored variable ["<<toString(m_mirroredVariable->browseName())<<"] result ["<<result.toString().toUtf8()<<"]";
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

bool addMirrorVariableToAddressSpace(
			AddressSpace::ASNodeManager* nm, 
			AddressSpace::ChangeNotifyingVariable* mirroredVariable, 
			UaNode* mirrorParent, 
			AddressSpace::ChangeNotifyingVariable* mirrorVariable)
{
    const UaStatus status = nm->addNodeAndReference( mirrorParent->nodeId(), mirrorVariable, OpcUaId_Organizes );
    if (!status.isGood())
    {
    	LOG(Log::ERR) << __FUNCTION__ << " failed to link mirror variable [" << mirrorVariable->browseName() << "]"
    			<< " to parent node [id:"<<toString(mirrorParent->nodeId())<<", name ["<<toString(mirrorParent->browseName())<<"]"
				<< " status ["<<status.toString().toUtf8()<<"]";
    	return false;
    }
    else
    {
    	LOG(Log::INF) << __FUNCTION__ << " added new mirror variable [ptr:"<<std::hex<<mirrorVariable<<", addr:"<< toString(mirrorVariable->nodeId()) << "]"
    			<< " to mirror existing variable ["<<std::hex<<mirroredVariable<<", addr:"<< toString(mirroredVariable->nodeId()) <<"]";
    	return true;
    }
}

AddressSpace::ChangeNotifyingVariable* createMirrorVariable(
			AddressSpace::ASNodeManager* nm,
			AddressSpace::ChangeNotifyingVariable* mirroredVariable,
			UaNode* mirrorParent,
			const std::string& mirrorVariableName,
			const OpcUa_Byte& accessLevel)
{
	const UaString mirrorVariableUaName(mirrorVariableName.c_str());
	const UaNodeId mirrorVariableNodeId = nm->makeChildNodeId(mirrorParent->nodeId(), mirrorVariableUaName);
	const UaVariant initialValue = *(mirroredVariable->value(nullptr).value());

    return new AddressSpace::ChangeNotifyingVariable (
      mirrorVariableNodeId,
	  mirrorVariableUaName,
      nm->getNameSpaceIndex(),
	  initialValue,
      accessLevel,
      nm);
}

bool instantiateReadOnlyMirrorVariable(
    	    AddressSpace::ASNodeManager* nm,
			AddressSpace::ChangeNotifyingVariable* mirroredVariable,
    	    UaNode* mirrorParent,
    		const std::string& mirrorVariableName)
{
	LOG(Log::INF) << __FUNCTION__ << " called, to add mirror variable ["<<mirrorVariableName<<"] to address space parent ["<<std::hex<<mirrorParent<<"] mirroring ["<<std::hex<<mirroredVariable<<"]"
			<< " mirroring [id:"<<toString(mirroredVariable->nodeId())<<", name["<<toString(mirroredVariable->browseName())<<"]"
			<< " parent node [id:"<<toString(mirrorParent->nodeId())<<", name ["<<toString(mirrorParent->browseName())<<"]";

	if(!verifyParameters(nm, mirroredVariable, mirrorParent, mirrorVariableName)) return false;
    AddressSpace::ChangeNotifyingVariable* mirrorVariable = createMirrorVariable(nm, mirroredVariable, mirrorParent, mirrorVariableName, OpcUa_AccessLevels_CurrentRead);

	WriteToMirroredVariable writeToMirrorVariable = [mirrorVariable](Session* s, const UaDataValue& v, OpcUa_Boolean l) {return mirrorVariable->setValue(s, v, OpcUa_False);};
    mirroredVariable->addChangeListener(OnChangeHandler(mirrorVariable, writeToMirrorVariable));

	return addMirrorVariableToAddressSpace(nm, mirroredVariable, mirrorParent, mirrorVariable);
}

bool instantiateReadWriteMirrorVariable(
    AddressSpace::ASNodeManager* nm,
	AddressSpace::ChangeNotifyingVariable* mirroredVariable,
    UaNode* mirrorParent,
	const std::string& mirrorVariableName,
	WriteToMirroredVariable writeToMirroredVariable)
{
	LOG(Log::INF) << __FUNCTION__ << " called, to add mirror variable ["<<mirrorVariableName<<"] to address space parent ["<<std::hex<<mirrorParent<<"] mirroring ["<<std::hex<<mirroredVariable<<"]"
			<< " mirroring [id:"<<toString(mirroredVariable->nodeId())<<", name["<<toString(mirroredVariable->browseName())<<"]"
			<< " parent node [id:"<<toString(mirrorParent->nodeId())<<", name ["<<toString(mirrorParent->browseName())<<"]"
			<< " writeToMirroredVariable ["<<(writeToMirroredVariable? "OK" : "NULL!")<<"]";

	if(!verifyParameters(nm, mirroredVariable, mirrorParent, mirrorVariableName)) return false;
	AddressSpace::ChangeNotifyingVariable* mirrorVariable = createMirrorVariable(nm, mirroredVariable, mirrorParent, mirrorVariableName, OpcUa_AccessLevels_CurrentReadOrWrite);

    mirrorVariable->addChangeListener(OnChangeHandler(mirroredVariable, writeToMirroredVariable)); // handle write to mirror (send to mirrored)
	WriteToMirroredVariable writeToMirrorVariable = [mirrorVariable](Session* s, const UaDataValue& v, OpcUa_Boolean l) {return mirrorVariable->setValue(s, v, l);};
    mirroredVariable->addChangeListener(OnChangeHandler(mirrorVariable, writeToMirrorVariable));

    return addMirrorVariableToAddressSpace(nm, mirroredVariable, mirrorParent, mirrorVariable);
}

} /* namespace MirrorVariable */
