// LibBeAdapter.cpp

#include <ByteOrder.h>
#include <Entry.h>
#include <List.h>
#include <Path.h>
#include <string.h>

status_t
entry_ref_to_path_adapter(dev_t device, ino_t directory, const char *name,
						  char *buffer, size_t size)
{
	status_t error = (name && buffer ? B_OK : B_BAD_VALUE);
	BEntry entry;
	if (error == B_OK) {
		entry_ref ref(device, directory, name);
		error = entry.SetTo(&ref);
	}
	BPath path;
	if (error == B_OK)
		error = entry.GetPath(&path);
	if (error == B_OK) {
		if (size >= strlen(path.Path()) + 1)
			strcpy(buffer, path.Path());
		else
			error = B_BAD_VALUE;
	}
	return error;
}

