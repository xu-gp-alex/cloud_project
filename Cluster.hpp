//
//  Cluster.hpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#ifndef Cluster_hpp
#define Cluster_hpp

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "Interfaces.h"
#include "Internal_Interfaces.h"

#define QUANTUM 60000

#ifdef IMPLEMENTED

class Cluster {
/*
public:
    Cluster(Simulator * s, Workload * wrkld, Scheduler * schdlr);
    void AddMachine(Machine * machine)                          { machines.push_back(machine); }
    unsigned long long ReportConsumedEnergy();
    unsigned NumMachines();
    void SetMachineState(unsigned machine_id, MachineState_t state);
    void SetPerformance(unsigned machine_id, unsigned core_id, CPUPerformance_t p_state);
    void AttachVM(MachineId_t mid, VM * vm);
    void AttachTask(TaskId_t task_id, MachineId_t machine_id, unsigned priority, VM * vm);
    void Migrate(VM * vm, MachineId_t from, MachineId_t to);
    MachineInfo_t GetInfo(unsigned machine_id);
private:
    Simulator * simulator;
    Workload * workload;
    Scheduler * scheduler;
    void HandleTimer();
    std::vector<Machine *> machines;
     
    class TimerEvent : public Event {
    public:
        TimerEvent(Time_t time, Cluster *cl)  : Event(time) { cluster = cl; }
        void Execute() const override                       { cluster->HandleTimer(); }
    private:
        Cluster * cluster;
    };
*/
};
#endif
#endif /* Cluster_hpp */
