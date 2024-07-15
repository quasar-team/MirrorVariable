/* © Copyright CERN, 2021.  All rights not expressly granted are reserved.
 * MirrorVariableEngine.h
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

#ifndef ADDRESSSPACE_INCLUDE_MIRRORVARIABLEENGINE_H_
#define ADDRESSSPACE_INCLUDE_MIRRORVARIABLEENGINE_H_

#include <functional>
#include <uanodeid.h>
#include <ChangeNotifyingVariable.h>
#include <LogIt.h>
#include <LogLevels.h>

namespace MirrorVariable
{

class ASNodeManager;

typedef std::function<UaStatus(Session* pSession, const UaDataValue& dataValue, OpcUa_Boolean checkAccessLevel)> WriteToMirroredVariable;

template<typename T>
std::string toString(const T& t)
{
	return t.toFullString().toUtf8();
}

bool instantiateReadOnlyMirrorVariable(
    	    AddressSpace::ASNodeManager* nm,
			AddressSpace::ChangeNotifyingVariable* mirroredVariable,
    	    UaNode* mirrorParent,
    		const std::string& mirrorVariableName);

bool instantiateReadWriteMirrorVariable(
    	    AddressSpace::ASNodeManager* nm,
			AddressSpace::ChangeNotifyingVariable* mirroredVariable,
    	    UaNode* mirrorParent,
    		const std::string& mirrorVariableName,
			WriteToMirroredVariable writeToMirroredVariable = WriteToMirroredVariable(/*empty*/));

} /* namespace MirrorVariable */

#endif /* ADDRESSSPACE_INCLUDE_MIRRORVARIABLEENGINE_H_ */
