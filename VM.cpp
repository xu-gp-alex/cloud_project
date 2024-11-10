//
//  VM.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/19/24.
//

#include "VM.hpp"

// VM Structures and objects
static vector<VM> VMs;
static VMId_t VMId_gen = 0;
static void ValidateVM(VMId_t vm_id, string err_msg) {
    if(vm_id >= VMs.size()) {
        ThrowException(err_msg);
    }
}

// External interface functions
void VM_Attach(VMId_t vm_id, MachineId_t machine_id) {
    ValidateVM(vm_id, "VM_Attach(): Bad VM identifier " + to_string(vm_id));
    VMs[vm_id].Attach(machine_id);

}

void VM_AddTask(VMId_t vm_id, TaskId_t task_id, Priority_t priority) {
    ValidateVM(vm_id, "VM_AddTask(): Bad VM identifier " + to_string(vm_id));
    VMs[vm_id].AddTask(task_id, priority);
}

VMId_t VM_Create(VMType_t vm_type, CPUType_t cpu) {
    VMId_t vm_id = VMId_gen++;
    VM vm(vm_type, cpu, vm_id);
    VMs.push_back(vm);
    return vm_id;
}

VMInfo_t VM_GetInfo(VMId_t vm_id) {
    ValidateVM(vm_id, "VM_GetInfo(): Bad VM identifier " + to_string(vm_id));
    return VMs[vm_id].GetVMInfo();
}

void VM_Migrate(VMId_t vm_id, MachineId_t machine_id) {
    ValidateVM(vm_id, "VM_Migrate(): Bad VM identifier " + to_string(vm_id));
    SimOutput("VM_Migrate(): Migration of VM " + to_string(vm_id) + " to " + to_string(machine_id) + " starting at time " + to_string(Now()), 4);
    VMs[vm_id].Migrate(machine_id);
}

void VM_RemoveTask(VMId_t vm_id, TaskId_t task_id) {
    ValidateVM(vm_id, "VM_RemoveTask(): Bad VM identifier " + to_string(vm_id));
    SimOutput("VM_RemoveTask(): Removing task " + to_string(task_id) + " from VM " + to_string(vm_id), 4);
    VMs[vm_id].RemoveTask(task_id);
}

void VM_Shutdown(VMId_t vm_id) {
    ValidateVM(vm_id, "VM_Shutdown(): Bad VM identifier " + to_string(vm_id));
    VMs[vm_id].Shutdown();
}

// Internal interface functions
bool VM_IsPendingMigration(VMId_t vm_id) {
    ValidateVM(vm_id, "VM_IsMigrating(): Bad VM identifier " + to_string(vm_id));
    return VMs[vm_id].IsPendingMigration();
}

void VM_MigrationCompleted(VMId_t vm_id) {
    ValidateVM(vm_id, "MigrationCompleted(): Bad VM identifier " + to_string(vm_id));
    VMs[vm_id].MigrationDone();
    MigrationDone(Now(), vm_id);
}

void VM_MigrationStarted(VMId_t vm_id) {
    ValidateVM(vm_id, "MigrationStarted(): Bad VM identifier " + to_string(vm_id));
    VMs[vm_id].MigrationStarted();
}

// Object definition
VM::VM(VMType_t vmt, CPUType_t cpu_, VMId_t vm_id) {
    vmType     = vmt;
    state      = CREATED;
    identifier = vm_id;
    cpu        = cpu_;
    if(vmt == AIX && cpu_ != POWER) {
        ThrowException("VM::VM(): Creating an AIX virtual machine on an inapporpriate CPU");
    }
    if((vmt == WIN && cpu_ == RISCV) || (vmt == WIN && cpu_ == POWER)) {
        ThrowException("VM::VM(): Creating Windows virtual machine on an inapporpriate CPU");
    }
}

void VM::Attach(MachineId_t machine_id) {
    if(state != CREATED) {
        ThrowException("VM::Attach(): Attaching a VM to a machine while the VM is already running");
    }
    SimOutput("The CPU is " + to_string(cpu) + " and the machine's CPU is " + to_string(Machine_GetCPUType(machine_id)), 4);
    if(Machine_GetCPUType(machine_id) != cpu) {
        ThrowException("VM::Attach(): Attaching a VM to a machine with incompatible CPU");
    }
    state = ACTIVE;
    machineId = machine_id;
    Machine_AttachVM(machine_id, identifier);
    if(Machine_CheckMemoryOverflow(machineId)) {
        MemoryWarning(Now(), machineId);
    }
}

void VM::AddTask(TaskId_t task_id, Priority_t priority) {
    if(state != ACTIVE) {
        ThrowException("VM::AddTask(): Adding a task to a VM that is not ready (either not allocated to a machine or migrating");
    }
    if(RequiredCPUType(task_id) != cpu) {
        ThrowException("VM::AddTask(): Adding a task to a VM with incompatible CPU");
    }
    taskSet.insert(task_id);
    SetTaskPriority(task_id, priority);
    Machine_AttachTask(machineId, task_id, identifier);
    if(Machine_CheckMemoryOverflow(machineId)) {
        MemoryWarning(Now(), machineId);
    }
}

VMInfo_t VM::GetVMInfo() {
    VMInfo_t vm_info;
    for(auto task_id: taskSet) {
        vm_info.active_tasks.push_back(task_id);
    }
    vm_info.cpu        = cpu;
    vm_info.machine_id = machineId;
    vm_info.vm_id      = identifier;
    vm_info.vm_type    = vmType;
    return vm_info;
}

bool VM::IsPendingMigration() {
    return state == PENDING_MIGRATION;
}

void VM::Migrate(MachineId_t to) {
    if(state != ACTIVE) {
        ThrowException("VM::Migrate(): Incorrect VM migration request");
    }
    if(Machine_GetCPUType(to) != cpu) {
        ThrowException("VM::Migrate(): Attaching a VM to a machine with incompatible CPU");
    }
    SimOutput("VM::Migrate(): Migration starting for VM " + to_string(identifier) + " from machine " + to_string(machineId) + " to machine " + to_string(to), 4);
    SimOutput("VM::Migrate(): Number of tasks " + to_string(taskSet.size()), 4);
    destinationId = to;
    state = PENDING_MIGRATION;
    Machine_MigrateVM(identifier, machineId, destinationId);
}

void VM::MigrationDone() {
    if(state != MIGRATING) {
        ThrowException("VM::MigrationDone(): Report of a migration completion to a VM that was not migrating!");
    }
    state = ACTIVE;
    machineId = destinationId;
    Machine_AttachVM(machineId, identifier);
    for(auto it: taskSet) {
        Machine_AttachTask(machineId, it, identifier);
    }
}

void VM::MigrationStarted() {
    if(state != PENDING_MIGRATION) {
        ThrowException("VM::MigrationStarted(): Report of a migration start to a VM that was not pending migration!");
    }
    state = MIGRATING;
}

void VM::RemoveTask(TaskId_t task_id) {
    if(state != ACTIVE && state != PENDING_MIGRATION) {
        ThrowException("VM::RemoveTask(): Removing a task from an inactive or migrating VM");
    }
    auto it = taskSet.find(task_id);
    if(it == taskSet.end()) {
        ThrowException("VM::RemoveTask(): VM is asked to remove a non existent task", unsigned (task_id));
    }
    taskSet.erase(it);
    SimOutput("VM::RemoveTask(): Removed task " + to_string(task_id) + " from VM " + to_string(identifier), 4);
}

void VM::Shutdown() {
    if(state != ACTIVE) {
        ThrowException("VM::Shutdown(): Shutting down an inactive VM");
    }
    if(taskSet.size() != 0) {
        ThrowException("VM::Shutdown(): Shutting down a VM while tasks are still running--likely a bug");
    }
    state = DEFUNCT;
    Machine_DetachVM(machineId, identifier);
}
