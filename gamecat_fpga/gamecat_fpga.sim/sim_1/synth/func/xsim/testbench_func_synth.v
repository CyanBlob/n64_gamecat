// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2023 Advanced Micro Devices, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2023.2 (win64) Build 4029153 Fri Oct 13 20:14:34 MDT 2023
// Date        : Fri Mar 15 19:21:36 2024
// Host        : ANDREWTHOMA7F28 running 64-bit major release  (build 9200)
// Command     : write_verilog -mode funcsim -nolib -force -file
//               C:/Users/andrew/Downloads/n64_gamecat/gamecat_fpga/gamecat_fpga.sim/sim_1/synth/func/xsim/testbench_func_synth.v
// Design      : gamecat
// Purpose     : This verilog netlist is a functional simulation representation of the design and should not be modified
//               or synthesized. This netlist cannot be used for SDF annotated simulation.
// Device      : xc7a35ticsg324-1L
// --------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

(* NotValidForBitStream *)
module gamecat
   (ad_console,
    ad_cartridge,
    write,
    read,
    ale_h,
    ale_l,
    clk,
    led,
    led3_r,
    led2_g,
    led1_b,
    led0_b,
    led0_g);
  inout [15:0]ad_console;
  inout [15:0]ad_cartridge;
  input write;
  input read;
  input ale_h;
  input ale_l;
  input clk;
  output [3:0]led;
  output led3_r;
  output led2_g;
  output led1_b;
  output led0_b;
  output led0_g;

  wire [15:0]ad_cartridge;
  wire [15:0]ad_cartridge_IBUF;
  wire [15:0]ad_cartridge_OBUF;
  wire \ad_cartridge_TRI[0] ;
  wire [15:0]ad_console;
  wire [15:0]ad_console_IBUF;
  wire \ad_console_TRI[0] ;
  wire \address_active_reg[0]_i_1_n_0 ;
  wire \address_active_reg[0]_i_2_n_0 ;
  wire \address_active_reg[10]_i_1_n_0 ;
  wire \address_active_reg[10]_i_2_n_0 ;
  wire \address_active_reg[11]_i_1_n_0 ;
  wire \address_active_reg[11]_i_2_n_0 ;
  wire \address_active_reg[12]_i_1_n_0 ;
  wire \address_active_reg[12]_i_2_n_0 ;
  wire \address_active_reg[13]_i_1_n_0 ;
  wire \address_active_reg[13]_i_2_n_0 ;
  wire \address_active_reg[14]_i_1_n_0 ;
  wire \address_active_reg[14]_i_2_n_0 ;
  wire \address_active_reg[15]_i_1_n_0 ;
  wire \address_active_reg[15]_i_2_n_0 ;
  wire \address_active_reg[15]_i_3_n_0 ;
  wire \address_active_reg[15]_i_4_n_0 ;
  wire \address_active_reg[1]_i_1_n_0 ;
  wire \address_active_reg[1]_i_2_n_0 ;
  wire \address_active_reg[2]_i_1_n_0 ;
  wire \address_active_reg[2]_i_2_n_0 ;
  wire \address_active_reg[3]_i_1_n_0 ;
  wire \address_active_reg[3]_i_2_n_0 ;
  wire \address_active_reg[4]_i_1_n_0 ;
  wire \address_active_reg[4]_i_2_n_0 ;
  wire \address_active_reg[5]_i_1_n_0 ;
  wire \address_active_reg[5]_i_2_n_0 ;
  wire \address_active_reg[6]_i_1_n_0 ;
  wire \address_active_reg[6]_i_2_n_0 ;
  wire \address_active_reg[7]_i_1_n_0 ;
  wire \address_active_reg[7]_i_2_n_0 ;
  wire \address_active_reg[8]_i_1_n_0 ;
  wire \address_active_reg[8]_i_2_n_0 ;
  wire \address_active_reg[9]_i_1_n_0 ;
  wire \address_active_reg[9]_i_2_n_0 ;
  wire [31:16]address_full;
  wire ale_h;
  wire ale_l;
  wire [3:0]led;
  wire led0_b;
  wire led0_g;
  wire led0_g_OBUF;
  wire led1_b;
  wire led1_b_OBUF;
  wire led2_g;
  wire led2_g_OBUF;
  wire led3_r;
  wire led3_r_OBUF;
  wire [3:3]led_OBUF;
  wire override;
  wire override1;
  wire override7_out;
  wire [1:1]override_data;
  wire override_reg_i_1_n_0;
  wire override_reg_i_2_n_0;
  wire override_reg_i_4_n_0;
  wire override_reg_i_5_n_0;
  wire override_reg_i_6_n_0;
  wire override_reg_i_7_n_0;
  wire override_reg_i_8_n_0;
  wire override_reg_i_9_n_0;
  wire read;
  wire read_state;
  wire read_state_reg_i_1_n_0;
  wire read_state_reg_i_2_n_0;
  wire step_reg_i_1_n_0;
  wire step_reg_i_2_n_0;
  wire write;

  IOBUF \ad_cartridge_IOBUF[0]_inst 
       (.I(ad_cartridge_OBUF[0]),
        .IO(ad_cartridge[0]),
        .O(ad_cartridge_IBUF[0]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[10]_inst 
       (.I(ad_cartridge_OBUF[10]),
        .IO(ad_cartridge[10]),
        .O(ad_cartridge_IBUF[10]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[11]_inst 
       (.I(ad_cartridge_OBUF[11]),
        .IO(ad_cartridge[11]),
        .O(ad_cartridge_IBUF[11]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[12]_inst 
       (.I(ad_cartridge_OBUF[12]),
        .IO(ad_cartridge[12]),
        .O(ad_cartridge_IBUF[12]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[13]_inst 
       (.I(ad_cartridge_OBUF[13]),
        .IO(ad_cartridge[13]),
        .O(ad_cartridge_IBUF[13]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[14]_inst 
       (.I(ad_cartridge_OBUF[14]),
        .IO(ad_cartridge[14]),
        .O(ad_cartridge_IBUF[14]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[15]_inst 
       (.I(ad_cartridge_OBUF[15]),
        .IO(ad_cartridge[15]),
        .O(ad_cartridge_IBUF[15]),
        .T(\ad_cartridge_TRI[0] ));
  LUT1 #(
    .INIT(2'h1)) 
    \ad_cartridge_IOBUF[15]_inst_i_1 
       (.I0(led2_g_OBUF),
        .O(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[1]_inst 
       (.I(ad_cartridge_OBUF[1]),
        .IO(ad_cartridge[1]),
        .O(ad_cartridge_IBUF[1]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[2]_inst 
       (.I(ad_cartridge_OBUF[2]),
        .IO(ad_cartridge[2]),
        .O(ad_cartridge_IBUF[2]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[3]_inst 
       (.I(ad_cartridge_OBUF[3]),
        .IO(ad_cartridge[3]),
        .O(ad_cartridge_IBUF[3]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[4]_inst 
       (.I(ad_cartridge_OBUF[4]),
        .IO(ad_cartridge[4]),
        .O(ad_cartridge_IBUF[4]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[5]_inst 
       (.I(ad_cartridge_OBUF[5]),
        .IO(ad_cartridge[5]),
        .O(ad_cartridge_IBUF[5]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[6]_inst 
       (.I(ad_cartridge_OBUF[6]),
        .IO(ad_cartridge[6]),
        .O(ad_cartridge_IBUF[6]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[7]_inst 
       (.I(ad_cartridge_OBUF[7]),
        .IO(ad_cartridge[7]),
        .O(ad_cartridge_IBUF[7]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[8]_inst 
       (.I(ad_cartridge_OBUF[8]),
        .IO(ad_cartridge[8]),
        .O(ad_cartridge_IBUF[8]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_cartridge_IOBUF[9]_inst 
       (.I(ad_cartridge_OBUF[9]),
        .IO(ad_cartridge[9]),
        .O(ad_cartridge_IBUF[9]),
        .T(\ad_cartridge_TRI[0] ));
  IOBUF \ad_console_IOBUF[0]_inst 
       (.I(ad_cartridge_OBUF[0]),
        .IO(ad_console[0]),
        .O(ad_console_IBUF[0]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[10]_inst 
       (.I(ad_cartridge_OBUF[10]),
        .IO(ad_console[10]),
        .O(ad_console_IBUF[10]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[11]_inst 
       (.I(ad_cartridge_OBUF[11]),
        .IO(ad_console[11]),
        .O(ad_console_IBUF[11]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[12]_inst 
       (.I(ad_cartridge_OBUF[12]),
        .IO(ad_console[12]),
        .O(ad_console_IBUF[12]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[13]_inst 
       (.I(ad_cartridge_OBUF[13]),
        .IO(ad_console[13]),
        .O(ad_console_IBUF[13]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[14]_inst 
       (.I(ad_cartridge_OBUF[14]),
        .IO(ad_console[14]),
        .O(ad_console_IBUF[14]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[15]_inst 
       (.I(ad_cartridge_OBUF[15]),
        .IO(ad_console[15]),
        .O(ad_console_IBUF[15]),
        .T(\ad_console_TRI[0] ));
  (* SOFT_HLUTNM = "soft_lutpair4" *) 
  LUT2 #(
    .INIT(4'hE)) 
    \ad_console_IOBUF[15]_inst_i_1 
       (.I0(led2_g_OBUF),
        .I1(led1_b_OBUF),
        .O(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[1]_inst 
       (.I(ad_cartridge_OBUF[1]),
        .IO(ad_console[1]),
        .O(ad_console_IBUF[1]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[2]_inst 
       (.I(ad_cartridge_OBUF[2]),
        .IO(ad_console[2]),
        .O(ad_console_IBUF[2]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[3]_inst 
       (.I(ad_cartridge_OBUF[3]),
        .IO(ad_console[3]),
        .O(ad_console_IBUF[3]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[4]_inst 
       (.I(ad_cartridge_OBUF[4]),
        .IO(ad_console[4]),
        .O(ad_console_IBUF[4]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[5]_inst 
       (.I(ad_cartridge_OBUF[5]),
        .IO(ad_console[5]),
        .O(ad_console_IBUF[5]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[6]_inst 
       (.I(ad_cartridge_OBUF[6]),
        .IO(ad_console[6]),
        .O(ad_console_IBUF[6]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[7]_inst 
       (.I(ad_cartridge_OBUF[7]),
        .IO(ad_console[7]),
        .O(ad_console_IBUF[7]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[8]_inst 
       (.I(ad_cartridge_OBUF[8]),
        .IO(ad_console[8]),
        .O(ad_console_IBUF[8]),
        .T(\ad_console_TRI[0] ));
  IOBUF \ad_console_IOBUF[9]_inst 
       (.I(ad_cartridge_OBUF[9]),
        .IO(ad_console[9]),
        .O(ad_console_IBUF[9]),
        .T(\ad_console_TRI[0] ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[0] 
       (.CLR(1'b0),
        .D(\address_active_reg[0]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[0]));
  LUT5 #(
    .INIT(32'hFFFFD888)) 
    \address_active_reg[0]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[0]),
        .I2(override),
        .I3(override_data),
        .I4(\address_active_reg[0]_i_2_n_0 ),
        .O(\address_active_reg[0]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair0" *) 
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[0]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[0]),
        .O(\address_active_reg[0]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[10] 
       (.CLR(1'b0),
        .D(\address_active_reg[10]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[10]));
  LUT5 #(
    .INIT(32'hFFFFD888)) 
    \address_active_reg[10]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[10]),
        .I2(override),
        .I3(override_data),
        .I4(\address_active_reg[10]_i_2_n_0 ),
        .O(\address_active_reg[10]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[10]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[10]),
        .O(\address_active_reg[10]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[11] 
       (.CLR(1'b0),
        .D(\address_active_reg[11]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[11]));
  LUT5 #(
    .INIT(32'hFFFFD888)) 
    \address_active_reg[11]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[11]),
        .I2(override),
        .I3(override_data),
        .I4(\address_active_reg[11]_i_2_n_0 ),
        .O(\address_active_reg[11]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[11]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[11]),
        .O(\address_active_reg[11]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[12] 
       (.CLR(1'b0),
        .D(\address_active_reg[12]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[12]));
  LUT5 #(
    .INIT(32'hFFFFD888)) 
    \address_active_reg[12]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[12]),
        .I2(override),
        .I3(override_data),
        .I4(\address_active_reg[12]_i_2_n_0 ),
        .O(\address_active_reg[12]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[12]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[12]),
        .O(\address_active_reg[12]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[13] 
       (.CLR(1'b0),
        .D(\address_active_reg[13]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[13]));
  LUT6 #(
    .INIT(64'hFFFFFFFFD8888888)) 
    \address_active_reg[13]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[13]),
        .I2(override_data),
        .I3(override),
        .I4(led_OBUF),
        .I5(\address_active_reg[13]_i_2_n_0 ),
        .O(\address_active_reg[13]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[13]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[13]),
        .O(\address_active_reg[13]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[14] 
       (.CLR(1'b0),
        .D(\address_active_reg[14]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[14]));
  LUT6 #(
    .INIT(64'hFFFFFFFF8888D888)) 
    \address_active_reg[14]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[14]),
        .I2(override_data),
        .I3(override),
        .I4(led_OBUF),
        .I5(\address_active_reg[14]_i_2_n_0 ),
        .O(\address_active_reg[14]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[14]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[14]),
        .O(\address_active_reg[14]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[15] 
       (.CLR(1'b0),
        .D(\address_active_reg[15]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[15]));
  LUT5 #(
    .INIT(32'hFFFFD888)) 
    \address_active_reg[15]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[15]),
        .I2(override),
        .I3(override_data),
        .I4(\address_active_reg[15]_i_4_n_0 ),
        .O(\address_active_reg[15]_i_1_n_0 ));
  LUT6 #(
    .INIT(64'hFF4F4F4FFF4FFF4F)) 
    \address_active_reg[15]_i_2 
       (.I0(led1_b_OBUF),
        .I1(read_state),
        .I2(led0_g_OBUF),
        .I3(led2_g_OBUF),
        .I4(led3_r_OBUF),
        .I5(override),
        .O(\address_active_reg[15]_i_2_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair1" *) 
  LUT3 #(
    .INIT(8'hDF)) 
    \address_active_reg[15]_i_3 
       (.I0(read_state),
        .I1(led1_b_OBUF),
        .I2(led0_g_OBUF),
        .O(\address_active_reg[15]_i_3_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[15]_i_4 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[15]),
        .O(\address_active_reg[15]_i_4_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[1] 
       (.CLR(1'b0),
        .D(\address_active_reg[1]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[1]));
  LUT6 #(
    .INIT(64'hFFFFFFFFD8888888)) 
    \address_active_reg[1]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[1]),
        .I2(override_data),
        .I3(override),
        .I4(led_OBUF),
        .I5(\address_active_reg[1]_i_2_n_0 ),
        .O(\address_active_reg[1]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair1" *) 
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[1]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[1]),
        .O(\address_active_reg[1]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[2] 
       (.CLR(1'b0),
        .D(\address_active_reg[2]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[2]));
  LUT5 #(
    .INIT(32'hFFFFD888)) 
    \address_active_reg[2]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[2]),
        .I2(override),
        .I3(override_data),
        .I4(\address_active_reg[2]_i_2_n_0 ),
        .O(\address_active_reg[2]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair2" *) 
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[2]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[2]),
        .O(\address_active_reg[2]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[3] 
       (.CLR(1'b0),
        .D(\address_active_reg[3]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[3]));
  LUT5 #(
    .INIT(32'hFFFFD888)) 
    \address_active_reg[3]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[3]),
        .I2(override),
        .I3(override_data),
        .I4(\address_active_reg[3]_i_2_n_0 ),
        .O(\address_active_reg[3]_i_1_n_0 ));
  (* SOFT_HLUTNM = "soft_lutpair3" *) 
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[3]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[3]),
        .O(\address_active_reg[3]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[4] 
       (.CLR(1'b0),
        .D(\address_active_reg[4]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[4]));
  (* SOFT_HLUTNM = "soft_lutpair6" *) 
  LUT3 #(
    .INIT(8'hF8)) 
    \address_active_reg[4]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[4]),
        .I2(\address_active_reg[4]_i_2_n_0 ),
        .O(\address_active_reg[4]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[4]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[4]),
        .O(\address_active_reg[4]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[5] 
       (.CLR(1'b0),
        .D(\address_active_reg[5]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[5]));
  LUT5 #(
    .INIT(32'hFFFFD888)) 
    \address_active_reg[5]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[5]),
        .I2(override),
        .I3(override_data),
        .I4(\address_active_reg[5]_i_2_n_0 ),
        .O(\address_active_reg[5]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[5]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[5]),
        .O(\address_active_reg[5]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[6] 
       (.CLR(1'b0),
        .D(\address_active_reg[6]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[6]));
  LUT6 #(
    .INIT(64'hFFFFFFFFD8888888)) 
    \address_active_reg[6]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[6]),
        .I2(override_data),
        .I3(override),
        .I4(led_OBUF),
        .I5(\address_active_reg[6]_i_2_n_0 ),
        .O(\address_active_reg[6]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[6]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[6]),
        .O(\address_active_reg[6]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[7] 
       (.CLR(1'b0),
        .D(\address_active_reg[7]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[7]));
  LUT5 #(
    .INIT(32'hFFFFD888)) 
    \address_active_reg[7]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[7]),
        .I2(override),
        .I3(override_data),
        .I4(\address_active_reg[7]_i_2_n_0 ),
        .O(\address_active_reg[7]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[7]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[7]),
        .O(\address_active_reg[7]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[8] 
       (.CLR(1'b0),
        .D(\address_active_reg[8]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[8]));
  (* SOFT_HLUTNM = "soft_lutpair6" *) 
  LUT3 #(
    .INIT(8'hF8)) 
    \address_active_reg[8]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[8]),
        .I2(\address_active_reg[8]_i_2_n_0 ),
        .O(\address_active_reg[8]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[8]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[8]),
        .O(\address_active_reg[8]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_active_reg[9] 
       (.CLR(1'b0),
        .D(\address_active_reg[9]_i_1_n_0 ),
        .G(\address_active_reg[15]_i_2_n_0 ),
        .GE(1'b1),
        .Q(ad_cartridge_OBUF[9]));
  LUT5 #(
    .INIT(32'hFFFFD888)) 
    \address_active_reg[9]_i_1 
       (.I0(\address_active_reg[15]_i_3_n_0 ),
        .I1(ad_console_IBUF[9]),
        .I2(override),
        .I3(override_data),
        .I4(\address_active_reg[9]_i_2_n_0 ),
        .O(\address_active_reg[9]_i_1_n_0 ));
  LUT5 #(
    .INIT(32'h00200000)) 
    \address_active_reg[9]_i_2 
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(override),
        .I4(ad_cartridge_IBUF[9]),
        .O(\address_active_reg[9]_i_2_n_0 ));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[16] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[0]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[16]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[17] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[1]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[17]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[18] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[2]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[18]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[19] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[3]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[19]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[20] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[4]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[20]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[21] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[5]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[21]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[22] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[6]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[22]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[23] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[7]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[23]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[24] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[8]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[24]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[25] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[9]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[25]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[26] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[10]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[26]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[27] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[11]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[27]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[28] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[12]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[28]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[29] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[13]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[29]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[30] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[14]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[30]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \address_full_reg[31] 
       (.CLR(1'b0),
        .D(ad_console_IBUF[15]),
        .G(override1),
        .GE(1'b1),
        .Q(address_full[31]));
  (* SOFT_HLUTNM = "soft_lutpair5" *) 
  LUT2 #(
    .INIT(4'h8)) 
    \address_full_reg[31]_i_1 
       (.I0(led3_r_OBUF),
        .I1(led2_g_OBUF),
        .O(override1));
  IBUF ale_h_IBUF_inst
       (.I(ale_h),
        .O(led3_r_OBUF));
  IBUF ale_l_IBUF_inst
       (.I(ale_l),
        .O(led2_g_OBUF));
  OBUF led0_b_OBUF_inst
       (.I(led0_g_OBUF),
        .O(led0_b));
  OBUF led0_g_OBUF_inst
       (.I(led0_g_OBUF),
        .O(led0_g));
  OBUF led1_b_OBUF_inst
       (.I(led1_b_OBUF),
        .O(led1_b));
  OBUF led2_g_OBUF_inst
       (.I(led2_g_OBUF),
        .O(led2_g));
  OBUF led3_r_OBUF_inst
       (.I(led3_r_OBUF),
        .O(led3_r));
  OBUF \led_OBUF[0]_inst 
       (.I(ad_cartridge_OBUF[0]),
        .O(led[0]));
  OBUF \led_OBUF[1]_inst 
       (.I(ad_cartridge_OBUF[1]),
        .O(led[1]));
  OBUF \led_OBUF[2]_inst 
       (.I(led1_b_OBUF),
        .O(led[2]));
  OBUF \led_OBUF[3]_inst 
       (.I(led_OBUF),
        .O(led[3]));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    \override_data_reg[1] 
       (.CLR(1'b0),
        .D(1'b1),
        .G(override7_out),
        .GE(1'b1),
        .Q(override_data));
  LDCP #(
    .INIT(1'b0)) 
    override_reg
       (.CLR(override_reg_i_2_n_0),
        .D(1'b0),
        .G(override_reg_i_1_n_0),
        .PRE(override7_out),
        .Q(override));
  (* SOFT_HLUTNM = "soft_lutpair5" *) 
  LUT3 #(
    .INIT(8'hD0)) 
    override_reg_i_1
       (.I0(override),
        .I1(led3_r_OBUF),
        .I2(led2_g_OBUF),
        .O(override_reg_i_1_n_0));
  LUT4 #(
    .INIT(16'h0004)) 
    override_reg_i_2
       (.I0(override),
        .I1(led2_g_OBUF),
        .I2(led3_r_OBUF),
        .I3(override7_out),
        .O(override_reg_i_2_n_0));
  LUT6 #(
    .INIT(64'h8000000000000000)) 
    override_reg_i_3
       (.I0(override_reg_i_4_n_0),
        .I1(override_reg_i_5_n_0),
        .I2(override_reg_i_6_n_0),
        .I3(override_reg_i_7_n_0),
        .I4(override_reg_i_8_n_0),
        .I5(override_reg_i_9_n_0),
        .O(override7_out));
  LUT6 #(
    .INIT(64'h0010000000000000)) 
    override_reg_i_4
       (.I0(ad_console_IBUF[1]),
        .I1(ad_console_IBUF[2]),
        .I2(ad_console_IBUF[3]),
        .I3(ad_console_IBUF[4]),
        .I4(ad_console_IBUF[5]),
        .I5(ad_console_IBUF[6]),
        .O(override_reg_i_4_n_0));
  LUT6 #(
    .INIT(64'h0000000000000004)) 
    override_reg_i_5
       (.I0(address_full[31]),
        .I1(ad_console_IBUF[0]),
        .I2(address_full[29]),
        .I3(address_full[30]),
        .I4(address_full[27]),
        .I5(address_full[28]),
        .O(override_reg_i_5_n_0));
  LUT6 #(
    .INIT(64'h0000000000040000)) 
    override_reg_i_6
       (.I0(ad_console_IBUF[7]),
        .I1(ad_console_IBUF[8]),
        .I2(ad_console_IBUF[9]),
        .I3(ad_console_IBUF[10]),
        .I4(ad_console_IBUF[11]),
        .I5(ad_console_IBUF[12]),
        .O(override_reg_i_6_n_0));
  LUT6 #(
    .INIT(64'h0000000800000000)) 
    override_reg_i_7
       (.I0(ad_console_IBUF[13]),
        .I1(ad_console_IBUF[14]),
        .I2(ad_console_IBUF[15]),
        .I3(override),
        .I4(led3_r_OBUF),
        .I5(led2_g_OBUF),
        .O(override_reg_i_7_n_0));
  LUT6 #(
    .INIT(64'h0000001000000000)) 
    override_reg_i_8
       (.I0(address_full[23]),
        .I1(address_full[24]),
        .I2(address_full[21]),
        .I3(address_full[22]),
        .I4(address_full[25]),
        .I5(address_full[26]),
        .O(override_reg_i_8_n_0));
  LUT5 #(
    .INIT(32'h00000001)) 
    override_reg_i_9
       (.I0(address_full[16]),
        .I1(address_full[17]),
        .I2(address_full[18]),
        .I3(address_full[20]),
        .I4(address_full[19]),
        .O(override_reg_i_9_n_0));
  IBUF read_IBUF_inst
       (.I(read),
        .O(led1_b_OBUF));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    read_state_reg
       (.CLR(1'b0),
        .D(read_state_reg_i_1_n_0),
        .G(read_state_reg_i_2_n_0),
        .GE(1'b1),
        .Q(read_state));
  (* SOFT_HLUTNM = "soft_lutpair3" *) 
  LUT2 #(
    .INIT(4'h1)) 
    read_state_reg_i_1
       (.I0(led1_b_OBUF),
        .I1(read_state),
        .O(read_state_reg_i_1_n_0));
  (* SOFT_HLUTNM = "soft_lutpair2" *) 
  LUT2 #(
    .INIT(4'hB)) 
    read_state_reg_i_2
       (.I0(led1_b_OBUF),
        .I1(read_state),
        .O(read_state_reg_i_2_n_0));
  (* XILINX_LEGACY_PRIM = "LD" *) 
  (* XILINX_TRANSFORM_PINMAP = "VCC:GE GND:CLR" *) 
  LDCE #(
    .INIT(1'b0)) 
    step_reg
       (.CLR(1'b0),
        .D(step_reg_i_1_n_0),
        .G(step_reg_i_2_n_0),
        .GE(1'b1),
        .Q(led_OBUF));
  (* SOFT_HLUTNM = "soft_lutpair4" *) 
  LUT4 #(
    .INIT(16'h10EF)) 
    step_reg_i_1
       (.I0(led0_g_OBUF),
        .I1(led1_b_OBUF),
        .I2(read_state),
        .I3(led_OBUF),
        .O(step_reg_i_1_n_0));
  (* SOFT_HLUTNM = "soft_lutpair0" *) 
  LUT3 #(
    .INIT(8'h2F)) 
    step_reg_i_2
       (.I0(read_state),
        .I1(led1_b_OBUF),
        .I2(led0_g_OBUF),
        .O(step_reg_i_2_n_0));
  IBUF write_IBUF_inst
       (.I(write),
        .O(led0_g_OBUF));
endmodule
`ifndef GLBL
`define GLBL
`timescale  1 ps / 1 ps

module glbl ();

    parameter ROC_WIDTH = 100000;
    parameter TOC_WIDTH = 0;
    parameter GRES_WIDTH = 10000;
    parameter GRES_START = 10000;

//--------   STARTUP Globals --------------
    wire GSR;
    wire GTS;
    wire GWE;
    wire PRLD;
    wire GRESTORE;
    tri1 p_up_tmp;
    tri (weak1, strong0) PLL_LOCKG = p_up_tmp;

    wire PROGB_GLBL;
    wire CCLKO_GLBL;
    wire FCSBO_GLBL;
    wire [3:0] DO_GLBL;
    wire [3:0] DI_GLBL;
   
    reg GSR_int;
    reg GTS_int;
    reg PRLD_int;
    reg GRESTORE_int;

//--------   JTAG Globals --------------
    wire JTAG_TDO_GLBL;
    wire JTAG_TCK_GLBL;
    wire JTAG_TDI_GLBL;
    wire JTAG_TMS_GLBL;
    wire JTAG_TRST_GLBL;

    reg JTAG_CAPTURE_GLBL;
    reg JTAG_RESET_GLBL;
    reg JTAG_SHIFT_GLBL;
    reg JTAG_UPDATE_GLBL;
    reg JTAG_RUNTEST_GLBL;

    reg JTAG_SEL1_GLBL = 0;
    reg JTAG_SEL2_GLBL = 0 ;
    reg JTAG_SEL3_GLBL = 0;
    reg JTAG_SEL4_GLBL = 0;

    reg JTAG_USER_TDO1_GLBL = 1'bz;
    reg JTAG_USER_TDO2_GLBL = 1'bz;
    reg JTAG_USER_TDO3_GLBL = 1'bz;
    reg JTAG_USER_TDO4_GLBL = 1'bz;

    assign (strong1, weak0) GSR = GSR_int;
    assign (strong1, weak0) GTS = GTS_int;
    assign (weak1, weak0) PRLD = PRLD_int;
    assign (strong1, weak0) GRESTORE = GRESTORE_int;

    initial begin
	GSR_int = 1'b1;
	PRLD_int = 1'b1;
	#(ROC_WIDTH)
	GSR_int = 1'b0;
	PRLD_int = 1'b0;
    end

    initial begin
	GTS_int = 1'b1;
	#(TOC_WIDTH)
	GTS_int = 1'b0;
    end

    initial begin 
	GRESTORE_int = 1'b0;
	#(GRES_START);
	GRESTORE_int = 1'b1;
	#(GRES_WIDTH);
	GRESTORE_int = 1'b0;
    end

endmodule
`endif
