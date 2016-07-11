#include "stdafx.h"

#ifndef BM_NETWORK_C
#define BM_NETWORK_C

#include "network.h"
#include "bm.h"
#include "memory.h"
#include "utils.h"


DWORD network::cur_conn_id = 0;

BM_SEED network::seed_list[16] = {
	{ "178.254.29.171", "8444" },
	{ "178.254.29.171", "8444" }
};

PBM_CONN_LIST network::con_list = NULL;



//
//
//	Initialization
//
//


void network::init()
{

	network::con_list = (PBM_CONN_LIST)ALLOC_(BM_MAX_CONNECTIONS * sizeof(PBM_CONN));
	ZERO_(network::con_list, BM_MAX_CONNECTIONS * sizeof(PBM_CONN));


}



//
//
//	Connection Functions
//
//

int network::start(HWND hwnd)
{

	long to_ip = NULL;
	int to_port = NULL;

	SOCKET s = NULL;
	PBM_CONN last_conn = NULL;
	PBM_MSG_HDR packet = NULL;
	DWORD packet_size = NULL;


	PBM_CONN new_conn = NULL;
	for (int i = 0; i < 1; i++) //only attempt 1 connection for now.
	{
		//	enumerate seed list and convert appropriately.
		new_conn = NULL;
	
		to_ip = inet_addr(network::seed_list[i].ip);
		to_port = atoi(network::seed_list[i].port);

		//	Attempt Connection
		network::connect(hwnd, &new_conn, to_ip, to_port);

		if (new_conn) 
		{

			OutputDebugStringA("\n\n--Found Peer--\n\n");

			// initiallize first packet
			
			packet_size = BM::init_con(&packet, to_ip, to_port);
		
			OutputDebugStringA("\n\n--Sending off Version--\n\n");
			

			send(new_conn->s, (const char*)packet, packet_size, NULL);
			

			OutputDebugStringA("\n\n--Creating Work Thread--\n\n");

	//		CreateThread(0, 0,(LPTHREAD_START_ROUTINE)network::start_work, last_conn, 0, 0);

			ZEROFREE_(packet, packet_size);
						
		}

	}
	
	return TRUE;
}

int network::connect(HWND hwnd, PBM_CONN* out, long hostname, int PortNo)
{
	SOCKET s = NULL;
	WSADATA w; //Winsock startup info
	SOCKADDR_IN target; //Information about host

	int error = WSAStartup(0x0202, &w);   // Fill in WSA info

	if (error)
	{ // there was an error
		return 0;
	}

	

	if (w.wVersion != 0x0202)
	{ // wrong WinSock version!
		WSACleanup(); // unload ws2_32.dll
		return 0;
	}

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create socket
	if (s == INVALID_SOCKET)
	{
		return 0;
	}

	target.sin_family = AF_INET;           // address family Internet
	target.sin_port = htons(PortNo);        // set server’s port number
	target.sin_addr.s_addr = hostname;  // set server’s IP
	
										//Try connecting...
	if (connect(s, (SOCKADDR *)&target, sizeof(target)) == SOCKET_ERROR) //Try binding
	{ // error
		return 0;
	}


	//Switch to Non-Blocking mode
	WSAAsyncSelect(s, hwnd, BM_WND_MSG, FD_READ | FD_CONNECT | FD_CLOSE);

	// ---

	*out = network::reg_conn(s, hostname, NULL, PortNo);
	

	return 1; //OK
}

PBM_CONN network::reg_conn(SOCKET s, long ipv4, LPBYTE ipv6, int PortNo)
{

	PBM_CONN conn = (PBM_CONN)ALLOC_(sizeof(BM_CONN));
	ZERO_(conn, sizeof(BM_CONN));

	int i = 0;
	int found = FALSE;

	do
	{

		if (!network::con_list->list[i])
		{
			network::con_list->list[i] = conn;
			found = TRUE;
			break;
		}
		i++;

	} while (i < BM_NODE_LIST_SIZE);


	if (found)
	{

		conn->id = network::cur_conn_id;

		conn->ipv4 = ipv4;

		if (ipv6)
		{
			memcpy_s(conn->ipv6, 16, ipv6, 16);
			conn->ipv4 = NULL;
		}

		conn->port = PortNo;
		conn->s = s;
		conn->time_started = BM::unix_time();
		conn->status = BMCS_INITIAL;

		network::cur_conn_id++;

	}
	else {

		ZEROFREE_(conn, sizeof(BM_CONN));
		conn = FALSE;
	}

	return conn;
}

PBM_CONN network::add_conn(SOCKET s, long to_ip, int to_port)
{
	network::cur_conn_id++;
	BOOL found = FALSE;
	PBM_CONN conn = NULL;

	for (int i = 0; i < 512; i++)
	{
		// find an empty entry
		if (!network::con_list->list[i]->ipv4 && !network::con_list->list[i]->status)
		{
			//	cleans the code up.
			conn = network::con_list->list[i];

			// set the information
			conn->id = network::cur_conn_id++;
			conn->ipv4 = to_ip;

			memset(conn->ipv6, 0, 16);

			conn->port = to_port;
			conn->s = s;
			conn->status = TRUE;
			conn->time_started = BM::unix_time();

			found = TRUE;
			break;
		}

	}

	if (found)
		return conn;

	return FALSE;
}

PBM_CONN network::find_conn(SOCKET s)
{

	PBM_CONN conn = NULL;

	for (int i = 0; i < BM_MAX_CONNECTIONS; i++)
	{
		conn = network::con_list->list[i];

		if (conn && conn->s == s)
		{

			return conn;
			break;
		}
	}

	return FALSE;
}



//
//
//	Thread Functions
//
//

void network::start_work(PBM_CONN in)
{

	LPBYTE in_buffer = (LPBYTE)ALLOC_(BM_RECV_BUFF_SIZE);
	ZERO_(in_buffer, BM_RECV_BUFF_SIZE);

	DWORD msg_type = NULL;
	DWORD con_ret = NULL;


	PBM_MSG_HDR msg_hdr = (PBM_MSG_HDR)in_buffer;
	LPSTR command = (LPSTR)msg_hdr->command;

	ULONG time = BM::unix_time();
		

	con_ret = recv(in->s, (char*)in_buffer, BM_RECV_BUFF_SIZE, NULL);
		
	if ((*(uint32_t*)msg_hdr->magic) != BM_MAGIC)
		return;

	DBGOUTw(L"\r\r--\r\r");
	do
	{
		if (con_ret != SOCKET_ERROR)
		{


			if (!lstrcmpA(command, "version"))
			{
				DBGOUTa(command);
				//	verify version
				//	send verack packet in response
				msg_type = BM_MT_VERSION;

				//  check if this is valid
				if (!(in->status | BMCS_INITIAL)) return;

				if (!BM::verify_version((PBM_PL_VER)msg_hdr->payload))
				{

					DBGOUTw(L"\r\r--Version failed to verify--\r\r");
					break;
					// close connection

				}
				else {

					//	success
					//	send verack

					ZERO_(in_buffer, BM_RECV_BUFF_SIZE);

					BM::init_verack(msg_hdr);

					send(in->s, (const char*)msg_hdr, sizeof(BM_MSG_HDR), NULL);

				}


			}
			else if (!lstrcmpA(command, "verack"))
			{
				DBGOUTa(command);
				//	Received and accpeted my version complete
				msg_type = BM_MT_VERACK;

				in->verack = TRUE;
				in->status = BMCS_CONNECTED;

			}
			else if (!lstrcmpA(command, "addr"))
			{
				DBGOUTa(command);
				//	to be received in response to sending out a verack msg
				//	parse the incoming list of known IPs
				//	add them to the main peer list
				//	
				//	also send out a random list of known IPs
				size_t int_len = NULL;
				uint64_t payload_size = BM::decodeVarint(msg_hdr->length, 6, &int_len);

				if (payload_size > 1024 * 50)
					break;

				BM::receive_addr_list(msg_hdr->payload, payload_size);
				
				msg_type = BM_MT_ADDR;


			}
			else if (!lstrcmpA(command, "inv"))
			{
				DBGOUTa(command);
				//	to be received in response to sending out an addr msg
				//	parse the incoming list
				//	do something with interesting vectors otherwise dont do anything
				//	only store objects that are relevent or that we receive until they expire.




				//size_t int_len = 0;
				//LPBYTE v_list = NULL;
				//uint64_t v = BM::decodeVarint(msg_hdr->payload, 6, &int_len);

				//if (v > 30000 || int_len > 5)
				//	return;

				//v_list = (LPBYTE)&msg_hdr->payload[int_len];


				//for (int i = 0; i < v * 32; i += 32)
				//{
				//	if (!v_list[i])
				//		continue;

				//	BM::list_add_vector(&v_list[i]);
				//}

				msg_type = BM_MT_INV;


			}
			else if (!lstrcmpA(command, "getdata"))
			{
				DBGOUTa(command);
				//	in response to sending an INV message
				//	requesting the content of an object withen the main inventory list


				msg_type = BM_MT_GETDATA;


			}
			else if (!lstrcmpA(command, "object"))
			{
				DBGOUTa(command);
				//	The only message that is propagated throughout the network entirely.
				//
				//	Create Inventory hash by taking the first 32 bytes of SHA512(SHA512(object))
				//	as well as store the hash along with object in the vectors list.


				msg_type = BM_MT_OBJECT;

				size_t int_len = NULL;

				BM::process_object((PBM_OBJECT)msg_hdr->payload, BM::decodeVarint(msg_hdr->length, 8, &int_len));

			}


		}

		break;
	}while (1);

	ZEROFREE_(in_buffer, BM_RECV_BUFF_SIZE);
		


	
	//ExitThread(0);
}

int network::handle_msg(PBM_CONN conn_, PBM_MSG_HDR in, DWORD in_size, DWORD msg_type)
{

	LPVOID payload = in->payload;
	DWORD payload_size = htonl(TYPEVAL(uint32_t, in->length));

	switch (msg_type)
	{

		//	First
	case BM_MT_VERACK:
		//	Set the Acknowledgement
		conn_->verack = BM_VERACK_SENT;

		break;

		//	Second
	case BM_MT_VERSION:

	{

		BOOL passed = handle_version(conn_, (PBM_PL_VER)payload);

		if (passed && conn_->verack == BM_VERACK_SENT)
		{
			conn_->verack = BM_VERACK_RECV;
			conn_->status = TRUE;
		}

		break;
	}

		//	Third
	case BM_MT_ADDR:
		// swap peers
		if (network::handle_addr(conn_, payload, payload_size))
		{
			conn_->peerswap = TRUE;
		}

		break;

		//	Fourth
	case BM_MT_INV:
		// receive list of inventory vectors
		// push out the vectors of all the current objects stored

		if (network::handle_inv(conn_, payload, payload_size))
		{
			conn_->invswap = TRUE;
		}

		break;

		//	Dynamic
	case BM_MT_GETDATA:
		// push out requested data otherwise propogate the object
		break;

		//	Dynamic
	case BM_MT_OBJECT:

		//	--Object Types
		//
		//	- getpubkey
		//	- pubkey
		//	- msg					<- encrypted
		//	- broadcast	(only v4-5)	<- encrypted <- this only suports v3 so not enabled

		BM::process_object((PBM_OBJECT)payload, payload_size);

		break;

	default:
		//	Fail
		return FALSE;
		break;
	}


	return TRUE;

}

int network::handle_version(PBM_CONN in, PBM_PL_VER vers)
{
	uint32_t version = htonl(TYPEVAL(uint32_t, vers->version));
	
	//	not much to do here other then verify version
	//	todo
	//	-> parse the user agent
	//	-> verify timestamp
	//	-> nonce
	//	-> stream #'s // not implemented by bitmessage yet <= v5

	//
	if (version < 2)
		return FALSE;

	DWORD hdr_s = sizeof(BM_MSG_HDR);

	PBM_MSG_HDR msg = (PBM_MSG_HDR)GlobalAlloc(LPTR, hdr_s);

	BM::init_verack(msg);

	send(in->s, (const char*)msg, hdr_s, NULL);

	ZeroMemory(msg, hdr_s);
	GlobalFree(msg);


	return TRUE;
}

int network::handle_addr(PBM_CONN conn, LPVOID in, DWORD in_size)
{
	if (!in || !in_size)
		return FALSE;

	size_t int_len = NULL;

	uint64_t count = BM::decodeVarint((uint8_t*)in, in_size, &int_len);

	if (count < 1)
		return FALSE;

	PBM_ADDR addr_list = (PBM_ADDR)in + (ULONG_PTR)int_len;

	for (int i = 0; i < count; i++)
	{
		//	check if node is already in the list

		if (!network::list_find_node(addr_list[i].ip, NULL))
			network::list_add_node(&addr_list[i]); // add it if not

	}


	// should send back list of IPs
	// spec says "should" so maybe we dont have to ??
	// send(conn->s, 0, 0, 0);

	return TRUE;
}

DWORD network::handle_inv(PBM_CONN conn, LPVOID in, DWORD in_size)
{

	if (!in || !in_size)
		return FALSE;

	size_t int_len = NULL;

	uint64_t count = BM::decodeVarint((uint8_t*)in, in_size, &int_len);

	if (count < 1)
		return FALSE;

	PBM_VECTOR vector = (PBM_VECTOR)in + (ULONG_PTR)int_len;

	for (int i = 0; i < count; i++)
	{
		//	check if node is already in the list

		if (!BM::list_find_vector(vector->hash))
			BM::list_add_vector(vector->hash); // add it if not

	}


	// should send back list of vectors
	// spec says "should" so maybe we dont have to ??
	// send(conn->s, 0, 0, 0);

	return TRUE;

}



//
//
//	List Functions
//
//

PBM_ADDR network::list_add_node(PBM_ADDR in)
{
	PBM_ADDR entry = NULL;
	

	for (int i = 0; i < BM_N_NODE_SLOTS; i++)
	{

		entry = BM::node_list->list[i];

		if (!entry->time)
		{

			memcpy_s(entry, sizeof(BM_ADDR), in, sizeof(BM_ADDR));
			break;
		}
		
	}

	return entry;
}

PBM_ADDR network::list_find_node(LPBYTE ip, DWORD* node_list_id)
{

	DWORD in_id = -1;
	
	if (node_list_id)
		in_id = *node_list_id;

	PBM_ADDR entry = NULL;

	if (
		!ip &&							
		in_id != -1 &&
		in_id < BM_N_NODE_SLOTS &&
		BM::node_list->list[in_id]->time
		)
	{
		
		entry = BM::node_list->list[in_id];

	}
	else if(ip) {

		for (int i = 0; i < BM_N_NODE_SLOTS; i++)
		{

			entry = BM::node_list->list[i];

			if (!memcmp(entry->ip, ip, 16));
			{
				*node_list_id = i;
				break;
			}

		}

	}

	return entry;
}

int network::list_remove_node(LPBYTE ip, DWORD node_list_id)
{

	DWORD id = node_list_id;

	PBM_ADDR entry = list_find_node(ip, &id);

	if (entry)
	{
		ZeroMemory(entry, sizeof(BM_ADDR));
		return TRUE;
	}

	return FALSE;
}

int network::list_count_nodes(void)
{

	DWORD count = 0;
	PBM_ADDR entry = NULL;


	for (int i = 0; i < BM_N_NODE_SLOTS; i++)
	{

		entry = BM::node_list->list[i];

		if (entry->time)
			count++;
	
	}

	return count;
}













#endif