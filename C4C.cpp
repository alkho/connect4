/*
 * A simple TCP client that sends messages to a server and display the message
   from the server. 
 * For use in CPSC 441 lectures
 * Instructor: Prof. Mea Wang
 */

#include <iostream>
#include <stdio.h>
#include <sys/socket.h> // for socket(), connect(), send(), and recv()
#include <arpa/inet.h>  // for sockaddr_in and inet_addr()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()

using namespace std;

const int BUFFERSIZE = 64;   // Size the message buffers
const int GRID_WIDTH = 7;
const int GRID_HEIGHT = 6;

void print_grid(int[][GRID_HEIGHT]);
void place_position(int, int[][GRID_HEIGHT], int);

int main(int argc, char *argv[])
{
    int sock;                        // A socket descriptor
    struct sockaddr_in serverAddr;   // Address of the server
    char inBuffer[BUFFERSIZE];       // Buffer for the message from the server
    int bytesRecv;                   // Number of bytes received

    int grid[GRID_WIDTH][GRID_HEIGHT];    
    char outBuffer[BUFFERSIZE];      // Buffer for message to the server
    int msgLength;                   // Length of the outgoing message
    int bytesSent;                   // Number of bytes sent
    int player = 2;
    int column = -1;
    int mode = 0;

    // Check for input errors
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <Server IP> <Server Port>" << endl;
        exit(1);
    }

    for (int i = 0; i < GRID_HEIGHT ; i++)
      for (int j = 0; j < GRID_WIDTH; j++)
	grid[j][i] = -1;

    // Create a TCP socket
    // * AF_INET: using address family "Internet Protocol address"
    // * SOCK_STREAM: Provides sequenced, reliable, bidirectional, connection-mode byte streams.
    // * IPPROTO_TCP: TCP protocol
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        cout << "socket() failed" << endl;
        exit(1);
    }

    // Free up the port before binding
    // * sock: the socket just created
    // * SOL_SOCKET: set the protocol level at the socket level
    // * SO_REUSEADDR: allow reuse of local addresses
    // * &yes: set SO_REUSEADDR on a socket to true (1)
    // * sizeof(int): size of the value pointed by "yes"
    int yes = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
    {
        cout << "setsockopt() failed" << endl;
        exit(1);
    }
    
    // Initialize the server information
    // Note that we can't choose a port less than 1023 if we are not privileged users (root)
    memset(&serverAddr, 0, sizeof(serverAddr));         // Zero out the structure
    serverAddr.sin_family = AF_INET;                    // Use Internet address family
    serverAddr.sin_port = htons(atoi(argv[2]));         // Server port number
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);    // Server IP address
    
    // Connect to the server
    // * sock: the socket for this connection
    // * serverAddr: the server address
    // * sizeof(*): the size of the server address
    if (connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
    {
        cout << "connect() failed" << endl;
        exit(1);
    }
        
    //GAME START
    cout << "---------Let's Play Connect 4 -----------" << endl;
    cout << "Select a gameplay mode" << endl;
    cout << "1: Single Player " << endl;
    cout << "2: Multiplayer " << endl;
    
    while(outBuffer[0] != '1' && outBuffer[0] != '2'){
      cout << "Choice: ";
      fgets(outBuffer, BUFFERSIZE, stdin);
    }

    outBuffer[3] = '\n';
    mode = outBuffer[2] =  outBuffer[0];
    outBuffer[1] = ' ';
    outBuffer[0] = '2';
    mode -= 48;

    msgLength = strlen(outBuffer);
    
    // Send the message to the server
    bytesSent = send(sock, (char *) &outBuffer, msgLength, 0);
    if (bytesSent < 0 || bytesSent != msgLength)
      {
	cout << "error in sending" << endl;
	exit(1); 
      }
      
    while (strncmp(outBuffer, "logout", 6) != 0 )
      {

	cout << "\nPlease wait for your opponent ";
	// Receive the response from the server                             
	bytesRecv = recv(sock, (char *) &inBuffer, BUFFERSIZE, 0);
	
	if (inBuffer[0] == '0'){
	  grid[column][inBuffer[2] - 48] = 0;
	  if (mode == 1)
	    grid[inBuffer[4] - 48][inBuffer[5] - 48] = 1;
	  print_grid(grid);
	}
	else if(inBuffer[0] == '1'){
	  grid[column][inBuffer[2] - 48] = 1;
	  print_grid(grid);
	}
	else if(inBuffer[0] == '3'){
	  grid[inBuffer[2] - 48][inBuffer[3] - 48] = player;
	  print_grid(grid);
	  cout << "CONGRATULATION!!!\nYou've won" << endl;
	  exit(0);
	  }
	else if(inBuffer[0] == '4'){
	  grid[inBuffer[2] - 48][inBuffer[3] - 48] = !player;
	  print_grid(grid);
	  cout << "TRY AGAIN!!\nYou've been defeated" <<endl;
	  exit(0);
	}
	else if(inBuffer[0] == '5'){

	  if(mode == 2){
	    if(player == 0)
	      grid[inBuffer[2] - 48][inBuffer[3] - 48] = 1;
	    if(player == 1)
	      grid[inBuffer[2] - 48][inBuffer[3] - 48] = 0;
	    print_grid(grid);
	  }
	  	  
	  // Clear the buffers
	  memset(&outBuffer, 0, BUFFERSIZE);
	  memset(&inBuffer, 0, BUFFERSIZE);
	  cout << "\nPlease enter your choice: ";
	  fgets(outBuffer, BUFFERSIZE, stdin);
	  column = outBuffer[0] - 97;
	  outBuffer[3] = '\n';
	  outBuffer[2] = outBuffer[0];
	  outBuffer[1] = ' ';
	  outBuffer[0] = player + '0';
	  
	  msgLength = strlen(outBuffer);
  
	  // Send the message to the server
	  bytesSent = send(sock, (char *) &outBuffer, msgLength, 0);
	  
	  if (bytesSent < 0 || bytesSent != msgLength)
	    {
	      cout << "error in sending" << endl;
	      exit(1); 
	    }
	  
       	}
	else if(inBuffer[0] == '6'){
	  cout << "Waiting for opponent. Please wait. " << endl;
	}
	else if (inBuffer[0] == '7'){
	  player = (int)inBuffer[2] - 48;
	}
	else if (inBuffer[0] == '8'){
	  cout << "\nInvalid input, please try again: ";
	  fgets(outBuffer, BUFFERSIZE, stdin);
	  column = outBuffer[0] - 97;
	  outBuffer[3] = '\n';
	  outBuffer[2] = outBuffer[0];
	  outBuffer[1] = ' ';
	  outBuffer[0] = player + '0';

	  msgLength = strlen(outBuffer);
  
	  // Send the message to the server
	  bytesSent = send(sock, (char *) &outBuffer, msgLength, 0);
	  
	  if (bytesSent < 0 || bytesSent != msgLength)
	    {
	      cout << "error in sending" << endl;
	      exit(1); 
	    }
	}
	else if (inBuffer[0] == '9'){
	  cout << "Game Fatal Error. Your game might have exited for the following reasons:\n1. Too many players (max 2)\n2. You have selected multiplayer and there is no player 2 connected yet\n3. A player has disconnected" <<endl;
	  exit(1);
	}
	else if (inBuffer[0] == 'H'){
	  grid[inBuffer[2] - 48][inBuffer[3] - 48] = 1;
	  print_grid(grid);
	  cout << "It's a tie!!!" << endl;
	  exit(0);
	}

	memset(&outBuffer, 0, BUFFERSIZE);                                    
	memset(&inBuffer, 0, BUFFERSIZE);                                     
	
	msgLength = strlen(outBuffer);             	
	
      }

    // Close the socket
    close(sock);
    exit(0);
}

void print_grid(int grid[][GRID_HEIGHT]){
  
  cout << "\n\n A B C D E F G " <<endl;
  
  for (int i = 0; i < GRID_HEIGHT; i++){
    for (int j = 0; j < GRID_WIDTH; j++){
      if (grid[j][i] == -1)
	cout << " _";
      else if(grid[j][i] == 0)
	cout << " X";
      else if(grid[j][i] == 1)
	cout << " O";
      else
	cout << grid[j][i];
    }
    
    cout << endl;

  }

}//End print_grid

void place_position(int column, int grid[][GRID_HEIGHT], int player){
   
  for (int i = GRID_HEIGHT - 1; i>=0; i--)
    if (grid[column][i] == -1){
      if (player == 0)
	grid[column][i] = 0;
      if (player == 1)
	grid[column][i] = 1;
      break;
    }
}//End place_position

