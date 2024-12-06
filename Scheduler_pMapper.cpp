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
    bool for_new_task; //true if new task, false if slawarning
    TaskId_t new_task_id;
    VMId_t sla_violating_vm;
};
unordered_map<unsigned, state_change_info> state_changing_machines;

int rand_machine_index;

//sorted list of machines based on their efficiency
vector<unsigned> eff_list;

// efficiency based comparator
bool efficiency_comparator(MachineId_t a, MachineId_t b) {
    double eff_a = (double) Machine_GetInfo(a).performance[0] / (double) Machine_GetInfo(a).s_states[0];
    double eff_b = (double) Machine_GetInfo(b).performance[0] / (double) Machine_GetInfo(b).s_states[0];
    return eff_a < eff_b;
}


bool state_changing_machines_contains(unsigned key)
{
    // Key is not present
    if (state_changing_machines.find(key) == state_changing_machines.end())
        return false;
    return true;
}

/*
Gets the total utilization of a vm
*/
double task_utilization(TaskId_t task_id, MachineId_t machine_id, Time_t curr) {
    TaskInfo_t info = GetTaskInfo(task_id);

    double remaining_instr = (double) info.remaining_instructions;
    double time_frame = (double) info.target_completion - curr;

    double eff_mips = remaining_instr / time_frame;

    MachineInfo_t machine_info = Machine_GetInfo(machine_id);
    double actual_mips = machine_info.performance[machine_info.p_state] * machine_info.num_cpus;
    
	return eff_mips / actual_mips;
}

// assumes one task per vm
double vm_utilization(VMId_t vm_id, MachineId_t machine_id, Time_t curr) {
    double utilization = 0.0;

    for(int i = 0; i < VM_GetInfo(vm_id).active_tasks.size(); i++) {
        utilization += task_utilization(VM_GetInfo(vm_id).active_tasks[i], machine_id, curr);
    }
    
    return utilization;
}

/*
Gets the total utilization of a machine
*/
double machine_utilization(MachineId_t machine_id, Time_t curr) {
	double eff_mips = 0.0;

    for (VMId_t vm_id : machine_matrix[machine_id]) {
        for (TaskId_t task_id : VM_GetInfo(vm_id).active_tasks) {
            TaskInfo_t info = GetTaskInfo(task_id);

            double remaining_instr = (double) info.remaining_instructions;
            double time_frame = (double) info.target_completion - curr;

            eff_mips += remaining_instr / time_frame;
        }
    }

    MachineInfo_t machine_info = Machine_GetInfo(machine_id);
    double actual_mips = machine_info.performance[machine_info.p_state] * machine_info.num_cpus;
    
	return eff_mips / actual_mips;
}

/*
compares the utilization of two machines
*/
bool util_comp(MachineId_t a, MachineId_t b) {
	double util_a = machine_utilization(a, Now());
	double util_b = machine_utilization(b, Now());
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
        if(i % 2 == 0) {
            inactive.push_back(i);
            Machine_SetState(i, S1);
            state_change_info info = {false, 0, 0};
            state_changing_machines[i] = info;
        }
        else {
            active.push_back(i);
        }
        eff_list.push_back(i);
        vector<VMId_t> temp = {};
        machine_matrix.push_back(temp);
        //doesn't actually matter what this value is
    }

    sort(eff_list.begin(), eff_list.end(), efficiency_comparator);    
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    MachineId_t machine_id = VM_GetInfo(vm_id).machine_id;
    migrating_vms.erase(vm_id);
}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    TaskInfo_t task_info = GetTaskInfo(task_id);

    // sort(active.begin(), active.end(), util_comp);
    for(MachineId_t id : eff_list) {
        MachineInfo_t curr_machine = Machine_GetInfo(id);
        double task_util = task_utilization(task_id, id, now);
        if(curr_machine.cpu == task_info.required_cpu && 
            task_util + machine_utilization(id, now) < 1.0 && 
            curr_machine.memory_used + task_info.required_memory + 8 < curr_machine.memory_size &&
            curr_machine.s_state == S0 && !state_changing_machines_contains(id)) {                
                VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
                VM_Attach(new_vm, id);
                VM_AddTask(new_vm, task_id, sla_to_prio(task_info.required_sla));

                machine_matrix[id].push_back(new_vm);
                task_to_vm[task_id] = new_vm;
                return;
            }
    }

    for(MachineId_t id : eff_list) {
        MachineInfo_t curr_machine = Machine_GetInfo(id);
        double task_util = task_utilization(task_id, id, now);
        if(curr_machine.cpu == task_info.required_cpu && 
            curr_machine.memory_used + task_info.required_memory + 8 < curr_machine.memory_size &&
            !state_changing_machines_contains(id)) {    
            if(curr_machine.s_state == S0) {
                VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
                VM_Attach(new_vm, id);
                VM_AddTask(new_vm, task_id, sla_to_prio(task_info.required_sla));

                machine_matrix[id].push_back(new_vm);
                task_to_vm[task_id] = new_vm;
                return;
            }
            Machine_SetState(id, S0);
            state_changing_machines[id] = {true, task_id, 0};
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
    // printf("Completed task #%u\n", task_id);

    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    // shutdown old vm
    VMId_t old_vm = task_to_vm[task_id];
    MachineId_t curr_machine = VM_GetInfo(old_vm).machine_id;
    task_to_vm.erase(task_id);

    vector<unsigned> util_list;
    for(int i = 0; i < total_machines; i++) {
        util_list.push_back(i);
    }

    sort(util_list.begin(), util_list.end(), util_comp);
    vector<unsigned> total_util;
    for(int i = 0; i < total_machines; i++) {
        total_util.push_back(machine_utilization(util_list[i], now));
    }

    for(int i = 0; i < total_machines / 2; i++) {
        MachineId_t curr_machine_id = util_list[i];
        MachineInfo_t curr_machine = Machine_GetInfo(curr_machine_id); 
        for(int j = 0; j < machine_matrix[curr_machine_id].size(); j++) {
            VMId_t vm_id = machine_matrix[curr_machine_id][j];
            TaskId_t task_id = 0;
            TaskInfo_t task_info;
            if(VM_GetInfo(vm_id).active_tasks.size() > 0) {
                task_id = VM_GetInfo(vm_id).active_tasks[0];
                task_info = GetTaskInfo(task_id);
                for(int k = total_machines - 1; k > total_machines / 2; k--) {
                    double vm_util = vm_utilization(vm_id, util_list[k], now);
                    MachineInfo_t new_machine = Machine_GetInfo((MachineId_t) util_list[k]);
                    if(new_machine.cpu == task_info.required_cpu && 
                        vm_util + total_util[k] < 1.0 && 
                        new_machine.memory_used + task_info.required_memory + 8 < new_machine.memory_size) {
                        VM_Migrate(vm_id, (MachineId_t) util_list[k]);
                        machine_matrix[curr_machine_id].erase(remove(machine_matrix[curr_machine_id].begin(), 
                            machine_matrix[curr_machine_id].end(), vm_id), machine_matrix[curr_machine_id].end());
                        machine_matrix[(MachineId_t) util_list[k]].push_back(vm_id);
                        migrating_vms.insert(vm_id);
                        return;
                    }
                }
            }
        }   
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

void SLAWarning(Time_t time, TaskId_t task_id) {
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    // Called in response to an earlier request to change the state of a machine

    MachineInfo_t machine_info = Machine_GetInfo(machine_id);
    
    if (machine_info.s_state == S0) {
        state_change_info info = state_changing_machines[machine_id];
        // for assigning tasks to inactive machine once woken
        // new task
        if (info.for_new_task) {
            TaskInfo_t task_info = GetTaskInfo(info.new_task_id);
            VMId_t new_vm = VM_Create(task_info.required_vm, task_info.required_cpu);
            VM_Attach(new_vm, machine_id);
            VM_AddTask(new_vm, info.new_task_id, sla_to_prio(task_info.required_sla));
            machine_matrix[machine_id].push_back(new_vm);
            task_to_vm[info.new_task_id] = new_vm;
        } 
    } 
    state_changing_machines.erase(machine_id);
}


