//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"

// own imports
// for erase and remove
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace std;
//stores machines and vms on the machines
vector<vector<VMId_t>> machine_matrix;
// list of vms per machine
int total_machines;

// lists of active and inactive machines
vector<MachineId_t> active;
vector<MachineId_t> inactive;

//map task-> vm info
unordered_map<unsigned, unsigned> task_to_vm;

// stores vms that are currently being migrated
unordered_set<unsigned> migrating_vms; 
// store the machines that are currently being state changed, and info about it

struct state_change_info {
    bool for_new_task;
    TaskId_t new_task_id;
    VMId_t sla_violating_vm;
};
unordered_map<unsigned, state_change_info> state_changing_machines;

int rand_machine_index;

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
        utilization += (double) task_info.remaining_instructions / 
                        (double) (machine_info.performance[machine_info.p_state] 
                                * machine_info.num_cpus * 1000000);
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

// convert sla to prio
Priority_t sla_to_prio(SLAType_t sla) {
    if (sla == SLA0 || sla == SLA1 || sla == SLA2) {
        return HIGH_PRIORITY;
    } 
    return LOW_PRIORITY;
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
    active = {};
    inactive = {};
    rand_machine_index = 0;
    for (int i = 0; i < total_machines; ++i) {
        vector<VMId_t> temp = {};
        machine_matrix.push_back(temp);
        Machine_SetState(i, S5);
        //doesn't actually matter what this value is
        state_change_info info = {false, 0, 0};
        state_changing_machines[i] = info;
    }
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    MachineId_t machine_id = VM_GetInfo(vm_id).machine_id;
    machine_matrix[machine_id].push_back(vm_id);
    migrating_vms.erase(vm_id);
}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    TaskInfo_t task_info = GetTaskInfo(task_id);

    sort(active.begin(), active.end(), util_comp);
    for(MachineId_t id : active) {
        MachineInfo_t curr_machine = Machine_GetInfo(id);
        double task_utilization = (double) task_info.remaining_instructions / 
                           (double) (curr_machine.performance[curr_machine.p_state] * 
                            curr_machine.num_cpus * 1000000);
        if(curr_machine.cpu == task_info.required_cpu && 
            task_utilization + machine_utilization(id) < 1.0 && 
            curr_machine.memory_used + task_info.required_memory + 8 < curr_machine.memory_size) {

                for (VMId_t vm_id : machine_matrix[id]) {
                    VMInfo_t vm_info = VM_GetInfo(vm_id);
                    if (vm_info.vm_type == task_info.required_vm) {
                        VM_AddTask(vm_id, task_id, sla_to_prio(task_info.required_sla));
                        task_to_vm[task_id] = vm_id;
                        return;
                    }
                }
                
                VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
                VM_Attach(new_vm, id);
                VM_AddTask(new_vm, task_id, sla_to_prio(task_info.required_sla));

                machine_matrix[id].push_back(new_vm);
                task_to_vm[task_id] = new_vm;
                return;
            }
        else if (task_utilization + machine_utilization(id) >= 1.0) {
            break;
        }
    }

    for(MachineId_t id : inactive) {
        MachineInfo_t curr_machine = Machine_GetInfo(id);
        if(curr_machine.cpu == task_info.required_cpu && !state_changing_machines.count(id)) {
            inactive.erase(remove(inactive.begin(), inactive.end(), id), inactive.end());
            Machine_SetState(id, S0);

            state_change_info info = {true, task_id, 0};
            state_changing_machines[id] = info;
            return;
        }
    }

    //put on a random machine
    for(int index = 0; index < total_machines; index++) {
        MachineId_t rand_machine = (rand_machine_index + index) % total_machines;
        rand_machine_index++;
        MachineInfo_t curr_machine = Machine_GetInfo(rand_machine);
        if(curr_machine.cpu == task_info.required_cpu) {
            for (VMId_t vm_id : machine_matrix[rand_machine]) {
                VMInfo_t vm_info = VM_GetInfo(vm_id);
                if (vm_info.vm_type == task_info.required_vm) {
                    VM_AddTask(vm_id, task_id, sla_to_prio(task_info.required_sla));
                    task_to_vm[task_id] = vm_id;
                    return;
                }
            }
            VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
            VM_Attach(new_vm, rand_machine);
            VM_AddTask(new_vm, task_id, sla_to_prio(task_info.required_sla));
            machine_matrix[rand_machine].push_back(new_vm);
            task_to_vm[task_id] = new_vm;
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
    // Shutdown everything to be tidy f:-)
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
    VMId_t old_vm = task_to_vm[task_id];
    MachineId_t curr_machine = VM_GetInfo(old_vm).machine_id;
    task_to_vm.erase(task_id);
    machine_matrix[curr_machine].erase(remove(machine_matrix[curr_machine].begin(), 
        machine_matrix[curr_machine].end(), old_vm), machine_matrix[curr_machine].end());
    if(VM_GetInfo(old_vm).active_tasks.size() == 0 && !migrating_vms.count(old_vm)) {
        VM_Shutdown(old_vm);
    }

    sort(active.begin(), active.end(), util_comp);
    //loop through all active machines
    for(MachineId_t machine_id : active) {
        if (Machine_GetInfo(curr_machine).active_vms == 0) {
            break;
        }
        if(machine_id != curr_machine) {
            //loop through vms on the current machine
            vector<VMId_t> &vms = machine_matrix[curr_machine];
            for(VMId_t vm_id : vms) {
                double vm_util = vm_utilization(machine_id, vm_id);
                MachineInfo_t new_machine = Machine_GetInfo(machine_id);
                VMInfo_t vm_info = VM_GetInfo(vm_id);
                unsigned vm_memory = 0;
                for (TaskId_t task : vm_info.active_tasks) {
                    vm_memory += GetTaskInfo(task).required_memory;
                }
                if (new_machine.cpu == vm_info.cpu &&
                    machine_utilization(machine_id) + vm_util < 1 &&
                    new_machine.memory_used + vm_memory + 8 < new_machine.memory_size) {
                    vms.erase(remove(vms.begin(), vms.end(), vm_id), vms.end());
                    VM_Migrate(vm_id, machine_id);
                    migrating_vms.insert(vm_id);
                    break;
                }
            }
        }
    }

    if(Machine_GetInfo(curr_machine).active_vms == 0) {
        active.erase(remove(active.begin(), active.end(), curr_machine), active.end());
        Machine_SetState(curr_machine, S5);
        state_changing_machines[curr_machine] = {false, 0, 0};
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
}

void SchedulerCheck(Time_t time) {
    // This function is called periodically by the simulator, no specific event
    // printf("scheduler check called\n");
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

int sla_violations = 0;

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

    VMId_t old_vm = task_to_vm[task_id];
    MachineId_t old_machine = VM_GetInfo(old_vm).machine_id;

    if(migrating_vms.count(old_vm)) {
        return;
    }

    sort(active.begin(), active.end(), util_comp);

    for (MachineId_t id : active) {
        if (id != old_machine) {
            MachineInfo_t curr_machine = Machine_GetInfo(id);
            double task_utilization = (double) task_info.remaining_instructions 
                            / (double) (curr_machine.performance[curr_machine.p_state]
                            * curr_machine.num_cpus * 1000000);
            
            if(curr_machine.cpu == task_info.required_cpu && 
                    task_utilization + machine_utilization(id) < 1.0 && 
                    curr_machine.memory_used + task_info.required_memory + 8 < curr_machine.memory_size) {

                VM_Migrate(old_vm, id);
                machine_matrix[id].erase(remove(machine_matrix[id].begin(), 
                    machine_matrix[id].end(), old_vm), machine_matrix[id].end());
                migrating_vms.insert(old_vm);
                return;
            }
        }
    }

    for (MachineId_t id : inactive) {
        MachineInfo_t curr_machine = Machine_GetInfo(id);
        
        if (curr_machine.cpu == task_info.required_cpu && !state_changing_machines.count(id)) {
            inactive.erase(remove(inactive.begin(), inactive.end(), id), inactive.end());
            Machine_SetState(id, S0);
            state_change_info info = {false, 0, old_vm};
            state_changing_machines[id] = info;
            return;
        }
    }
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine

    MachineInfo_t machine_info = Machine_GetInfo(machine_id);
    
    // true means from task_complete
    if (machine_info.s_state == S0) {
        state_change_info info = state_changing_machines[machine_id];
        if (info.for_new_task) {
            TaskInfo_t task_info = GetTaskInfo(info.new_task_id);

            active.push_back(machine_id);
            VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
            VM_Attach(new_vm, machine_id);
            VM_AddTask(new_vm, info.new_task_id, sla_to_prio(task_info.required_sla));
            machine_matrix[machine_id].push_back(new_vm);
            task_to_vm[info.new_task_id] = new_vm;
        } else {
            active.push_back(machine_id);

            VM_Migrate(info.sla_violating_vm, machine_id);
            machine_matrix[machine_id].erase(remove(machine_matrix[machine_id].begin(), 
                    machine_matrix[machine_id].end(), info.sla_violating_vm), machine_matrix[machine_id].end());
            migrating_vms.insert(info.sla_violating_vm);
        }
    } else if (machine_info.s_state == S5) {
        inactive.push_back(machine_id);
    }

    state_changing_machines.erase(machine_id);
}


