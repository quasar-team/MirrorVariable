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
	OnChangeHandler(AddressSpace::ChangeNotifyingVariable* mirroredVariable, WriteToMirroredVariable writeToMirroredVariable = WriteToMirroredVariable(/*empty*/))
	  :m_mirroredVariable(mirroredVariable), m_writeToMirroredVariable(writeToMirroredVariable){};
	virtual ~OnChangeHandler(){};

	void operator()(AddressSpace::ChangeNotifyingVariable& fromWhere, const UaDataValue& newValue)
	{
		const auto currentValue = m_mirroredVariable->value(nullptr/* ? erm */);
		if(currentValue != newValue)
		{
			// use specific handler if provided; otherwise just change value in AS layer
			if(m_writeToMirroredVariable)
			{
				LOG(Log::TRC) << "MirroredNodeUpdater: updating (via handler) mirrored variable ["<<m_mirroredVariable->browseName().toFullString().toUtf8()<<"] from mirrored variable ["<<fromWhere.browseName().toFullString().toUtf8()<<"]";
				m_writeToMirroredVariable(nullptr/* session ? */, newValue, OpcUa_False);
			}
			else
			{
				LOG(Log::TRC) << "MirroredNodeUpdater: updating (AS layer only) mirrored variable ["<<m_mirroredVariable->browseName().toFullString().toUtf8()<<"] from mirrored variable ["<<fromWhere.browseName().toFullString().toUtf8()<<"]";
				m_mirroredVariable->setValue(nullptr/* session ? */, newValue, OpcUa_False);
			}
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

bool instantiateMirrorVariable(
    AddressSpace::ASNodeManager* nm,
	AddressSpace::ChangeNotifyingVariable* mirroredVariable,
    UaNode* mirrorParent,
	const std::string& mirrorVariableName,
	WriteToMirroredVariable writeToMirroredVariable)
{
	LOG(Log::INF) << __FUNCTION__ << " called, to add mirror variable ["<<mirrorVariableName<<"] to address space parent ["<<std::hex<<mirrorParent<<"] mirroring ["<<std::hex<<mirroredVariable<<"]"
			<< " mirroring [id:"<<mirroredVariable->nodeId().toFullString().toUtf8()<<", name:"<<mirroredVariable->browseName().toFullString().toUtf8()<<"]"
			<< " parent node [id:"<<mirrorParent->nodeId().toFullString().toUtf8()<<", name "<<mirrorParent->browseName().toFullString().toUtf8()<<"]"
			<< " writeToMirroredVariable ["<<(writeToMirroredVariable? "OK" : "NULL!")<<"]";

	// TODO... add checks
	// 1. check all params are not NULL/empty etc.

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

    mirrorVariable->setValue(nullptr, mirroredVariable->value(nullptr/* ? erm */), OpcUa_False); // initial value
    mirrorVariable->addChangeListener(OnChangeHandler(mirroredVariable, writeToMirroredVariable));
    mirroredVariable->addChangeListener(OnChangeHandler(mirrorVariable));

    // add to address space
    UaStatus status = nm->addNodeAndReference( mirrorParent->nodeId(), mirrorVariable, OpcUaId_Organizes );
    if (!status.isGood())
    {
    	LOG(Log::ERR) << __FUNCTION__ << " failed to link mirror variable [" << mirrorVariableName << "]"
    			<< " to parent node [id:"<<mirrorParent->nodeId().toFullString().toUtf8()<<", name "<<mirrorParent->browseName().toFullString().toUtf8()<<"]"
				<< " status ["<<status.toString().toUtf8()<<"]";
    	return false;
    }
    else
    {
    	LOG(Log::INF) << __FUNCTION__ << " added new mirror variable [ptr:"<<std::hex<<mirrorVariable<<", addr:"<< mirrorVariable->nodeId().toFullString().toUtf8() << "]"
    			<< " to mirror existing variable [ptr:"<<std::hex<<mirroredVariable<<", addr:"<< mirroredVariable->nodeId().toFullString().toUtf8() <<"]";
    	return true;
    }
}

} /* namespace MirrorVariable */
