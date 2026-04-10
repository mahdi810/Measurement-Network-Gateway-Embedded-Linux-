############################
# On-board leds            #
############################
# all 8 leds
set_property PACKAGE_PIN T22 [get_ports zLED[0]]
set_property PACKAGE_PIN T21 [get_ports zLED[1]]
set_property PACKAGE_PIN U22 [get_ports zLED[2]]
set_property PACKAGE_PIN U21 [get_ports zLED[3]]
set_property PACKAGE_PIN V22 [get_ports zLED[4]]
set_property PACKAGE_PIN W22 [get_ports zLED[5]]
set_property PACKAGE_PIN U19 [get_ports zLED[6]]
set_property PACKAGE_PIN U14 [get_ports zLED[7]]
set_property IOSTANDARD LVCMOS33 [get_ports zLED]

# zSwitch [7:0]
############################
# On-board switches        #
############################
set_property PACKAGE_PIN F22 [get_ports zSwitch[0]]
set_property PACKAGE_PIN G22 [get_ports zSwitch[1]]
set_property PACKAGE_PIN H22 [get_ports zSwitch[2]]
set_property PACKAGE_PIN F21 [get_ports zSwitch[3]]
set_property PACKAGE_PIN H19 [get_ports zSwitch[4]]
set_property PACKAGE_PIN H18 [get_ports zSwitch[5]]
set_property PACKAGE_PIN H17 [get_ports zSwitch[6]]
set_property PACKAGE_PIN M15 [get_ports zSwitch[7]]
set_property IOSTANDARD LVCMOS18 [get_ports zSwitch]

# zPushB [4:0]
############################
# On-board pushbuttons     #
############################
set_property PACKAGE_PIN P16 [get_ports zPushB[0]]
set_property PACKAGE_PIN R16 [get_ports zPushB[1]]
set_property PACKAGE_PIN N15 [get_ports zPushB[2]]
set_property PACKAGE_PIN R18 [get_ports zPushB[3]]
set_property PACKAGE_PIN T18 [get_ports zPushB[4]]
set_property IOSTANDARD LVCMOS18 [get_ports zPushB]


# PMOD JA
############################
# PmodAD1                 #
############################
#NET JA1         LOC = Y11   | IOSTANDARD=LVCMOS33;
#NET JA2         LOC = AA11  | IOSTANDARD=LVCMOS33;
#NET JA3         LOC = Y10   | IOSTANDARD=LVCMOS33;
#NET JA4         LOC = AA9   | IOSTANDARD=LVCMOS33;
set_property PACKAGE_PIN Y11 [get_ports AD1csq]
set_property IOSTANDARD LVCMOS33 [get_ports AD1csq]
set_property PACKAGE_PIN AA9 [get_ports AD1Sclk]
set_property IOSTANDARD LVCMOS33 [get_ports AD1Sclk]
set_property PACKAGE_PIN Y10 [get_ports sdiAD_1]
set_property IOSTANDARD LVCMOS33 [get_ports sdiAD_1]
set_property PACKAGE_PIN AA11 [get_ports sdiAD_0]
set_property IOSTANDARD LVCMOS33 [get_ports sdiAD_0]


# PMOD JC1
############################
# MAXPMB1#                 #
############################
#NET JC1_N         LOC = AB6  | IOSTANDARD=LVCMOS33;  # "JC1_N" - 2
#NET JC1_P         LOC = AB7  | IOSTANDARD=LVCMOS33;  # "JC1_P" - 1
#NET JC2_N         LOC = AA4  | IOSTANDARD=LVCMOS33;  # "JC2_N" - 4
#NET JC2_P         LOC = Y4   | IOSTANDARD=LVCMOS33;  # "JC2_P" - 3
#NET JC3_N         LOC = T6   | IOSTANDARD=LVCMOS33;  # "JC3_N" - 8
#NET JC3_P         LOC = R6   | IOSTANDARD=LVCMOS33;  # "JC3_P" - 7
#NET JC4_N         LOC = U4   | IOSTANDARD=LVCMOS33;  # "JC4_N" -10
#NET JC4_P         LOC = T4   | IOSTANDARD=LVCMOS33;  # "JC4_P" - 9
set_property PACKAGE_PIN AB7 [get_ports mCSn]
set_property IOSTANDARD LVCMOS33 [get_ports mCSn]
set_property PACKAGE_PIN AB6 [get_ports mMOSI]
set_property IOSTANDARD LVCMOS33 [get_ports mMOSI]
set_property PACKAGE_PIN Y4 [get_ports mMISO]
set_property IOSTANDARD LVCMOS33 [get_ports mMISO]
set_property PACKAGE_PIN AA4 [get_ports mSCLK]
set_property IOSTANDARD LVCMOS33 [get_ports mSCLK]
set_property PACKAGE_PIN R6 [get_ports mDRDYn]
set_property IOSTANDARD LVCMOS33 [get_ports mDRDYn]
