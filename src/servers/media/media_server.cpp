/*
 * Copyright (c) 2002, 2003 Marcus Overhagen <Marcus@Overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */


/* to comply with the license above, do not remove the following line */
char __dont_remove_copyright_from_binary[] = "Copyright (c) 2002, 2003 "
	"Marcus Overhagen <Marcus@Overhagen.de>";


#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <Directory.h>
#include <Roster.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <Messenger.h>

#include <syscalls.h>

#include "AddOnManager.h"
#include "AppManager.h"
#include "BufferManager.h"
#include "DataExchange.h"
#include "FormatManager.h"
#include "MediaMisc.h"
#include "MediaFilesManager.h"
#include "NodeManager.h"
#include "NotificationManager.h"
#include "ServerInterface.h"
#include "debug.h"
#include "media_server.h"


AddOnManager* gAddOnManager;
AppManager* gAppManager;
BufferManager* gBufferManager;
FormatManager* gFormatManager;
MediaFilesManager* gMediaFilesManager;
NodeManager* gNodeManager;
NotificationManager* gNotificationManager;


#define REPLY_TIMEOUT ((bigtime_t)500000)


class ServerApp : BApplication {
public:
	ServerApp();
	~ServerApp();

protected:
	virtual void				ArgvReceived(int32 argc, char** argv);
	virtual void				ReadyToRun();
	virtual bool				QuitRequested();
	virtual void				MessageReceived(BMessage* message);

private:
			void				_HandleMessage(int32 code, void* data,
									size_t size);
			void				_LaunchAddOnServer();
			void				_QuitAddOnServer();

private:
			port_id				_ControlPort() const { return fControlPort; }

	static	int32				_ControlThread(void* arg);

			BLocker				fLocker;
			port_id				fControlPort;
			thread_id			fControlThread;
};


ServerApp::ServerApp()
 	:
 	BApplication(B_MEDIA_SERVER_SIGNATURE),
	fLocker("media server locker")
{
 	gNotificationManager = new NotificationManager;
 	gBufferManager = new BufferManager;
	gAppManager = new AppManager;
	gNodeManager = new NodeManager;
	gMediaFilesManager = new MediaFilesManager;
	gFormatManager = new FormatManager;
	gAddOnManager = new AddOnManager;

	fControlPort = create_port(64, MEDIA_SERVER_PORT_NAME);
	fControlThread = spawn_thread(_ControlThread, "media_server control", 105,
		this);
	resume_thread(fControlThread);
}


ServerApp::~ServerApp()
{
	TRACE("ServerApp::~ServerApp()\n");

	delete_port(fControlPort);
	wait_for_thread(fControlThread, NULL);

	delete gAddOnManager;
	delete gNotificationManager;
	delete gBufferManager;
	delete gAppManager;
	delete gNodeManager;
	delete gMediaFilesManager;
	delete gFormatManager;
}


void
ServerApp::ReadyToRun()
{
	gNodeManager->LoadState();
	gFormatManager->LoadState();

	// make sure any previous media_addon_server is gone
	_QuitAddOnServer();
	// and start a new one
	_LaunchAddOnServer();

	gAddOnManager->LoadState();
}


bool
ServerApp::QuitRequested()
{
	TRACE("ServerApp::QuitRequested()\n");
	gMediaFilesManager->SaveState();
	gNodeManager->SaveState();
	gFormatManager->SaveState();
	gAddOnManager->SaveState();

	_QuitAddOnServer();

	return true;
}


void
ServerApp::ArgvReceived(int32 argc, char **argv)
{
	for (int arg = 1; arg < argc; arg++) {
		if (strstr(argv[arg], "dump") != NULL) {
			gAppManager->Dump();
			gNodeManager->Dump();
			gBufferManager->Dump();
			gNotificationManager->Dump();
			gMediaFilesManager->Dump();
		}
		if (strstr(argv[arg], "buffer") != NULL)
			gBufferManager->Dump();
		if (strstr(argv[arg], "node") != NULL)
			gNodeManager->Dump();
		if (strstr(argv[arg], "files") != NULL)
			gMediaFilesManager->Dump();
		if (strstr(argv[arg], "quit") != NULL)
			PostMessage(B_QUIT_REQUESTED);
	}
}


void
ServerApp::_LaunchAddOnServer()
{
	// Try to launch media_addon_server by mime signature.
	// If it fails (for example on the Live CD, where the executable
	// hasn't yet been mimesetted), try from this application's
	// directory
	status_t err = be_roster->Launch(B_MEDIA_ADDON_SERVER_SIGNATURE);
	if (err == B_OK)
		return;

	app_info info;
	BEntry entry;
	BDirectory dir;
	entry_ref ref;

	err = GetAppInfo(&info);
	err |= entry.SetTo(&info.ref);
	err |= entry.GetParent(&entry);
	err |= dir.SetTo(&entry);
	err |= entry.SetTo(&dir, "media_addon_server");
	err |= entry.GetRef(&ref);

	if (err == B_OK)
		be_roster->Launch(&ref);
	if (err == B_OK)
		return;

	(new BAlert("media_server", "Launching media_addon_server failed.\n\n"
		"media_server will terminate", "OK"))->Go();
	fprintf(stderr, "Launching media_addon_server (%s) failed: %s\n",
		B_MEDIA_ADDON_SERVER_SIGNATURE, strerror(err));
	exit(1);
}


void
ServerApp::_QuitAddOnServer()
{
	// nothing to do if it's already terminated
	if (!be_roster->IsRunning(B_MEDIA_ADDON_SERVER_SIGNATURE))
		return;

	// send a quit request to the media_addon_server
	BMessenger msger(B_MEDIA_ADDON_SERVER_SIGNATURE);
	if (!msger.IsValid()) {
		ERROR("Trouble terminating media_addon_server. Messenger invalid\n");
	} else {
		BMessage msg(B_QUIT_REQUESTED);
		status_t err = msger.SendMessage(&msg, (BHandler *)NULL, 2000000);
			// 2 sec timeout
		if (err != B_OK) {
			ERROR("Trouble terminating media_addon_server (2): %s\n",
				strerror(err));
		}
	}

	// wait 5 seconds for it to terminate
	for (int i = 0; i < 50; i++) {
		if (!be_roster->IsRunning(B_MEDIA_ADDON_SERVER_SIGNATURE))
			return;
		snooze(100000); // 100 ms
	}

	// try to kill it (or many of them), up to 10 seconds
	for (int i = 0; i < 50; i++) {
		team_id id = be_roster->TeamFor(B_MEDIA_ADDON_SERVER_SIGNATURE);
		if (id < 0)
			break;
		kill_team(id);
		snooze(200000); // 200 ms
	}

	if (be_roster->IsRunning(B_MEDIA_ADDON_SERVER_SIGNATURE)) {
		ERROR("Trouble terminating media_addon_server, it's still running\n");
	}
}


void
ServerApp::_HandleMessage(int32 code, void* data, size_t size)
{
	status_t rv;
	TRACE("ServerApp::HandleMessage %#lx enter\n", code);
	switch (code) {
		case SERVER_CHANGE_ADDON_FLAVOR_INSTANCES_COUNT:
		{
			const server_change_addon_flavor_instances_count_request *request
				= reinterpret_cast<
					const server_change_addon_flavor_instances_count_request *>(
						data);
			server_change_addon_flavor_instances_count_reply reply;
			ASSERT(request->delta == 1 || request->delta == -1);
			if (request->delta == 1) {
				rv = gNodeManager->IncrementAddonFlavorInstancesCount(
					request->addon_id, request->flavor_id, request->team);
			} else {
				rv = gNodeManager->DecrementAddonFlavorInstancesCount(
					request->addon_id, request->flavor_id, request->team);
			}
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_RESCAN_DEFAULTS:
		{
			gNodeManager->RescanDefaultNodes();
			break;
		}

		case SERVER_REGISTER_APP:
		{
			const server_register_app_request *request
				= reinterpret_cast<const server_register_app_request *>(data);
			server_register_app_reply reply;
			rv = gAppManager->RegisterTeam(request->team, request->messenger);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_UNREGISTER_APP:
		{
			const server_unregister_app_request *request
				= reinterpret_cast<const server_unregister_app_request *>(
					data);
			server_unregister_app_reply reply;
			rv = gAppManager->UnregisterTeam(request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_MEDIAADDON_REF:
		{
			server_get_mediaaddon_ref_request *msg
				= (server_get_mediaaddon_ref_request *)data;
			server_get_mediaaddon_ref_reply reply;
			entry_ref tempref;
			reply.result = gNodeManager->GetAddonRef(&tempref, msg->addon_id);
			reply.ref = tempref;
			write_port(msg->reply_port, 0, &reply, sizeof(reply));
			break;
		}

		case SERVER_NODE_ID_FOR:
		{
			const server_node_id_for_request *request
				= reinterpret_cast<const server_node_id_for_request *>(data);
			server_node_id_for_reply reply;
			rv = gNodeManager->FindNodeID(&reply.node_id, request->port);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_LIVE_NODE_INFO:
		{
			const server_get_live_node_info_request *request
				= reinterpret_cast<const server_get_live_node_info_request *>(
					data);
			server_get_live_node_info_reply reply;
			rv = gNodeManager->GetLiveNodeInfo(&reply.live_info,
				request->node);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_LIVE_NODES:
		{
			const server_get_live_nodes_request *request
				= reinterpret_cast<const server_get_live_nodes_request *>(
					data);
			server_get_live_nodes_reply reply;
			Stack<live_node_info> livenodes;
			rv = gNodeManager->GetLiveNodes(
					&livenodes,
					request->maxcount,
					request->has_input ? &request->inputformat : NULL,
					request->has_output ? &request->outputformat : NULL,
					request->has_name ? request->name : NULL,
					request->require_kinds);
			reply.count = livenodes.CountItems();
			if (reply.count <= MAX_LIVE_INFO) {
				for (int32 index = 0; index < reply.count; index++)
					livenodes.Pop(&reply.live_info[index]);
				reply.area = -1;
			} else {
				// we create an area here, and pass it to the library,
				// where it will be deleted.
				live_node_info *start_addr;
				size_t size;
				size = ((reply.count * sizeof(live_node_info)) + B_PAGE_SIZE
					- 1) & ~(B_PAGE_SIZE - 1);
				reply.area = create_area("get live nodes",
					reinterpret_cast<void **>(&start_addr), B_ANY_ADDRESS,
					size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
				if (reply.area < B_OK) {
					ERROR("SERVER_GET_LIVE_NODES: failed to create area, "
						"error %#lx\n", reply.area);
					reply.count = 0;
					rv = B_ERROR;
				} else {
					for (int32 index = 0; index < reply.count; index++)
						livenodes.Pop(&start_addr[index]);
				}
			}
			rv = request->SendReply(rv, &reply, sizeof(reply));
			if (rv != B_OK) {
				// if we couldn't send the message, delete the area
				delete_area(reply.area);
			}
			break;
		}

		case SERVER_GET_NODE_FOR:
		{
			const server_get_node_for_request *request
				= reinterpret_cast<const server_get_node_for_request *>(data);
			server_get_node_for_reply reply;
			rv = gNodeManager->GetCloneForID(&reply.clone, request->node_id,
				request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_RELEASE_NODE:
		{
			const server_release_node_request *request
				= reinterpret_cast<const server_release_node_request *>(data);
			server_release_node_reply reply;
			rv = gNodeManager->ReleaseNode(request->node, request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_REGISTER_NODE:
		{
			const server_register_node_request *request
				= reinterpret_cast<const server_register_node_request *>(data);
			server_register_node_reply reply;
			rv = gNodeManager->RegisterNode(&reply.node_id, request->addon_id,
				request->addon_flavor_id, request->name, request->kinds,
				request->port, request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_UNREGISTER_NODE:
		{
			const server_unregister_node_request *request
				= reinterpret_cast<const server_unregister_node_request *>(
					data);
			server_unregister_node_reply reply;
			rv = gNodeManager->UnregisterNode(&reply.addon_id, &reply.flavor_id,
				request->node_id, request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_PUBLISH_INPUTS:
		{
			const server_publish_inputs_request *request
				= reinterpret_cast<const server_publish_inputs_request *>(
					data);
			server_publish_inputs_reply reply;
			if (request->count <= MAX_INPUTS) {
				rv = gNodeManager->PublishInputs(request->node,
					request->inputs, request->count);
			} else {
				media_input *inputs;
				area_id clone;
				clone = clone_area("media_inputs clone",
					reinterpret_cast<void **>(&inputs), B_ANY_ADDRESS,
					B_READ_AREA | B_WRITE_AREA, request->area);
				if (clone < B_OK) {
					ERROR("SERVER_PUBLISH_INPUTS: failed to clone area, "
						"error %#lx\n", clone);
					rv = B_ERROR;
				} else {
					rv = gNodeManager->PublishInputs(request->node, inputs,
						request->count);
					delete_area(clone);
				}
			}
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_PUBLISH_OUTPUTS:
		{
			const server_publish_outputs_request *request
				= reinterpret_cast<const server_publish_outputs_request *>(
					data);
			server_publish_outputs_reply reply;
			if (request->count <= MAX_OUTPUTS) {
				rv = gNodeManager->PublishOutputs(request->node,
					request->outputs, request->count);
			} else {
				media_output *outputs;
				area_id clone;
				clone = clone_area("media_outputs clone", reinterpret_cast<void **>(&outputs), B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, request->area);
				if (clone < B_OK) {
					ERROR("SERVER_PUBLISH_OUTPUTS: failed to clone area, "
						"error %#lx\n", clone);
					rv = B_ERROR;
				} else {
					rv = gNodeManager->PublishOutputs(request->node, outputs,
						request->count);
					delete_area(clone);
				}
			}
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_NODE:
		{
			const server_get_node_request *request
				= reinterpret_cast<const server_get_node_request *>(data);
			server_get_node_reply reply;
			rv = gNodeManager->GetClone(&reply.node, reply.input_name,
				&reply.input_id, request->type, request->team);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_SET_NODE:
		{
			const server_set_node_request *request
				= reinterpret_cast<const server_set_node_request *>(data);
			server_set_node_reply reply;
			rv = gNodeManager->SetDefaultNode(request->type,
				request->use_node ? &request->node : NULL,
				request->use_dni ? &request->dni : NULL,
				request->use_input ?  &request->input : NULL);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_DORMANT_NODE_FOR:
		{
			const server_get_dormant_node_for_request *request
				= reinterpret_cast<
					const server_get_dormant_node_for_request *>(data);
			server_get_dormant_node_for_reply reply;
			rv = gNodeManager->GetDormantNodeInfo(&reply.node_info,
				request->node);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_INSTANCES_FOR:
		{
			const server_get_instances_for_request *request
				= reinterpret_cast<const server_get_instances_for_request *>(
					data);
			server_get_instances_for_reply reply;
			rv = gNodeManager->GetInstances(reply.node_id, &reply.count,
				min_c(request->maxcount, MAX_NODE_ID), request->addon_id,
				request->addon_flavor_id);
			if (reply.count == MAX_NODE_ID
				&& request->maxcount > MAX_NODE_ID) {
					// XXX might be fixed by using an area
				PRINT(1, "Warning: SERVER_GET_INSTANCES_FOR: returning "
					"possibly truncated list of node id's\n");
			}
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_REGISTER_MEDIAADDON:
		{
			server_register_mediaaddon_request *msg
				= (server_register_mediaaddon_request *)data;
			server_register_mediaaddon_reply reply;
			gNodeManager->RegisterAddon(msg->ref, &reply.addon_id);
			write_port(msg->reply_port, 0, &reply, sizeof(reply));
			break;
		}

		case SERVER_UNREGISTER_MEDIAADDON:
		{
			server_unregister_mediaaddon_command *msg
				= (server_unregister_mediaaddon_command *)data;
			gNodeManager->UnregisterAddon(msg->addon_id);
			break;
		}

		case SERVER_REGISTER_DORMANT_NODE:
		{
			xfer_server_register_dormant_node* msg
				= (xfer_server_register_dormant_node*)data;
			if (msg->purge_id > 0)
				gNodeManager->InvalidateDormantFlavorInfo(msg->purge_id);

			dormant_flavor_info dormantFlavorInfo;
			status_t status = dormantFlavorInfo.Unflatten(msg->type,
				msg->flattened_data, msg->flattened_size);
			if (status == B_OK)
				gNodeManager->AddDormantFlavorInfo(dormantFlavorInfo);
			break;
		}

		case SERVER_GET_DORMANT_NODES:
		{
			xfer_server_get_dormant_nodes *msg
				= (xfer_server_get_dormant_nodes *)data;

			xfer_server_get_dormant_nodes_reply reply;
			reply.count = msg->max_count;

			dormant_node_info* infos
				= new(std::nothrow) dormant_node_info[reply.count];
			if (infos != NULL) {
				reply.result = gNodeManager->GetDormantNodes(infos,
					&reply.count, msg->has_input ? &msg->input_format : NULL,
					msg->has_output ? &msg->output_format : NULL,
					msg->has_name ? msg->name : NULL, msg->require_kinds,
					msg->deny_kinds);
			} else
				reply.result = B_NO_MEMORY;

			if (reply.result != B_OK)
				reply.count = 0;
			write_port(msg->reply_port, 0, &reply, sizeof(reply));
			if (reply.count > 0) {
				write_port(msg->reply_port, 0, infos,
					reply.count * sizeof(dormant_node_info));
			}
			delete[] infos;
			break;
		}

		case SERVER_GET_DORMANT_FLAVOR_INFO:
		{
			xfer_server_get_dormant_flavor_info *msg
				= (xfer_server_get_dormant_flavor_info *)data;
			dormant_flavor_info dormantFlavorInfo;
			status_t rv;

			rv = gNodeManager->GetDormantFlavorInfoFor(msg->addon,
				msg->flavor_id, &dormantFlavorInfo);
			if (rv != B_OK) {
				xfer_server_get_dormant_flavor_info_reply reply;
				reply.result = rv;
				write_port(msg->reply_port, 0, &reply, sizeof(reply));
			} else {
				size_t replySize
					= sizeof(xfer_server_get_dormant_flavor_info_reply)
						+ dormantFlavorInfo.FlattenedSize();
				xfer_server_get_dormant_flavor_info_reply* reply
					= (xfer_server_get_dormant_flavor_info_reply*)malloc(
						replySize);
				if (reply != NULL) {
					reply->type = dormantFlavorInfo.TypeCode();
					reply->flattened_size = dormantFlavorInfo.FlattenedSize();
					reply->result = dormantFlavorInfo.Flatten(
						reply->flattened_data, reply->flattened_size);

					write_port(msg->reply_port, 0, reply, replySize);
					free(reply);
				} else {
					xfer_server_get_dormant_flavor_info_reply reply;
					reply.result = B_NO_MEMORY;
					write_port(msg->reply_port, 0, &reply, sizeof(reply));
				}
			}
			break;
		}

		case SERVER_SET_NODE_CREATOR:
		{
			const server_set_node_creator_request* request
				= reinterpret_cast<const server_set_node_creator_request*>(
					data);
			server_set_node_creator_reply reply;
			status_t status = gNodeManager->SetNodeCreator(request->node,
				request->creator);
			request->SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_SHARED_BUFFER_AREA:
		{
			const server_get_shared_buffer_area_request *request
				= reinterpret_cast<
					const server_get_shared_buffer_area_request *>(data);
			server_get_shared_buffer_area_reply reply;

			reply.area = gBufferManager->SharedBufferListArea();
			request->SendReply(B_OK, &reply, sizeof(reply));
			break;
		}

		case SERVER_REGISTER_BUFFER:
		{
			const server_register_buffer_request *request
				= reinterpret_cast<const server_register_buffer_request *>(
					data);
			server_register_buffer_reply reply;
			status_t status;
			if (request->info.buffer == 0) {
				reply.info = request->info;
				// size, offset, flags, area is kept
				// get a new beuffer id into reply.info.buffer
				status = gBufferManager->RegisterBuffer(request->team,
					request->info.size, request->info.flags,
					request->info.offset, request->info.area,
					&reply.info.buffer);
			} else {
				reply.info = request->info; // buffer id is kept
				status = gBufferManager->RegisterBuffer(request->team,
					request->info.buffer, &reply.info.size, &reply.info.flags,
					&reply.info.offset, &reply.info.area);
			}
			request->SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_UNREGISTER_BUFFER:
		{
			const server_unregister_buffer_command *cmd = reinterpret_cast<
				const server_unregister_buffer_command *>(data);

			gBufferManager->UnregisterBuffer(cmd->team, cmd->buffer_id);
			break;
		}

		case SERVER_GET_MEDIA_FILE_TYPES:
		{
			const server_get_media_types_request& request
				= *reinterpret_cast<const server_get_media_types_request*>(
					data);

			server_get_media_types_reply reply;
			area_id area = gMediaFilesManager->GetTypesArea(reply.count);
			if (area >= 0) {
				// transfer the area to the target team
				reply.area = _kern_transfer_area(area, &reply.address,
					B_ANY_ADDRESS, request.team);
				if (reply.area < 0) {
					delete_area(area);
					reply.area = B_ERROR;
					reply.count = 0;
				}
			}

			status_t status = request.SendReply(
				reply.area < 0 ? reply.area : B_OK, &reply, sizeof(reply));
			if (status != B_OK) {
				// if we couldn't send the message, delete the area
				delete_area(reply.area);
			}
			break;
		}

		case SERVER_GET_MEDIA_FILE_ITEMS:
		{
			const server_get_media_items_request& request
				= *reinterpret_cast<const server_get_media_items_request*>(
					data);

			server_get_media_items_reply reply;
			area_id area = gMediaFilesManager->GetItemsArea(request.type,
				reply.count);
			if (area >= 0) {
				// transfer the area to the target team
				reply.area = _kern_transfer_area(area, &reply.address,
					B_ANY_ADDRESS, request.team);
				if (reply.area < 0) {
					delete_area(area);
					reply.area = B_ERROR;
					reply.count = 0;
				}
			} else
				reply.area = area;

			status_t status = request.SendReply(
				reply.area < 0 ? reply.area : B_OK, &reply, sizeof(reply));
			if (status != B_OK) {
				// if we couldn't send the message, delete the area
				delete_area(reply.area);
			}
			break;
		}

		case SERVER_GET_REF_FOR:
		{
			const server_get_ref_for_request* request
				= reinterpret_cast<const server_get_ref_for_request*>(data);
			server_get_ref_for_reply reply;
			entry_ref* ref;

			status_t status = gMediaFilesManager->GetRefFor(request->type,
				request->item, &ref);
			if (status == B_OK)
				reply.ref = *ref;

			request->SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_SET_REF_FOR:
		{
			const server_set_ref_for_request* request
				= reinterpret_cast<const server_set_ref_for_request*>(data);
			server_set_ref_for_reply reply;
			entry_ref ref = request->ref;

			status_t status = gMediaFilesManager->SetRefFor(request->type,
				request->item, ref);
			request->SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_REMOVE_REF_FOR:
		{
			const server_remove_ref_for_request* request
				= reinterpret_cast<const server_remove_ref_for_request*>(data);
			server_remove_ref_for_reply reply;

			status_t status = gMediaFilesManager->InvalidateItem(
				request->type, request->item);
			request->SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_REMOVE_MEDIA_ITEM:
		{
			const server_remove_media_item_request* request
				= reinterpret_cast<const server_remove_media_item_request*>(
					data);
			server_remove_media_item_reply reply;

			status_t status = gMediaFilesManager->RemoveItem(request->type,
				request->item);
			request->SendReply(status, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_READERS:
		{
			const server_get_readers_request *request
				= reinterpret_cast<const server_get_readers_request *>(data);
			server_get_readers_reply reply;
			rv = gAddOnManager->GetReaders(reply.ref, &reply.count,
				MAX_READERS);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_DECODER_FOR_FORMAT:
		{
			const server_get_decoder_for_format_request *request
				= reinterpret_cast<
					const server_get_decoder_for_format_request *>(data);
			server_get_decoder_for_format_reply reply;
			rv = gAddOnManager->GetDecoderForFormat(&reply.ref,
				request->format);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_WRITER_FOR_FORMAT_FAMILY:
		{
			const server_get_writer_request *request
				= reinterpret_cast<const server_get_writer_request *>(data);
			server_get_writer_reply reply;
			rv = gAddOnManager->GetWriter(&reply.ref, request->internal_id);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_FILE_FORMAT_FOR_COOKIE:
		{
			const server_get_file_format_request *request
				= reinterpret_cast<
					const server_get_file_format_request *>(data);
			server_get_file_format_reply reply;
			rv = gAddOnManager->GetFileFormat(&reply.file_format,
				request->cookie);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_CODEC_INFO_FOR_COOKIE:
		{
			const server_get_codec_info_request *request
				= reinterpret_cast<
					const server_get_codec_info_request *>(data);
			server_get_codec_info_reply reply;
			rv = gAddOnManager->GetCodecInfo(&reply.codec_info,
				&reply.format_family, &reply.input_format,
				&reply.output_format, request->cookie);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		case SERVER_GET_ENCODER_FOR_CODEC_INFO:
		{
			const server_get_encoder_for_codec_info_request *request
				= reinterpret_cast<
					const server_get_encoder_for_codec_info_request *>(data);
			server_get_encoder_for_codec_info_reply reply;
			rv = gAddOnManager->GetEncoder(&reply.ref, request->id);
			request->SendReply(rv, &reply, sizeof(reply));
			break;
		}

		default:
			printf("media_server: received unknown message code %#08lx\n",
				code);
	}
	TRACE("ServerApp::HandleMessage %#lx leave\n", code);
}


status_t
ServerApp::_ControlThread(void* _server)
{
	ServerApp* server = (ServerApp*)_server;

	char data[B_MEDIA_MESSAGE_SIZE];
	ssize_t size;
	int32 code;
	while ((size = read_port_etc(server->_ControlPort(), &code, data,
			sizeof(data), 0, 0)) > 0) {
		server->_HandleMessage(code, data, size);
	}

	return B_OK;
}


void
ServerApp::MessageReceived(BMessage* msg)
{
	TRACE("ServerApp::MessageReceived %lx enter\n", msg->what);
	switch (msg->what) {
		case MEDIA_SERVER_REQUEST_NOTIFICATIONS:
		case MEDIA_SERVER_CANCEL_NOTIFICATIONS:
		case MEDIA_SERVER_SEND_NOTIFICATIONS:
			gNotificationManager->EnqueueMessage(msg);
			break;

		case MEDIA_FILES_MANAGER_SAVE_TIMER:
			gMediaFilesManager->TimerMessage();
			break;

		case MEDIA_SERVER_GET_FORMATS:
			gFormatManager->GetFormats(*msg);
			break;

		case MEDIA_SERVER_MAKE_FORMAT_FOR:
			gFormatManager->MakeFormatFor(*msg);
			break;

		case MEDIA_SERVER_ADD_SYSTEM_BEEP_EVENT:
			gMediaFilesManager->HandleAddSystemBeepEvent(msg);
			break;
		default:
			BApplication::MessageReceived(msg);
			printf("\nmedia_server: unknown message received:\n");
			msg->PrintToStream();
			break;
	}
	TRACE("ServerApp::MessageReceived %lx leave\n", msg->what);
}


//	#pragma mark -


int
main()
{
	new ServerApp;
	be_app->Run();
	delete be_app;
	return 0;
}
