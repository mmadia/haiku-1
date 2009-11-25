/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <ByteOrder.h>
#include <Messenger.h>
#include <MessengerPrivate.h>


status_t
swap_data(type_code type, void *_data, size_t length, swap_action action)
{
	if (_data == NULL || length == 0)
		return B_BAD_VALUE;

	switch (type) {
		// allowed types
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
		case B_FLOAT_TYPE:
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
		case B_POINTER_TYPE:
		case B_RECT_TYPE:
		case B_POINT_TYPE:
		case B_DOUBLE_TYPE:
		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		case B_OFF_T_TYPE:
		case B_MESSENGER_TYPE:
			break;

		default:
			// not swappable or recognized type!
			return B_BAD_VALUE;
	}

	// is there anything to do?
#if B_HOST_IS_LENDIAN
	if (action == B_SWAP_HOST_TO_LENDIAN || action == B_SWAP_LENDIAN_TO_HOST)
		return B_OK;
#else
	if (action == B_SWAP_HOST_TO_BENDIAN || action == B_SWAP_BENDIAN_TO_HOST)
		return B_OK;
#endif

	switch (type) {
		// 16 bit types
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
		{
			uint16 *data = (uint16 *)_data;
			uint16 *end = (uint16 *)((addr_t)_data + length);

			while (data < end) {
				*data = __swap_int16(*data);
				data++;
			}
			break;
		}

		// 32 bit types
		case B_FLOAT_TYPE:
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
		case B_POINTER_TYPE:
		case B_RECT_TYPE:
		case B_POINT_TYPE:
		{
			// ToDo: some of these types may not be 32-bit on 64-bit platforms!
			uint32 *data = (uint32 *)_data;
			uint32 *end = (uint32 *)((addr_t)_data + length);

			while (data < end) {
				*data = __swap_int32(*data);
				data++;
			}
			break;
		}

		// 64 bit types
		case B_DOUBLE_TYPE:
		case B_INT64_TYPE:
		case B_UINT64_TYPE:
		case B_OFF_T_TYPE:
		{
			uint64 *data = (uint64 *)_data;
			uint64 *end = (uint64 *)((addr_t)_data + length);

			while (data < end) {
				*data = __swap_int64(*data);
				data++;
			}
			break;
		}

		// special types
		case B_MESSENGER_TYPE:
		{
			BMessenger *messenger = (BMessenger *)_data;
			BMessenger *end = (BMessenger *)((addr_t)_data + length);

			while (messenger < end) {
				BMessenger::Private messengerPrivate(messenger);
				// ToDo: if the additional fields change, this function has to be updated!
				messengerPrivate.SetTo(
					__swap_int32(messengerPrivate.Team()),
					__swap_int32(messengerPrivate.Port()),
					__swap_int32(messengerPrivate.Token()));
				messenger++;
			}
			break;
		}
	}

	return B_OK;
}


bool
is_type_swapped(type_code type)
{
	// Returns true when the type is in the host's native format
	// Looks like a pretty strange function to me :)

	switch (type) {
		case B_ANY_TYPE:
		case B_BOOL_TYPE:
		case B_CHAR_TYPE:
		case B_COLOR_8_BIT_TYPE:
		case B_DOUBLE_TYPE:
		case B_FLOAT_TYPE:
		case B_GRAYSCALE_8_BIT_TYPE:
		case B_INT64_TYPE:
		case B_INT32_TYPE:
		case B_INT16_TYPE:
		case B_INT8_TYPE:
		case B_MESSAGE_TYPE:
		case B_MESSENGER_TYPE:
		case B_MIME_TYPE:
		case B_MONOCHROME_1_BIT_TYPE:
		case B_OBJECT_TYPE:
		case B_OFF_T_TYPE:
		case B_PATTERN_TYPE:
		case B_POINTER_TYPE:
		case B_POINT_TYPE:
		case B_RAW_TYPE:
		case B_RECT_TYPE:
		case B_REF_TYPE:
		case B_RGB_32_BIT_TYPE:
		case B_RGB_COLOR_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_STRING_TYPE:
		case B_TIME_TYPE:
		case B_UINT64_TYPE:
		case B_UINT32_TYPE:
		case B_UINT16_TYPE:
		case B_UINT8_TYPE:
		case B_MEDIA_PARAMETER_TYPE:
		case B_MEDIA_PARAMETER_WEB_TYPE:
		case B_MEDIA_PARAMETER_GROUP_TYPE:
			return true;
	}

	return false;
}

