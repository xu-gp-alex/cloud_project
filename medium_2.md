# MEDIUM + OLD Mainframe, UT Gameday, gpu

# x86-based
machine class:
{
        Number of machines: 225
        CPU type: X86
        Number of cores: 48
        Memory: 65536
        S-States: [350, 295, 295, 235, 120, 30, 0]
        P-States: [6, 5, 3, 1]
        C-States: [6, 5, 3, 0]
        MIPS: [1900, 1500, 1300, 1000]
        GPUs: yes
}

# arm-based
machine class:
{
        Number of machines: 225
        CPU type: ARM
        Number of cores: 48
        Memory: 65536
        S-States: [280, 235, 235, 185, 95, 24, 0]
        P-States: [5, 4, 2, 1]
        C-States: [5, 4, 2, 0]
        MIPS: [1900, 1500, 1300, 1000]
        GPUs: yes
}

# power-based
machine class:
{
        Number of machines: 50
        CPU type: POWER
        Number of cores: 16
        Memory: 65536
        S-States: [380, 325, 325, 265, 150, 60, 0]
        P-States: [7, 6, 4, 1]
        C-States: [7, 6, 4, 0]
        MIPS: [1200, 1000, 800, 600]
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