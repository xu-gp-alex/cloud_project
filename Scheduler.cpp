//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"

// own imports...
#include <bits/stdc++.h>
#include <iostream>
#include <unordered_map>
#include <utility>

using namespace std;

// vms on a machine
typedef struct {
    MachineId_t machine;
    vector<VMId_t> vms;
} Box;


// todo: figure out why active[j] is required to update correctly
vector<Box> active;
vector<Box> inactive;

// to find task --> machine/vm links
// todo: find soln w/out casting?
// map<TaskId_t, pair<MachineId_t, VMId_t>>
unordered_map<unsigned int, pair<unsigned int, unsigned int> > task_to_vm;

// todo: reenable migrating var?
// static bool migrating = false;

// actually good soln?
// list of vms per machine
vector<vector<VMId_t>> resources;
// lists of active and inactive machines
vector<MachineId_t> active;
vector<MachineId_t> inactive;

// todo: perhaps should compare mem_size - mem_used
bool mem_rev_comp(Box a, Box b) {
    return Machine_GetInfo(a.machine).memory_used > Machine_GetInfo(b.machine).memory_used;
}

// todo: find soln so the following can be deleted
// Box get_box_by_machine_id(MachineId_t tgt, vector<Box> &list) {
//     for (int i = 0; i < (int) list.size(); i++) {
//         if ()
//     }
//     return NULL;
// }

void Scheduler::Init() {
    // Find the parameters of the clusters
    // Get the total number of machines
    // For each machine:
    //      Get the type of the machine
    //      Get the memory of the machine
    //      Get the number of CPUs
    //      Get if there is a GPU or not
    // 
    SimOutput("Scheduler::Init(): Total number of machines is " + to_string(Machine_GetTotal()), 3);
    SimOutput("Scheduler::Init(): Initializing scheduler", 1);
    int totalMachines = Machine_GetTotal();
    for (int i = 0; i < totalMachines; i++) 
    {
        MachineInfo_t newMachine = Machine_GetInfo(i);
        // cout << "always the case: " << (unsigned int) newMachine.machine_id << " = " << i << endl;

        Box curr;
        curr.machine = newMachine.machine_id;
        inactive.push_back(curr);
    }
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks
}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    // Get the task parameters
    //  IsGPUCapable(task_id);
    //  GetMemory(task_id);
    //  RequiredVMType(task_id);
    //  RequiredSLA(task_id);
    //  RequiredCPUType(task_id);
    // Decide to attach the task to an existing VM, 
    //      vm.AddTask(taskid, Priority_T priority); or
    // Create a new VM, attach the VM to a machine
    //      VM vm(type of the VM)
    //      vm.Attach(machine_id);
    //      vm.AddTask(taskid, Priority_t priority) or
    // Turn on a machine, create a new VM, attach it to the VM, then add the task
    //
    // Turn on a machine, migrate an existing VM from a loaded machine....
    //
    // Other possibilities as desired

    // starting with active machines, then inactive machines
    // assumption: one task per vm
    TaskInfo_t task = GetTaskInfo(task_id);
    unsigned v = task.required_memory;

    for (int j = 0; j < (int) active.size(); j++) {
        MachineInfo_t curr = Machine_GetInfo(active[j].machine);
        unsigned u = curr.memory_used;

        if (u + v < curr.memory_size && task.required_cpu == curr.cpu) {
            VMId_t new_vm = VM_Create(task.required_vm, task.required_cpu);
            VM_Attach(new_vm, curr.machine_id);
            VM_AddTask(new_vm, task_id, task.priority);
            active[j].vms.push_back(new_vm);

            // new code for task --> vm link
            task_to_vm[(unsigned int) task_id] = make_pair(
                    (unsigned int) curr.machine_id, (unsigned int) new_vm
            );

            return;
        }
    }

    for (int j = 0; j < (int) inactive.size(); j++) {
        MachineInfo_t curr = Machine_GetInfo(inactive[j].machine);
        unsigned u = curr.memory_used;

        if (u + v < curr.memory_size && task.required_cpu == curr.cpu) {
            Machine_SetState(curr.machine_id, S0);

            VMId_t new_vm = VM_Create(task.required_vm, task.required_cpu);
            VM_Attach(new_vm, curr.machine_id);
            VM_AddTask(new_vm, task_id, task.priority);
            inactive[j].vms.push_back(new_vm);

            // new code for task --> vm link
            task_to_vm[(unsigned int) task_id] = make_pair(
                    (unsigned int) curr.machine_id, (unsigned int) new_vm
            );

            active.push_back(inactive[j]);
            inactive.erase(inactive.begin() + j);
            return;
        }
    }
}

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
}

void Scheduler::Shutdown(Time_t time) {
    // Do your final reporting and bookkeeping here.
    // Report about the total energy consumed
    // Report about the SLA compliance
    // Shutdown everything to be tidy :-)
    for(auto & vm: vms) {
        VM_Shutdown(vm);
    }
    SimOutput("SimulationComplete(): Finished!", 4);
    SimOutput("SimulationComplete(): Time is " + to_string(time), 4);
}

void Scheduler::TaskComplete(Time_t now, TaskId_t task_id) {
    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
}

// Public interface below

static Scheduler Scheduler;

void InitScheduler() {
    SimOutput("InitScheduler(): Initializing scheduler", 4);
    Scheduler.Init();
}

void HandleNewTask(Time_t time, TaskId_t task_id) {
    SimOutput("HandleNewTask(): Received new task " + to_string(task_id) + " at time " + to_string(time), 4);
    Scheduler.NewTask(time, task_id);
}

void HandleTaskCompletion(Time_t time, TaskId_t task_id) {
    SimOutput("HandleTaskCompletion(): Task " + to_string(task_id) + " completed at time " + to_string(time), 4);
    Scheduler.TaskComplete(time, task_id);
}

void MemoryWarning(Time_t time, MachineId_t machine_id) {
    // The simulator is alerting you that machine identified by machine_id is overcommitted
    // SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) + " was detected at time " + to_string(time), 0);
}

void MigrationDone(Time_t time, VMId_t vm_id) {
    // The function is called on to alert you that migration is complete
    SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) + " was completed at time " + to_string(time), 4);
    Scheduler.MigrationComplete(time, vm_id);
    // migrating = false;
}

void SchedulerCheck(Time_t time) {
    // This function is called periodically by the simulator, no specific event
    SimOutput("SchedulerCheck(): SchedulerCheck() called at " + to_string(time), 4);
    Scheduler.PeriodicCheck(time);
    // static unsigned counts = 0;
    // counts++;
    // if(counts == 10) {
    //     migrating = true;
    //     VM_Migrate(1, 9);
    // }
}

void SimulationComplete(Time_t time) {
    // This function is called before the simulation terminates Add whatever you feel like.
    cout << "SLA violation report" << endl;
    cout << "SLA0: " << GetSLAReport(SLA0) << "%" << endl;
    cout << "SLA1: " << GetSLAReport(SLA1) << "%" << endl;
    cout << "SLA2: " << GetSLAReport(SLA2) << "%" << endl;     // SLA3 do not have SLA violation issues
    cout << "Total Energy " << Machine_GetClusterEnergy() << "KW-Hour" << endl;
    cout << "Simulation run finished in " << double(time)/1000000 << " seconds" << endl;
    SimOutput("SimulationComplete(): Simulation finished at time " + to_string(time), 4);
    
    Scheduler.Shutdown(time);
}

void SLAWarning(Time_t time, TaskId_t task_id) {
    -- Pseudocode --
    Assume workload i violates SLA on machine J
    Sort all j in m in a set s in ascending order of u
    Find a machine that can accommodate the load factor of i (other than J)
    Found? Migrate i
    Else PM on standby? Migrate i
    Else Failure

    assumption: look thru active machines, if unsuccessful then inactive machine
    std::sort(active.begin(), active.end(), mem_rev_comp);
    MachineId_t prev_machine = (MachineId_t) task_to_vm[task_id].first;

    TaskInfo_t task = GetTaskInfo(task_id);
    unsigned v = task.required_memory;

    VMId_t curr_vm = (VMId_t) task_to_vm[task_id].second;

    for (int j = 0; j < active.size(); j++) {
        if (active[j].machine == prev_machine) {
            continue;
        }
        
        MachineInfo_t curr = Machine_GetInfo(active[j].machine);

        unsigned u = curr.memory_used;
        if (u + v < curr.memory_size && task.required_cpu == curr.cpu) {
            VM_Migrate(curr_vm, active[j].machine);
            // todo: update unordered_map<> task_to_vm
            task_to_vm[task_id].first = active[j].machine;
            // todo: update active[j].machine (new machine)
            active[j].vms.push_back(curr_vm);
            // todo: update prev_machine (old machine)??
            // vector<VMId_t> temp = find(active.)
            // "erase-remove idiom"
            temp.erase(remove(temp.begin(), temp.end(), curr_vm), temp.end());
            return;
            
        }
    }

    // if no active machines avaliable, move to inactive machine
    for (int j = 0; j < (int) inactive.size(); j++) {
        // Box curr_box = inactive[j];
        MachineInfo_t curr = Machine_GetInfo(inactive[j].machine);
        unsigned u = curr.memory_used;

        if (u + v < curr.memory_size && task.required_cpu == curr.cpu) {
            Machine_SetState(curr.machine_id, S0);

            VMId_t new_vm = VM_Create(task.required_vm, task.required_cpu);
            VM_Attach(new_vm, curr.machine_id);
            VM_Migrate((VMId_t) task_to_vm[task_id].second, inactive[j].machine);
            inactive[j].vms.push_back(new_vm);

            // new code for task --> vm link
            task_to_vm[task_id].first = curr.machine_id;
            task_to_vm[task_id].second = new_vm;

            active.push_back(inactive[j]);
            inactive.erase(inactive.begin() + j);
            return;
        }
    }

}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine
}

