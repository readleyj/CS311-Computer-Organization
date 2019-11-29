#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <map>
#include <bitset>
#include <regex>

const int BASE_DATA_ADDRESS = 0x10000000;
const int BASE_TEXT_ADDRESS = 0x400000;
const int MASK = 0xFFFF;

std::map <std::string, std::vector<std::string>> data_labels;
std::map <std::string, std::vector<std::vector<std::string>>> text_branches;
std::map<std::string, int> label_addresses;
std::map<std::string, int> branch_addresses;
std::vector <std::string> data_labels_order;
std::vector <std::string> text_branches_order;

unsigned int num_words = 0;
unsigned int num_instructions = 0;
unsigned int current_ins_line = 0;

std::map <std::string, std::string> R_OP_VALUES;
std::map  <std::string, std::string> I_OP_VALUES;
std::map <std::string, std::string> J_OP_VALUES;
std::map <std::string, std::string> I_BRANCH_OP_VALUES;
std::map <std::string, std::string> R_SHIFTER_OP_VALUES;
std::map<std::string, std::string> I_MEMORY_OP_VALUES;

unsigned int convert_to_int(std::string value)
{
    unsigned int int_val = (value.size() >= 2 && value[1] == 'x') ? std::stoi(value, nullptr, 16) : std::stoi(value, nullptr, 10);
    return int_val;
}

void get_data()
{
    std::string input;
    std::string previous_data_label;
    std::string previous_text_branch;
    std::regex dec_or_hex("\\b(0x[0-9a-fA-F]+|[0-9]+)\\b");
    std::regex label_matcher("[^:]+");
    std::regex text_parser("[-a-z0-9]+");

    while (getline(std::cin, input))
    {
        if (input.find(".data") != std::string::npos)
        {
            continue;
        }

        if (input.find(".text") != std::string::npos)
        {
            break;
        }

        if (input.find(".word") != std::string::npos)
        {
            std::smatch match;

            if (input.find(':') != std::string::npos)
            {
                std::smatch label;
                regex_search(input, label, label_matcher);
                previous_data_label =  label[0];
                label_addresses[previous_data_label] = BASE_DATA_ADDRESS + num_words * 4;
                data_labels_order.push_back(previous_data_label);
            }

            while (regex_search(input, match, dec_or_hex))
            {
                data_labels[previous_data_label].push_back(match[0]);
                input = match.suffix().str();
            }

            num_words++;
        }
    }

    while (getline(std::cin, input))
    {
        if (input.find(':') != std::string::npos)
        {
            std::smatch label;
            regex_search(input, label, label_matcher);
            previous_text_branch = label[0];
            branch_addresses[previous_text_branch] = BASE_TEXT_ADDRESS + num_instructions * 4;
            text_branches_order.push_back(previous_text_branch);
            continue;
        }

        num_instructions++;
        std::smatch match;
        std::vector<std::string> instructions;

        while (regex_search(input, match, text_parser))
        {
            for (auto m : match)
            {
                instructions.push_back(m);
            }

            input = match.suffix().str();
        }

        text_branches[previous_text_branch].push_back(instructions);

        if (instructions[0] == "la")
        {
            unsigned int address = label_addresses[instructions[2]];
            if ((address & MASK) != 0) num_instructions++;
        }
    }
}

std::string make_r(std::vector<std::string> instruction)
{
    std::string ins_name = instruction[0];
    std::string res;
    std::bitset<5> sa;
    std::bitset<6> op;
    std::bitset<5> rd;
    std::bitset<5> rs;
    std::bitset<5> rt;
    std::string funct = R_OP_VALUES[ins_name];

    if (ins_name == "jr") {
        rs = std::bitset<5>(convert_to_int(instruction[1]));
    } else
    {
        rd = std::bitset<5>(convert_to_int(instruction[1]));
        rs = std::bitset<5>(convert_to_int(instruction[2]));
        rt = std::bitset<5>(convert_to_int(instruction[3]));
    }

    res += op.to_string() + rs.to_string() + rt.to_string() + rd.to_string() + sa.to_string() + funct;
    return res;
}

std::string make_i(std::vector<std::string> instruction)
{
    std::string res;
    std::string op = I_OP_VALUES[instruction[0]];
    std::bitset<5> rt = convert_to_int(instruction[1]);
    std::bitset<5> rs;
    std::bitset<16> imm;

    if (instruction[0] != "lui") {
        rs = convert_to_int(instruction[2]);
        imm = convert_to_int(instruction[3]);
    } else
    {
        imm = convert_to_int(instruction[2]);
    }

    res += op + rs.to_string() + rt.to_string() + imm.to_string();
    return res;
}

std::string make_r_shifter(std::vector<std::string> instruction)
{
    std::string res;
    std::string funct = R_SHIFTER_OP_VALUES[instruction[0]];
    std::bitset<5> rs;
    std::bitset<6> op;
    std::bitset<5> rd = convert_to_int(instruction[1]);
    std::bitset<5> rt = convert_to_int(instruction[2]);
    std::bitset<5> sa = convert_to_int(instruction[3]);

    res += op.to_string() + rs.to_string() + rt.to_string() + rd.to_string() + sa.to_string() + funct;
    return res;
}

std::string make_i_memory(std::vector<std::string> instruction)
{
    std::string res;
    std::string op = I_MEMORY_OP_VALUES[instruction[0]];
    std::bitset<5> rt = convert_to_int(instruction[1]);
    std::bitset<16> offset = convert_to_int(instruction[2]);
    std::bitset<5> rs = convert_to_int(instruction[3]);

    res += op + rs.to_string() + rt.to_string() + offset.to_string();
    return res;
}

std::string make_i_branch(std::vector<std::string> instruction)
{
    std::string res;
    std::string op = I_BRANCH_OP_VALUES[instruction[0]];
    std::bitset<5> rs = convert_to_int(instruction[1]);
    std::bitset<5> rt = convert_to_int(instruction[2]);
    std::string branch = instruction[3];
    int branch_location = (branch_addresses[branch] - BASE_TEXT_ADDRESS) / 4;
    int offset = branch_location - current_ins_line - 1;

    std::bitset<16> offset_binary(offset);
    res += op + rs.to_string() + rt.to_string() + offset_binary.to_string();
    return res;
}

std::string make_j(std::vector<std::string> instruction)
{
    std::string res;
    std::string op = J_OP_VALUES[instruction[0]];
    unsigned int target_address = branch_addresses[instruction[1]];

    target_address /= 4;
    std::bitset<26> target(target_address);

    res += op + target.to_string();
    return res;
}

std::string make_la(std::vector<std::string> instruction)
{
    std::string res;
    std::string val = instruction[1];
    unsigned int address = (label_addresses[instruction[2]]);

    std::vector<std::string> lui_ins {"lui", val, std::to_string(address >> 16)};
    res += make_i(lui_ins);

    if (address & MASK)
    {
        std::vector<std::string> ori_ins {"ori", val, val, std::to_string((address & MASK))};
        current_ins_line++;
        res += make_i(ori_ins);
    }

    return res;
}

std::string make_type(std::vector<std::string> instruction)
{
    std::string ins_name = instruction[0];

    if (R_OP_VALUES.find(ins_name) != R_OP_VALUES.end())
        return make_r(instruction);

    if (I_OP_VALUES.find(ins_name) != I_OP_VALUES.end())
        return make_i(instruction);

    if (J_OP_VALUES.find(ins_name) != J_OP_VALUES.end())
        return make_j(instruction);

    if (I_MEMORY_OP_VALUES.find(ins_name) != I_MEMORY_OP_VALUES.end())
        return make_i_memory(instruction);

    if (I_BRANCH_OP_VALUES.find(ins_name) != I_BRANCH_OP_VALUES.end())
        return make_i_branch(instruction);

    if (R_SHIFTER_OP_VALUES.find(ins_name) != R_SHIFTER_OP_VALUES.end())
        return make_r_shifter(instruction);

    return make_la(instruction);
}

std::string encode_section_sizes()
{
    std::bitset<32> data_length(num_words * 4);
    std::bitset<32> text_length(num_instructions * 4);

    return text_length.to_string() + data_length.to_string();
}

std::string encode_instructions()
{
    std::string res;

    for (const std::string& branch : text_branches_order)
    {
        for (const std::vector<std::string>& instruction : text_branches[branch])
        {
            res += make_type(instruction);
            current_ins_line++;
        }
    }

    return res;
}

std::string encode_data_values()
{
    std::string encoded_values;

    for (const auto& label : data_labels_order)
    {
        for (const auto& value : data_labels[label])
        {
            unsigned int int_val = convert_to_int(value);
            std::bitset<32> data_encoded(int_val);
            encoded_values += data_encoded.to_string();
        }
    }

    return encoded_values;
}

std::string encode_everything()
{
    std::string result;
    result += encode_section_sizes() + encode_instructions() + encode_data_values();
    return result;
}

int main(int argc, char* argv[]){

    R_OP_VALUES.insert(std::make_pair("addu", "100001"));
    R_OP_VALUES.insert(std::make_pair("and", "100100"));
    R_OP_VALUES.insert(std::make_pair("sltu", "101011"));
    R_OP_VALUES.insert(std::make_pair("subu", "100011"));
    R_OP_VALUES.insert(std::make_pair("and", "100100"));
    R_OP_VALUES.insert(std::make_pair("nor", "100111"));
    R_OP_VALUES.insert(std::make_pair("or", "100101"));
    R_OP_VALUES.insert(std::make_pair("jr", "001000"));

    I_OP_VALUES.insert(std::make_pair("addiu", "001001"));
    I_OP_VALUES.insert(std::make_pair("sltiu", "001011"));
    I_OP_VALUES.insert(std::make_pair("andi", "001100"));
    I_OP_VALUES.insert(std::make_pair("lui", "001111"));
    I_OP_VALUES.insert(std::make_pair("ori", "001101"));

    J_OP_VALUES.insert(std::make_pair("j", "000010"));
    J_OP_VALUES.insert(std::make_pair("jal", "000011"));

    I_BRANCH_OP_VALUES.insert(std::make_pair("beq", "000100"));
    I_BRANCH_OP_VALUES.insert(std::make_pair("bne", "000101"));

    R_SHIFTER_OP_VALUES.insert(std::make_pair("sll", "000000"));
    R_SHIFTER_OP_VALUES.insert(std::make_pair("srl", "000010"));

    I_MEMORY_OP_VALUES.insert(std::make_pair("lw", "100011"));
    I_MEMORY_OP_VALUES.insert(std::make_pair("sw", "101011"));

    std::string input_line;

    if(argc != 2) {
        exit(0);
    }
    else
    {
        char *file = (char *) malloc(strlen(argv[1]) + 3);
        strncpy(file,argv[1],strlen(argv[1]));

        if(freopen(file, "r", stdin) == nullptr) {
            printf("File open Error!\n");
            exit(1);
        }

        get_data();
        auto result = encode_everything();

        file[strlen(file) - 1] = 'o';
        freopen(file,"w", stdout);

        std::cout << result;
    }

    return 0;
}