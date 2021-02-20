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
    
    input clk
    );

reg[15:0] address_low;
reg[15:0] address_high;
reg[15:0] address_active;

reg step = 0;

reg low;
reg high;

assign ad_cartridge[15:0] = read ? 16'bZ : address_active[15:0];
assign ad_console[15:0] = read ? address_active[15:0] : 16'bZ;
//assign ad_cartridge = read ? address_active : ad_console;
//assign ad_console = read ? 1'bZ : address_active;

//assign ad_cartridge = read ? 1'bZ : ad_console;
//assign ad_console = read ? ad_cartridge : 1'bZ;

always @(negedge clk) begin
    if (ale_l && ale_h && read) begin
        address_high <= ad_console;
        address_active <= ad_console;
    end
    
    else if (ale_l && !ale_h && read) begin
        address_low <= ad_console;
        address_active <= ad_console;
    end
    
    if (!read) begin
        if (step) begin
            address_high <= ad_cartridge;
            address_active <= ad_cartridge;
        end else begin
            address_low <= ad_cartridge;
            address_active <= ad_cartridge;
        end
        step <= ~step;
    end
    if (!write) begin
        if (step) begin
            address_high <= ad_console;
            address_active <= ad_console;
        end else begin
            address_low <= ad_console;
            address_active <= ad_console;
        end
        step <= ~step;
    end
end

/*always @(negedge ale_h) begin
    address_high <= ad_console;
end

always @(negedge ale_l) begin
    address_low <= ad_console;
end*/

/*always @(negedge read) begin
    if (~write) begin
        if (step) begin
            address_active <= address_low;
        end else begin
            address_active <= address_high;
        end
        step <= ~step;
    end
end*/

/*always @(negedge write or negedge read) begin
    if (!read || !write) begin
        if (step) begin
            address_active <= address_low;
        end else begin
            address_active <= address_high;
        end
        step <= ~step;
    end 
end*/

endmodule