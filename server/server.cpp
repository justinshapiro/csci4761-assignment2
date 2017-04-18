/* Server: receives bytes and transforms them into data types that are accepted by Calender.h
 * Here are the response codes
 *
 * request_id     data passed               response                 calender function
 * ----------     -----------               --------                 -----------------
 * 0             appt id                  vector<int>                appointment conflict list
 * 1             string array             bool success               create user
 * 2             string array             bool success, int token    login/authenticate user
 * 3             int, string array        bool success, int appt_id  create new appointment
 * 4             int                      string array (x3)          get all user appointments
 * 5             int, int, string array   bool success               update appointment
 * 6             int                      bool success               remove appointment
 * 7             int                      string array               get account details
 * 8             int, string array        bool                       edit account details
 * 9             int                      bool                       remove user
 * 2,*           int                      bool                       logout user
 * 2,$           int                      bool                       client disconnect
 */
#include "Calender.h"
#include <iostream>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define SERVER_PORT 3490 // the port users will be connecting to
#define BACKLOG 10 // how many pending connections queue will hold
#define MAX_CLIENTS 30 // define a max of 30 clients connecting at one time

using namespace std;

void sigchld_handler(int);
char* get_request(char*, int);
char* handle_request(int, char*);
vector<string> parse_data(char*);

static Calender calender;
bool running = true;

int main() {
    /*
     * The best way to achieve web server concurrency in this case is through socket sets rather than thread or fork()
     * Socket sets take advantage of the select method to detect incoming connections or I/O requests from existing ones
     *
     * The method for implementing socket sets was take from here:
     * http://www.binarytides.com/multiple-socket-connections-fdset-select-linux/
     * */
    int main_socket, new_socket, client_socket[MAX_CLIENTS];
    int read_val, sd, max_sd;
    fd_set socket_set;
    struct sockaddr_in my_address; // my address information
    struct sockaddr_in thier_address; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char* receive_buffer;

    // Initialize our array of client sockets to 0
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_socket[i] = 0;
    }

    // Create the listener socket "main_socket"
    if ((main_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Allow the listener socket to accept multiple connections
    if (setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    my_address.sin_family = AF_INET; // host byte order, which is Little Endian
    my_address.sin_port = htons(SERVER_PORT); // network byte order, which is is Big Endian
    my_address.sin_addr.s_addr = INADDR_ANY; // automatically fill with the server IP address
    memset(&(my_address.sin_zero), '\0', 8); // zero out the rest of the struct

    // Bind the socket to a specific port
    if (bind(main_socket, (struct sockaddr*) &my_address, sizeof(struct sockaddr)) == -1) {
        perror("bind");
        exit(1);
    }

    // Define a maximum of BACKLOG pending connections to the listener socket
    if (listen(main_socket, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while (1) {
        FD_ZERO(&socket_set); // clear the socket set
        FD_SET(main_socket, &socket_set); // add the listener socket to the socket set
        max_sd = main_socket; // preserve the socket

        // add all client sockets to the set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (sd > 0) {
                FD_SET(client_socket[i], &socket_set);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // scan ("select") all the sockets for activity
        if ((select(max_sd + 1, &socket_set, NULL, NULL, NULL) < 0) && (errno != EINTR)) {
            cout << "Concurrency error\n";
        }

        // detect new connection requests
        if (FD_ISSET(main_socket, &socket_set)) {
            sin_size = sizeof(struct sockaddr_in);
            if ((new_socket = accept(main_socket, (struct sockaddr *) &thier_address, &sin_size)) == -1) {
                perror("accept");
                continue;
            }
            printf("Client at IP address %s has connected.\n", inet_ntoa(thier_address.sin_addr));

            // add the new socket, which is the identified connection, to our socket list
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    break;
                }
            }
        }

        // handle requests from existing connections
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_socket[i];
            if (FD_ISSET(sd, &socket_set)) {
                if (string(get_request(receive_buffer, sd)) == "quit") {
                    close(sd);
                    cout << "Client on socket " << i << " has disconnected.\n";
                    client_socket[i] = 0;
                }
            }
        }

        if (!running) {
            break;
        }
    }
    return 0;
}

char* get_request(char* receive_buffer, int child_socket) {
    receive_buffer = (char *) calloc(128, sizeof(char));
    int num_bytes = recv(child_socket, receive_buffer, 128, 0);
    if (num_bytes <= 0) {
        return (char*) "quit";
    }
    printf("Received from client on socket %d: %s\n", child_socket, receive_buffer);

    int request_id;
    char *data_segment = (char *) calloc(128, sizeof(char));
    int i;
    for (i = 0; receive_buffer[i] != '\0'; i++) {
        if (i == 0) {
            request_id = receive_buffer[i] - '0';
        } else {
            data_segment[i - 1] = receive_buffer[i];
        }
    }
    data_segment[i + 1] = '\0';

    char *response_buffer = handle_request(request_id, data_segment);

    if (send(child_socket, response_buffer, strlen(response_buffer), 0) == -1) {
        perror("send");
        close(child_socket);
        exit(1);
    }

    printf("Sent to client on socket %d: %s\n", child_socket, response_buffer);

    return response_buffer;
}

char* handle_request(int request_id, char* data_segment) {
    char* response_buffer = (char*) calloc(128, sizeof(char));
    vector<string> data = parse_data(data_segment);

    switch (request_id) {
        case 0: {
            int appt_id = stoi(data[0]);
            vector<int> conflict_list = calender.get_conflict_list(appt_id);

            if (conflict_list.size() == 0) {
                response_buffer = "false\0";
            } else {
                string s = "";
                for (unsigned int i = 0; i < conflict_list.size(); i++) {
                  s += to_string(conflict_list.at(i)) + ",";
                }
                response_buffer = (char *) s.c_str();
            }
        }
            break;
        case 1: {
            if (calender.add_user(data)) {
                response_buffer = "true\0";
            } else {
                response_buffer = "false\0";
            }
        }
            break;
        case 2: {
            if (data[0] == "*") {
               int user_id = stoi(data[1]);

               bool success = calender.logout_user(user_id);

                if (success) {
                    response_buffer = "logout\0";
                } else {
                    response_buffer = "false\0";
                }
            } else if (data[0] == "$") {
                response_buffer = "quit\0";
            } else if(data[0] == "#") {
                response_buffer = "!#!#\0";
            } else {
                pair<bool, int> login_answer = calender.login_user(data[0], data[1]);
                if (login_answer.first) {
                    string s = "true," + to_string(login_answer.second);
                    response_buffer = (char *) s.c_str();
                } else {
                    response_buffer = "false\0";
                }
            }
        }
            break;
        case 3: {
            int user_id = stoi(data[0]);
            vector<long> appt_data = {user_id, stol(data[1]), stol(data[2])};
            pair<bool, int> add_appt_answer = calender.add_appt(user_id, appt_data);

            if (add_appt_answer.first) {
                string s = "true," + to_string(add_appt_answer.second);
                response_buffer = (char *) s.c_str();
            } else {
                response_buffer = "false\0";
            }
        }
            break;
        case 4: {
            pair<vector<int>, vector<pair<long, long>>> user_appt_list = calender.get_all_appts(stoi(data[0]));

            if (user_appt_list.first.size() == 0) {
                response_buffer = "false\0";
            } else {
                string s = "";
                for (unsigned int i = 0; i < user_appt_list.first.size(); i++) {
                    s += to_string(user_appt_list.first.at(i));
                    if (i + 1 < user_appt_list.first.size()) {
                        s += ",";
                    }
                }
                for (unsigned int i = 0; i < user_appt_list.second.size(); i++) {
                    s += "$" + to_string(user_appt_list.second.at(i).first) + ",";
                    s+= to_string(user_appt_list.second.at(i).second);
                }
                s += "$";
                response_buffer = (char *) s.c_str();
            }
        }
            break;
        case 5: {
            int user_id = stoi(data[0]);
            int appt_id = stoi(data[1]);
            long start_date = stol(data[2]);
            long end_date = stol(data[3]);
            pair<long, long> appt_info(start_date, end_date);
            pair<int, pair<long, long>> appt_data(appt_id, appt_info);

            bool success = calender.update_appt(user_id, appt_data);

            if (success) {
                response_buffer = "true\0";
            } else {
                response_buffer = "false\0";
            }
        }
            break;
        case 6: {
            int appt_id = stoi(data[0]);
            if (calender.remove_appt(appt_id)) {
                response_buffer = "true\0";
            } else {
                response_buffer = "false\0";
            }
        }
            break;
        case 7: {
            int user_id = stoi(data[0]);
            vector<string> account_details = calender.get_account_details(user_id);
            string s = "";
            for (unsigned int i = 0; i < account_details.size(); i++) {
                s += account_details[i] + ",";
            }
            response_buffer = (char *) s.c_str();
        }
            break;
        case 8: {
            int user_id = stoi(data[0]);
            data.erase(data.begin());

            bool success = calender.edit_account_details(user_id, data);

            if (success) {
                response_buffer = "true\0";
            } else {
                response_buffer = "false\0";
            }
        }
            break;
        case 9: {
            int user_id = stoi(data[0]);

            bool success = calender.remove_user(user_id);

            if (success) {
                response_buffer = "true\0";
            } else {
                response_buffer = "false\0";
            }
        }
            break;
        default: break;
    }

    return response_buffer;
}

vector<string> parse_data(char* c) {
    string s = "";
    vector<string> v;

    int i = 0;
    while(1) {
        if (c[i] == ',' || c[i] == '\0') {
            v.push_back(s);
            s = "";
            if (c[i] == '\0') {
                break;
            }
        } else {
            s += c[i];
        }
        i++;
    }

    return v;
}

void sigchld_handler(int s) {
    while(wait(NULL) > 0);
}