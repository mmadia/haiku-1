/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NO_OP_STACK_FRAME_DEBUG_INFO_H
#define NO_OP_STACK_FRAME_DEBUG_INFO_H


#include "StackFrameDebugInfo.h"


class NoOpStackFrameDebugInfo : public StackFrameDebugInfo {
public:
								NoOpStackFrameDebugInfo();
	virtual						~NoOpStackFrameDebugInfo();

	virtual	status_t			ResolveObjectDataLocation(
									StackFrame* stackFrame, Type* type,
									target_addr_t objectAddress,
									ValueLocation*& _location);
	virtual	status_t			ResolveBaseTypeLocation(
									StackFrame* stackFrame, Type* type,
									BaseType* baseType,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);
	virtual	status_t			ResolveDataMemberLocation(
									StackFrame* stackFrame, Type* type,
									DataMember* member,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);
};


#endif	// NO_OP_STACK_FRAME_DEBUG_INFO_H