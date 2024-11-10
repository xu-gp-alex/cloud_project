//
//  Init.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/22/24.
//

#include "Init.hpp"

static void CleanUpString(string & input) {
    input.erase(0, input.find_first_not_of(" \t\r\n"));
    input.erase(input.find_last_not_of(" \t\r\n") + 1);
}

static vector<unsigned> ConvertBracketedStringToValues(string values) {
    CleanUpString(values);
    if (values.front() != '[' || values.back() != ']') {
        ThrowException("ConvertBracketedStringToValues(): Invalid format. Expected '[' at the start and ']' at the end.");
    }
    
    // Split values by commas after removing the square brackets
    istringstream iss(values.substr(1, values.length() - 2));
    string value;

    vector<unsigned> result;
    while (getline(iss, value, ',')) {
        CleanUpString(value);
        if (!value.empty()) {
            stringstream stream(value);
            unsigned val;
            stream >> val;
            result.push_back(val);
        }
    }
    return result;
}

static vector<unsigned> CheckAndGetVector(map<string, string> & parser, string key, string err_msg) {
    auto it = parser.find(key);
    if(it == parser.end()) {
        ThrowException(err_msg);
    }
    return ConvertBracketedStringToValues(it->second);
}

static string CheckAndGetString(map<string, string> & parser, string key, string err_msg) {
    auto it = parser.find(key);
    if(it == parser.end()) {
        ThrowException(err_msg);
    }
    return it->second;
}

static uint64_t CheckAndGetLongValue(map<string, string> & parser, string key, string err_msg) {
    stringstream ss(CheckAndGetString(parser, key, err_msg));
    uint64_t result;
    ss >> result;
    return result;
}

static unsigned CheckAndGetValue(map<string, string> & parser, string key, string err_msg) {
    stringstream ss(CheckAndGetString(parser, key, err_msg));
    unsigned result;
    ss >> result;
    return result;
}

static unsigned MapNameToType(string input) {
    static const unordered_map<string, unsigned> string_to_type = {
        {"AI", unsigned(AI_TRAINING)},
        {"CRYPTO", unsigned(CRYPTO)},
        {"HPC", unsigned(SCIENTIFIC)},
        {"STREAM", unsigned(STREAMING)},
        {"WEB", unsigned(WEB_REQUEST)},
        {"SLA0", unsigned(SLA0)},
        {"SLA1", unsigned(SLA1)},
        {"SLA2", unsigned(SLA2)},
        {"SLA3", unsigned(SLA3)},
        {"LINUX", unsigned(LINUX)},
        {"LINUX_RT", unsigned(LINUX_RT)},
        {"WIN", unsigned(WIN)},
        {"AIX", unsigned(AIX)},
        {"ARM", unsigned(ARM)},
        {"X86", unsigned(X86)},
        {"RISCV", unsigned(RISCV)},
        {"POWER", unsigned(POWER)},
        {"yes", unsigned(true)},
        {"no", unsigned(false)}
    };
    auto it = string_to_type.find(input);
    if(it == string_to_type.end()) {
        ThrowException("MapNameToType(): Failed to map string " + input + " while reading input file.");
    }
    return it->second;
}

static void Parse(ifstream & file, map<string, string> & parser) {
    string line;
    bool openingBraceFound = false;
    bool closingBraceFound = false;

    while (getline(file, line)) {
        CleanUpString(line);
        // Ignore blank lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Check if line is the opening brace "{"
        if (!openingBraceFound) {
            if (line == "{") {
                openingBraceFound = true;
                continue;
            } else {
                ThrowException("Parse(): Parsing error while reading task parameters: Expected { but found\n ", line);
            }
        }

        // Check if line is the closing brace "}"
        if (line == "}") {
            closingBraceFound = true;
            break;
        }

        // Parse name: value pairs
        istringstream iss(line);
        string name, value;
        
        if (getline(iss, name, ':') && getline(iss, value)) {
            CleanUpString(name);
            CleanUpString(value);
            parser[name] = value;
        } else {
            ThrowException("Parse(): Parsing error while reading input: Expected 'keyword: value' but found\n ", line);
        }
    }
    if (!closingBraceFound) {
        ThrowException("Parse(): Parsing error while reading input: Expected } but found\n ", line);
    }
}

static void ReadMachineClass(ifstream & file) {
    SimOutput("ReadMachineClass(): Reading machine descriptor", 1);
    map<string, string> parser;
    Parse(file, parser);

    unsigned memory = CheckAndGetValue(parser, "Memory", "Failed: No memory requirement for machine class");
    unsigned num_cores = CheckAndGetValue(parser, "Number of cores", "Failed: No core specified for machine class");
    unsigned num_machines = CheckAndGetValue(parser, "Number of machines", "Failed: No number of machines for machine class");
    bool gpu_flag = bool(MapNameToType(CheckAndGetString(parser, "GPUs", "Failed: No GPU flag for machine class")));
    vector<unsigned> s_states = CheckAndGetVector(parser, "S-States", "Failed: Could not read s states for machine class");
    vector<unsigned> p_states = CheckAndGetVector(parser, "P-States", "Failed: Could not read p states for machine class");
    vector<unsigned> c_states = CheckAndGetVector(parser, "C-States", "Failed: Could not read c states for machine class");
    vector<unsigned> mips = CheckAndGetVector(parser, "MIPS", "Failed: Could not read MIPS for machine class");
    CPUType_t cpu = CPUType_t(MapNameToType(CheckAndGetString(parser, "CPU type", "Failed: No CPU type for machine class")));

    for(unsigned i = 0; i < num_machines; i++) {
        Machine_Add(memory, num_cores, s_states, c_states, p_states, mips, gpu_flag, cpu);
    }
}

static void ReadTaskClass(ifstream & file) {
    SimOutput("ReadTaskClass(): Reading task descriptor", 1);
    map<string, string> parser;
    Parse(file, parser);
    
    Time_t start = CheckAndGetLongValue(parser, "Start time", "Failed: No starting time for the task class");
    Time_t end = CheckAndGetLongValue(parser, "End time", "Failed: No ending time for the task class");
    Time_t inter_arrival = CheckAndGetLongValue(parser, "Inter arrival", "Failed: No inter arrival time for task class");
    Time_t estimated_runtime = CheckAndGetLongValue(parser, "Expected runtime", "Failed: No estimated runtime for task class");
    TaskClass_t task_class = TaskClass_t(MapNameToType(CheckAndGetString(parser, "Task type", "Failed: No type for task class")));
    CPUType_t cpu = CPUType_t(MapNameToType(CheckAndGetString(parser, "CPU type", "Failed: No CPU type for task class")));
    SLAType_t sla = SLAType_t(MapNameToType(CheckAndGetString(parser, "SLA type", "Failed: No SLA for task class")));
    VMType_t vm = VMType_t(MapNameToType(CheckAndGetString(parser, "VM type", "Failed: No VM type for task class")));
    unsigned memory = CheckAndGetValue(parser, "Memory", "Failed: No memory requirement for task class");
    unsigned seed = CheckAndGetValue(parser, "Seed", "Failed: No seed for task class");
    bool gpu_flag = bool(MapNameToType(CheckAndGetString(parser, "GPU enabled", "Failed: No GPU flag for task class")));

    uint64_t runtime_dev  = estimated_runtime / 5;
    uint64_t factor = (sla == SLA0) ? 3 : ((sla == SLA1) ? 8 : 12);
    uint64_t response = estimated_runtime * factor;

    exponential_distribution<> exp_dist(1000.0/double(inter_arrival));
    double range_half = runtime_dev * sqrt(12.0) / 2.0;
    uniform_real_distribution<> dist(double(estimated_runtime) - range_half, double(estimated_runtime) + range_half);

    mt19937 gen(seed); // Mersenne Twister random number engine

    for (Time_t arrival_time = start; arrival_time < end; ) {
        Time_t interval = Time_t(exp_dist(gen) * 1000);
        arrival_time += interval;
        
        unsigned runtime = unsigned(dist(gen));

        uint64_t target = runtime + response + arrival_time;
        uint64_t instr = runtime * 1000;
        
        TaskId_t task_id = AddTask(instr, arrival_time, target, vm, sla, cpu, gpu_flag, memory, task_class);
        SimOutput("ReadTaskClass(): Task " + to_string(task_id) + " with " + to_string(instr) + " instructions added at " + to_string(arrival_time), 1);
    }
}

void ReadInput(string filename) {
    ifstream file(filename);
    if(!file.is_open()) {
        ThrowException("ReadInput(): Could not input file ", filename);
    }
    string line;
    
    while(getline(file, line)) {
        if(line == "machine class:") {
            ReadMachineClass(file);
        }
        else if (line == "task class:") {
            ReadTaskClass(file);
        }
    }
    file.close();
    return;
}

void Init(string filename) {
    SimOutput("Init(): About to read input file", 1);
    ReadInput(filename);
    SimOutput(string("Init(): Found ") + to_string(GetNumTasks()) + " tasks", 1);
    SimOutput(string("Init(): Found ") + to_string(Machine_GetTotal()) + " machines", 1);
    SimOutput("Init(): About to initialize scheduler", 1);
    InitScheduler();
    SimOutput("Init(): Starting simulation", 1);
    StartSimulation();
}
