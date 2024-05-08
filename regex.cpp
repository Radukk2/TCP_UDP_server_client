#include <iostream>
#include <regex>
#include <string>

int main() {
    std::string str = "hello, world";
    std::string pattern = "hel.*ld"; // Matches "hel" + zero or more characters + "ld"

    std::regex regexPattern(pattern);

    // Check if the pattern matches the string
    if (std::regex_search(str, regexPattern)) {
        std::cout << "Pattern '" << pattern << "' matches the string." << std::endl;
    } else {
        std::cout << "Pattern '" << pattern << "' does not match the string." << std::endl;
    }

    return 0;
}
