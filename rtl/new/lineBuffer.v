`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 06/27/2021 04:24:34 PM
// Design Name: 
// Module Name: lineBuffer
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


module lineBuffer(
input   i_clk,
input   i_rst,
input [7:0] i_data,
input   i_data_valid,
output [23:0] o_data, // 8*3 bits for output data
input i_rd_data  // ready signal
);

reg [7:0] line [511:0]; //line buffer
reg [8:0] wrPntr;   // which position of memory to give log(512)== 9 bits
reg [8:0] rdPntr;   

always @(posedge i_clk)
begin
    if(i_data_valid)
        line[wrPntr] <= i_data;
end

always @(posedge i_clk)
begin
    if(i_rst)
        wrPntr <= 'd0;
    else if(i_data_valid)
        wrPntr <= wrPntr + 'd1;
end

assign o_data = {line[rdPntr],line[rdPntr+1],line[rdPntr+2]};  // 13 byte in one shot

always @(posedge i_clk)
begin
    if(i_rst)
        rdPntr <= 'd0;
    else if(i_rd_data)
        rdPntr <= rdPntr + 'd1;    // same as write
end


endmodule
