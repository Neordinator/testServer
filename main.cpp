#include <iostream>
#include <uwebsockets/App.h>
using namespace std;

struct PerSocketData {
	unsigned int id;
	string name;
};

const string PRIVATE_MESSAGE = "PRIVATE_MESSAGE";
const string SET_NAME = "SET_NAME";
const string ONLINE = "ONLINE";
const string OFFLINE = "OFFLINE";

string  online(PerSocketData* userData) {
    return ONLINE + "::" + to_string(userData->id) + "::" + userData->name;
}
string  offline(PerSocketData* userData) {
    return OFFLINE + "::" + to_string(userData->id) + "::" + userData->name;
}

string	parsePrivateMessageID(string event) {
	string rest = event.substr(PRIVATE_MESSAGE.size() + 2);
	rest.find("::");
	int pos = rest.find("::");
	return rest.substr(0, pos); // 0-2 => "22"
}

string	parsePrivateMessage(string event) {
	string rest = event.substr(PRIVATE_MESSAGE.size() + 2);
	rest.find("::");
	int pos = rest.find("::");
	return rest.substr(pos + 2); // с 4 символа до конца => "Привет, Петруха"
}

// 10, "Йоу" => "PRIVATE_MESSAGE::10::Йоу!"
string	createPrivateMessage(string sender_id, string message, string senderName) {
	return PRIVATE_MESSAGE + "::" + sender_id + "::[" + senderName + "]: " + message;
}

string parseUserName(string event) {
    return event.substr(SET_NAME.size() + 2);
}

bool isPrivateMessage(string event) {
    return event.find(PRIVATE_MESSAGE) == 0;
}

bool isSetName(string event) {
    return event.find(SET_NAME) == 0;
}

map<unsigned int, string> userNames;
void setUser(PerSocketData* userData) {
    userNames[userData->id] = userData->name;
}

int	main() {
    unsigned int last_user_id = 10;
    unsigned int all_users = 0;
    /* Keep in mind that uWS::SSLApp({options}) is the same as uWS::App() when compiled without SSL support.
     * You may swap to using uWS:App() if you don't need SSL */
    /*uWS::SSLApp({
        //Это всё для защищенного HTTPS
        There are example certificates in uWebSockets.js repo
        .key_file_name = "../misc/key.pem",
        .cert_file_name = "../misc/cert.pem",
        .passphrase = "1234"
        })*/
    uWS::App().ws<PerSocketData>("/*", {
        .idleTimeout = 3600,
        .open = [&last_user_id, &all_users](auto* ws) {
            /* Open event here, you may access ws->getUserData() which points to a PerSocketData struct */
            PerSocketData* userData = ws->getUserData();
            userData->name = "UNNAMED";
            userData->id = last_user_id++;
            all_users++;
            ws->subscribe("user" + to_string(userData->id));
            ws->subscribe("broadcast");
            cout << "New user connected: " << userData->id;
            for (auto entry : userNames) {
                ws->send(online(entry), uWS::OpCode::TEXT);
            }
            setUser(userData);
            cout << "Total users connected: " << all_users << endl;
        },
        .message = [](auto* ws, std::string_view eventText, uWS::OpCode opCode) {
            PerSocketData* userData = ws->getUserData();
            string eventString(eventText);

            if (isPrivateMessage(eventString)) {
                string recieverId = parsePrivateMessageID(eventString);
                string text = parsePrivateMessage(eventString);
                string senderId = to_string(userData->id);
                string eventToSend = createPrivateMessage(senderId, text, userData->name);
                ws->publish("user" + recieverId, eventToSend);
                cout << "User " << senderId << " sent message to " << recieverId << endl;
            }
            if (isSetName(eventString)) {
                string name = parseUserName(eventString);
                userData->name = name;
                ws->publish("broadcast", online(userData));
                cout << "User " << userData->id << " set his name" << endl;
            }
        },
        .close = [](auto* ws, int code, std::string_view message) {
            PerSocketData* userData = ws->getUserData();
            ws->publish("broadcast", offline(userData));
            cout << "User disconnected" << userData->id;
            // При отключении
        },
   
            
            ).listen(9001, [](auto* listen_socket) {
                if (listen_socket) {
                    std::cout << "Listening on port " << 9001 << std::endl;
                }
                }).run();
	return 0;
}