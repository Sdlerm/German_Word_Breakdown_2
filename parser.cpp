#include <fstream>
#include <iostream>
#include <regex>
#include <string>

using namespace std;

ifstream open_input_file()
{
    string filename;

    cout << "Input file: ";
    getline(cin, filename);

    ifstream in(filename);

    if (!in)
    {
        cerr << "Failed to open: "
             << filename
             << '\n';

        exit(1);
    }

    return in;
}

ofstream open_output_file()
{
    string filename;

    cout << "Output file: ";
    getline(cin, filename);

    ofstream out(filename);

    if (!out)
    {
        cerr << "Failed to create: "
             << filename
             << '\n';

        exit(1);
    }

    return out;
}

bool is_valid_word(const string& word)
{
    static const regex letters_only(
        "^[A-Za-zÄÖÜäöüß]+$"
    );

    return regex_match(word, letters_only);
}



int main()
{
    ifstream in = open_input_file();
    ofstream out = open_output_file();

    string word;

    int kept = 0;
    int discarded = 0;

    while (getline(in, word))
    {
        if (is_valid_word(word))
        {
            out << word << '\n';
            ++kept;
        }
        else
        {
            ++discarded;
        }
    }

    cout << "\nFinished!\n";
    cout << "Kept: " << kept << '\n';
    cout << "Discarded: " << discarded << '\n';

    return 0;
}
