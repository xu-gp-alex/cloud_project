# Flood of quick to complete tasks
# Used to evaluate how the scheduler will make decisions
# when migrations will take longer than just finishing the task

machine class:
{
        Number of machines: 256
        CPU type: X86
        Number of cores: 4
        Memory: 8192
        S-States: [120, 100, 80, 60, 40, 10, 0]
        P-States: [8, 6, 4, 2]
        C-States: [8, 3, 1, 0]
        MIPS: [800, 600, 400, 100]
        GPUs: no
}

task class:
{
        Start time: 100000
        End time : 1000000
        Inter arrival: 10
        Expected runtime: 1000
        Memory: 512
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

task class:
{
        Start time: 100000
        End time : 1000000
        Inter arrival: 2
        Expected runtime: 100
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
        Start time: 100000
        End time : 1000000
        Inter arrival: 1
        Expected runtime: 50
        Memory: 128
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}
