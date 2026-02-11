#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <random>
#include <verilated.h>
#include <queue>
#include <verilated_vcd_c.h>
#include "Vaxis_fifo.h"

#define MAX_SIM_TIME 1000000
vluint64_t sim_time = 0;
vluint64_t posedge_cnt = 0;


void dut_reset (Vaxis_fifo *dut, vluint64_t &sim_time){
    if( sim_time < 100 ){
        dut->arstn_i = 0;
    }
    else {
        dut->arstn_i = 1;
    }
}

int main(int argc, char** argv, char** env) {
    Verilated::commandArgs(argc, argv);
    Vaxis_fifo *dut = new Vaxis_fifo;
    Verilated::traceEverOn(true);
    VerilatedVcdC* sim_trace = new VerilatedVcdC;
    dut->trace(sim_trace, 10);
    sim_trace->open("./waveform.vcd");

    // Initialize inputs.
    dut->clk_i = 0;
    dut->arstn_i = 0;
    dut->s_axis_tdata_i = 0;
    dut->s_axis_tvalid_i = 0;
    dut->s_axis_tlast_i = 0;
    dut->m_axis_tready_i = 0;

    // Setup random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    // int seed = 42;
    // std::mt19937 gen(seed);
    std::uniform_int_distribution<uint8_t> dist(0, std::numeric_limits<uint8_t>::max());
    std::uniform_int_distribution<int> valid_dist(0, 99);
    std::uniform_int_distribution<int> last_dist(0, 99);
    std::uniform_int_distribution<int> ready_dist(0, 99);

    uint8_t s_axis_tdata_i = 0;
    int s_axis_tvalid_i = 0;
    int s_axis_tlast_i  = 0;

    int m_axis_tready_i = 0;

    std::queue<uint8_t> expected_data;
    std::queue<uint8_t> expected_last;
    bool pass = true;

    int mistakes = 0;

while (sim_time < MAX_SIM_TIME && (mistakes < 15)) {
    dut_reset(dut, sim_time);

    dut->clk_i ^= 1; // Toggle clock
    dut->eval(); // Apply changes

    // --- Driving Logic ---
    if (dut->clk_i == 1) {

        // Only update inputs when not in reset
        if (dut->arstn_i) {
            // AXI Master Logic: Decide if we want to send data
            // We only pick NEW data if the previous transfer finished
            if (!dut->s_axis_tvalid_i || (dut->s_axis_tvalid_i && dut->s_axis_tready_o)) {
                dut->s_axis_tvalid_i = (valid_dist(gen) < 70);
                dut->s_axis_tdata_i  = static_cast<uint8_t>(dist(gen));
                dut->s_axis_tlast_i  = (last_dist(gen) < 10);
            }

            // AXI Slave Logic: Decide if we are ready to receive
            dut->m_axis_tready_i = (ready_dist(gen) < 50);
        }
    }

    // --- Sampling/Scoreboard Logic (On Rising Edge) ---
    if (dut->clk_i == 1 && dut->arstn_i) {

        // 1. Monitor Input (Push to Scoreboard)
        if (dut->s_axis_tvalid_i && dut->s_axis_tready_o) {
            expected_data.push(dut->s_axis_tdata_i);
            expected_last.push(dut->s_axis_tlast_i);
        }

        // 2. Monitor Output (Check against Scoreboard)
        if (dut->m_axis_tvalid_o && dut->m_axis_tready_i) {
            if (expected_data.empty()) {
                std::cerr << "ERROR: DUT asserted valid with no expected data at " << sim_time << std::endl;
                pass = false;
            } else {
                uint8_t actual_data = dut->m_axis_tdata_o;
                uint8_t exp_data    = expected_data.front();
                bool actual_last    = dut->m_axis_tlast_o;
                bool exp_last       = expected_last.front();

                if (actual_data != exp_data || actual_last != exp_last) {
                    mistakes++;
                    std::cout << "FAIL! Time: " << std::dec << sim_time << std::setfill('0')
                              << " | Exp: 0x" << std::hex << std::setw(2) << (int)exp_data
                              << " [L:" << (int)exp_last << "]"
                              << " | Got: 0x" << std::hex << std::setw(2) << (int)actual_data
                              << " [L:" << (int)actual_last << "]"
                              << std::endl;
                    pass = false;
                }
                expected_data.pop();
                expected_last.pop();
            }
        }
    }

    sim_trace->dump(sim_time);
    sim_time++;
}

    if (pass) std::cout << "PASS\n";
    else std::cout << "FAIL\n";

    sim_trace->close();
    delete sim_trace;
    delete dut;
    exit(EXIT_SUCCESS);
}
