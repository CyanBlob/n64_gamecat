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
    
    output led0_r
);

reg[15:0] address_low = 16'h0000;
reg[15:0] address_high= 16'h0000;
reg[15:0] address_active= 16'h0000;

reg[63:0] led_timer = 64'h0000000000000000;

reg step = 0;
reg led_state = 0;

reg readLow = 0;
reg readHigh = 0;

assign ad_cartridge[15:0] = (read && write) ? address_active[15:0] : 16'bZ;
assign ad_console[15:0] = (read && write) ? 16'bZ : address_active[15:0];

assign led0_r = led_state;

//always @(negedge clk) begin
always @(read || ale_l || ale_h || write) begin
    if (ale_l && ~ale_h && read && write && ~readHigh && ~step) begin
        address_high = ad_console;
        address_active = ad_console;
        readHigh = 1;
    end
    
    else if (~ale_l && ~ale_h && read && write && ~readLow && ~step) begin
        address_low = ad_console;
        address_active = ad_console;
        readLow = 1;
    end
    
    if (~read) begin
        if (step) begin
            address_high = ad_cartridge;
            address_active = ad_cartridge;
            readHigh = 0;
        end
        if (~step) begin
            address_low = ad_cartridge;
            address_active = ad_cartridge;
            readLow = 0;
        end
        step = ~step;
    end
    if (~write) begin
        if (step) begin
            address_high = ad_console;
            address_active = ad_console;
        end else begin
            address_low = ad_console;
            address_active = ad_console;
        end
        step = ~step;
    end
    
    led_timer = led_timer + 1;
    if (led_timer == 100000000) begin
        led_state = !led_state;
        led_timer = 0;
    end
    
end

endmodule