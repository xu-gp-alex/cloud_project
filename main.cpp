//
//  main.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/19/24.
//

#include <iostream>
#include <sstream>
#include <string>

#include "Interfaces.h"
#include "Internal_Interfaces.h"

static unsigned verbose_level = 0;

int main(int argc, char * argv[]) {
    try {
        switch(argc) {
            case 1:
                verbose_level = 0;
                Init("/tmp/Input");
                break;
            case 2:
                verbose_level = 0;
                Init(argv[1]);
                break;
            case 4:
                if(string(argv[1]) == "-v") {
                    verbose_level = atoi(argv[2]);
                    Init(argv[3]);
                }
                else {
                    ThrowException(string("Usage ") + argv[0] + "[-v] input_file");
                }
                break;
            default:
                ThrowException(string("Usage ") + argv[0] + "[-v] input_file");
                break;
        }
        return 0;
    }
    catch(const runtime_error & error) {
        cerr << "Caught an exception!" << endl;
        cerr << error.what() << endl;
        cerr << "Bailing out!" << endl;
        return -1;
    }
}

void ThrowException(string err_msg) {
    throw(runtime_error(err_msg));
}

void ThrowException(string err_msg, string further_input) {
    throw(runtime_error(err_msg + further_input));
}

void ThrowException(string err_msg, unsigned further_input) {
    stringstream err_stream;
    err_stream << err_msg << further_input;
    throw(runtime_error(err_stream.str()));
}

void SimOutput(string msg, unsigned level) {
    if(verbose_level >= level) {
        cout << msg << endl;
    }
}
