//
//  VM.hpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/19/24.
//


#ifndef VM_hpp
#define VM_hpp

#include <iostream>
#include <set>
#include <string>

#include "Interfaces.h"
#include "Internal_Interfaces.h"

class Cluster;
using namespace std;

class VM {
public:
    VM(VMType_t vmt, CPUType_t cpu, VMId_t vm_id);
    void AddTask(TaskId_t taskid, Priority_t priority);
    void Attach(MachineId_t machine_id);
    VMInfo_t GetVMInfo();
    bool IsPendingMigration();
    void Migrate(MachineId_t to);
    void MigrationDone();
    void MigrationStarted();
    void RemoveTask(TaskId_t taskid);
    void Shutdown();
private:
    CPUType_t cpu;
    VMId_t identifier;
    MachineId_t destinationId;      // For migration
    MachineId_t machineId;
    enum {CREATED, ACTIVE, PENDING_MIGRATION, MIGRATING, DEFUNCT} state;
    set<TaskId_t> taskSet;
    VMType_t vmType;
};

#endif /* VM_hpp */

