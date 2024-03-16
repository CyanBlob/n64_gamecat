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
reg override;

/*wire readLow;
wire readHigh;
wire step;

wire override;
wire[31:0] override_data;

wire[31:0] address_full;*/

gamecat uut(
    .ad_console (ad_console),
    .ad_cartridge (ad_cartridge),
    .write (write),
    .read (read),
    .ale_h (ale_h),
    .ale_l (ale_l),
    .clk (clk)
    /*.readLow (readLow),
    .readHigh (readHigh),
    .step (step),
    .override (override),
    .override_data (override_data),
    .address_full (address_full)*/
);

integer i = 0;

reg[15:0] cartReturn = 16'h0000;
reg[15:0] consoleRequest = 16'h0000;

//assign ad_cartridge[15:0] = (~read || ~write) ? cartReturn : 16'bZ;
//assign ad_console[15:0] = (~read || ~write) ? 16'bZ : consoleRequest;

assign ad_cartridge[15:0] = (!ale_l) ? cartReturn : 16'bZ;
assign ad_console[15:0] = (!ale_l && !read) ?  16'bZ : consoleRequest;
//assign ad_console[15:0] = (override) ?  16'bZ : consoleRequest;
    
initial begin

    clk = 0;
    read = 1;
    write = 1;
    ale_l = 0;
    ale_h = 1;
    
    for (i = 0; i < 90; i = i + 1) begin
        #10
        clk = ~clk;
        
        if (i == 5) begin
            consoleRequest = 16'h0420;
            ale_l = 1;
        end
        
        if (i == 5 + 19) begin
            consoleRequest = 16'h6969;
        end
        
        if (i == 5 + 23) begin
            ale_l = 0;
        end
        
        if (i == 5 + 12) begin
            ale_h = 0;
        end
        
        if (i == 40) begin
            read = 0;
        end
        
        if (i == 41) begin
            cartReturn = 16'hF0F0;
        end
        
        if (i == 40 + 15) begin
            read = 1;
        end
        
        if (i == 40 + 20) begin
            read = 0;
        end
        
        if (i == 40 + 21) begin
            cartReturn = 16'h1234;
        end
        
        if (i == 40 + 35) begin
            read = 1;
        end
        
        if (i == 40 + 40) begin
            read = 0;
        end
        
        if (i == 40 + 45) begin
            read = 1;
        end
    end
end

endmodule
