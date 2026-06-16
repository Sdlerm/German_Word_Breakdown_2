#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Entry {
    std::string meaning;
    std::vector<std::string> subparts;
};

std::unordered_map<std::string, Entry> dict;
const std::string DICTIONARY_FILE = "dictionary.txt";

// ------------------------------------------------------------
// String utilities
// ------------------------------------------------------------

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string lowerCase(std::string s) {
    for (char& c : s)
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

std::vector<std::string> splitString(const std::string& line, char delimiter) {
    std::vector<std::string> result;
    std::string item;
    std::stringstream ss(line);
    while (std::getline(ss, item, delimiter))
        result.push_back(trim(item));
    return result;
}

std::vector<std::string> splitCSV(const std::string& line) {
    std::vector<std::string> raw = splitString(line, ',');
    std::vector<std::string> result;
    for (const std::string& part : raw) {
        std::string cleaned = lowerCase(trim(part));
        if (!cleaned.empty()) result.push_back(cleaned);
    }
    return result;
}

std::string joinCSV(const std::vector<std::string>& items) {
    std::string result;
    for (size_t i = 0; i < items.size(); ++i) {
        result += items[i];
        if (i + 1 < items.size()) result += ',';
    }
    return result;
}

// ------------------------------------------------------------
// Dictionary I/O
// ------------------------------------------------------------

void addLinkingElements() {
    for (const std::string& e : {"s", "es", "n", "en", "er", "e"})
        dict[e] = {"linking element", {}};
}

bool loadDictionary(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) return false;

    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::vector<std::string> fields = splitString(line, '|');
        if (fields.size() < 2) {
            std::cerr << "Skipping malformed line " << lineNumber << ": " << line << '\n';
            continue;
        }

        std::string word    = lowerCase(trim(fields[0]));
        std::string meaning = trim(fields[1]);
        std::vector<std::string> subparts;
        if (fields.size() >= 3) subparts = splitCSV(fields[2]);
        if (!word.empty()) dict[word] = {meaning, subparts};
    }
    return true;
}

bool saveEntryToFile(const std::string& filename, const std::string& word) {
    auto it = dict.find(word);
    if (it == dict.end()) return false;

    std::ofstream file(filename, std::ios::app);
    if (!file) { std::cerr << "Could not write to " << filename << '\n'; return false; }

    file << word << '|' << it->second.meaning << '|' << joinCSV(it->second.subparts) << '\n';
    return true;
}

void createStarterDictionaryFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file) { std::cerr << "Could not create " << filename << '\n'; return; }

    file << "# Format: word|meaning|subpart1,subpart2,...\n";
    file << "# Subparts are words only, not definitions.\n";
    file << "# Example: anrufen|to call (phone)|an,rufen\n";

    std::vector<std::string> keys;
    keys.reserve(dict.size());
    for (const auto& pair : dict) keys.push_back(pair.first);
    std::sort(keys.begin(), keys.end());

    for (const std::string& k : keys)
        file << k << '|' << dict[k].meaning << '|' << joinCSV(dict[k].subparts) << '\n';
}

// ------------------------------------------------------------
// Verb list import
// ------------------------------------------------------------
//
// Import file format (UTF-8 plain text, e.g. verbs.txt):
//
//   # Lines starting with # are comments.
//   #
//   # Each data line:  word|meaning|subpart1,subpart2,...
//   #
//   # The subparts field is optional but strongly recommended for
//   # separable verbs so the analyzer can show the tree.
//   #
//   # Atomic base words (prefixes, roots) can be listed too:
//   #   rufen|to call|
//   #   an|separable prefix: on / toward / at|
//   #
//   # Then the compound:
//   #   anrufen|to call (phone)|an,rufen
//   #
//   # Any subpart not found in the dict AND not defined elsewhere
//   # in the import file gets a placeholder entry so the tree
//   # doesn't print "unknown".  Fill it in manually later.
//
// Usage inside the program: choose option 3 (skip existing) or
// option 4 (overwrite existing entries).
//
// You can also bulk-generate verbs.txt from any online list and
// paste it in; the only requirement is the pipe-delimited format.
// ------------------------------------------------------------

struct ImportStats {
    int imported  = 0;
    int skipped   = 0;
    int malformed = 0;
};

// Write one entry to dict + file.  Returns true if actually written.
static bool writeEntry(const std::string& word,
                       const std::string& meaning,
                       const std::vector<std::string>& subparts,
                       bool overwrite,
                       const std::string& filename,
                       ImportStats& stats)
{
    if (!overwrite && dict.count(word)) { ++stats.skipped; return false; }

    dict[word] = {meaning, subparts};

    std::ofstream file(filename, std::ios::app);
    if (!file) { std::cerr << "Cannot append to " << filename << '\n'; return false; }

    file << word << '|' << meaning << '|' << joinCSV(subparts) << '\n';
    ++stats.imported;
    return true;
}

// For any subpart that has no dict entry yet, add a placeholder so the
// tree resolves.  The user can fill in proper meanings later.
static void stubMissingSubparts(const std::vector<std::string>& subparts,
                                const std::string& parentWord,
                                bool overwrite,
                                const std::string& filename,
                                ImportStats& stats)
{
    for (const std::string& part : subparts) {
        if (part.empty() || dict.count(part)) continue;
        std::string stub = "component of " + parentWord + " -- add meaning manually";
        writeEntry(part, stub, {}, overwrite, filename, stats);
    }
}

ImportStats importVerbList(const std::string& importFile,
                           const std::string& dictFile,
                           bool overwrite)
{
    ImportStats stats;

    std::ifstream file(importFile);
    if (!file) {
        std::cerr << "Cannot open: " << importFile << '\n';
        return stats;
    }

    // --- Pass 1: collect all words defined in this file so that
    //     cross-references within the file don't generate stubs. ---
    {
        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            std::vector<std::string> fields = splitString(line, '|');
            if (fields.size() < 2) continue;
            std::string word = lowerCase(trim(fields[0]));
            std::string meaning = trim(fields[1]);
            std::vector<std::string> subparts;
            if (fields.size() >= 3) subparts = splitCSV(fields[2]);
            // Pre-load into dict (tentative) so stubMissingSubparts
            // skips these during pass 2.
            if (!word.empty() && !dict.count(word))
                dict[word] = {meaning, subparts};
        }
    }

    // --- Pass 2: write everything to disk in file order. ---
    file.clear();
    file.seekg(0);

    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        ++lineNum;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::vector<std::string> fields = splitString(line, '|');
        if (fields.size() < 2) {
            std::cerr << "  Malformed line " << lineNum << ": " << line << '\n';
            ++stats.malformed;
            continue;
        }

        std::string word    = lowerCase(trim(fields[0]));
        std::string meaning = trim(fields[1]);
        std::vector<std::string> subparts;
        if (fields.size() >= 3) subparts = splitCSV(fields[2]);

        if (word.empty()) { ++stats.malformed; continue; }

        writeEntry(word, meaning, subparts, overwrite, dictFile, stats);
        stubMissingSubparts(subparts, word, overwrite, dictFile, stats);
    }

    return stats;
}

void runImport(bool overwrite) {
    std::string importFile;
    std::cout << "Import file path (e.g. verbs.txt): ";
    std::getline(std::cin, importFile);
    importFile = trim(importFile);
    if (importFile.empty()) { std::cout << "No file specified.\n"; return; }

    std::cout << (overwrite ? "Importing (overwrite mode)" : "Importing (skip existing)")
              << " from \"" << importFile << "\"...\n";

    ImportStats s = importVerbList(importFile, DICTIONARY_FILE, overwrite);

    std::cout << "Done.\n"
              << "  Imported : " << s.imported  << '\n'
              << "  Skipped  : " << s.skipped   << "  (already in dict)\n"
              << "  Malformed: " << s.malformed  << "  lines\n";
}

// ------------------------------------------------------------
// Word analysis
// ------------------------------------------------------------

void printTree(const std::string& word, int depth = 0,
               std::unordered_set<std::string> visited = {})
{
    std::string indent(depth * 2, ' ');

    if (visited.count(word)) {
        std::cout << indent << "- " << word << " (cycle detected)\n";
        return;
    }

    auto it = dict.find(word);
    if (it == dict.end()) {
        std::cout << indent << "- " << word << " = unknown\n";
        return;
    }

    std::cout << indent << "- " << word << " = " << it->second.meaning << '\n';
    visited.insert(word);
    for (const std::string& sub : it->second.subparts)
        printTree(sub, depth + 1, visited);
}

bool segmentHelper(const std::string& word, size_t start,
                   std::vector<std::string>& parts,
                   std::unordered_map<size_t, bool>& memo)
{
    if (start == word.size()) return true;
    auto it = memo.find(start);
    if (it != memo.end() && !it->second) return false;

    for (size_t len = word.size() - start; len >= 1; --len) {
        std::string piece = word.substr(start, len);
        if (dict.count(piece)) {
            parts.push_back(piece);
            if (segmentHelper(word, start + len, parts, memo)) {
                memo[start] = true;
                return true;
            }
            parts.pop_back();
        }
    }
    memo[start] = false;
    return false;
}

std::vector<std::string> greedySplit(const std::string& word) {
    std::vector<std::string> parts;
    size_t i = 0;
    while (i < word.size()) {
        std::string best;
        for (size_t len = word.size() - i; len >= 1; --len) {
            if (dict.count(word.substr(i, len))) { best = word.substr(i, len); break; }
        }
        if (!best.empty()) { parts.push_back(best); i += best.size(); }
        else               { parts.push_back(std::string(1, word[i])); ++i; }
    }
    return parts;
}

std::vector<std::string> splitUnknownCompound(const std::string& word) {
    std::vector<std::string> parts;
    std::unordered_map<size_t, bool> memo;
    if (segmentHelper(word, 0, parts, memo)) return parts;
    return greedySplit(word);
}

void analyzeWord() {
    std::string input;
    std::cout << "German word to analyze: ";
    std::getline(std::cin, input);
    input = lowerCase(trim(input));
    if (input.empty()) { std::cout << "No input.\n"; return; }

    std::cout << "\n=== Breakdown ===\n";
    if (dict.count(input)) {
        printTree(input);
    } else {
        std::cout << "- " << input << " = unknown full compound\n";
        for (const std::string& part : splitUnknownCompound(input))
            printTree(part, 1);
    }
    std::cout << '\n';
}

// ------------------------------------------------------------
// Manual entry
// ------------------------------------------------------------

void addEntry() {
    std::string word, meaning, subpartLine;

    std::cout << "Word: ";
    std::getline(std::cin, word);
    word = lowerCase(trim(word));
    if (word.empty()) { std::cout << "No word entered.\n"; return; }

    if (dict.count(word)) {
        std::cout << "Warning: '" << word << "' already exists. Overwrite? (y/n): ";
        std::string confirm;
        std::getline(std::cin, confirm);
        if (trim(lowerCase(confirm)) != "y") { std::cout << "Cancelled.\n"; return; }
    }

    std::cout << "Meaning: ";
    std::getline(std::cin, meaning);
    meaning = trim(meaning);

    std::cout << "Subparts (comma-separated words, e.g. an,rufen):\n> ";
    std::getline(std::cin, subpartLine);

    dict[word] = {meaning, splitCSV(subpartLine)};
    std::cout << "Added: " << word << '\n';
    if (saveEntryToFile(DICTIONARY_FILE, word))
        std::cout << "Saved to " << DICTIONARY_FILE << '\n';
}

// ------------------------------------------------------------
// Main loop
// ------------------------------------------------------------

void menu() {
    std::cout << "\nGerman Compound Analyzer\n"
              << "1. Analyze word\n"
              << "2. Add dictionary entry\n"
              << "3. Import verb list        (skip existing entries)\n"
              << "4. Import verb list        (overwrite existing entries)\n"
              << "5. Quit\n"
              << "> ";
}

int main() {
    addLinkingElements();

    if (!loadDictionary(DICTIONARY_FILE)) {
        std::cout << "No dictionary.txt found. Creating starter dictionary.txt...\n";
        createStarterDictionaryFile(DICTIONARY_FILE);
    }

    while (true) {
        menu();
        std::string choice;
        std::getline(std::cin, choice);
        choice = trim(choice);

        if      (choice == "1") analyzeWord();
        else if (choice == "2") addEntry();
        else if (choice == "3") runImport(false);
        else if (choice == "4") runImport(true);
        else if (choice == "5") { std::cout << "Tschüss, Wörterzerleger.\n"; break; }
        else                    std::cout << "Unknown option.\n";
    }

    return 0;
}
