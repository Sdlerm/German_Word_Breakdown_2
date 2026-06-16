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
    vector<string> subparts;
};

unordered_map<string, Entry> dict;
const string DICTIONARY_FILE = "dictionary.txt";

string trim(string s) {
    while (!s.empty() && isspace(static_cast<unsigned char>(s.front()))) {
        s.erase(s.begin());
    }

    while (!s.empty() && isspace(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }

    return s;
}

string lowerCase(string s) {
    for (char& c : s) {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

vector<string> splitString(const string& line, char delimiter) {
    vector<string> result;
    string item;
    stringstream ss(line);

    while (getline(ss, item, delimiter)) {
        result.push_back(trim(item));
    }

    return result;
}

vector<string> splitCSV(const string& line) {
    vector<string> rawParts = splitString(line, ',');
    vector<string> result;

    for (const string& part : rawParts) {
        string cleaned = lowerCase(trim(part));

        if (!cleaned.empty()) {
            result.push_back(cleaned);
        }
    }

    return result;
}

string joinCSV(const vector<string>& items) {
    string result;

    for (size_t i = 0; i < items.size(); ++i) {
        result += items[i];

        if (i + 1 < items.size()) {
            result += ',';
        }
    }

    return result;
}

void addBuiltInFallbackEntries() {
    dict["s"] = {"linking element", {}};
    dict["es"] = {"linking element", {}};
    dict["n"] = {"linking element", {}};
    dict["en"] = {"linking element", {}};
    dict["er"] = {"linking element", {}};
    dict["e"] = {"linking element", {}};

    dict["lesezeichen"] = {"bookmark", {"lese", "zeichen"}};
    dict["lese"] = {"reading / from lesen", {"lesen"}};
    dict["lesen"] = {"to read", {}};
    dict["zeichen"] = {"sign / mark / symbol", {"zeichnen"}};
    dict["zeichnen"] = {"to draw / mark", {}};

    dict["geschlechtsverkehr"] = {
        "sexual intercourse",
        {"geschlecht", "s", "verkehr"}
    };
    dict["geschlecht"] = {"sex / gender", {}};
    dict["verkehr"] = {"traffic / transport / interaction / intercourse", {}};

    dict["mannschaft"] = {"team / crew", {"mann", "schaft"}};
    dict["mann"] = {"man / person", {}};
    dict["schaft"] = {
        "group / collective / state suffix, like English -ship",
        {}
    };

    dict["kraftfahrzeughaftpflichtversicherung"] = {
        "motor vehicle liability insurance",
        {"kraftfahrzeug", "haftpflicht", "versicherung"}
    };
    dict["kraftfahrzeug"] = {"motor vehicle", {"kraft", "fahrzeug"}};
    dict["kraft"] = {"power / force", {}};
    dict["fahrzeug"] = {"vehicle", {"fahr", "zeug"}};
    dict["fahr"] = {"drive/travel root", {"fahren"}};
    dict["fahren"] = {"to drive / go", {}};
    dict["zeug"] = {"thing / stuff", {}};

    dict["haftpflicht"] = {"liability", {"haft", "pflicht"}};
    dict["haft"] = {"liability / responsibility", {}};
    dict["pflicht"] = {"duty / obligation", {}};

    dict["versicherung"] = {"insurance", {"ver", "sicher", "ung"}};
    dict["ver"] = {"prefix; often changes/intensifies meaning", {}};
    dict["sicher"] = {"safe / secure / certain", {}};
    dict["ung"] = {"noun-forming suffix", {}};

    dict["geschwindigkeitsbegrenzungsschild"] = {
        "speed limit sign",
        {"geschwindigkeit", "s", "begrenzung", "s", "schild"}
    };
    dict["geschwindigkeit"] = {"speed", {"geschwind", "igkeit"}};
    dict["geschwind"] = {"swift / quick", {}};
    dict["igkeit"] = {"-ness suffix", {}};
    dict["begrenzung"] = {"limitation", {"be", "grenze", "ung"}};
    dict["be"] = {"prefix; often makes a verb transitive", {}};
    dict["grenze"] = {"border / limit", {}};
    dict["schild"] = {"sign / shield", {}};
}

bool loadDictionary(const string& filename) {
    ifstream file(filename);

    if (!file) {
        return false;
    }

    string line;
    int lineNumber = 0;

    while (getline(file, line)) {
        ++lineNumber;
        line = trim(line);

        if (line.empty() || line[0] == '#') {
            continue;
        }

        vector<string> fields = splitString(line, '|');

        if (fields.size() < 2) {
            cerr << "Skipping malformed line "
                 << lineNumber << ": " << line << '\n';
            continue;
        }

        string word = lowerCase(trim(fields[0]));
        string meaning = trim(fields[1]);
        vector<string> subparts;

        if (fields.size() >= 3) {
            subparts = splitCSV(fields[2]);
        }

        if (!word.empty()) {
            dict[word] = {meaning, subparts};
        }
    }

    return true;
}

bool saveEntryToFile(const string& filename, const string& word) {
    auto it = dict.find(word);

    if (it == dict.end()) {
        return false;
    }

    ofstream file(filename, ios::app);

    if (!file) {
        cerr << "Could not write to " << filename << '\n';
        return false;
    }

    file << word << '|'
         << it->second.meaning << '|'
         << joinCSV(it->second.subparts)
         << '\n';

    return true;
}

void createStarterDictionaryFile(const string& filename) {
    ofstream file(filename);

    if (!file) {
        cerr << "Could not create " << filename << '\n';
        return;
    }

    file << "# Format: word|meaning|subpart1,subpart2,subpart3\n";
    file << "# Subparts should be only words, not definitions.\n";
    file << "# Example: geschlechtsverkehr|sexual intercourse|geschlecht,s,verkehr\n";

    for (const auto& pair : dict) {
        file << pair.first << '|'
             << pair.second.meaning << '|'
             << joinCSV(pair.second.subparts)
             << '\n';
    }
}

void printTree(const string& word, int depth = 0) {
    string indent(depth * 2, ' ');

    auto it = dict.find(word);

    if (it == dict.end()) {
        cout << indent << "- " << word << " = unknown\n";
        return;
    }

    cout << indent << "- " << word << " = "
         << it->second.meaning << '\n';

    for (const string& sub : it->second.subparts) {
        printTree(sub, depth + 1);
    }
}

vector<string> splitUnknownCompound(const string& word) {
    vector<string> parts;
    size_t i = 0;

    while (i < word.size()) {
        string best;

        for (size_t len = word.size() - i; len >= 1; --len) {
            string piece = word.substr(i, len);

            if (dict.count(piece)) {
                best = piece;
                break;
            }

            if (len == 1) {
                break;
            }
        }

        if (!best.empty()) {
            parts.push_back(best);
            i += best.size();
        }
        else {
            parts.push_back(string(1, word[i]));
            ++i;
        }
    }

    return parts;
}

void analyzeWord() {
    string input;

    cout << "German word to analyze: ";
    getline(cin, input);

    input = lowerCase(trim(input));

    cout << "\n=== Breakdown ===\n";

    if (dict.count(input)) {
        printTree(input);
    }
    else {
        cout << "- " << input << " = unknown full compound\n";

        vector<string> parts = splitUnknownCompound(input);

        for (const string& part : parts) {
            printTree(part, 1);
        }
    }

    cout << '\n';
}

void addEntry() {
    string word;
    string meaning;
    string subpartLine;

    cout << "Word: ";
    getline(cin, word);
    word = lowerCase(trim(word));

    if (word.empty()) {
        cout << "No word entered. Entry not added.\n";
        return;
    }

    cout << "Meaning: ";
    getline(cin, meaning);
    meaning = trim(meaning);

    cout << "Subparts, separated by commas.\n";
    cout << "Use only words, not definitions.\n";
    cout << "Example: geschlecht,s,verkehr\n> ";
    getline(cin, subpartLine);

    dict[word] = {meaning, splitCSV(subpartLine)};

    cout << "Added: " << word << '\n';

    if (saveEntryToFile(DICTIONARY_FILE, word)) {
        cout << "Saved to " << DICTIONARY_FILE << '\n';
    }
}

void menu() {
    cout << "\nGerman Compound Analyzer\n";
    cout << "1. Analyze word\n";
    cout << "2. Add dictionary entry\n";
    cout << "3. Quit\n";
    cout << "> ";
}

int main() {
    addBuiltInFallbackEntries();

    if (!loadDictionary(DICTIONARY_FILE)) {
        cout << "No dictionary.txt found. Creating starter dictionary.txt...\n";
        createStarterDictionaryFile(DICTIONARY_FILE);
    }

    while (true) {
        menu();

        string choice;
        getline(cin, choice);
        choice = trim(choice);

        if (choice == "1") {
            analyzeWord();
        }
        else if (choice == "2") {
            addEntry();
        }
        else if (choice == "3") {
            cout << "Tschüss, Wörterzerleger.\n";
            break;
        }
        else {
            cout << "Unknown option.\n";
        }
    }

    return 0;
}
