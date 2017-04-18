#include "Calender.h"
#include <iostream>
#include <fstream>

using namespace std;

static unsigned int user_count;
static unsigned int appt_count;
char* DB_FILE = (char *) "db.txt";

Calender::Calender() {
    load_db();
}

Calender::User::User() {
    id = 0;
    name = "";
    password = "";
    phone = "";
    email = "";
}

Calender::User::User(vector<string> u) {
    user_count++;
    id = user_count;
    name = u.at(0);
    email = u.at(1);
    phone = u.at(2);
    password = u.at(3);
}

Calender::Appointment::Appointment() {
    id = 0;
    owner = 0;
    start = 0;
    end = 0;
}

Calender::Appointment::Appointment(vector<long> a) {
    appt_count++;
    owner = (int) a.at(0);
    start = a.at(1);
    end = a.at(2);
    id = appt_count;
}

pair<bool, int> Calender::login_user(string username, string password) {
    bool login_success = false;
    int token = 0;

    for (unsigned int i = 0; i < user_list.size(); i++) {
        if (user_list.at(i).name == username ) {
            if (user_list.at(i).password == password) {
                login_success = true;
                bool logged_on = false;

                for (unsigned int j = 0; j < active_tokens.size(); j++) {
                    if (user_list.at(i).id == active_tokens.at(j)) {
                        logged_on = true;
                        break;
                    }
                }

                if (!logged_on) {
                    token = user_list.at(i).id;
                    active_tokens.push_back(token);
                    break;
                } else {
                    token = -1;
                }
            }
        }
    }

    return pair<bool, int>(login_success, token);
}

bool Calender::add_user(vector<string> new_user) {
    bool success = false;
    bool user_exists = false;

    for (unsigned int i = 0; i < user_list.size(); i++) {
        if (new_user.at(0) == user_list.at(i).name) {
            user_exists = true;
        }
    }

    if (!user_exists) {
        User u(new_user);
        if (u.id > 0) {
            user_list.push_back(u);
            success = true;
        }
    }

    if (success) {
        update_db();
    }

    return success;
}

bool Calender::remove_user(int user_id) {
    bool found = false;
    for (unsigned int i = 0; i < user_list.size(); i++) {
        if (user_list.at(i).id == user_id) {
            for (unsigned int j = 0; j < appt_list.size(); j++) {
                if (appt_list.at(j).owner == user_list.at(i).id) {
                    appt_list.erase(appt_list.begin() + j);
                }
            }
            user_list.erase(user_list.begin() + i);
            found = true;
            break;
        }
    }

    if (found) {
        update_db();
    }

    return found;
}

pair<bool, int> Calender::add_appt(int user_id, vector<long> new_appt) {
    bool success = false;
    Appointment a(new_appt);

    if (a.id > 0) {
        appt_list.push_back(a);
        success = true;
        refresh_conflicts(user_id);
    }

    if (success) {
        update_db();
    }

    return pair<bool, int>(success, a.id);
}

bool Calender::logout_user(int user_id) {
    bool success = false;

    for (unsigned int i = 0; i < active_tokens.size(); i++) {
        if (active_tokens.at(i) == user_id) {
            active_tokens.erase(active_tokens.begin() + i);
            success = true;
            break;
        }
    }

    return success;
}

bool Calender::update_appt(int user_id, pair<int, pair<long, long>> appt) {
    bool success = false;
    int appt_id = appt.first;

    for (unsigned int i = 0; i < appt_list.size(); i++) {
        if (appt_list.at(i).owner == user_id && appt_list.at(i).id == appt_id) {
            appt_list.at(i).start = appt.second.first;
            appt_list.at(i).end = appt.second.second;
            appt_list.at(i).conflict.clear();
            success = true;

            for (unsigned int j = 0; j < appt_list.size(); j++) {
                if (appt_list.at(j).owner == user_id) {
                    for (unsigned int k = 0; k < appt_list.at(j).conflict.size(); k++) {
                        if (appt_list.at(j).conflict.at(k) == appt_id) {
                            appt_list.at(j).conflict.erase(appt_list.at(j).conflict.begin() + k);
                        }
                    }
                }
            }

            refresh_conflicts(user_id);
            break;
        }
    }

    if (success) {
        update_db();
    }

    return success;
}
bool Calender::remove_appt(int appt_id) {
    bool success = false;

    for (unsigned int i = 0; i < appt_list.size(); i++) {
        if (appt_list.at(i).id == appt_id) {
            appt_list.erase(appt_list.begin() + i);
            success = true;

            for (unsigned int j = 0; j < appt_list.size(); j++) {
                for (unsigned int k = 0; k < appt_list.at(j).conflict.size(); k++) {
                    if (appt_list.at(j).conflict.at(k) == appt_id) {
                        appt_list.at(j).conflict.erase(appt_list.at(j).conflict.begin() + k);
                    }
                }
            }
            break;
        }
    }

    if (success) {
        update_db();
    }

    return success;
}

pair<vector<int>, vector<pair<long, long>>> Calender::get_all_appts(int user_id) {
    vector<int> appt_ids;
    vector<pair<long, long>> appt_data;
    for (unsigned int i = 0; i < appt_list.size(); i++) {
        if (appt_list.at(i).owner == user_id) {
            appt_ids.push_back(appt_list.at(i).id);
            appt_data.push_back(pair<long, long>(appt_list.at(i).start, appt_list.at(i).end));
        }
    }
    return pair<vector<int>, vector<pair<long, long>>>(appt_ids, appt_data);
}

void Calender::refresh_conflicts(int user_id) {
    pair<vector<int>, vector<pair<long, long>>> all_user_appts = get_all_appts(user_id);

    for (unsigned int i = 0; i < all_user_appts.first.size(); i++) {
        for (unsigned int j = 0; j < all_user_appts.first.size(); j++) {
            int id_a = all_user_appts.first.at(i);
            int id_b = all_user_appts.first.at(j);
            if (id_a != id_b) {
                long start_a = all_user_appts.second.at(i).first;
                long end_a = all_user_appts.second.at(i).second;
                long start_b = all_user_appts.second.at(j).first;
                long end_b = all_user_appts.second.at(j).second;

                // If A has a conflict with B, then B will have always have a conflict with A
                if (start_a >= start_b && start_a <= end_b ||
                    end_a >= start_b && end_a <= end_b ||
                    start_a <= start_b && end_a >= end_b) {

                    for (unsigned int k = 0; k < appt_list.size(); k++) {
                        if (appt_list.at(k).owner == user_id) {
                            if (appt_list.at(k).id == id_a) {
                                bool appt_id_exist = false;
                                for (unsigned int z = 0; z < appt_list.at(k).conflict.size(); z++) {
                                    if (appt_list.at(k).conflict.at(z) == id_b) {
                                        appt_id_exist = true;
                                        break;
                                    }
                                }

                                if (!appt_id_exist) {
                                    appt_list.at(k).conflict.push_back(id_b);
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    update_db();
}

vector<int> Calender::get_conflict_list(int appt_id) {
    vector<int> conflict_list;

    for (unsigned int i = 0; i < appt_list.size(); i++) {
        if (appt_list.at(i).id == appt_id) {
            if (!appt_list.at(i).conflict.empty()) {
                conflict_list = appt_list.at(i).conflict;
            }
        }
    }

    return conflict_list;
}

vector<string> Calender::get_account_details(int user_id) {
    vector<string> user_details;
    for (unsigned int i = 0; i < user_list.size(); i++) {
        if (user_list.at(i).id == user_id) {
            user_details.push_back(user_list.at(i).name);
            user_details.push_back(user_list.at(i).email);
            user_details.push_back(user_list.at(i).phone);
            user_details.push_back(user_list.at(i).password);
            break;
        }
    }
    return user_details;
}

bool Calender::edit_account_details(int user_id, vector<string> account_details) {
    bool success = false;
    for (unsigned int i = 0; i < user_list.size(); i++) {
        if (user_list.at(i).id == user_id) {
            user_list.at(i).name = account_details.at(0);
            user_list.at(i).email = account_details.at(1);
            user_list.at(i).phone = account_details.at(2);
            user_list.at(i).password = account_details.at(3);
            success = true;
            break;
        }
    }

    if (success) {
        update_db();
    }

    return success;
}

void Calender::load_db() {
    ifstream db_file(DB_FILE);
    string curr_line;

    if (db_file.good()) {
        getline(db_file, curr_line); user_count = (unsigned int) stoi(curr_line);
        getline(db_file, curr_line); appt_count = (unsigned int) stoi(curr_line);

        bool read_users = true;
        getline(db_file, curr_line); // curr_line == "# Users #
        getline(db_file, curr_line); // curr_line == "*"
        while (read_users) {
            User u;
            if (curr_line == "*") {
                getline(db_file, curr_line);
                u.id = (unsigned int) stoi(curr_line);
                getline(db_file, curr_line);
                u.name = curr_line;
                getline(db_file, curr_line);
                u.email = curr_line;
                getline(db_file, curr_line);
                u.phone = curr_line;
                getline(db_file, curr_line);
                u.password = curr_line;
                user_list.push_back(u);

                getline(db_file, curr_line); // curr_line == "@"
                getline(db_file, curr_line);
                if (curr_line != "*") {
                    read_users = false;
                }
            } else {
                read_users = false;
            }
        }

        bool read_appts = true;
        getline(db_file, curr_line); // curr_line == "*"
        while (read_appts) {
            Appointment a;
            if (curr_line == "*") {
                getline(db_file, curr_line); a.id = stoi(curr_line);
                getline(db_file, curr_line); a.owner = stoi(curr_line);
                getline(db_file, curr_line); a.start = stol(curr_line);
                getline(db_file, curr_line); a.end = stol(curr_line);
                getline(db_file, curr_line);
                for (int i = 0; i < curr_line.length(); i++) {
                    string conflict;
                    while (curr_line[i] != ' ') {
                        conflict += curr_line[i];
                        i++;
                    }
                    a.conflict.push_back(stoi(conflict));
                }
                appt_list.push_back(a);

                getline(db_file, curr_line); // curr_line == "@"
                getline(db_file, curr_line);
                if (curr_line != "*") {
                    read_appts = false;
                }
            } else {
                read_appts = false;
            }
        }
        db_file.close();
        cout << "DB file loaded\n";
    } else {
        ofstream {DB_FILE};
        cout << "DB file created\n";
        update_db();
    }
}

void Calender::update_db() {
    ofstream db_out;
    db_out.open(DB_FILE, ofstream::out | ofstream::trunc);

    db_out << user_count << endl << appt_count << endl;

    db_out << "# Users #\n";
    for (unsigned int i = 0; i < user_list.size(); i++) {
        User u = user_list.at(i);
        db_out << "*\n"
               << u.id << endl
               << u.name << endl
               << u.email << endl
               << u.phone << endl
               << u.password << endl
               << "@\n";
    }

    db_out << "# Appointments #\n";
    for (unsigned int i = 0; i < appt_list.size(); i++) {
        Appointment a = appt_list.at(i);
        db_out << "*\n"
               << a.id << endl
               << a.owner << endl
               << a.start << endl
               << a.end << endl;
        for (unsigned int j = 0; j < a.conflict.size(); j++) {
            db_out << a.conflict.at(j) << " ";
        }
        db_out << "\n@\n";
    }

    db_out.close();
    cout << "DB updated\n";
}