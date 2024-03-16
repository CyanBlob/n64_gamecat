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
    
    /*reg readLow,
    reg readHigh,
    reg step,
    
    reg override,
    reg[31:0] override_data,
    
    reg[31:0] address_full,*/
        
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

reg[15:0] read_state = 0;

reg[63:0] led_timer = 64'h0000000000000000;
reg led_state = 1'b0;

reg step = 1'b0;
reg readLow = 0;
reg readHigh = 0;
reg override = 0;
reg[31:0] override_data = 32'h00000000;
reg[31:0] address_full = 32'h00000000;

assign ad_cartridge[15:0] = (ale_l) ? address_active : 16'bZ;
assign ad_console[15:0] = (ale_l || read) ? 16'bZ : address_active;

// LED monitors
assign led[0] = address_active[0];
assign led[1] = address_active[1];
assign led[2] = read;
assign led[3] = step;
assign led3_r = ale_h;
assign led2_g = ale_l;
assign led1_b  = read;
assign led0_b  = write;
assign led0_g = write;

always @(negedge clk) begin
//always @* begin

    // console request high incoming
    if (ale_h && ale_l) begin
        address_high <= ad_console;
        address_active <= ad_console;
        address_full[31:16] <= ad_console;
        
        override <= 0;
     end
    
    // console request low incoming
    if (!ale_h && ale_l && !override) begin
        address_low <= ad_console;
        address_active <= ad_console;
        address_full[15:0] <= ad_console;
        
        if (address_full == 32'h04206969) begin
            override <= 1;
            override_data <= 32'hDEADBEEF;
        end
    end

    
    // update read states
    // new read
     if (ale_h && ale_l && read) begin
        read_state <= 0;
     end
     
     // ready to read upper bytes
     if (read_state == 0 && read == 0) begin
        read_state <= 1;
     end
     
     // read set high; no reading
     if (read_state == 1 && read == 1) begin
        read_state <= 2;
     end
     
     // read goes back low; ready to read lower bytes
     if (read_state == 2 && read == 0) begin
        read_state <= 3;
     end
     
      // read goes low again; consecutive reads don't need a new ALE round
      // TODO: Need to handle overrides occuring during this period
     if (read_state == 3 && read == 1) begin
        read_state <= 4;
     end
    
    
    if (read == 0 && read_state == 3) begin        

        address_high <= ad_cartridge;
        
        if (override) begin
            address_active <= override_data[15:0];
        end
        else begin
            address_active <= ad_cartridge;
        end   

    end else if (read == 0 && read_state == 1) begin
        address_low <= ad_cartridge;
        
        if (override) begin
            address_active <= override_data[31:16];
        end
        else begin
            address_active <= ad_cartridge;
        end
        
    end else if (read == 0 && read_state == 4) begin
       address_active <= ad_cartridge;
    end
    
    if (!write) begin
        if (step) begin
            address_high <= ad_console;
            address_active <= ad_console;
        end else begin
            address_low <= ad_console;
            address_active <= ad_console;
        end
        step = !step;
    end    
end

always @(negedge clk) begin
    led_timer = led_timer + 1;
    if (led_timer == 100000000) begin
        led_state <= !led_state;
        led_timer <= 0;
    end
end

endmodule