#include <iostream>
#include "unilink/util/input_validator.hpp"

int main() {
    bool valid = unilink::util::InputValidator::is_valid_ipv4("0.0.0.0");
    std::cout << "is_valid_ipv4(\"0.0.0.0\"): " << (valid ? "true" : "false") << std::endl;
    return valid ? 0 : 1;
}
