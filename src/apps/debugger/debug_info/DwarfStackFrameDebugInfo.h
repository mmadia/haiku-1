/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DWARF_INTERFACE_FACTORY_H
#define DWARF_INTERFACE_FACTORY_H


#include <String.h>

#include "StackFrameDebugInfo.h"
#include "Type.h"


class CompilationUnit;
class DIEAddressingType;
class DIEArrayType;
class DIEBaseType;
class DIECompoundType;
class DIEEnumerationType;
class DIEFormalParameter;
class DIEModifiedType;
class DIEPointerToMemberType;
class DIESubprogram;
class DIESubrangeType;
class DIESubroutineType;
class DIEType;
class DIETypedef;
class DIEUnspecifiedType;
class DIEVariable;
class DwarfFile;
class DwarfTargetInterface;
class FunctionID;
class GlobalTypeLookup;
class GlobalTypeLookupContext;
class LocationDescription;
class MemberLocation;
class ObjectID;
class RegisterMap;
class Variable;


class DwarfStackFrameDebugInfo : public StackFrameDebugInfo {
public:
								DwarfStackFrameDebugInfo(
									Architecture* architecture, DwarfFile* file,
									CompilationUnit* compilationUnit,
									DIESubprogram* subprogramEntry,
									GlobalTypeLookup* typeLookup,
									GlobalTypeLookupContext* typeLookupContext,
									target_addr_t instructionPointer,
									target_addr_t framePointer,
									DwarfTargetInterface* targetInterface,
									RegisterMap* fromDwarfRegisterMap);
								~DwarfStackFrameDebugInfo();

			status_t			Init();

	virtual	status_t			ResolveObjectDataLocation(
									StackFrame* stackFrame, Type* type,
									const ValueLocation& objectLocation,
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
	virtual	status_t			ResolveArrayElementLocation(
									StackFrame* stackFrame, ArrayType* type,
									const ArrayIndexPath& indexPath,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);

			status_t			CreateType(DIEType* typeEntry, Type*& _type);
									// returns reference
			status_t			CreateParameter(FunctionID* functionID,
									DIEFormalParameter* parameterEntry,
									Variable*& _parameter);
									// returns reference
			status_t			CreateLocalVariable(FunctionID* functionID,
									DIEVariable* variableEntry,
									Variable*& _variable);
									// returns reference

private:
			struct DwarfFunctionParameterID;
			struct DwarfLocalVariableID;
			struct DwarfType;
			struct DwarfInheritance;
			struct DwarfDataMember;
			struct DwarfEnumerationValue;
			struct DwarfArrayDimension;
			struct DwarfFunctionParameter;
			struct DwarfPrimitiveType;
			struct DwarfCompoundType;
			struct DwarfModifiedType;
			struct DwarfTypedefType;
			struct DwarfAddressType;
			struct DwarfEnumerationType;
			struct DwarfSubrangeType;
			struct DwarfArrayType;
			struct DwarfUnspecifiedType;
			struct DwarfFunctionType;
			struct DwarfPointerToMemberType;

private:
			status_t			_ResolveDataMemberLocation(
									StackFrame* stackFrame,
									DwarfCompoundType* type,
									Type* memberType,
									const MemberLocation* memberLocation,
									const ValueLocation& parentLocation,
									ValueLocation*& _location);
									// returns a new location

			status_t			_CreateType(DIEType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateTypeInternal(const BString& name,
									DIEType* typeEntry, DwarfType*& _type);

			status_t			_CreateCompoundType(const BString& name,
									DIECompoundType* typeEntry,
									DwarfType*& _type);
			status_t			_CreatePrimitiveType(const BString& name,
									DIEBaseType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateAddressType(const BString& name,
									DIEAddressingType* typeEntry,
									address_type_kind addressKind,
									DwarfType*& _type);
			status_t			_CreateModifiedType(const BString& name,
									DIEModifiedType* typeEntry,
									uint32 modifiers, DwarfType*& _type);
			status_t			_CreateTypedefType(const BString& name,
									DIETypedef* typeEntry, DwarfType*& _type);
			status_t			_CreateArrayType(const BString& name,
									DIEArrayType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateEnumerationType(const BString& name,
									DIEEnumerationType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateSubrangeType(const BString& name,
									DIESubrangeType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateUnspecifiedType(const BString& name,
									DIEUnspecifiedType* typeEntry,
									DwarfType*& _type);
			status_t			_CreateFunctionType(const BString& name,
									DIESubroutineType* typeEntry,
									DwarfType*& _type);
			status_t			_CreatePointerToMemberType(const BString& name,
									DIEPointerToMemberType* typeEntry,
									DwarfType*& _type);

			status_t			_CreateVariable(ObjectID* id,
									const BString& name, DIEType* typeEntry,
									LocationDescription* locationDescription,
									Variable*& _variable);

			status_t			_ResolveTypedef(DIETypedef* entry,
									DIEType*& _baseTypeEntry);
			status_t			_ResolveTypeByteSize(DIEType* typeEntry,
									uint64& _size);

			status_t			_ResolveLocation(
									const LocationDescription* description,
									target_addr_t objectAddress, Type* type,
									ValueLocation& _location);

	template<typename EntryType>
	static	DIEType*			_GetDIEType(EntryType* entry);

private:
			DwarfFile*			fFile;
			CompilationUnit*	fCompilationUnit;
			DIESubprogram*		fSubprogramEntry;
			GlobalTypeLookup*	fTypeLookup;
			GlobalTypeLookupContext* fTypeLookupContext;
			target_addr_t		fInstructionPointer;
			target_addr_t		fFramePointer;
			DwarfTargetInterface* fTargetInterface;
			RegisterMap*		fFromDwarfRegisterMap;
};


#endif	// DWARF_INTERFACE_FACTORY_H
