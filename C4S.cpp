/*
 * A simple TCP select server that accepts multiple connections and echo message back to the clients
 * For use in CPSC 441 lectures
 * Instructor: Prof. Mea Wang
 */
 
#include <iostream>
#include <sys/socket.h> // for socket(), connect(), send(), and recv()
#include <arpa/inet.h>  // for sockaddr_in and inet_addr()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()
#include <stdio.h>
#include <sstream>

using namespace std;

const int BUFFERSIZE = 64;   // Size the message buffers
const int MAXPENDING = 10;    // Maximum pending connections
const int GRID_WIDTH = 7;
const int GRID_HEIGHT = 6;

fd_set recvSockSet;   // The set of descriptors for incoming connections
int maxDesc = 0;      // The max descriptor
bool terminated = false; 
int player_num = -1;  
int grid[GRID_WIDTH][GRID_HEIGHT]; 
int mode = -1;
int whos_turn = 0;
bool multiplayer_start = false;
int last_row = -1;
int last_col = -1;
int p1sock = -1;
int p2sock = -1;

void initServer (int&, int port);
void processSockets (fd_set);
void sendData (int, char[], int);
void receiveData (int, char[], int&);
int winner();
int gamestate(char);
int place_position(int, int);
void ai(int&, int&);
void print_grid();
string game_init(char[]);

int main(int argc, char *argv[])
{
    int serverSock;                  // server socket descriptor
    int clientSock;                  // client socket descriptor
    struct sockaddr_in clientAddr;   // address of the client
    int player = 0;

    struct timeval timeout = {0, 10};  // The timeout value for select()
    struct timeval selectTime;
    fd_set tempRecvSockSet;            // Temp. receive socket set for select()

    for (int i = 0; i < GRID_HEIGHT ; i++)
      for (int j = 0; j < GRID_WIDTH; j++)
	grid[j][i] = -1;
    
    // Check for input errors
    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " <Listening Port>" << endl;
        exit(1);
    }
    
    // Initilize the server
    initServer(serverSock, atoi(argv[1]));
    
    // Clear the socket sets    
    FD_ZERO(&recvSockSet);

    // Add the listening socket to the set
    FD_SET(serverSock, &recvSockSet);
    maxDesc = max(maxDesc, serverSock);
    
    // Run the server until a "terminate" command is received)
    while (!terminated)
      {
        // copy the receive descriptors to the working set
        memcpy(&tempRecvSockSet, &recvSockSet, sizeof(recvSockSet));
        
        // Select timeout has to be reset every time before select() is
        // called, since select() may update the timeout parameter to
        // indicate how much time was left.
        selectTime = timeout;
        int ready = select(maxDesc + 1, &tempRecvSockSet, NULL, NULL, &selectTime);
        if (ready < 0)
	  {
            cout << "select() failed" << endl;
            break;
	  }
	
        // First, process new connection request, if any.
        if (FD_ISSET(serverSock, &tempRecvSockSet))
	  {
            // set the size of the client address structure
            unsigned int size = sizeof(clientAddr);

            // Establish a connection
	      if ((clientSock = accept(serverSock, (struct sockaddr *) &clientAddr, &size)) < 0)
                break;
	      cout << "Accepted a connection from " << inet_ntoa(clientAddr.sin_addr) << ":" << clientAddr.sin_port << endl;
	      
	      // Add the new connection to the receive socket set
	      FD_SET(clientSock, &recvSockSet);
	      maxDesc = max(maxDesc, clientSock);
	      
	  }
        
        // Then process messages waiting at each ready socket
        else
	  processSockets(tempRecvSockSet);
      }

    // Close the connections with the client
    for (int sock = 0; sock <= maxDesc; sock++)
      {
        if (FD_ISSET(sock, &recvSockSet))
	  close(sock);
      }
    
    // Close the server sockets
    close(serverSock);
}

void initServer(int& serverSock, int port)
{
    struct sockaddr_in serverAddr;   // address of the server

    // Create a TCP socket
    // * AF_INET: using address family "Internet Protocol address"
    // * SOCK_STREAM: Provides sequenced, reliable, bidirectional, connection-mode byte streams.
    // * IPPROTO_TCP: TCP protocol
    if ((serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
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
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
    {
        cout << "setsockopt() failed" << endl;
        exit(1);
    }
    
    // Initialize the server information
    // Note that we can't choose a port less than 1023 if we are not privileged users (root)
    memset(&serverAddr, 0, sizeof(serverAddr));         // Zero out the structure
    serverAddr.sin_family = AF_INET;                    // Use Internet address family
    serverAddr.sin_port = htons(port);                  // Server port number
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);     // Any incoming interface
    
    // Bind to the local address
    if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        cout << "bind() failed" << endl;
        exit(1);
    }
    
    // Listen for connection requests
    if (listen(serverSock, MAXPENDING) < 0)
    {
        cout << "listen() failed" << endl;
        exit(1);
    }
}

void processSockets (fd_set readySocks)
{
    char* buffer = new char[BUFFERSIZE];       // Buffer for the message from the server
    int size;              // Actual size of the message 
    string tempBuffer = "";
    stringstream ss;
    string last_move = "";

      // Loop through the descriptors and process
      for (int sock = 0; sock <= maxDesc; sock++){
	
	if (!FD_ISSET(sock, &readySocks))
	  continue;
	
        // Clear the buffers
	memset(buffer, 0, BUFFERSIZE);
	
	// Receive data from the client
	receiveData(sock, buffer, size);

	if (buffer[0] == '2'){
	  if (player_num == -1)
	    p1sock = sock;
	  if (player_num == 0)
	    p2sock = sock;
	}

	string tempBuffer = game_init(buffer);
      
	// Echo the message back to the client
	sendData(sock, (char *)tempBuffer.c_str(), BUFFERSIZE);
	
	if (tempBuffer[0] <= '1' || (tempBuffer[0] == '7' || tempBuffer[0] == '3')){
	  ss << last_col;
	  ss << last_row;
	  last_move = ss.str();
	  if (player_num == 1 && whos_turn == 0){
	    if (strncmp((char *)tempBuffer.c_str(), "3", 1) == 0){
	      tempBuffer = "4 " + last_move;
	      sendData(p2sock, (char *)tempBuffer.c_str(), BUFFERSIZE);
	    }
	    else{
	      tempBuffer = "5 " + last_move;
	      sendData(p1sock, (char *)tempBuffer.c_str(), BUFFERSIZE);
	    }
	  }else if(player_num == 1 && whos_turn == 1){
	    if (strncmp((char *)tempBuffer.c_str(), "3", 1) == 0){
	      tempBuffer = "4 " + last_move;
	      sendData(p1sock, (char *)tempBuffer.c_str(), BUFFERSIZE);
	    }
	    else{
	      tempBuffer = "5 " + last_move;
	      sendData(p2sock, (char *)tempBuffer.c_str(), BUFFERSIZE);
	    }
	  }//End tempBuffer if statement
	  if (tempBuffer[0] == 'H'){
	    cout << "RAN" <<endl;
	    sendData(p1sock, (char *)tempBuffer.c_str(), BUFFERSIZE);
	  }

	  if (mode == 1){
	    tempBuffer = "5";
	    sendData(sock, (char *)tempBuffer.c_str(), BUFFERSIZE);
	  }
	}
	
      }//End For loop

    delete[] buffer;
}

int gamestate(char position){
   
    if((position -97 < 'a'-97) || (position -97 > 'g'-97)){
      cout << "Invalid column. Columns must be from a-g (Lowercase). Try again: ";
      return -1;
    }
    else if(grid[position-97][0] != -1){
      cout << "Column is full. Try another column: ";
      return -1;
    }
  
  int column = position - 97;
  return column;

}//End gamestate

void receiveData (int sock, char* inBuffer, int& size)
{
    // Receive the message from client
    size = recv(sock, (char *) inBuffer, BUFFERSIZE, 0);
    
    // Check for connection close (0) or errors (< 0)
    if (size <= 0)
    {
        cout << "recv() failed, or the connection is closed. " << endl;
	string tempBuffer = "9";
	if (sock == p1sock){
	  sendData(p2sock, (char *)tempBuffer.c_str(), BUFFERSIZE);
	}

	if (sock == p2sock){
	  sendData(p1sock, (char *)tempBuffer.c_str(), BUFFERSIZE);
	}

        FD_CLR(sock, &recvSockSet);
        
        // Update the max descriptor
        while (FD_ISSET(maxDesc, &recvSockSet) == false)
              maxDesc -= 1;
        return; 
    }

}

void sendData (int sock, char* buffer, int size)
{
    int bytesSent = 0;                   // Number of bytes sent
    
    // Sent the data
    bytesSent += send(sock, (char *) buffer + bytesSent, size - bytesSent, 0);

    if (bytesSent < 0 || bytesSent != size)
    {
        cout << "error in sending" << endl;
        return;
    }
        
    if (strncmp(buffer, "terminate", 9) == 0 ){
      cout << "Terminate";
      terminated = true;
    }
}
    
int place_position(int column, int player){
  
 
  for (int i = GRID_HEIGHT - 1; i>=0; i--)
    if (grid[column][i] == -1){
      if (player == 0)
	grid[column][i] = 0;
      if (player == 1)
	grid[column][i] = 1;
      return i;
    }
  
  return -1;
  
}//End Place_Position

int winner(){

  int is_winner = 0;
 
  //Check for tie
  for (int i = 0; i < GRID_WIDTH; i++)
    if (grid[i][0] == -1){
      is_winner = 0;
      break;
    }
    else if (i == GRID_WIDTH-1){
      is_winner = 2;  
      break;
    }
  
  //Check Horizontal
  for (int i = 0; i < GRID_WIDTH - 3; i++)
    for (int j = 0; j < GRID_HEIGHT; j++)
      if ((grid[i][j] == grid[i+1][j]) && (grid[i+1][j]==grid[i+2][j]) && (grid[i+2][j]==grid[i+3][j]) && (grid[i][j] != -1))
	is_winner = 1;
  
  //Check Vertical
  for (int i = 0; i < GRID_WIDTH; i++)
    for (int j = 0; j < GRID_HEIGHT - 3; j++)
      if((grid[i][j] == grid[i][j+1]) && (grid[i][j+1] == grid[i][j+2]) && (grid[i][j+2] == grid[i][j+3]) && (grid[i][j] != -1))
	is_winner = 1;
 
  //Check diagonally downwards
  for (int i = 0; i < GRID_WIDTH - 3; i++)
    for (int j = 0; j < GRID_HEIGHT - 3; j++)
      if ((grid[i][j] == grid[i+1][j+1]) && (grid[i+1][j+1] == grid[i+2][j+2]) && (grid[i+2][j+2] == grid[i+3][j+3]) && (grid[i][j] != -1))
	is_winner = 1;

  //Check diagonally upwards
  for (int i = 0; i < GRID_WIDTH - 3; i++)
    for (int j = 3; j < GRID_HEIGHT; j++)
      if((grid[i][j] == grid[i+1][j-1]) && (grid[i+1][j-1] == grid[i+2][j-2]) && (grid[i+2][j-2] == grid[i+3][j-3]) && (grid[i][j] != -1))
	is_winner = 1;
  
  return is_winner;

}//End winner

void ai(int& row, int& column){
  
  srand (time(NULL));
  column = rand() % 10 - 2;

  while(grid[column][0] != -1){
    column = rand() % 10 - 2;
  }

  for (int i = GRID_HEIGHT - 1; i>=0; i--)
    if (grid[column][i] == -1){
      grid[column][i] = 1;
      row = i;
      break;
    }
 
      
}//End AI

void print_grid(){

  cout << " A B C D E F G " <<endl;

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

string game_init(char* buffer){

  int column = -1;
  bool AI_playing = false;
  int status = 0;
  int ai_row;
  int ai_column;
  stringstream ss;

  if (player_num >= 2 || (mode == 1 && player_num >= 1) || (buffer[0] == '0' && player_num <= 0 && mode == 2))
    return "9";

  if(buffer[0] == '2'){
    if (player_num == -1)
      mode = (int)buffer[2] - 48;
    string temp = "7 ";
    ss << ++player_num;
    temp += ss.str();
    return temp;
  }

  if(buffer[0] - 48 != whos_turn)
    return "6";

  column = gamestate(buffer[2]);
  
  if (column == -1)
    return "8";

  last_col = column;

  int row = place_position(column, whos_turn);
  last_row = row;
  print_grid();
  status = winner();
  if (status == 0 && mode == 1){
    AI_playing = true;
    ai(ai_row, ai_column);
    print_grid();
    status = winner();
  }
  
  if (status != 0){
    string temp = "";
    if(AI_playing){
      ss << ai_column;
      ss << ai_row;
      return "4 " + ss.str();
    }
    ss << column;
    ss << row;
    if (status == 2)
      return "H " + ss.str();
    return "3 " + ss.str();
  }
  
  string temp = "";
  ss << whos_turn;
  temp += ss.str();
  ss.str(std::string());
  ss << row;
  temp += " " + ss.str();
  if (mode == 1){
    ss.str(std::string());
    ss << ai_column;
    ss << ai_row;
    temp += " " + ss.str();
  }
  
  if (whos_turn == 0 && mode == 2){
    whos_turn = 1;
  }
  else if(whos_turn == 1 && mode == 2)
    whos_turn = 0;

  return temp;

}
