#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <winsock.h>
#include "Chat1.h"
#include "Encryption.h"
#include "BM.h"
#include "ecc.h"
#include "memory.h"
#include "utils.h"
#include "network.h"



//Dialog procedures
BOOL CALLBACK DlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM  lParam);
BOOL CALLBACK ConnectDlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM  lParam);
BOOL CALLBACK ListenDlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM  lParam);
    
int TryConnect(long hostname, int PortNo);
int ListenOnPort(int PortNo);

char Title[] = "ChatMate";

HINSTANCE hInst = NULL;
HWND hwnd, hStatus;

SOCKET s;
SOCKADDR_IN from;
int fromlen = sizeof(from);





int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	
	/*								TO DO
							_____________________

-Connections
 
	-	Create timer to maintain the network health (attempt a connection to each node on the list every 20 mins?)
	-	if a connection drops, remove it from the connection list.(the list is for active connections only).
	-	create counters to keep track of lists
	-	stop deleting nodes if the list goes under 1000
	-	max nodes == 20 000
	-	create Ping and Pong functions	
	-	start connecting to network?


-Messaging
	-	make sure all objects are stored as vectors when we receive them or create them.
	-	create a list for Address's that is seperate from the Vectors list
	-	modify appropriate functions to reflect the above change.
	-	create the function to handle pubkey incoming objects
	-	finish the handling of public keys (sending & receiving)

	
							_____________________
	*/				



	Memory::init();
	
	BM::init();
	
	Encryption::init();
	
	ECC::init();

	network::init();

	//PBM_MSG_ADDR addr = NULL;
	//PBM_ENC_PL_256 pl = (PBM_ENC_PL_256)ALLOC_(1024);
	//DWORD pl_size = 1024;


	//BM::create_addr(&addr);
	//
	//BM::encrypt_payload(addr, (LPBYTE)"testtesttesttest", 16, pl, &pl_size);

	//BM::decrypt_payload(addr, pl, sizeof(BM_ENC_PL_256));

	
	
	//ZEROFREE_(pl, 1024);



	// Initialize the dialog box and begin the connecting to the BM network via network::start();
	hInst = hInstance;

	return DialogBox(hInstance, MAKEINTRESOURCE(DLG_MAIN), NULL, DlgProc);



	return FALSE;
}

void GetTextandAddLine(char Line[], HWND hParent, int IDC)
{
    HWND hEdit = GetDlgItem(hParent, IDC);
	int nTxtLen = GetWindowTextLength(hEdit); // Get length of existing text
	SendMessage(hEdit, EM_SETSEL, nTxtLen, nTxtLen);	// move caret to end
	SendMessage(hEdit, EM_REPLACESEL, 0, (LPARAM)Line);	    // append text
	SendMessage(hEdit, EM_SCROLLCARET, 0, 0);		// scroll to caret
} //End function   

BOOL CALLBACK DlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM
  lParam)
{

	SOCKET _s = 0;
	PBM_CONN conn = 0;

switch(Message)
    {
    
    case WM_INITDIALOG:
    {
        //Our dialog box is being created
		hwnd = hdwnd;
		BM::main_hwnd = hdwnd;

		
		network::start(BM::main_hwnd);


        hStatus = GetDlgItem(BM::main_hwnd, ID_STATUS_MAIN);
    }
    return TRUE;
	









    //Winsock related message...
	//
	//
	//
	
    case BM_WND_MSG:

		_s = wParam;
		conn = network::find_conn(_s);

        switch (lParam)
        {
                case FD_CONNECT: //Connected OK
                    MessageBeep(MB_OK);
               
                break;
                
                case FD_CLOSE: //Lost connection
                    MessageBeep(MB_ICONERROR);
                    
                    //Clean up
                    if (s) closesocket(s);
                    WSACleanup();

                break;
                
                case FD_READ: //Incoming data to receive
				{
					
					
					network::start_work(conn);


					break;
				}
                case FD_ACCEPT: //Connection request
				{
					MessageBeep(MB_OK);

					break;
				}
        }
    break;






















    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
                case ID_BTN_CONNECT:
                  return DialogBox(hInst, MAKEINTRESOURCE(DLG_CONNECT), NULL, ConnectDlgProc);
                break;

                case ID_BTN_LISTEN:
                  return DialogBox(hInst, MAKEINTRESOURCE(DLG_LISTEN), NULL, ListenDlgProc);                
                break;
                
                case ID_BTN_CLEAR: //Clear edit and disconnect
                {
                    if (s) //If there's a connection
                    {
                        int a = MessageBox(hdwnd, "Are you sure you want to end the current connection?", "End Connection", MB_ICONQUESTION | MB_YESNO);
                        
						if (a == IDYES)
                        {
                            SendDlgItemMessage(hdwnd, ID_EDIT_DATA, WM_SETTEXT, 0, (LPARAM)"");
                            closesocket(s); //Shut down socket
                            WSACleanup(); //Clean up Winsock
                        }
                    }
                }
                break;
                
                case ID_BTN_SEND: //Send data
                {
                    int len = GetWindowTextLength(GetDlgItem(hdwnd, ID_EDIT_SEND));
			        
          	        if (len && len < MAX_PATH - sizeof(char)) //If there's text in the reply box...
			        {
			            if (s)
			            {

							PBM_MSG_HDR msg = (PBM_MSG_HDR)GlobalAlloc(GPTR, sizeof(BM_MSG_HDR) + sizeof(BM_PL_VER) + 1);

							//BM::init_con(msg, "178.254.29.171", "127.0.0.1");

							send(s, (const char*)msg, sizeof(BM_MSG_HDR) + sizeof(BM_PL_VER), NULL);


							GlobalFree((HANDLE)msg); //Free the memory: Important!!




			            }
			            else
			            {
			            //We aren't connected!!
			            MessageBox(hwnd, "No established connection detected.",
			              Title, MB_ICONERROR | MB_OK);
			            }
			        }
                }
                break;
                
                case IDCANCEL:
                    //Clean up
                    if (s) closesocket(s);
                    WSACleanup();
                    
                    EndDialog(hdwnd, IDOK);
                break;
        } //End switch
        default:
            return FALSE;
    break;
    } //End Message switch
    return TRUE;
}

BOOL CALLBACK ConnectDlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM
  lParam)
{
switch(Message)
    {
    case WM_INITDIALOG:
    {
        //Our dialog box is being created
    }
    return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
                case ID_BTN_GO:
                {
                    int len = GetWindowTextLength(GetDlgItem(hdwnd, ID_EDIT_HOST));
                    int lenport = GetWindowTextLength(GetDlgItem(hdwnd, ID_EDIT_PORT));
                    
                    if (!lenport) return 0; //Was the port specified?
                    
                    int portno = GetDlgItemInt(hdwnd, ID_EDIT_PORT, 0, 0);
                    
                    if (len)
                    {
                            char* Data;
                            Data = (char*)GlobalAlloc(GPTR, len + 1); //Allocate memory
                    
                            GetDlgItemText(hdwnd, ID_EDIT_HOST, Data, len + 1); //Get text into buffer
                    
                            if (!gethostbyname(Data))
                            {
                            //Couldn't get hostname; assume it's an IP Address
                                long hostname = inet_addr(Data);
                                if(!TryConnect(hostname, portno))
                                {
                                    MessageBox(hdwnd, "Could not connect to remote host.", Title, MB_ICONERROR | MB_OK);
                                    if (s) closesocket(s); //Shut down socket
                                }
                            }
                            
                            GlobalFree((HANDLE)Data); //Free memory
                            
                            EndDialog(hdwnd, IDOK);
                    }
                }
                break;

                case IDCANCEL:
                    EndDialog(hdwnd, IDOK);
                break;
        } //End switch
        default:
            return FALSE;
    break;
    } //End Message switch
    return TRUE;
}

BOOL CALLBACK ListenDlgProc(HWND hdwnd, UINT Message, WPARAM wParam, LPARAM
  lParam)
{
switch(Message)
    {
    case WM_INITDIALOG:
    {
        //Our dialog box is being created
    }
    return TRUE;

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
                case ID_BTN_GO:
                {
                    int lenport = GetWindowTextLength(GetDlgItem(hdwnd, ID_EDIT_PORT));
                    if (!lenport) return 0; //Was the port specified?
                    
                    int portno = GetDlgItemInt(hdwnd, ID_EDIT_PORT, 0, 0);
                    
                    if (!ListenOnPort(portno)) 
                    {
                        if (s) closesocket(s);
                        MessageBox(hdwnd, "Error listening on specified port.", Title, MB_ICONERROR | MB_OK);
                    }                                                            
                    EndDialog(hdwnd, IDOK);
                }
                break;

                case IDCANCEL:
                    EndDialog(hdwnd, IDOK);
                break;
        } //End switch
        default:
            return FALSE;
    break;
    } //End Message switch
    return TRUE;
}



// Programming Windows TCP Sockets in C++ for the Beginner -  lol.
// http://www.codeproject.com/Articles/13071/Programming-Windows-TCP-Sockets-in-C-for-the-Begin

int TryConnect(long hostname, int PortNo)
{
    WSADATA w; //Winsock startup info
    SOCKADDR_IN target; //Information about host
    
    int error = WSAStartup (0x0202, &w);   // Fill in WSA info
     
    if (error)
    { // there was an error
      return 0;
    }
    if (w.wVersion != 0x0202)
    { // wrong WinSock version!
      WSACleanup (); // unload ws2_32.dll
      return 0;
    }
    
    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create socket
    if (s == INVALID_SOCKET)
    {
        return 0;
    }

    target.sin_family = AF_INET;           // address family Internet
    target.sin_port = htons (PortNo);        // set server’s port number
    target.sin_addr.s_addr = hostname;  // set server’s IP
     
    //Try connecting...
    if (connect(s, (SOCKADDR *)&target, sizeof(target)) == SOCKET_ERROR) //Try binding
    { // error
          return 0;
    }      
	
	
	//Switch to Non-Blocking mode
    WSAAsyncSelect (s, hwnd, 1045, FD_READ | FD_CONNECT | FD_CLOSE); 

      
	// register the connection
	PBM_CONN conn = (PBM_CONN)ALLOC_(sizeof(BM_CONN));

	// set the id of the connection context
	conn->id = network::cur_conn_id;

	//increment it for the next connection.
	network::cur_conn_id++;



    SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)"Connected to Remote Host.");
    
    return 1; //OK
}

int ListenOnPort(int PortNo)
{
    WSADATA w;
    
    int error = WSAStartup (0x0202, &w);   // Fill in WSA info
     
    if (error)
    { // there was an error
        SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)"Could not initialize Winsock.");
      return 0;
    }
    if (w.wVersion != 0x0202)
    { // wrong WinSock version!
      WSACleanup (); // unload ws2_32.dll
      SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)"Wrong Winsock version.");
      return 0;
    }
    
    SOCKADDR_IN addr; // the address structure for a TCP socket
    SOCKET client; //The connected socket handle
    
    addr.sin_family = AF_INET;      // Address family Internet
    addr.sin_port = htons (PortNo);   // Assign port to this socket
    addr.sin_addr.s_addr = htonl (INADDR_ANY);   // No destination
    
    s = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP); // Create socket
    
    if (s == INVALID_SOCKET)
    {
        SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)"Could not create socket.");
        return 0;
    }
    
    if (bind(s, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR) //Try binding
    { // error
        SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)"Could not bind to IP.");
        return 0;
    }
    
    listen(s, 10); //Start listening
    WSAAsyncSelect (s, hwnd, 1045, FD_READ | FD_CONNECT | FD_CLOSE | FD_ACCEPT); //Switch to Non-Blocking mode
    
    char szTemp[100];
    wsprintf(szTemp, "Listening on port %d...", PortNo);
    
    SendMessage(hStatus, WM_SETTEXT, 0, (LPARAM)szTemp);  
    return 1;
}


