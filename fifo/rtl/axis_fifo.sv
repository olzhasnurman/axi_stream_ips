/* Copyright (c) 2026 Maveric NU. All rights reserved. */

//--------------------------------
// Engineer     : Olzhas Nurman
// Create Date  : 11/02/2026
// Last Revision: 11/02/2026
// Design Name  : AXI Stream FIFO
// Module Name  : axis_fifo
//--------------------------------


//---------------------------------------------------
// Description:
// This module implements an AXI Stream FIFO buffer.
//---------------------------------------------------

module axis_fifo
// Parameters.
#(
    parameter DATA_WIDTH  = 8,  // Width of the data bus.
    parameter TUSER_WIDTH = 1,  // Width of the user signal.
    parameter FIFO_DEPTH  = 32  // Depth of the FIFO buffer.
)
(
    // Input interface.
    input  logic                     clk_i,
    input  logic                     arstn_i,

    // AXI Stream Slave Interface.
    input  logic [DATA_WIDTH  - 1:0] s_axis_tdata_i,
    input  logic                     s_axis_tvalid_i,
    input  logic                     s_axis_tlast_i,
    input  logic [TUSER_WIDTH - 1:0] s_axis_tuser_i,
    output logic                     s_axis_tready_o,

    // AXI Stream Master Interface.
    input  logic                     m_axis_tready_i,
    output logic [DATA_WIDTH  - 1:0] m_axis_tdata_o,
    output logic                     m_axis_tvalid_o,
    output logic                     m_axis_tlast_o,
    output logic [TUSER_WIDTH - 1:0] m_axis_tuser_o
);

    //-----------------------------------
    // Local params.
    //-----------------------------------
    localparam ADDR_WIDTH = $clog2(FIFO_DEPTH); // Address width for FIFO depth.


    //-----------------------------------
    // Internal signals.
    // ----------------------------------
    logic [DATA_WIDTH  - 1:0] fifo_data_mem [FIFO_DEPTH - 1:0];
    logic [TUSER_WIDTH - 1:0] fifo_user_mem [FIFO_DEPTH - 1:0];
    logic                     fifo_last_mem [FIFO_DEPTH - 1:0];
    logic [ADDR_WIDTH  - 1:0] fifo_write_ptr_s;
    logic [ADDR_WIDTH  - 1:0] fifo_read_ptr_s;
    logic                     fifo_full_s;
    logic                     fifo_empty_s;

    //-----------------------------------
    // FIFO Write Logic.
    // ----------------------------------
    always_ff @(posedge clk_i or negedge arstn_i) begin
        if (!arstn_i) begin
            fifo_write_ptr_s <= '0;
        end else if (s_axis_tvalid_i & s_axis_tready_o) begin
            fifo_data_mem[fifo_write_ptr_s] <= s_axis_tdata_i;
            fifo_user_mem[fifo_write_ptr_s] <= s_axis_tuser_i;
            fifo_last_mem[fifo_write_ptr_s] <= s_axis_tlast_i;
            fifo_write_ptr_s <= fifo_write_ptr_s + {{(ADDR_WIDTH - 1){1'b0}}, 1'b1};
        end
    end

    //-----------------------------------
    // FIFO Read Logic.
    // ----------------------------------
    always_ff @(posedge clk_i or negedge arstn_i) begin
        if (!arstn_i) begin
            fifo_read_ptr_s <= '0;
        end else if (m_axis_tready_i & m_axis_tvalid_o) begin
            fifo_read_ptr_s <= fifo_read_ptr_s + {{(ADDR_WIDTH - 1){1'b0}}, 1'b1};
        end
    end

    assign fifo_empty_s = (fifo_read_ptr_s == fifo_write_ptr_s);
    assign fifo_full_s  = (fifo_write_ptr_s + {{(ADDR_WIDTH - 1){1'b0}}, 1'b1} == fifo_read_ptr_s);

    //-----------------------------------
    // Output Logic.
    // ----------------------------------
    assign s_axis_tready_o = !fifo_full_s;
    assign m_axis_tdata_o  = fifo_data_mem[fifo_read_ptr_s];
    assign m_axis_tuser_o  = fifo_user_mem[fifo_read_ptr_s];
    assign m_axis_tvalid_o = !fifo_empty_s;
    assign m_axis_tlast_o  = fifo_last_mem[fifo_read_ptr_s];

    logic [ADDR_WIDTH:0] fifo_count_s;
    always_comb begin
        if (fifo_write_ptr_s >= fifo_read_ptr_s) begin
            fifo_count_s = fifo_write_ptr_s - fifo_read_ptr_s;
        end else begin
            fifo_count_s = FIFO_DEPTH - (fifo_read_ptr_s - fifo_write_ptr_s);
        end
    end

endmodule
