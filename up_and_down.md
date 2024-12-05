# Meant to evaluate how the system handles cycles of large workloads and downtimes
# How well does it balance saving energy by turning machines off and avoiding SLA 
# violations by not having to start up new machines and VMs

machine class:
{
        Number of machines: 128
        CPU type: X86
        Number of cores: 8
        Memory: 32768
        S-States: [200, 120, 100, 80, 40, 10, 0]
        P-States: [12, 8, 6, 4]
        C-States: [12, 3, 1, 0]
        MIPS: [1400, 800, 600, 400]
        GPUs: yes
}
task class:
{
        Start time: 100000
        End time : 500000
        Inter arrival: 500
        Expected runtime: 10000
        Memory: 256
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}
task class:
{
        Start time: 1000000
        End time : 1500000
        Inter arrival: 500
        Expected runtime: 10000
        Memory: 256
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}
task class:
{
        Start time: 2000000
        End time : 2500000
        Inter arrival: 500
        Expected runtime: 10000
        Memory: 256
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}
