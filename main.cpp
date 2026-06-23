#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct Entry {
    string meaning;
    vector<string> parts;
};

unordered_map<string, Entry> dictionary;
const string FILE_NAME = "dictionary.txt";

// Remove spaces from the start and end of a string
string trim(string s) {
    while (!s.empty() && isspace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }
    while (!s.empty() && isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    return s;
}

// Convert text to lowercase
string lower(string s) {
    for (char& c : s) {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

// Split a string by a delimiter
vector<string> split(const string& s, char delimiter) {
    vector<string> items;
    string item;
    stringstream ss(s);

    while (getline(ss, item, delimiter)) {
        item = trim(item);
        if (!item.empty()) {
            items.push_back(item);
        }
    }

    return items;
}

// Join a list of strings with commas
string join(const vector<string>& items) {
    string out;
    for (size_t i = 0; i < items.size(); i++) {
        out += items[i];
        if (i + 1 < items.size()) {
            out += ',';
        }
    }
    return out;
}

// Add a few built-in words so the program can work right away
void addBuiltInWords() {
    dictionary["mann"] = {"man", {}};
    dictionary["schaft"] = {"group", {}};
    dictionary["mannschaft"] = {"team", {"mann", "schaft"}};

    dictionary["lesen"] = {"to read", {}};
    dictionary["lesezeichen"] = {"bookmark", {"lesen", "zeichen"}};
    dictionary["zeichen"] = {"sign", {}};
}

// Load dictionary data from file
bool loadDictionary(const string& filename) {
    ifstream file(filename);
    if (!file) {
        return false;
    }

    string line;
    while (getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        vector<string> fields = split(line, '|');
        if (fields.size() < 2) {
            continue;
        }

        string word = lower(trim(fields[0]));
        string meaning = trim(fields[1]);
        vector<string> parts;

        if (fields.size() >= 3) {
            parts = split(fields[2], ',');
            for (string& part : parts) {
                part = lower(part);
            }
        }

        dictionary[word] = {meaning, parts};
    }

    return true;
}

// Save one word to the file
void saveEntry(const Entry& e) {
    ofstream file(FILE_NAME, ios::app);
    if (!file) {
        return;
    }

    file << e.word << '|' << e.meaning << '|' << join(e.parts) << '\n';
}

// Print a word and its parts
void printWord(const string& word, int depth = 0) {
    for (int i = 0; i < depth; i++) {
        cout << "  ";
    }

    auto it = dictionary.find(word);
    if (it == dictionary.end()) {
        cout << "- " << word << " = unknown\n";
        return;
    }

    cout << "- " << word << " = " << it->second.meaning << '\n';
    for (const string& part : it->second.parts) {
        printWord(part, depth + 1);
    }
}

// Ask the user for a word and show the breakdown
void analyzeWord() {
    string input;
    cout << "German word: ";
    getline(cin, input);
    input = lower(trim(input));

    printWord(input);
}

// Add a new entry
void addEntry() {
    Entry e;
    string partsLine;

    cout << "Word: ";
    getline(cin, e.word);
    e.word = lower(trim(e.word));

    cout << "Meaning: ";
    getline(cin, e.meaning);
    e.meaning = trim(e.meaning);

    cout << "Parts (comma separated): ";
    getline(cin, partsLine);
    e.parts = split(partsLine, ',');
    for (string& part : e.parts) {
        part = lower(part);
    }

    dictionary[e.word] = e;
    saveEntry(e);

    cout << "Saved!\n";
}

int main() {
    addBuiltInWords();

    if (!loadDictionary(FILE_NAME)) {
        ofstream file(FILE_NAME);
        file << "# word|meaning|parts\n";
    }

    while (true) {
        cout << "\n1. Analyze word\n2. Add entry\n3. Quit\n> ";
        string choice;
        getline(cin, choice);

        if (choice == "1") {
            analyzeWord();
        } else if (choice == "2") {
            addEntry();
        } else if (choice == "3") {
            break;
        } else {
            cout << "Unknown option.\n";
        }
    }

    return 0;
}
