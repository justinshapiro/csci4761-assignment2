Calender @ CSEGRID
-------------------

### Instructions to run:

1. Extract the "client" and "server" folders from the tar.gz file
	
2. Copy these folders to your home folder on CSEGRID using WinSCP or through UNC path on Windows
	
3. Establish two separate connections to CSEGRID using your SSH client. One connection is for the client and one connection is for the server
	
4. Take note of the hostname of the server window. The hostname should be something like ucdencsecnode0[1-5]. This hostname will be used by the client to establish connection
	
5. In the client session window, navigate to the "client" directory (i.e. "cd path_to_client")
	
6. Type "make" to compile client.cpp
	
7. Do the same for the server: navigate to its directory in the server session window and type "make" to compile server.cpp
	
8. Run the server application by typing ./server
	
9. Run the client application by typing ./client
	
### How to use:

This application meets all of the requirements posted in the instructions. However, this is a quick guide on how the program is intended to be used
	
1. After executing both the client and server on CSEGRID, on the client you are prompted to type in the hostname of the server. This is the name you found in step 4 of the run instructions.
	
- For example: "Enter the hostname of the app server: ucdencsecnode05"
2. If you typed the hostname correctly and it is the correct hostname of the computer where the server program is running, you will be taken to a portal where you can log in, create an account, or quit
	
3. The program is NOT preloaded with any usernames, so you will have to create an account using option 2 first
	
4. When creating the account, be sure to remember what you put for the "Name" and "Password" fields, as these will be your login credentials
	
5. Login to your account by selecting option 1 and typing your Name and Password from when you created your account
	
6. After hitting enter, assuming you have entered your credentials correctly, you will be brought to the Main Menu where you can manage your appointments and also your account
	
7. When entering dates, the program strictly expects the format YYYY/MM/DD hh:mm. So it will reject input like "04-02-17 2pm" or even "2017-04-02 2:00". An example of a correctly formatted date is: "2017/04/02 17:00" (note the 24-hour time format)
	
Notes 
---------

- This is an application with two components that run independently of each other: client.cpp and server.cpp. "make" must be typed twice on two different machines to compile two different applications which happen to communicate with each other over TCP sockets

- Calender.cpp defines a Calender object that manages the database file and helps manage appointments and accounts. Calender.cpp assists server.cpp and therefore should only be on the server computer.

- client.cpp knows absolutely nothing about Calender.cpp, it is simply a program that contains forms for the user to enter data and parses this data into a format accepted by the server

- The database file always exists on the server, so once an account has been created an populated with appointments, that account will be accessible on subsequent runs even after terminating the client server program

- Multiple users can use the server concurrently. The server is setup to allow 30 connections from different clients, but this number was picked arbitrarily can be increased in the code
