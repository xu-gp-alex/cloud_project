# Approximation of STAMPEDE3 with realistic test cases

# Sapphire Rapids Compute Nodes
# CPU: Intel Xeon CPU MAX 9480 ("Sapphire Rapids HBM")
# TDP: 350 W
machine class:
{
        Number of machines: 560
        CPU type: X86
        Number of cores: 112
        Memory: 131072
        S-States: [350, 295, 295, 235, 120, 30, 0]
        P-States: [3, 2.5, 2, 1]
        C-States: [3, 2.5, 1.5, 0]
        MIPS: [1900, 1500, 1300, 1000]
        GPUs: no
}

# Ponte Vecchio Compute Nodes
# CPU: Intel Xeon Platinum 8480 ("Sapphire Rapids")
# TDP: 350 W
# GPU: Intel Data Center GPU Max 1550s ("Ponte Vecchio")
machine class:
{
        Number of machines: 20
        Number of cores: 96
        CPU type: X86
        Memory: 519168
        S-States: [350, 295, 295, 235, 120, 30, 0]
        P-States: [3, 2.5, 2, 1]
        C-States: [3, 2.5, 1.5, 0]
        MIPS: [2000, 1600, 1200, 800]
        GPUs: yes
}

# Skylake Compute Nodes
# CPU: Intel Xeon Platinum 8160 ("Skylake")
# TDP: 150 W
machine class:
{
        Number of machines: 1060
        Number of cores: 48
        CPU type: X86
        Memory: 196608
        S-States: [120, 100, 100, 80, 40, 10, 0]
        P-States: [12, 8, 6, 4]
        C-States: [12, 3, 1, 0]
        MIPS: [2100, 1800, 1600, 1400]
        GPUs: no
}

# ICX Compute Nodes
# CPU: Intel Xeon Platinum 8380 ("Ice Lake")
# TDP:

# GoCJ Dataset
# question: how frequently do tasks arrive in real world?
# arbitrarily, we start with 10,000 tasks
# arbitrarily, we have t_0 = 0, t_1 = 100,000
# arbitrarily, set expected runtime to avg of extremes
# Our task classes are based on the Google Cloud Jobs (GoCJ) Dataset 
# (https://www.mdpi.com/2306-5729/3/4/38). Based off real world Google cluster 
# traces, they found that 20% of tasks will be small, 40% medium, 30% large, 
# 4% extra-large, and 6% huge
# 

# "small" tasks (15,000 - 55,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 1000
        Expected runtime: 35000000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 1000
        Expected runtime: 35000000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA1
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

# "medium" tasks (59,000 - 99,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 500
        Expected runtime: 79000000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 500
        Expected runtime: 79000000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA1
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

# "large" tasks (101,000 - 135,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 333
        Expected runtime: 118000000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA1
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

# "extra-large" tasks (150,000 - 375,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 2500
        Expected runtime: 262500000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA2
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

# "huge" tasks (525,000 - 900,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 1667
        Expected runtime: 712500000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA3
        CPU type: X86
        Task type: WEB
        Seed: 520230
}