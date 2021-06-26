`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 02/20/2021 12:26:35 PM
// Design Name: 
// Module Name: testbench
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module testbench();

reg ale_l;
reg ale_h;
reg read;
wire[15:0] ad_console;
wire[15:0] ad_cartridge;
reg write;
reg clk;

wire readLow;
wire readHigh;
wire step;

gamecat uut(
    .ad_console (ad_console),
    .ad_cartridge (ad_cartridge),
    .write (write),
    .read (read),
    .ale_h (ale_h),
    .ale_l (ale_l),
    .clk (clk),
    .readLow (readLow),
    .readHigh (readHigh),
    .step (step)
);

integer i = 0;

reg[15:0] cartReturn = 16'h0000;
reg[15:0] consoleRequest = 16'h0000;

assign ad_cartridge[15:0] = (~read || ~write) ? cartReturn : 16'bZ;
assign ad_console[15:0] = (~read || ~write) ? 16'bZ : consoleRequest;
    
initial begin

    clk = 0;
    read = 1;
    write = 1;
    ale_l = 0;
    ale_h = 1;
    
    for (i = 0; i < 70; i = i + 1) begin
        #10
        clk = ~clk;
        
        if (i == 5) begin
            consoleRequest = 16'h0420;
            ale_l = 1;
        end
        
        if (i == 5 + 18) begin
            consoleRequest = 16'h6969;
        end
        
        if (i == 5 + 23) begin
            ale_l = 0;
        end
        
        if (i == 5 + 12) begin
            ale_h = 0;
        end
        
        if (i == 40) begin
            cartReturn = 16'hF0F0;
            read = 0;
        end
        
        if (i == 40 + 15) begin
            cartReturn = 16'h1234;
            read = 1;
        end
        
        if (i == 40 + 20) begin
            read = 0;
        end
         if (i == 40 + 30) begin
            cartReturn = 16'h0F0F;
        end
    end
end

endmodule
