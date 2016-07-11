#pragma once
#include "stdafx.h"

#ifndef BM_NETWORK_H
#define BM_NETWORK_H

namespace network {

	extern DWORD cur_conn_id;
	extern BM_SEED seed_list[16];
	extern PBM_CONN_LIST con_list;

	void init();
	//	connection function
	int start(HWND hwnd);
	int connect(HWND hwnd, PBM_CONN* out, long hostname, int PortNo);
	
	PBM_CONN reg_conn(SOCKET s, long ipv4, LPBYTE ipv6, int PortNo);
	PBM_CONN find_conn(SOCKET s);
	PBM_CONN add_conn(SOCKET s, long to_ip, int to_port);
	int remove_conn(DWORD id);
	int count_conn(void);


	//	threads
	void start_work(PBM_CONN in);


	// Message Handling functions

	int handle_msg(PBM_CONN conn_, PBM_MSG_HDR in, DWORD in_size, DWORD msg_type);

	int handle_version(PBM_CONN in, PBM_PL_VER vers);
	//int handle_verack();
	int handle_addr(PBM_CONN conn, LPVOID in, DWORD in_size);
	DWORD handle_inv(PBM_CONN conn, LPVOID in, DWORD in_size);
	int handle_getdata();
	int handle_object();


	//	Object Handling Functions

	int handle_obj_getpubkey();
	int handle_obj_pubkey();
	int handle_obj_msg();
	int handle_obj_broadcast();


	// peer list handling functions



	PBM_ADDR list_add_node(PBM_ADDR in);
	int list_remove_node(LPBYTE ip, DWORD node_list_id);
	PBM_ADDR list_find_node(LPBYTE ip, DWORD* node_list_id);
	int list_count_nodes(void);




}
















#endif // !BM_NETWORK_H