#ifndef CALENDER_H
#define CALENDER_H

#include <vector>
#include <string>

using namespace std;

class Calender {
private:
    struct User {
        unsigned int id;
        string name;
        string password;
        string phone;
        string email;

        User();
        User(vector<string> u);
    };

    struct Appointment {
        int id;
        int owner;
        long start;
        long end;
        vector<int> conflict; // -1 if no conflict, appt_id if yes

        Appointment();
        Appointment(vector<long> a);
    };

    typedef vector<User> Users;
    typedef vector<int> TokenList;
    typedef vector<Appointment> Appointments;

    Users user_list;
    TokenList active_tokens;
    Appointments appt_list;
    void refresh_conflicts(int);


public:
    Calender();
    pair<bool, int> login_user(string, string);
    bool add_user(vector<string>);
    bool remove_user(int);
    bool logout_user(int);
    pair<bool, int> add_appt(int, vector<long>);
    bool update_appt(int, pair<int, pair<long, long>>);
    bool remove_appt(int);
    pair<vector<int>, vector<pair<long, long>>> get_all_appts(int);
    vector<int> get_conflict_list(int);
    vector<string> get_account_details(int);
    bool edit_account_details(int, vector<string>);
    void load_db();
    void update_db();
};
#endif