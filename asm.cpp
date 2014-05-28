#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

struct deferment {
    unsigned int line_number;
    uint32_t address;
    std::vector<std::string> words;
    deferment(unsigned int line_number_, uint32_t address_, const std::vector<std::string>& words_) :
        line_number(line_number_),
        address(address_),
        words(words_)
    {}
} 

int main(int argc, char **argv)
{
    unsigned int line_number = 0;
    std::string line;
    uint32_t address = 0;
    std::map<std::string, uint32_t> label_addresses;
    std::vector<deferment> deferred;

    while(!std::getline(std::cin, line).eof()) {

        // XXX Can't actually do these; strings must be preserved

        // clean up arguments " *, *" -> ","
        boost::regex re("\\s*,\\s*");
        line = boost::regex_replace(line, re, ",");

        // compress whitespace "  " -> " "
        re = boost::regex("\\s+");
        line = boost::regex_replace(line, re, " ");

        // strip of leading whitespace "^ *" -> ""
        re = boost::regex("^\\s*");
        line = boost::regex_replace(line, re, "");

        // remove comments "//.*" -> ""
        re = boost::regex("\\s*//.*");
        line = boost::regex_replace(line, re, "");

        if(line.empty())
            continue;

        std::vector<std::string> words;
        boost::split(words, line, boost::is_any_of(" "));

        if(words.size() == 0)
            continue;

        if(words[0][words[0].size() - 1] == ':') {
            words[0].erase(words[0].size() - 1);
            std::cout << words[0] << " is a label." << std::endl;
            words.erase(words.begin(), words.begin() + 1);
        }

        if(words.size() == 0)
            continue;

        std::cout << words[0] << std::endl;

        // split instr name at .
        if(words[0] == ".org") {
        } else if(words[1] == ".byte") {
            for(auto it = words.begin() + 1; it != words.end(); it++) {
                unsigned int v = strtol(*it, NULL, 0);
                if(v > 255) {
                    printf("line %d: value %u exceeds byte range\n", line_number, v);
                    exit(EXIT_FAILURE);
                }
                memory_write_byte(address, strtol(*it, NULL, 0));
                address += 1;
            }
        } else if(words[1] == ".short") {
            for(auto it = words.begin() + 1; it != words.end(); it++) {
                unsigned int v = strtol(*it, NULL, 0);
                if(v > 65535) {
                    printf("line %d: value %u exceeds word range\n", line_number, v);
                    exit(EXIT_FAILURE);
                }
                memory_write_byte(address, strtol(*it, NULL, 0));
                address += 1;
            }
        } else if(words[1] == ".word") {
            for(auto it = words.begin() + 1; it != words.end(); it++) {
                unsigned int v = strtol(*it, NULL, 0);
                memory_write_word(address, strtol(*it, NULL, 0));
                address += 1;
            }
        } else if(words[1] == ".string") {
            for(auto it = words.begin() + 1; it != words.end(); it++) {
                std::string& s = *it;
                if(s[0] != "\"" || s[s.size() - 1] != "\"") {
                    printf("line %d: expected double quotes at beginning and end of string\n");
                    exit(EXIT_FAILURE);
                }
                memory_write_word(address, strtol(*it, NULL, 0));
                address += 1;
            }
        } else
            deferred.push_back(deferment(line_number, address, words));
        line_number++;
    }

#if 0
    for all deferred,
        if(rx), parse rx
        if(ry), parse ry
        if(mod)
            if has ., parse modifier
            else error
        else
            if has ., error
            else nothing
        number = parse(data) # do dec, hex, labels, .h, .l
        shift number 
        add address if necessary
        verify number fits in datasize
        verify number is signed or unsigned
#endif
}

#if 0
.org addr
.byte value [, value]
.short value [, value] // fail on bad alignment?
.word value [, value] // fail on bad alignment?
.string "blarg"
[label:] instr[.modifier] [r# [, r# [, data]]]

// always align to 4 bytes before instruction
instr_struct_map
    'mnemonic' : opcode, need_rx, need_ry, mod, datasize, signed, shift, relative
    'and', 0x00, 1, 1, 0, 0, 0, 0, 0
    'or', 0x01, 1, 1, 0, 0, 0, 0, 0
    'xor', 0x02, 1, 1, 0, 0, 0, 0, 0
    'not', 0x03, 1, 1, 0, 0, 0, 0, 0
    'add', 0x04, 1, 1, 0, 0, 0, 0, 0
    'adc', 0x05, 1, 1, 0, 0, 0, 0, 0
    'sub', 0x06, 1, 1, 0, 0, 0, 0, 0
    'mult', 0x07, 1, 1, 0, 0, 0, 0, 0
    'div', 0x07, 1, 1, 0, 0, 0, 0, 0
    'cmp', 0x09, 1, 1, 0, 0, 0, 0, 0
    'xchg', 0x0a, 1, 1, 0, 0, 0, 0, 0
    'mov', 0x0b, 1, 1, 0, 0, 0, 0, 0
    'moviu, 0x10, 1, 0, 0, 16, 0, 0, 0
    'addi, 0x11, 1, 0, 0, 24, 1, 0, 0
    'addiu, 0x12, 1, 0, 0, 24, 0, 0, 0
    'cmpiu, 0x13, 1, 0, 0, 24, 0, 0, 0
    'shift, 0x14, 1, 0, 0, 5, 0, 0, 0
    'jl', 0x15, 0, 0, 0, 27, 1, 2, 1, 
    'jne', 0x16, 0, 0, 0, 27, 1, 2, 1, 
    'jr', 0x17, 0, 0, 0, 24, 1, 2, 1, 
    'jsr', 0x18, 0, 0, 0, 24, 1, 2, 1, 
    'rsr', 0x19, 1, 0, 0, 0, 0, 0, 0,
    'jmp', 0x1a, 0, 0, 0, 27, 0, 2, 0, 
    'sys', 0x1b, 0, 0, 0, 6, 0, 0, 0, 
    'swapcc', 0x1c, 1, 0, 0, 0, 0, 0, 0, 
    'halt', 0x1f, 0, 0, 0, 0, 0, 0, 0, 
#endif
