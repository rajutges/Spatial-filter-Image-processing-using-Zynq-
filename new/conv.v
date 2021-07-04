`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 06/27/2021 04:47:24 PM
// Design Name: 
// Module Name: conv
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


module conv(
input        i_clk,
input [71:0] i_pixel_data, // 3*24 (as we are taking 3 line buffers at a time)
input        i_pixel_data_valid,
output reg [7:0] o_convolved_data,
output reg   o_convolved_data_valid
    );
    
integer i; 
reg [7:0] kernel [8:0];    //kernel of 3*3 so require 9 bits
reg [15:0] multData[8:0];
reg [15:0] sumDataInt;
reg [15:0] sumData;
reg multDataValid;
reg sumDataValid;
reg convolved_data_valid;

// initialize the kernel

initial
begin
    for(i=0;i<9;i=i+1)
    begin
        kernel[i] = 1;
    end
end    
  // do the multiplication
always @(posedge i_clk)
begin
    for(i=0;i<9;i=i+1)
    begin
        multData[i] <= kernel[i]*i_pixel_data[i*8+:8];
    end
    multDataValid <= i_pixel_data_valid;
end

// adding (use of combinational ckt as to avoid problem of non blocking and blocking condition
// sumData<=0 is non blocking
// sumData=0 is blocking 
always @(*)
begin
    sumDataInt = 0;
    for(i=0;i<9;i=i+1)
    begin
        sumDataInt = sumDataInt + multData[i];
    end
end
// assign to sumDataInt
always @(posedge i_clk)
begin
    sumData <= sumDataInt;
    sumDataValid <= multDataValid;
end
    // division by 9 for kernel
    // in xilinx direct division is supported
always @(posedge i_clk)
begin
    o_convolved_data <= sumData/9;
    o_convolved_data_valid <= sumDataValid;
end
    
endmodule
