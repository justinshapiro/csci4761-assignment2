/* Client: Connects to the server, retrieves input from the user and parses it into a message that the server can decode
 *         Receive messages from server and interpret it as response to a previously issued request
 *
 */
#include <iostream>
#include <string>
#include <stdlib.h>
#include <vector>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <limits>

#define PORT 3490 // the port client will be connecting to

using namespace std;

// Forward declarations
void login();
void logout_user();
void create_account();
void delete_account();
void manage_account();
void create_appt_form();
void update_appt_form();
void remove_appt_form();
void get_appt_range_form();
void get_appt_conflict_form();
pair<int, pair<long, long>> get_existing_appt(short, long, long);
pair<vector<int>, vector<pair<long, long>>> get_all_appts();
string enter_dmz();
void enter_portal();
void home_screen();
long parse_date(string);
long get_appointment(short);
string date_to_string(long);
char* send_to_server(char*);
void connect_to_server();

int user_token;
int client_socket;
struct hostent *server_hostent;
struct sockaddr_in their_address;
string server_hostname;

int main() {
    server_hostname = enter_dmz();
    connect_to_server();
    if (string(send_to_server((char*) "2#")) == "!#!#") {
        enter_portal();
        close(client_socket);
    } else {
        system("clear");
        cout << "You have connected to a rouge server that was running a TCP socket service,"
             << " but not the one intended to serve this application. \n\n"
             << "Please check the hostname you entered and try again\n"
             << "(You entered the hostname: " << server_hostname << endl;
    }

    return 0;
}

char* send_to_server(char* data) {
    char* receive_buffer = (char*) calloc(128, sizeof(char));

    for (;;) {
        if ((send(client_socket, data, strlen(data), 0)) == -1) {
            perror("send");
            close(client_socket);
            exit(1);
        }

        if ((recv(client_socket, receive_buffer, 127, 0)) == -1) {
            perror("recv");
            exit(1);
        } else {
            break;
        }
    }

    return receive_buffer;
}

void connect_to_server() {
    if ((server_hostent = gethostbyname(server_hostname.c_str())) == NULL) { // get the host info
        cout << "Unknown host. Could not connect to the Calender server.\n";
        exit(1);
    }

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_address.sin_family = AF_INET; // host byte order is Little Endian
    their_address.sin_port = htons(PORT); // convert to network byte order, which is in Big Endian
    their_address.sin_addr = *((struct in_addr*) server_hostent->h_addr);
    memset(&(their_address.sin_zero), '\0', 8); // zero out the rest of the struct

    if (connect(client_socket, (struct sockaddr*) &their_address, sizeof(struct sockaddr)) == -1) {
        cout << "That host exists, but a Calender service is not running.\n";
        exit(1);
    }
}

string enter_dmz() {
    system("clear");
    string server_hostname = "";

    cout << "Enter the hostname of the app server: ";
    getline(cin, server_hostname);

    while (!cin) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Unexpected input detected. Try again.\n";
        cout << "Enter the hostname of the app server: ";
        getline(cin, server_hostname);
    }

    return server_hostname;
}

void enter_portal() {
    system("clear");
    short login_option = 0;
    while (login_option != 3) {
        system("clear");

        cout << "Welcome to the portal for Calender @ CSEGRID\n"
             << "--------------------------------------------\n\n"
             << "What would you like to do?\n"
             << "     1. Login\n"
             << "     2. Create account\n"
             << "     3. Quit\n\n"
             << "Select from an option above: ";

        cin >> login_option;

        switch (login_option) {
            case 1:
                cin.ignore();
                login();
                break;
            case 2:
                cin.ignore();
                create_account();
                break;
            case 3: {
                string send_buffer = "2$," + to_string(user_token);
                char* _send_buffer = (char *) send_buffer.c_str();
                send_to_server(_send_buffer);
            } break;
            default:
                while (login_option < 1 || login_option > 3) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid choice. Enter the number of the choice you are selecting.\n\n"
                         << "Select from an option above: ";
                    cin >> login_option;
                }
        }
    }
}

void login() {
    system("clear");

    string username = ""; string password = "";
    cout << "Login to Calender @ CSEGRID:\n--------------------------------\n\n";
    cout << "Name: ";  getline(cin, username);

    while (!cin) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Unexpected input detected. Try again.\n";
        cout << "Name: ";
        getline(cin, username);
    }

    if (username != "-1") {
        cout << "Password: ";
        getline(cin, password);

        while (!cin) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Unexpected input detected. Try again.\n";
            cout << "Password: ";
            getline(cin, password);
        }

        string send_buffer = "2" + username + "," + password;
        char *_send_buffer = (char *) send_buffer.c_str();
        string response(send_to_server(_send_buffer));

        while (response == "false") {
            cout << "Bad username/password. Try again or type -1 to return to portal.\n\n";
            cout << "Name: ";
            getline(cin, username);
            while (!cin) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Unexpected input detected. Try again.\n";
                cout << "Name: ";
                getline(cin, username);
            }

            if (username != "-1") {
                cout << "Password: ";
                getline(cin, password);

                while (!cin) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Unexpected input detected. Try again.\n";
                    cout << "Password: ";
                    getline(cin, password);
                }

                send_buffer = "2" + username + "," + password;
                _send_buffer = (char *) send_buffer.c_str();
                response = send_to_server(_send_buffer);
            } else {
                break;
            }
        }

        if (username != "-1") {
            bool found_token = false;
            string token_str = "";
            for (int i = 0; i < response.length(); i++) {
                if (response[i] == ',') {
                    found_token = true;
                    i++;
                }

                if (found_token) {
                    token_str += response[i];
                }
            }

            int token = stoi(token_str);

            if (token > 0) {
                user_token = token;
                home_screen();
            } else {
                cout << "You are already logged into the system, so we cannot log you on a second time\n"
                     << "If you wish to sign in on this system, end your existing session and try again.\n\n"
                     << "Press ENTER to return to the main portal...";
                cin.get();
            }
        }
    }
}

void create_account() {
    system("clear");

    string name = ""; string email = ""; string phone = ""; string password = "";
    cout << "New user creation:\n------------------\n\n";
    cout << "Name: ";     getline(cin, name);
    while (!cin) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Unexpected input detected. Try again.\n";
        cout << "Name: ";
        getline(cin, name);
    }
    cout << "Email: ";    getline(cin, email);
    while (!cin) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Unexpected input detected. Try again.\n";
        cout << "Email: ";
        getline(cin, email);
    }
    cout << "Phone: ";    getline(cin, phone);
    while (!cin) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Unexpected input detected. Try again.\n";
        cout << "Phone: ";
        getline(cin, phone);
    }
    cout << "Password: "; getline(cin, password);
    while (!cin) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Unexpected input detected. Try again.\n";
        cout << "Password: ";
        getline(cin, password);
    }

    string send_buffer = "1" + name + "," + email + "," + phone + "," + password;

    char* _send_buffer = (char *) send_buffer.c_str();
    string response(send_to_server(_send_buffer));

    if (response == "true") {
        login();
    } else {
        cout << "A user with the same name already exists. User account not created.\n"
             << "Press ENTER to return to main menu..."; cin.get();
    }
}

void home_screen() {
    system("clear");

    short selection = 0;
    cout << "You are connected to Calender @ CSEGRID! | Your User ID is: " << user_token << endl
         << "----------------------------------------\n\n"
         << "Select what you want to do...\n\n"
         << "1) Add an appointment\n"
         << "2) Update an appointment\n"
         << "3) Remove an appointment\n"
         << "4) Get all appointments that fall within a date range\n"
         << "5) Check appointment conflicts\n"
         << "6) View/edit account details\n"
         << "7) Delete account\n"
         << "8) Logout\n\n"
         << "Select from an option above: ";
    cin >> selection;

    switch (selection) {
        case 1: cin.ignore(); create_appt_form();       break;
        case 2: cin.ignore(); update_appt_form();       break;
        case 3: cin.ignore(); remove_appt_form();       break;
        case 4: cin.ignore(); get_appt_range_form();    break;
        case 5: cin.ignore(); get_appt_conflict_form(); break;
        case 6: cin.ignore(); manage_account();         break;
        case 7: cin.ignore(); delete_account();         break;
        case 8: cin.ignore(); logout_user();            break;
        default:
            while (selection < 1 || selection > 8) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid choice. Enter the number of the choice you are selecting.\n\n"
                     << "Select from an option above: ";
                cin >> selection;
            }
    }
}

void create_appt_form() {
    system("clear");

    string start_date = ""; string end_date = "";
    long start_date_id = 0; long end_date_id = 0;

    cout << "Create New Appointment:\n-----------------------\n\n";
    start_date_id = get_appointment(0); cout << endl;
    end_date_id = get_appointment(1);

    while (end_date_id <= start_date_id) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "End Date cannot be before or at the same time as Start Date.\n\n";
        end_date_id = get_appointment(1);
    }

    string send_buffer = "3" + to_string(user_token) + "," + to_string(start_date_id) + "," + to_string(end_date_id);
    char* _send_buffer = (char *) send_buffer.c_str();
    string response(send_to_server(_send_buffer));

    if (response == "false") {
        cout << "Network error: Appointment could not be added.\n\n"
             << "Press ENTER to return to the main menu...";
    } else {
        bool found_token = false;
        string id_str = "";
        for (int i = 0; i < response.length(); i++) {
            if (response[i] == ',') {
                found_token = true;
                i++;
            }

            if (found_token) {
                id_str += response[i];
            }
        }

        cout << "\nAppointment successfully added to the system!\n\n"
             << "Your appointment details are as follows: \n"
             << "   -> Appointment ID:        " << stoi(id_str) << endl
             << "   -> Appointment starts at: " << date_to_string(start_date_id) << endl
             << "   -> Appointment ends at:   " << date_to_string(end_date_id) << endl << endl
             << "Press ENTER to return to the main menu...";
    }

    cin.get();
    home_screen();
}

void update_appt_form() {
    system("clear");

    cout << "Update an Appointment:\n---------------------\n\n";

    pair<int, pair<long, long>> selected_appt = get_existing_appt(0, 0, 0);

    if (selected_appt.first > -1) {
        cout << "\nYou have chosen to update the following appointment: "
             << date_to_string(selected_appt.second.first) << " - "
             << date_to_string(selected_appt.second.second) << endl << endl;

        cout << "What do you want to do?\n"
             << "    1. Update the starting date/time\n"
             << "    2. Update the ending date/time\n"
             << "    3. Update both\n\n";

        short selection = 0;
        cout << "Enter choice: ";
        cin >> selection;
        while (selection <= 0 || selection > 3) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid selection. Enter a number from the choices above.\n\n";
            cout << "Enter choice: ";
            cin >> selection;
        }

        cin.ignore();

        bool update_success = false;
        long appt_id = selected_appt.first;
        long start_date_code = 0;
        long end_date_code = 0;
        pair<int, pair<long, long>> new_appt;
        switch (selection) { // request_id: 5
            case 1:
                start_date_code = get_appointment(0);
                end_date_code = selected_appt.second.second;
                while (start_date_code >= end_date_code) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Start Date cannot be after or at the same time as End Date.\n\n";
                    start_date_code = get_appointment(0);
                }
                break;
            case 2:
                end_date_code = get_appointment(1);
                start_date_code = selected_appt.second.first;
                while (end_date_code <= start_date_code) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "End Date cannot be before or at the same time as Start Date.\n\n";
                    end_date_code = get_appointment(1);
                }
                break;
            case 3:
                start_date_code = get_appointment(0);
                end_date_code = get_appointment(1);
                while (end_date_code <= start_date_code) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "End Date cannot be before or at the same time as Start Date.\n\n";
                    end_date_code = get_appointment(1);
                }


            default:
                break;
        }

        string send_buffer = "5" + to_string(user_token) + "," + to_string(appt_id) + ",";
        send_buffer += to_string(start_date_code) + "," + to_string(end_date_code);
        char* _send_buffer = (char *) send_buffer.c_str();
        string response(send_to_server(_send_buffer));

        if (response == "true") {
            update_success = true;
        }

        if (update_success) {
            cout << "Appointment successfully updated\n\n";
        } else {
            cout << "Network error: Appointment could not be updated due to system error\n\n";
        }
    }

    cout << "Press ENTER to return to main menu...";

    cin.get();
    home_screen();
}

void remove_appt_form() {
    system("clear");

    cout << "Remove an Appointment:\n---------------------\n\n";
    pair<int, pair<long, long>> selected_appt = get_existing_appt(0, 0, 0);

    if (selected_appt.first > -1) {
        string send_buffer = "6" + to_string(selected_appt.first);
        char *_send_buffer = (char *) send_buffer.c_str();
        string response(send_to_server(_send_buffer));

        if (response == "false") {
            cout << "Network error: Appointment could not be removed due to system error\n\n";
        } else {
            cout << "Appointment successfully removed\n\n";
        }
    }

    cout << "Press ENTER to return to main menu...";

    cin.get();
    home_screen();
}

void get_appt_range_form() {
    system("clear");

    cout << "Get appointments that fall within a specified range:\n"
         << "---------------------------------------------------\n\n";

    long start_range = get_appointment(2);
    long end_range = get_appointment(3);

    while (end_range <= start_range) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "End Range Date cannot be before or at the same time as Start Range Date.\n\n";
        end_range = get_appointment(3);
    }

    if (get_existing_appt(1, start_range, end_range).first == -1) {
        cout << "\nNo appointments in the specified range\n";
    }

    cout << "\nPress ENTER to return to main menu...";

    cin.get();
    home_screen();
}

void get_appt_conflict_form() {
    system("clear");

    cout << "Your schedule conflicts:\n-----------------------\n";
    pair<vector<int>, vector<pair<long, long>>> all_appts = get_all_appts();
    bool conflicts_found = false;

    if (!all_appts.first.empty()) {
        for (unsigned int i = 0; i < all_appts.first.size(); i++) {
            int appt_id = all_appts.first.at(i);
            pair<long, long> conflicted_info = all_appts.second[i];

            string send_buffer = "0" + to_string(appt_id);
            char* _send_buffer = (char *) send_buffer.c_str();
            string response(send_to_server(_send_buffer));

            if (response != "false") {
                conflicts_found = true;

                vector<int> conflicting_list;
                string appt_id_str = "";
                for (unsigned int j = 0; j < response.length(); j++) {
                    if (response[j] == ',') {
                        conflicting_list.push_back(stoi(appt_id_str));
                        appt_id_str = "";
                    } else {
                        appt_id_str += response[j];
                    }
                }

                cout << "\nAppointment ID #" << appt_id << " from " << date_to_string(conflicted_info.first)
                     << " - " << date_to_string(conflicted_info.second)
                     << " is in conflict with the following appointment(s): \n";

                for (unsigned int j = 0; j < conflicting_list.size(); j++) {
                    int conflicting_id = conflicting_list.at(j);
                    pair<long, long> conflicting_info;
                    for (unsigned int k = 0; k < all_appts.first.size(); k++) {
                        if (all_appts.first.at(k) == conflicting_id) {
                            conflicting_info.first = all_appts.second.at(k).first;
                            conflicting_info.second = all_appts.second.at(k).second;
                        }
                    }

                    cout << "      -> Appt. ID #" << conflicting_id << " from "
                         << date_to_string(conflicting_info.first) << " - "
                         << date_to_string(conflicting_info.second) << endl;
                }
            }
        }
    }

    if (!conflicts_found) {
        cout << "No scheduling conflicts found.\n";
    }

    cout << "\nPress ENTER to return to main menu...";

    cin.get();
    home_screen();
}

pair<int, pair<long, long>> get_existing_appt(short type, long start_range, long end_range) {
    pair<int, pair<long, long>> selected_appt;
    selected_appt.first = -1;

    pair<vector<int>, vector<pair<long, long>>> user_appt_list = get_all_appts();
    if (!user_appt_list.first.empty()) {
        vector<int> valid_choices;

        if (type == 0) {
            cout << "Select from the appointments below: \n";
        } else {
            selected_appt.first = 0;
            cout << "You have the following appointments in the given range: \n";
        }

        for (unsigned int i = 0; i < user_appt_list.first.size(); i++) {
            if (type == 0 || (user_appt_list.second.at(i).first >= start_range &&
                              user_appt_list.second.at(i).second <= end_range)) {
                int appt_num = user_appt_list.first.at(i);
                valid_choices.push_back(appt_num);

                cout << "Appointment ID: " << user_appt_list.first.at(i) << " :: "
                     << date_to_string(user_appt_list.second.at(i).first) << " - "
                     << date_to_string(user_appt_list.second.at(i).second) << endl;
            }
        }

        if (type == 0) {
            unsigned int appt_choice = 0;
            cout << endl << "Enter the number corresponding to the appointment: ";
            cin >> appt_choice;
            bool correct_choice = false;
            for (unsigned int i = 0; i < valid_choices.size(); i++) {
                if (appt_choice == valid_choices.at(i)) {
                    correct_choice = true;
                    break;
                }
            }
            while (!correct_choice) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid selection. Enter a valid number from the list above.\n\n";
                cout << "Enter the number corresponding to the appointment: ";
                cin >> appt_choice;
                for (unsigned int i = 0; i < valid_choices.size(); i++) {
                    if (appt_choice == valid_choices.at(i)) {
                        correct_choice = true;
                        break;
                    }
                }
            }

            for (unsigned int i = 0; i < user_appt_list.first.size(); i++) {
                if (user_appt_list.first.at(i) == appt_choice) {
                    selected_appt = pair<int, pair<long, long>>(user_appt_list.first.at(i),
                                                                user_appt_list.second.at(i));
                    break;
                }
            }
        }
    }

    return selected_appt;
}

pair<vector<int>, vector<pair<long, long>>> get_all_appts() {
    vector<int> appt_ids;
    vector<pair<long, long>> appt_info;

    string send_buffer = "4" + to_string(user_token);
    char* _send_buffer = (char *) send_buffer.c_str();
    string response(send_to_server(_send_buffer));

    if (response == "false") {
        cout << "You have no appointments.\n\n";
    } else {
        string appt_id = "";
        string appt_info1 = "";
        string appt_info2 = "";
        bool part_flag = false;
        bool end = false;
        pair<long, long> appt_info_temp;
        for (unsigned int i = 0; i < response.length(); i++) {
            if (!part_flag) {
                if (response[i] == ',' || response[i] == '$') {
                    appt_ids.push_back(stoi(appt_id));
                    appt_id = "";
                    if (response[i] == '$') {
                        part_flag = true;
                    }
                } else {
                    appt_id += response[i];
                }
            } else {
                if (response[i] == ',') {
                    end = true;
                } else if (response[i] == '$') {
                    end = false;
                    appt_info_temp.first = stol(appt_info1);
                    appt_info_temp.second = stol(appt_info2);
                    appt_info.push_back(appt_info_temp);
                    appt_info1 = "";
                    appt_info2 = "";
                } else {
                    if (!end) {
                        appt_info1 += response[i];
                    } else {
                        appt_info2 += response[i];
                    }
                }
            }
        }
    }

    return pair<vector<int>, vector<pair<long, long>>>(appt_ids, appt_info);
}

long get_appointment(short type) {
    string date = "";
    if (type == 0) {
        date = "Start";
    } else if (type == 1) {
        date = "End";
    } else if (type == 2) {
        date = "Start Range";
    } else if (type == 3) {
        date = "End Range";
    }

    string input_date = "";

    cout << "Enter " << date << " date.\n"
         << "(Please put dates in the following form EXACTLY: YYYY/MM/DD hh:mm)\n\n";
    cout << date << " Date: "; getline(cin, input_date);

    while (input_date.size() != 16 || input_date.at(4) != '/'  ||
           input_date.at(7) != '/' || input_date.at(10) != ' ' || input_date.at(13) != ':'  ) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "You entered the date in an incorrect format.\n"
             << "Please put date in the following form EXACTLY: YYYY/MM/DD hh:mm\n\n";
        cout << date << " Date: "; getline(cin, input_date);
    }

    cout << endl;

    return parse_date(input_date);
}

void manage_account() {
    system("clear");

    cout << "View/edit account details:\n-------------------------\n\n";

    string send_buffer = "7" + to_string(user_token);
    char* _send_buffer = (char *) send_buffer.c_str();
    string response(send_to_server(_send_buffer));

    vector<string> account_details;
    string acct_info = "";
    for (unsigned int i = 0; i < response.length(); i++) {
        if (response[i] == ',') {
            account_details.push_back(acct_info);
            acct_info = "";
        } else {
            acct_info += response[i];
        }
    }

    cout << "Name: "     << account_details.at(0) << endl
         << "Email: "    << account_details.at(1) << endl
         << "Phone: "    << account_details.at(2) << endl
         << "Password: " << account_details.at(3) << endl;

    bool done = false;
    bool edited = false;
    short selection = 0;
    while (!done) {
        cout << endl << "What do you want to do?\n"
             << "    1. Edit account details\n"
             << "    2. Return to main menu\n\n"
             << "Enter selection: "; cin >> selection;

        while (!cin) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Unexpected input detected. Try again.\n";
            cout << "Enter selection: ";
            cin >> selection;
        }

        switch (selection) {
            case 1: {
                edited = true;
                short choice = 0;
                cout << "Which detail do you want to edit?\n"
                     << "    1. Name\n"
                     << "    2. Email\n"
                     << "    3. Phone\n"
                     << "    4. Password\n\n"
                     << "Enter selection: ";
                cin >> choice;

                while (!cin) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Unexpected input detected. Try again.\n";
                    cout << "Enter selection: ";
                    cin >> choice;
                }

                cin.ignore();

                switch (choice) {
                    case 1: {
                        string new_name = "";
                        cout << "Enter a new name: ";
                        getline(cin, new_name);
                        while (!cin) {
                            cin.clear();
                            cin.ignore(numeric_limits<streamsize>::max(), '\n');
                            cout << "Unexpected input detected. Try again.\n";
                            cout << "Enter a new name: ";
                            getline(cin, new_name);
                        }
                        account_details.at(0) = new_name;
                    }
                        break;
                    case 2: {
                        string new_email = "";
                        cout << "Enter a new email: ";
                        getline(cin, new_email);
                        while (!cin) {
                            cin.clear();
                            cin.ignore(numeric_limits<streamsize>::max(), '\n');
                            cout << "Unexpected input detected. Try again.\n";
                            cout << "Enter a new email: ";
                            getline(cin, new_email);
                        }
                        account_details.at(1) = new_email;
                    }
                        break;
                    case 3: {
                        string new_phone = "";
                        cout << "Enter a new phone: ";
                        getline(cin, new_phone);
                        while (!cin) {
                            cin.clear();
                            cin.ignore(numeric_limits<streamsize>::max(), '\n');
                            cout << "Unexpected input detected. Try again.\n";
                            cout << "Enter a new phone: ";
                            getline(cin, new_phone);
                        }
                        account_details.at(2) = new_phone;
                    }
                        break;
                    case 4: {
                        string new_password = "";
                        cout << "Enter a new password: ";
                        getline(cin, new_password);
                        while (!cin) {
                            cin.clear();
                            cin.ignore(numeric_limits<streamsize>::max(), '\n');
                            cout << "Unexpected input detected. Try again.\n";
                            cout << "Enter a new password: ";
                            getline(cin, new_password);
                        }
                        account_details.at(3) = new_password;
                    }
                        break;
                    default: {
                        while (choice < 1 || choice > 5) {
                            cin.clear();
                            cin.ignore(numeric_limits<streamsize>::max(), '\n');
                            cout << "Invalid selection. Choose a number from the list above.\n\n"
                                 << "Enter selection: ";
                            cin >> choice;
                        }
                    }
                }
                break;
            }
            case 2: done = true; break;
            default: {
                while (selection < 1 || selection > 2) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Invalid selection. Choose a number from the list above.\n\n"
                         << "Enter selection: ";
                    cin >> selection;
                }
            }
        }
    }

    if (edited) {
        send_buffer = "8" + to_string(user_token) + ",";
        for (unsigned int i = 0; i < account_details.size(); i++) {
            send_buffer += account_details[i] + ",";
        }

        _send_buffer = (char *) send_buffer.c_str();
        string this_response(send_to_server(_send_buffer));

        if (this_response == "false") {
            cout << "Network error: Account details could not be updated due to system error.";
        }
    }

    home_screen();
}

void delete_account() {
    system("clear");

    char confirm;
    cout << "Delete Account:\n--------------\n\n"
         << "You have chosen to delete your account. Do you want to proceed? (Y/N): ";
    cin >> confirm;

    if (confirm == 'Y' || confirm == 'y') {
        string send_buffer = "9" + to_string(user_token);
        char* _send_buffer = (char *) send_buffer.c_str();
        string response(send_to_server(_send_buffer));

        if (response == "true") {
            user_token = 0;
        } else {
            cout << "User account could not be deleted because it was not found in the system.\n";
        }
    } else {
        home_screen();
    }
}

void logout_user() {
    string send_buffer = "2*," + to_string(user_token);
    char* _send_buffer = (char *) send_buffer.c_str();
    send_to_server(_send_buffer);
}

long parse_date(string date) {
    string date_str = "";

    for (unsigned int i = 0; i < date.size(); i++) {
        switch (i) {
            case 4: i++; break;
            case 7: i++; break;
            case 10: i++; break;
            case 13: i++; break;
            default: break;
        }

        date_str += date.at(i);
    }

    return stol(date_str.c_str());
}

string date_to_string(long date_code) {
    long mm = date_code % 100;                   string mm_str = "";
    long hr = (date_code % 10000) / 100;         string hr_str = "";
    long DD = (date_code % 1000000) / 10000;     string DD_str = to_string(DD);
    long MM = (date_code % 100000000) / 1000000; string MM_str = "";
    long YYYY = date_code / 100000000;           string YYYY_str = to_string(YYYY);

    if (mm < 10) {
        mm_str = "0" + to_string(mm);
    } else {
        mm_str = to_string(mm);
    }

    if (hr < 10) {
        hr_str = "0" + to_string(hr);
    } else {
        hr_str = to_string(hr);
    }

    switch (MM) {
        case 1:  MM_str = "January";   break;
        case 2:  MM_str = "February";  break;
        case 3:  MM_str = "March";     break;
        case 4:  MM_str = "April";     break;
        case 5:  MM_str = "May";       break;
        case 6:  MM_str = "June";      break;
        case 7:  MM_str = "July";      break;
        case 8:  MM_str = "August";    break;
        case 9:  MM_str = "September"; break;
        case 10: MM_str = "October";   break;
        case 11: MM_str = "November";  break;
        case 12: MM_str = "December";  break;
        default: break;
    }

    string suffix = "";
    if (DD_str == "1" || DD_str == "21" || DD_str == "31") {
        suffix = "st";
    } else if (DD_str == "2" || DD_str == "22") {
        suffix = "nd";
    } else if (DD_str == "3" || DD_str == "23") {
        suffix = "rd";
    } else {
        suffix = "th";
    }

    return MM_str + " " + DD_str + suffix + ", " + YYYY_str + " " + hr_str + ":" + mm_str;
}