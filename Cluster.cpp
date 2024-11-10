//
//  Cluster.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Cluster.hpp"

#ifdef IMPLEMENTED

Cluster * cluster;

Cluster::Cluster(Simulator * s, Workload * wrkld, Scheduler * schdlr) {
    simulator = s;
    workload = wrkld;
    scheduler = schdlr;
    cluster = this;
    simulator->AddEvent(std::make_shared<TimerEvent>(QUANTUM, this));       // Create the first timer
}

void Cluster::HandleTimer() {
    for(auto machine : machines) {
        machine->HandleTimer(simulator->Now());
    }
    if(workload->NumActiveTasks() > 0) {
        simulator->AddEvent(std::make_shared<TimerEvent>(simulator->Now() + QUANTUM, this));
    }
    SimOutput("Timer event at " + to_string(simulator->Now()));
}

unsigned long long Cluster::ReportConsumedEnergy() {
    unsigned long long energy = 0;
    for(auto machine: machines) {
        energy += machine->GetEnergy(simulator->Now());
    }
    return energy;
}

void Cluster::AttachVM(MachineId_t mid, VM *vm) {
    // Check if the VM is compatible with machine CPU
}

void Cluster::AttachTask(TaskId_t task_id, MachineId_t machine_id, unsigned int priority, VM *vm) {
    // Check if the needed VM for the task is the same as this VM
    // Check that the CPU on which the VM is the same as the task's
    // Add the task to the machine
}

void Cluster::Migrate(VM * vm, MachineId_t from, MachineId_t to) {
    // For all the tasks, remove the existing tasks from the machine
    // Detach VM from memory
    // Schedule an event for VM migration
}

unsigned Cluster::NumMachines() {
    return unsigned(machines.size());
}

void Cluster::SetMachineState(unsigned machine_id, MachineState_t state) {
    machines[machine_id]->SetState(state, simulator->Now());
}

void Cluster::SetPerformance(unsigned machine_id, unsigned core_id, CPUPerformance_t pstate) {
    machines[machine_id]->SetPerformance(core_id, pstate);
}

MachineInfo_t Cluster::GetInfo(unsigned machine_id) {
    return machines[machine_id]->GetInfo(simulator->Now());
}
#endif
