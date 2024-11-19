# Weak compute, stress testcase, multiple different VM-HW configs, no gpu

# X86
machine class:
{
        Number of machines: 16
        CPU type: X86
        Number of cores: 8
        Memory: 16384
        S-States: [300, 250, 250, 200, 100, 50, 0]
        P-States: [12, 8, 4, 2]
        C-States: [12, 8, 4, 0]
        MIPS: [2100, 1800, 1600, 1400]
        GPUs: yes
}
# ARM
machine class:
{
        Number of machines: 32
        Number of cores: 12
        CPU type: ARM
        Memory: 16384
        S-States: [120, 100, 100, 80, 40, 10, 0]
        P-States: [12, 8, 6, 4]
        C-States: [12, 3, 1, 0]
        MIPS: [1200, 1000, 600, 500]
        GPUs: yes
}

# GoCJ Dataset

# Our task classes are based on the Google Cloud Jobs (GoCJ) Dataset 
# (https://www.mdpi.com/2306-5729/3/4/38). Based off real world Google cluster 
# traces, they found that 20% of tasks will be small, 40% medium, 30% large, 
# 4% extra-large, and 6% huge. For each task class, we set the run time to the average
# of the bounds for each class of tasks in the Google cluster data. Stress test, so all tasks are SLA0
# with 10,000 tasks assigned 0 and 100,000 microseconds. CPU types vary between ARM and X86, and Windows and Linux are used for VM

# MI = Millions of Instructions
# "small" tasks (15,000 - 55,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 100
        Expected runtime: 35000000
        Memory: 2048
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: ARM
        Task type: WEB
        Seed: 318282
}

task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 100
        Expected runtime: 35000000
        Memory: 4096
        VM type: WIN
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

# "medium" tasks (59,000 - 99,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 50
        Expected runtime: 79000000
        Memory: 8192
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 50
        Expected runtime: 79000000
        Memory: 16384
        VM type: WIN
        GPU enabled: no
        SLA type: SLA0
        CPU type: ARM
        Task type: WEB
        Seed: 318282
}

# "large" tasks (101,000 - 135,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 33.3
        Expected runtime: 118000000
        Memory: 32768
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

# "extra-large" tasks (150,000 - 375,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 250
        Expected runtime: 262500000
        Memory: 65536
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: ARM
        Task type: HPC
        Seed: 318282
}

# "huge" tasks (525,000 - 900,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 166.7
        Expected runtime: 712500000
        Memory: 131072
        VM type: WIN
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}


