/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "DebugInfoEntries.h"

#include <new>

#include "AttributeValue.h"
#include "dwarf.h"
#include "SourceLanguageInfo.h"


// #pragma mark - DIECompileUnitBase


DIECompileUnitBase::DIECompileUnitBase()
	:
	fName(NULL),
	fCompilationDir(NULL),
	fLowPC(0),
	fHighPC(0),
	fStatementListOffset(0),
	fMacroInfoOffset(0),
		// TODO: Is 0 a good invalid offset?
	fBaseTypesUnit(NULL),
	fLanguage(0),
	fIdentifierCase(0),
	fUseUTF8(true)
{
}


status_t
DIECompileUnitBase::InitAfterAttributes(DebugInfoEntryInitInfo& info)
{
	switch (fLanguage) {
		case 0:
			info.languageInfo = &kUnknownLanguageInfo;
			return B_OK;
		case DW_LANG_C89:
			info.languageInfo = &kC89LanguageInfo;
			return B_OK;
		case DW_LANG_C:
			info.languageInfo = &kCLanguageInfo;
			return B_OK;
		case DW_LANG_C_plus_plus:
			info.languageInfo = &kCPlusPlusLanguageInfo;
			return B_OK;
		case DW_LANG_C99:
			info.languageInfo = &kC99LanguageInfo;
			return B_OK;
		default:
			info.languageInfo = &kUnsupportedLanguageInfo;
			return B_OK;
	}
}


const char*
DIECompileUnitBase::Name() const
{
	return fName;
}


status_t
DIECompileUnitBase::AddChild(DebugInfoEntry* child)
{
	if (child->IsType())
		fTypes.Add(child);
	else
		fOtherChildren.Add(child);
	return B_OK;
}


status_t
DIECompileUnitBase::AddAttribute_name(uint16 attributeName,
	const AttributeValue& value)
{
	fName = value.string;
	return B_OK;
}


status_t
DIECompileUnitBase::AddAttribute_comp_dir(uint16 attributeName,
	const AttributeValue& value)
{
	fCompilationDir = value.string;
	return B_OK;
}


status_t
DIECompileUnitBase::AddAttribute_low_pc(uint16 attributeName,
	const AttributeValue& value)
{
	fLowPC = value.address;
	return B_OK;
}


status_t
DIECompileUnitBase::AddAttribute_high_pc(uint16 attributeName,
	const AttributeValue& value)
{
	fHighPC = value.address;
	return B_OK;
}


status_t
DIECompileUnitBase::AddAttribute_producer(uint16 attributeName,
	const AttributeValue& value)
{
	// not interesting
	return B_OK;
}


status_t
DIECompileUnitBase::AddAttribute_stmt_list(uint16 attributeName,
	const AttributeValue& value)
{
	fStatementListOffset = value.pointer;
	return B_OK;
}


status_t
DIECompileUnitBase::AddAttribute_macro_info(uint16 attributeName,
	const AttributeValue& value)
{
	fMacroInfoOffset = value.pointer;
	return B_OK;
}


status_t
DIECompileUnitBase::AddAttribute_base_types(uint16 attributeName,
	const AttributeValue& value)
{
	fBaseTypesUnit = dynamic_cast<DIECompileUnitBase*>(value.reference);
	return fBaseTypesUnit != NULL ? B_OK : B_BAD_DATA;
}


status_t
DIECompileUnitBase::AddAttribute_language(uint16 attributeName,
	const AttributeValue& value)
{
	fLanguage = value.constant;
	return B_OK;
}


status_t
DIECompileUnitBase::AddAttribute_identifier_case(uint16 attributeName,
	const AttributeValue& value)
{
	fIdentifierCase = value.constant;
	return B_OK;
}


status_t
DIECompileUnitBase::AddAttribute_use_UTF8(uint16 attributeName,
	const AttributeValue& value)
{
	fUseUTF8 = value.flag;
	return B_OK;
}


// #pragma mark - DIEType


DIEType::DIEType()
	:
	fName(NULL)
{
	fAllocated.SetTo((uint64)0);
	fAssociated.SetTo((uint64)0);
}


bool
DIEType::IsType() const
{
	return true;
}


const char*
DIEType::Name() const
{
	return fName;
}


status_t
DIEType::AddAttribute_name(uint16 attributeName,
	const AttributeValue& value)
{
	fName = value.string;
	return B_OK;
}


status_t
DIEType::AddAttribute_allocated(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fAllocated, value);
}


status_t
DIEType::AddAttribute_associated(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fAssociated, value);
}


// #pragma mark - DIEModifiedType


DIEModifiedType::DIEModifiedType()
	:
	fType(NULL)
{
}


status_t
DIEModifiedType::AddAttribute_type(uint16 attributeName,
	const AttributeValue& value)
{
	fType = dynamic_cast<DIEType*>(value.reference);
	return fType != NULL ? B_OK : B_BAD_DATA;
}


// #pragma mark - DIEAddressingType


DIEAddressingType::DIEAddressingType()
	:
	fAddressClass(0)
{
}


status_t
DIEAddressingType::AddAttribute_address_class(uint16 attributeName,
	const AttributeValue& value)
{
// TODO: How is the address class handled?
	fAddressClass = value.constant;
	return B_OK;
}


// #pragma mark - DIEDeclaredType


DIEDeclaredType::DIEDeclaredType()
	:
	fDescription(NULL),
	fAbstractOrigin(NULL),
	fAccessibility(0),
	fDeclaration(false)
{
}


const char*
DIEDeclaredType::Description() const
{
	return fDescription;
}


status_t
DIEDeclaredType::AddAttribute_accessibility(uint16 attributeName,
	const AttributeValue& value)
{
	fAccessibility = value.constant;
	return B_OK;
}


status_t
DIEDeclaredType::AddAttribute_declaration(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclaration = value.flag;
	return B_OK;
}


status_t
DIEDeclaredType::AddAttribute_description(uint16 attributeName,
	const AttributeValue& value)
{
	fDescription = value.string;
	return B_OK;
}


status_t
DIEDeclaredType::AddAttribute_abstract_origin(uint16 attributeName,
	const AttributeValue& value)
{
	fAbstractOrigin = value.reference;
	return B_OK;
}


DeclarationLocation*
DIEDeclaredType::GetDeclarationLocation()
{
	return &fDeclarationLocation;
}


// #pragma mark - DIEDerivedType


DIEDerivedType::DIEDerivedType()
	:
	fType(NULL)
{
}


status_t
DIEDerivedType::AddAttribute_type(uint16 attributeName,
	const AttributeValue& value)
{
	fType = dynamic_cast<DIEType*>(value.reference);
	return fType != NULL ? B_OK : B_BAD_DATA;
}




// #pragma mark - DIECompoundType


DIECompoundType::DIECompoundType()
	:
	fSpecification(NULL)
{
}


status_t
DIECompoundType::AddChild(DebugInfoEntry* child)
{
	if (child->Tag() == DW_TAG_member) {
		// TODO: Not for interfaces!
		fDataMembers.Add(child);
		return B_OK;
	}

	return DIEDeclaredType::AddChild(child);
}


status_t
DIECompoundType::AddAttribute_byte_size(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fByteSize, value);
}


status_t
DIECompoundType::AddAttribute_specification(uint16 attributeName,
	const AttributeValue& value)
{
	fSpecification = dynamic_cast<DIECompoundType*>(value.reference);
	return fSpecification != NULL ? B_OK : B_BAD_DATA;
}


// #pragma mark - DIEClassBaseType


DIEClassBaseType::DIEClassBaseType()
{
}


status_t
DIEClassBaseType::AddChild(DebugInfoEntry* child)
{
	switch (child->Tag()) {
		case DW_TAG_inheritance:
			fDataMembers.Add(child);
			return B_OK;
		case DW_TAG_friend:
			fFriends.Add(child);
			return B_OK;
		case DW_TAG_access_declaration:
			fAccessDeclarations.Add(child);
			return B_OK;
		case DW_TAG_subprogram:
			fMemberFunctions.Add(child);
			return B_OK;
// TODO: Templates!
// TODO: Variants!
		default:
			return DIECompoundType::AddChild(child);
	}
}


// #pragma mark - DIENamedBase


DIENamedBase::DIENamedBase()
	:
	fName(NULL),
	fDescription(NULL)
{
}


const char*
DIENamedBase::Name() const
{
	return fName;
}


const char*
DIENamedBase::Description() const
{
	return fDescription;
}


status_t
DIENamedBase::AddAttribute_name(uint16 attributeName,
	const AttributeValue& value)
{
	fName = value.string;
	return B_OK;
}


status_t
DIENamedBase::AddAttribute_description(uint16 attributeName,
	const AttributeValue& value)
{
	fDescription = value.string;
	return B_OK;
}


// #pragma mark - DIEDeclaredBase


DIEDeclaredBase::DIEDeclaredBase()
{
}


DeclarationLocation*
DIEDeclaredBase::GetDeclarationLocation()
{
	return &fDeclarationLocation;
}


// #pragma mark - DIEDeclaredNamedBase


DIEDeclaredNamedBase::DIEDeclaredNamedBase()
	:
	fName(NULL),
	fDescription(NULL),
	fAccessibility(0),
	fVisibility(0),
	fDeclaration(false)
{
}


const char*
DIEDeclaredNamedBase::Name() const
{
	return fName;
}


const char*
DIEDeclaredNamedBase::Description() const
{
	return fDescription;
}


status_t
DIEDeclaredNamedBase::AddAttribute_name(uint16 attributeName,
	const AttributeValue& value)
{
	fName = value.string;
	return B_OK;
}


status_t
DIEDeclaredNamedBase::AddAttribute_description(uint16 attributeName,
	const AttributeValue& value)
{
	fDescription = value.string;
	return B_OK;
}


status_t
DIEDeclaredNamedBase::AddAttribute_accessibility(uint16 attributeName,
	const AttributeValue& value)
{
	fAccessibility = value.constant;
	return B_OK;
}


status_t
DIEDeclaredNamedBase::AddAttribute_declaration(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclaration = value.flag;
	return B_OK;
}


// #pragma mark - DIEArrayType


DIEArrayType::DIEArrayType()
	:
	fSpecification(NULL),
	fOrdering(DW_ORD_row_major)
{
}


uint16
DIEArrayType::Tag() const
{
	return DW_TAG_array_type;
}


status_t
DIEArrayType::InitAfterHierarchy(DebugInfoEntryInitInfo& info)
{
	fOrdering = info.languageInfo->arrayOrdering;
	return B_OK;
}


status_t
DIEArrayType::AddChild(DebugInfoEntry* child)
{
	// a dimension child must be of subrange or enumeration type
	uint16 tag = child->Tag();
	if (tag == DW_TAG_subrange_type || tag == DW_TAG_enumeration_type) {
		fDimensions.Add(child);
		return B_OK;
	}

	return DIEDerivedType::AddChild(child);
}


status_t
DIEArrayType::AddAttribute_ordering(uint16 attributeName,
	const AttributeValue& value)
{
	fOrdering = value.constant;
	return B_OK;
}


status_t
DIEArrayType::AddAttribute_bit_stride(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fBitStride, value);
}


status_t
DIEArrayType::AddAttribute_stride_size(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fBitStride, value);
}


status_t
DIEArrayType::AddAttribute_byte_size(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fByteSize, value);
}


status_t
DIEArrayType::AddAttribute_specification(uint16 attributeName,
	const AttributeValue& value)
{
	fSpecification = dynamic_cast<DIEArrayType*>(value.reference);
	return fSpecification != NULL ? B_OK : B_BAD_DATA;
}


// #pragma mark - DIEClassType


DIEClassType::DIEClassType()
{
}


uint16
DIEClassType::Tag() const
{
	return DW_TAG_class_type;
}


// #pragma mark - DIEEntryPoint


DIEEntryPoint::DIEEntryPoint()
{
}


uint16
DIEEntryPoint::Tag() const
{
	return DW_TAG_entry_point;
}


// #pragma mark - DIEEnumerationType


DIEEnumerationType::DIEEnumerationType()
	:
	fSpecification(NULL)
{
}


uint16
DIEEnumerationType::Tag() const
{
	return DW_TAG_enumeration_type;
}


status_t
DIEEnumerationType::AddChild(DebugInfoEntry* child)
{
	if (child->Tag() == DW_TAG_enumerator) {
		fEnumerators.Add(child);
		return B_OK;
	}

	return DIEDerivedType::AddChild(child);
}


status_t
DIEEnumerationType::AddAttribute_bit_stride(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fBitStride, value);
}


status_t
DIEEnumerationType::AddAttribute_byte_size(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fByteSize, value);
}


status_t
DIEEnumerationType::AddAttribute_byte_stride(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fByteStride, value);
}


status_t
DIEEnumerationType::AddAttribute_specification(uint16 attributeName,
	const AttributeValue& value)
{
	fSpecification = dynamic_cast<DIEEnumerationType*>(value.reference);
	return fSpecification != NULL ? B_OK : B_BAD_DATA;
}


// #pragma mark - DIEFormalParameter


DIEFormalParameter::DIEFormalParameter()
{
}


uint16
DIEFormalParameter::Tag() const
{
	return DW_TAG_formal_parameter;
}


// #pragma mark - DIEImportedDeclaration


DIEImportedDeclaration::DIEImportedDeclaration()
{
}


uint16
DIEImportedDeclaration::Tag() const
{
	return DW_TAG_imported_declaration;
}


// #pragma mark - DIELabel


DIELabel::DIELabel()
{
}


uint16
DIELabel::Tag() const
{
	return DW_TAG_label;
}


// #pragma mark - DIELexicalBlock


DIELexicalBlock::DIELexicalBlock()
{
}


uint16
DIELexicalBlock::Tag() const
{
	return DW_TAG_lexical_block;
}


// #pragma mark - DIEMember


DIEMember::DIEMember()
{
}


uint16
DIEMember::Tag() const
{
	return DW_TAG_member;
}


// #pragma mark - DIEPointerType


DIEPointerType::DIEPointerType()
	:
	fSpecification(NULL)
{
}


uint16
DIEPointerType::Tag() const
{
	return DW_TAG_pointer_type;
}


status_t
DIEPointerType::AddAttribute_specification(uint16 attributeName,
	const AttributeValue& value)
{
	fSpecification = dynamic_cast<DIEPointerType*>(value.reference);
	return fSpecification != NULL ? B_OK : B_BAD_DATA;
}


// #pragma mark - DIEReferenceType


DIEReferenceType::DIEReferenceType()
{
}


uint16
DIEReferenceType::Tag() const
{
	return DW_TAG_reference_type;
}


// #pragma mark - DIECompileUnit


DIECompileUnit::DIECompileUnit()
{
}


uint16
DIECompileUnit::Tag() const
{
	return DW_TAG_compile_unit;
}


// #pragma mark - DIEStringType


DIEStringType::DIEStringType()
{
}


uint16
DIEStringType::Tag() const
{
	return DW_TAG_string_type;
}


status_t
DIEStringType::AddAttribute_byte_size(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fByteSize, value);
}


// #pragma mark - DIEStructureType


DIEStructureType::DIEStructureType()
{
}


uint16
DIEStructureType::Tag() const
{
	return DW_TAG_structure_type;
}


// #pragma mark - DIESubroutineType


DIESubroutineType::DIESubroutineType()
	:
	fReturnType(NULL),
	fAddressClass(0),
	fPrototyped(false)
{
}


uint16
DIESubroutineType::Tag() const
{
	return DW_TAG_subroutine_type;
}


status_t
DIESubroutineType::AddChild(DebugInfoEntry* child)
{
	switch (child->Tag()) {
		case DW_TAG_formal_parameter:
		case DW_TAG_unspecified_parameters:
			fParameters.Add(child);
			return B_OK;
		default:
			return DIEDeclaredType::AddChild(child);
	}
}


status_t
DIESubroutineType::AddAttribute_address_class(uint16 attributeName,
	const AttributeValue& value)
{
// TODO: How is the address class handled?
	fAddressClass = value.constant;
	return B_OK;
}


status_t
DIESubroutineType::AddAttribute_prototyped(uint16 attributeName,
	const AttributeValue& value)
{
	fPrototyped = value.flag;
	return B_OK;
}


status_t
DIESubroutineType::AddAttribute_type(uint16 attributeName,
	const AttributeValue& value)
{
	fReturnType = dynamic_cast<DIEType*>(value.reference);
	return fReturnType != NULL ? B_OK : B_BAD_DATA;
}


// #pragma mark - DIETypedef


DIETypedef::DIETypedef()
{
}


uint16
DIETypedef::Tag() const
{
	return DW_TAG_typedef;
}


// #pragma mark - DIEUnionType


DIEUnionType::DIEUnionType()
{
}


uint16
DIEUnionType::Tag() const
{
	return DW_TAG_union_type;
}


// #pragma mark - DIEUnspecifiedParameters


DIEUnspecifiedParameters::DIEUnspecifiedParameters()
{
}


uint16
DIEUnspecifiedParameters::Tag() const
{
	return DW_TAG_unspecified_parameters;
}


// #pragma mark - DIEVariant


DIEVariant::DIEVariant()
{
}


uint16
DIEVariant::Tag() const
{
	return DW_TAG_variant;
}


// #pragma mark - DIECommonBlock


DIECommonBlock::DIECommonBlock()
{
}


uint16
DIECommonBlock::Tag() const
{
	return DW_TAG_common_block;
}


// #pragma mark - DIECommonInclusion


DIECommonInclusion::DIECommonInclusion()
{
}


uint16
DIECommonInclusion::Tag() const
{
	return DW_TAG_common_inclusion;
}


// #pragma mark - DIEInheritance


DIEInheritance::DIEInheritance()
{
}


uint16
DIEInheritance::Tag() const
{
	return DW_TAG_inheritance;
}


// #pragma mark - DIEInlinedSubroutine


DIEInlinedSubroutine::DIEInlinedSubroutine()
{
}


uint16
DIEInlinedSubroutine::Tag() const
{
	return DW_TAG_inlined_subroutine;
}


// #pragma mark - DIEModule


DIEModule::DIEModule()
{
}


uint16
DIEModule::Tag() const
{
	return DW_TAG_module;
}


// #pragma mark - DIEPointerToMemberType


DIEPointerToMemberType::DIEPointerToMemberType()
	:
	fContainingType(NULL),
	fAddressClass(0)
{
}


uint16
DIEPointerToMemberType::Tag() const
{
	return DW_TAG_ptr_to_member_type;
}


status_t
DIEPointerToMemberType::AddAttribute_address_class(uint16 attributeName,
	const AttributeValue& value)
{
// TODO: How is the address class handled?
	fAddressClass = value.constant;
	return B_OK;
}


status_t
DIEPointerToMemberType::AddAttribute_containing_type(uint16 attributeName,
	const AttributeValue& value)
{
	fContainingType = dynamic_cast<DIECompoundType*>(value.reference);
	return fContainingType != NULL ? B_OK : B_BAD_DATA;
}


// #pragma mark - DIESetType


DIESetType::DIESetType()
{
}


uint16
DIESetType::Tag() const
{
	return DW_TAG_set_type;
}


status_t
DIESetType::AddAttribute_byte_size(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fByteSize, value);
}


// #pragma mark - DIESubrangeType


DIESubrangeType::DIESubrangeType()
	:
	fThreadsScaled(false)
{
}


uint16
DIESubrangeType::Tag() const
{
	return DW_TAG_subrange_type;
}


status_t
DIESubrangeType::AddAttribute_bit_stride(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fBitStride, value);
}


status_t
DIESubrangeType::AddAttribute_byte_size(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fByteSize, value);
}


status_t
DIESubrangeType::AddAttribute_byte_stride(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fByteStride, value);
}


status_t
DIESubrangeType::AddAttribute_count(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fCount, value);
}


status_t
DIESubrangeType::AddAttribute_lower_bound(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fLowerBound, value);
}


status_t
DIESubrangeType::AddAttribute_upper_bound(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fUpperBound, value);
}


status_t
DIESubrangeType::AddAttribute_threads_scaled(uint16 attributeName,
	const AttributeValue& value)
{
	fThreadsScaled = value.flag;
	return B_OK;
}


// #pragma mark - DIEWithStatement


DIEWithStatement::DIEWithStatement()
{
}


uint16
DIEWithStatement::Tag() const
{
	return DW_TAG_with_stmt;
}


// #pragma mark - DIEAccessDeclaration


DIEAccessDeclaration::DIEAccessDeclaration()
{
}


uint16
DIEAccessDeclaration::Tag() const
{
	return DW_TAG_access_declaration;
}


// #pragma mark - DIEBaseType


DIEBaseType::DIEBaseType()
	:
	fEncoding(0),
	fEndianity(0)
{
}


uint16
DIEBaseType::Tag() const
{
	return DW_TAG_base_type;
}


status_t
DIEBaseType::AddAttribute_encoding(uint16 attributeName,
	const AttributeValue& value)
{
	fEncoding = value.constant;
	return B_OK;
}


status_t
DIEBaseType::AddAttribute_byte_size(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fByteSize, value);
}


status_t
DIEBaseType::AddAttribute_bit_size(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fBitSize, value);
}


status_t
DIEBaseType::AddAttribute_bit_offset(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fBitOffset, value);
}


status_t
DIEBaseType::AddAttribute_endianity(uint16 attributeName,
	const AttributeValue& value)
{
	fEndianity = value.constant;
	return B_OK;
}


// #pragma mark - DIECatchBlock


DIECatchBlock::DIECatchBlock()
{
}


uint16
DIECatchBlock::Tag() const
{
	return DW_TAG_catch_block;
}


// #pragma mark - DIEConstType


DIEConstType::DIEConstType()
{
}


uint16
DIEConstType::Tag() const
{
	return DW_TAG_const_type;
}


// #pragma mark - DIEConstant


DIEConstant::DIEConstant()
{
}


uint16
DIEConstant::Tag() const
{
	return DW_TAG_constant;
}


// #pragma mark - DIEEnumerator


DIEEnumerator::DIEEnumerator()
{
}


uint16
DIEEnumerator::Tag() const
{
	return DW_TAG_enumerator;
}


status_t
DIEEnumerator::AddAttribute_const_value(uint16 attributeName,
	const AttributeValue& value)
{
	return SetConstantAttributeValue(fValue, value);
}


// #pragma mark - DIEFileType


DIEFileType::DIEFileType()
{
}


uint16
DIEFileType::Tag() const
{
	return DW_TAG_file_type;
}


status_t
DIEFileType::AddAttribute_byte_size(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fByteSize, value);
}


// #pragma mark - DIEFriend


DIEFriend::DIEFriend()
{
}


uint16
DIEFriend::Tag() const
{
	return DW_TAG_friend;
}


// #pragma mark - DIENameList


DIENameList::DIENameList()
{
}


uint16
DIENameList::Tag() const
{
	return DW_TAG_namelist;
}


// #pragma mark - DIENameListItem


DIENameListItem::DIENameListItem()
{
}


uint16
DIENameListItem::Tag() const
{
	return DW_TAG_namelist_item;
}


// #pragma mark - DIEPackedType


DIEPackedType::DIEPackedType()
{
}


uint16
DIEPackedType::Tag() const
{
	return DW_TAG_packed_type;
}


// #pragma mark - DIESubprogram


DIESubprogram::DIESubprogram()
{
}


uint16
DIESubprogram::Tag() const
{
	return DW_TAG_subprogram;
}


// #pragma mark - DIETemplateTypeParameter


DIETemplateTypeParameter::DIETemplateTypeParameter()
{
}


uint16
DIETemplateTypeParameter::Tag() const
{
	return DW_TAG_template_type_parameter;
}


// #pragma mark - DIETemplateValueParameter


DIETemplateValueParameter::DIETemplateValueParameter()
{
}


uint16
DIETemplateValueParameter::Tag() const
{
	return DW_TAG_template_value_parameter;
}


// #pragma mark - DIEThrownType


DIEThrownType::DIEThrownType()
{
}


uint16
DIEThrownType::Tag() const
{
	return DW_TAG_thrown_type;
}


// #pragma mark - DIETryBlock


DIETryBlock::DIETryBlock()
{
}


uint16
DIETryBlock::Tag() const
{
	return DW_TAG_try_block;
}


// #pragma mark - DIEVariantPart


DIEVariantPart::DIEVariantPart()
{
}


uint16
DIEVariantPart::Tag() const
{
	return DW_TAG_variant_part;
}


// #pragma mark - DIEVariable


DIEVariable::DIEVariable()
{
}


uint16
DIEVariable::Tag() const
{
	return DW_TAG_variable;
}


// #pragma mark - DIEVolatileType


DIEVolatileType::DIEVolatileType()
{
}


uint16
DIEVolatileType::Tag() const
{
	return DW_TAG_volatile_type;
}


status_t
DIEVolatileType::AddAttribute_decl_file(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclarationLocation.SetFile(value.constant);
	return B_OK;
}


status_t
DIEVolatileType::AddAttribute_decl_line(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclarationLocation.SetLine(value.constant);
	return B_OK;
}


status_t
DIEVolatileType::AddAttribute_decl_column(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclarationLocation.SetColumn(value.constant);
	return B_OK;
}


// #pragma mark - DIEDwarfProcedure


DIEDwarfProcedure::DIEDwarfProcedure()
{
}


uint16
DIEDwarfProcedure::Tag() const
{
	return DW_TAG_dwarf_procedure;
}


// #pragma mark - DIERestrictType


DIERestrictType::DIERestrictType()
{
}


uint16
DIERestrictType::Tag() const
{
	return DW_TAG_restrict_type;
}


// #pragma mark - DIEInterfaceType


DIEInterfaceType::DIEInterfaceType()
{
}


uint16
DIEInterfaceType::Tag() const
{
	return DW_TAG_interface_type;
}


// #pragma mark - DIENamespace


DIENamespace::DIENamespace()
{
}


uint16
DIENamespace::Tag() const
{
	return DW_TAG_namespace;
}


// #pragma mark - DIEImportedModule


DIEImportedModule::DIEImportedModule()
{
}


uint16
DIEImportedModule::Tag() const
{
	return DW_TAG_imported_module;
}


// #pragma mark - DIEUnspecifiedType


DIEUnspecifiedType::DIEUnspecifiedType()
{
}


uint16
DIEUnspecifiedType::Tag() const
{
	return DW_TAG_unspecified_type;
}


status_t
DIEUnspecifiedType::AddAttribute_decl_file(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclarationLocation.SetFile(value.constant);
	return B_OK;
}


status_t
DIEUnspecifiedType::AddAttribute_decl_line(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclarationLocation.SetLine(value.constant);
	return B_OK;
}


status_t
DIEUnspecifiedType::AddAttribute_decl_column(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclarationLocation.SetColumn(value.constant);
	return B_OK;
}


// #pragma mark - DIEPartialUnit


DIEPartialUnit::DIEPartialUnit()
{
}


uint16
DIEPartialUnit::Tag() const
{
	return DW_TAG_partial_unit;
}


// #pragma mark - DIEImportedUnit


DIEImportedUnit::DIEImportedUnit()
{
}


uint16
DIEImportedUnit::Tag() const
{
	return DW_TAG_imported_unit;
}


// #pragma mark - DIECondition


DIECondition::DIECondition()
{
}


uint16
DIECondition::Tag() const
{
	return DW_TAG_condition;
}


// #pragma mark - DIESharedType


DIESharedType::DIESharedType()
{
	fBlockSize.SetTo(DWARF_ADDRESS_MAX);
}


uint16
DIESharedType::Tag() const
{
	return DW_TAG_shared_type;
}


status_t
DIESharedType::AddAttribute_count(uint16 attributeName,
	const AttributeValue& value)
{
	return SetDynamicAttributeValue(fBlockSize, value);
}


status_t
DIESharedType::AddAttribute_decl_file(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclarationLocation.SetFile(value.constant);
	return B_OK;
}


status_t
DIESharedType::AddAttribute_decl_line(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclarationLocation.SetLine(value.constant);
	return B_OK;
}


status_t
DIESharedType::AddAttribute_decl_column(uint16 attributeName,
	const AttributeValue& value)
{
	fDeclarationLocation.SetColumn(value.constant);
	return B_OK;
}


// #pragma mark - DebugInfoEntryFactory


DebugInfoEntryFactory::DebugInfoEntryFactory()
{
}


status_t
DebugInfoEntryFactory::CreateDebugInfoEntry(uint16 tag, DebugInfoEntry*& _entry)
{
	DebugInfoEntry* entry = NULL;

	switch (tag) {
		case DW_TAG_array_type:
			entry = new(std::nothrow) DIEArrayType;
			break;
		case DW_TAG_class_type:
			entry = new(std::nothrow) DIEClassType;
			break;
		case DW_TAG_entry_point:
			entry = new(std::nothrow) DIEEntryPoint;
			break;
		case DW_TAG_enumeration_type:
			entry = new(std::nothrow) DIEEnumerationType;
			break;
		case DW_TAG_formal_parameter:
			entry = new(std::nothrow) DIEFormalParameter;
			break;
		case DW_TAG_imported_declaration:
			entry = new(std::nothrow) DIEImportedDeclaration;
			break;
		case DW_TAG_label:
			entry = new(std::nothrow) DIELabel;
			break;
		case DW_TAG_lexical_block:
			entry = new(std::nothrow) DIELexicalBlock;
			break;
		case DW_TAG_member:
			entry = new(std::nothrow) DIEMember;
			break;
		case DW_TAG_pointer_type:
			entry = new(std::nothrow) DIEPointerType;
			break;
		case DW_TAG_reference_type:
			entry = new(std::nothrow) DIEReferenceType;
			break;
		case DW_TAG_compile_unit:
			entry = new(std::nothrow) DIECompileUnit;
			break;
		case DW_TAG_string_type:
			entry = new(std::nothrow) DIEStringType;
			break;
		case DW_TAG_structure_type:
			entry = new(std::nothrow) DIEStructureType;
			break;
		case DW_TAG_subroutine_type:
			entry = new(std::nothrow) DIESubroutineType;
			break;
		case DW_TAG_typedef:
			entry = new(std::nothrow) DIETypedef;
			break;
		case DW_TAG_union_type:
			entry = new(std::nothrow) DIEUnionType;
			break;
		case DW_TAG_unspecified_parameters:
			entry = new(std::nothrow) DIEUnspecifiedParameters;
			break;
		case DW_TAG_variant:
			entry = new(std::nothrow) DIEVariant;
			break;
		case DW_TAG_common_block:
			entry = new(std::nothrow) DIECommonBlock;
			break;
		case DW_TAG_common_inclusion:
			entry = new(std::nothrow) DIECommonInclusion;
			break;
		case DW_TAG_inheritance:
			entry = new(std::nothrow) DIEInheritance;
			break;
		case DW_TAG_inlined_subroutine:
			entry = new(std::nothrow) DIEInlinedSubroutine;
			break;
		case DW_TAG_module:
			entry = new(std::nothrow) DIEModule;
			break;
		case DW_TAG_ptr_to_member_type:
			entry = new(std::nothrow) DIEPointerToMemberType;
			break;
		case DW_TAG_set_type:
			entry = new(std::nothrow) DIESetType;
			break;
		case DW_TAG_subrange_type:
			entry = new(std::nothrow) DIESubrangeType;
			break;
		case DW_TAG_with_stmt:
			entry = new(std::nothrow) DIEWithStatement;
			break;
		case DW_TAG_access_declaration:
			entry = new(std::nothrow) DIEAccessDeclaration;
			break;
		case DW_TAG_base_type:
			entry = new(std::nothrow) DIEBaseType;
			break;
		case DW_TAG_catch_block:
			entry = new(std::nothrow) DIECatchBlock;
			break;
		case DW_TAG_const_type:
			entry = new(std::nothrow) DIEConstType;
			break;
		case DW_TAG_constant:
			entry = new(std::nothrow) DIEConstant;
			break;
		case DW_TAG_enumerator:
			entry = new(std::nothrow) DIEEnumerator;
			break;
		case DW_TAG_file_type:
			entry = new(std::nothrow) DIEFileType;
			break;
		case DW_TAG_friend:
			entry = new(std::nothrow) DIEFriend;
			break;
		case DW_TAG_namelist:
			entry = new(std::nothrow) DIENameList;
			break;
		case DW_TAG_namelist_item:
			entry = new(std::nothrow) DIENameListItem;
			break;
		case DW_TAG_packed_type:
			entry = new(std::nothrow) DIEPackedType;
			break;
		case DW_TAG_subprogram:
			entry = new(std::nothrow) DIESubprogram;
			break;
		case DW_TAG_template_type_parameter:
			entry = new(std::nothrow) DIETemplateTypeParameter;
			break;
		case DW_TAG_template_value_parameter:
			entry = new(std::nothrow) DIETemplateValueParameter;
			break;
		case DW_TAG_thrown_type:
			entry = new(std::nothrow) DIEThrownType;
			break;
		case DW_TAG_try_block:
			entry = new(std::nothrow) DIETryBlock;
			break;
		case DW_TAG_variant_part:
			entry = new(std::nothrow) DIEVariantPart;
			break;
		case DW_TAG_variable:
			entry = new(std::nothrow) DIEVariable;
			break;
		case DW_TAG_volatile_type:
			entry = new(std::nothrow) DIEVolatileType;
			break;
		case DW_TAG_dwarf_procedure:
			entry = new(std::nothrow) DIEDwarfProcedure;
			break;
		case DW_TAG_restrict_type:
			entry = new(std::nothrow) DIERestrictType;
			break;
		case DW_TAG_interface_type:
			entry = new(std::nothrow) DIEInterfaceType;
			break;
		case DW_TAG_namespace:
			entry = new(std::nothrow) DIENamespace;
			break;
		case DW_TAG_imported_module:
			entry = new(std::nothrow) DIEImportedModule;
			break;
		case DW_TAG_unspecified_type:
			entry = new(std::nothrow) DIEUnspecifiedType;
			break;
		case DW_TAG_partial_unit:
			entry = new(std::nothrow) DIEPartialUnit;
			break;
		case DW_TAG_imported_unit:
			entry = new(std::nothrow) DIEImportedUnit;
			break;
		case DW_TAG_condition:
			entry = new(std::nothrow) DIECondition;
			break;
		case DW_TAG_shared_type:
			entry = new(std::nothrow) DIESharedType;
			break;
		default:
			break;
	}

	_entry = entry;
	return B_OK;
}