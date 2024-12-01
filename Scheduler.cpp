//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"

// own imports
#include <bits/stdc++.h>

using namespace std;

// list of vms per machine
vector<vector<VMId_t>> machine_matrix;
int total_machines;

// lists of active and inactive machines
vector<MachineId_t> active;
vector<MachineId_t> inactive;

/*
Gets the total utilization of a vm
*/
double vm_utilization(MachineId_t machine_id, VMId_t vm_id) {
	double utilization = 0.0;
    VMInfo_t vm = VM_GetInfo(vm_id);
    vector<TaskId_t> &tasks = vm.active_tasks;

    for (TaskId_t task_id : tasks) {
        TaskInfo_t task_info = GetTaskInfo(task_id);
        MachineInfo_t machine_info = Machine_GetInfo(machine_id);

        utilization += 1.0 * task_info.remaining_instructions / 
                machine_info.performance[machine_info.p_state];
    }

	return utilization;
}

/*
Gets the total utilization of a machine
*/
double machine_utilization(MachineId_t machine_id) {
	double utilization = 0.0;
    vector<VMId_t> &vms = machine_matrix[machine_id];

    for (const VMId_t vm_id : vms) {
		utilization += vm_utilization(machine_id, vm_id);
    }
    
	return utilization;
}

/*
compares the utilization of two machines
*/
bool util_comp(MachineId_t a, MachineId_t b) {
	double util_a = machine_utilization(a);
	double util_b = machine_utilization(b);
	return util_a < util_b;
}


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
    total_machines = Machine_GetTotal();

    printf("total_machines: %d\n", total_machines);
    for (int i = 0; i < total_machines; ++i) {
        inactive.push_back((MachineId_t) i);
        vector<VMId_t> temp = {};
        machine_matrix.push_back(temp);
        Machine_SetState(i, S5);
    }
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks
    MachineId_t machine_id = VM_GetInfo(vm_id).machine_id;
    machine_matrix[machine_id].push_back(vm_id);
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
    TaskInfo_t task_info = GetTaskInfo(task_id);

    for(MachineId_t id : active) {
        MachineInfo_t curr_machine = Machine_GetInfo(id);
        double task_utilization = 1.0 * task_info.remaining_instructions / curr_machine.performance[curr_machine.p_state];
        if(curr_machine.cpu == task_info.required_cpu && 
            task_utilization + machine_utilization(id) < 1.0 && 
            curr_machine.memory_used + task_info.required_memory + 8 < curr_machine.memory_size) {
                VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
                VM_Attach(new_vm, id);
                VM_AddTask(new_vm, task_id, HIGH_PRIORITY);
                machine_matrix[id].push_back(new_vm);
                return;
            }
    }

    for(MachineId_t id : inactive) {
        MachineInfo_t curr_machine = Machine_GetInfo(id);
        if(curr_machine.cpu == task_info.required_cpu) {
            inactive.erase(remove(inactive.begin(), inactive.end(), id), inactive.end());
            Machine_SetState(id, S0);
            active.push_back(id);
            VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
            VM_Attach(new_vm, id);
            VM_AddTask(new_vm, task_id, HIGH_PRIORITY);
            machine_matrix[id].push_back(new_vm);
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
// shutdown old vm
    MachineId_t curr_machine = total_machines + 1;
    for(MachineId_t id : active) {
        vector<VMId_t> &vms = machine_matrix[id];
        for(VMId_t vm_id : vms) {
            VMInfo_t vm = VM_GetInfo(vm_id);
            vector<TaskId_t> &tasks = vm.active_tasks;
            for(TaskId_t task : tasks) {
                printf("comparing %u against %u\n", (unsigned) task, (unsigned) task_id);
                if(task == task_id) {
                    printf("equality for TaskComplete\n");
                    curr_machine = id;
                    VM_Shutdown(vm_id);
                    break;
                }
            }
        }
    }

    sort(active.begin(), active.end(), util_comp);
    for(MachineId_t machine_id : active) {
        if(machine_id != curr_machine) {
            vector<VMId_t> &vms = machine_matrix[machine_id];
            for(VMId_t vm_id : vms) {
                double vm_util = vm_utilization(curr_machine, vm_id);
                MachineInfo_t new_machine = Machine_GetInfo(machine_id);
                VMInfo_t vm_info = VM_GetInfo(vm_id);
                unsigned vm_memory = 0;
                for(TaskId_t task : vm_info.active_tasks) {
                    vm_memory += GetTaskInfo(task).required_memory;
                }
                if(new_machine.cpu == vm_info.cpu &&
                    machine_utilization(machine_id) + vm_util < 1 &&
                    new_machine.memory_used + vm_memory + 8 < new_machine.memory_size) {
                        VM_Migrate(vm_id, machine_id);
                        break;
                    }
            }
        }
    }

    printf("curr machine is %d\n", curr_machine);
    if(curr_machine < total_machines && Machine_GetInfo(curr_machine).active_vms == 0) {
        Machine_SetState(curr_machine, S5);
    }
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
    SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) + " was detected at time " + to_string(time), 0);
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
    // -- Pseudocode --
    // Assume workload i violates SLA on machine J
    // Sort all j in m in a set s in ascending order of u
    // Find a machine that can accommodate the load factor of i (other than J)
    // Found? Migrate i
    // Else PM on standby? Migrate i
    // Else Failure

    // assumption: look thru active machines, if unsuccessful then inactive machine
    TaskInfo_t task_info = GetTaskInfo(task_id);

    MachineId_t old_machine = total_machines + 1;
    VMId_t old_vm = total_machines + 1;
    
    for(MachineId_t machine_id : active) {
        vector<VMId_t> &vms = machine_matrix[machine_id];

        for(VMId_t vm_id : vms) {
            VMInfo_t vm = VM_GetInfo(vm_id);
            vector<TaskId_t> &tasks = vm.active_tasks;
            
            for(TaskId_t task : tasks) {
                if(task == task_id) {
                    printf("equality for SLAWarning\n");
                    old_machine = machine_id;
                    old_vm = vm_id;
                    break;
                }
            }
        }
    }
    
    sort(active.begin(), active.end(), util_comp);

    for (MachineId_t id : active) {
        
        if (id != old_machine) {
            MachineInfo_t curr_machine = Machine_GetInfo(id);
            double task_utilization = 1.0 * task_info.remaining_instructions / curr_machine.performance[curr_machine.p_state];
            
            if(curr_machine.cpu == task_info.required_cpu && 
                    task_utilization + machine_utilization(id) < 1.0 && 
                    curr_machine.memory_used + task_info.required_memory + 8 < curr_machine.memory_size) {
                
                VM_Migrate(old_vm, id);
                vector<VMId_t> &vms = machine_matrix[id];
                vms.erase(remove(vms.begin(), vms.end(), old_vm), vms.end());
                return;
            }
        }
    }

    for (MachineId_t id : inactive) {
        MachineInfo_t curr_machine = Machine_GetInfo(id);
        
        if (curr_machine.cpu == task_info.required_cpu) {
            inactive.erase(remove(inactive.begin(), inactive.end(), id), inactive.end());
            Machine_SetState(id, S0);
            active.push_back(id);

            VM_Migrate(old_vm, id);
            vector<VMId_t> &vms = machine_matrix[id];
            vms.erase(remove(vms.begin(), vms.end(), old_vm), vms.end());

            return;
        }
    }
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine
}

