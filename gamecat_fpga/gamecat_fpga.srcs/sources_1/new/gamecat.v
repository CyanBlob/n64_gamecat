`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/19/2021 07:14:26 PM
// Design Name: N64 GameCat Arty A7 35
// Module Name: gamecat
// Project Name: N64 GameCat
// Target Devices: Arty A7 35
// Tool Versions: Vivado 2020.2
// Description: An Arty A7 35-based GameShark / GameGenie clone for the N64
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module gamecat(
    inout [15:0] ad_console,
    inout [15:0] ad_cartridge,
    
    input write,
    input read,
    input ale_h,
    input ale_l,
    
    input clk,
    
    output readLow,
    output  readHigh,
    output step,
        
    output [3:0] led,    //lower 4 bits of address_active
    output led3_r, //ale_l
    output led2_g, //ale_h
    output led1_b, //read
    output led0_b, //write
    output  led0_g  //write
);

reg[15:0] address_low = 16'h0000;
reg[15:0] address_high= 16'h0000;
reg[15:0] address_active= 16'h0000;

reg[63:0] led_timer = 64'h0000000000000000;

reg step = 1'b0;
reg led_state = 1'b0;

reg readLow = 0;
reg readHigh = 0;

assign ad_cartridge[15:0] = (read && write) ? address_active[15:0] : 16'bZ;
assign ad_console[15:0] = (read && write) ? 16'bZ : address_active[15:0];

// LED monitors
assign led[0] = address_active[0];
assign led[1] = address_active[1];
assign led[2] = address_active[2];
assign led[3] = address_active[3];
assign led3_r = ale_l;
assign led2_g = ale_h;
assign led1_b  = read;
assign led0_b  = write;
assign led0_g = write;

//always @(negedge clk) begin
always @* begin
    if (ale_l && !ale_h && read && write && !readHigh && !step) begin
        address_high = ad_console;
        address_active = ad_console;
        readHigh = 1;
    end
    
    else if (!ale_l && !ale_h && read && write && !readLow && !step) begin
        address_low = ad_console;
        address_active = ad_console;
        readLow = 1;
    end
    
    if (!read) begin
        if (step) begin
            address_high = ad_cartridge;
            address_active = ad_cartridge;
            readHigh = 0;
        end
        if (!step) begin
            address_low = ad_cartridge;
            address_active = ad_cartridge;
            readLow = 0;
        end
        step = !step;
    end
    if (!write) begin
        if (step) begin
            address_high = ad_console;
            address_active = ad_console;
        end else begin
            address_low = ad_console;
            address_active = ad_console;
        end
        step = !step;
    end    
end

always @(negedge clk) begin
    led_timer = led_timer + 1;
    if (led_timer == 100000000) begin
        led_state = !led_state;
        led_timer = 0;
    end
end

endmodule