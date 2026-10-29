#include <cstdlib>
#include <iostream> 
#include <vector>
#include <string>

struct Message {
    std::string role;
    std::string content;
};

std::string chat(const std::vector<Message> &messages) {
    for (const Message &message : messages) 
        std::cout << message.role << ": " << message.content << '\n';

    return "test reply";
}

int main() {
    std::vector<Message> messages;

    messages.push_back({"user", "My name is batman"});
    std::string reply = chat(messages);

    messages.push_back({"assistant", reply});

    messages.push_back({"user", "What is my name?"});
    reply = chat(messages);

    messages.push_back({"assistant", reply});
    
    std::cout << reply << '\n';

    return 0;
}
