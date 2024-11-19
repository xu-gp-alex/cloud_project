# Weak compute, UT Gameday, multiple different VM-HW configs, gpu

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
        Number of cores: 16
        CPU type: ARM
        Memory: 16384
        S-States: [120, 100, 100, 80, 40, 10, 0]
        P-States: [12, 8, 6, 4]
        C-States: [12, 3, 1, 0]
        MIPS: [1200, 1000, 600, 500]
        GPUs: yes
}

# this testcase is supposed to represent a UT Gameday. We have some basic tasks 
# that aren't too important, then we have a surge of attendees watching the 
# live stream, then we have some more unimportant tasks

# REDDIT
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 6000
        Expected runtime: 600300
        Memory: 400
        VM type: WIN
        GPU enabled: no
        SLA type: SLA1
        CPU type: ARM
        Task type: WEB
        Seed: 520230
}

# CRYPTO
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 100
        Expected runtime: 2000000
        Memory: 4096
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA2
        CPU type: X86
        Task type: CRYPTO
        Seed: 318288
}

# CANVAS
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 1000
        Expected runtime: 95220
        Memory: 100
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA1
        CPU type: X86
        Task type: WEB
        Seed: 318082
}


# game stream
task class:
{
        Start time: 20000
        End time : 95000
        Inter arrival: 1
        Expected runtime: 700000
        Memory: 2048
        VM type: WIN
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}


