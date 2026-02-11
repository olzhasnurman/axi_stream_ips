import os

VERILATE_COMMAND_TRACE = "verilator --assert -I./rtl/ --Wall --trace --cc ./rtl/axis_fifo.sv --exe test/test_axis_fifo.cpp"
MAKE_COMMAND = "make -C obj_dir -f Vaxis_fifo.mk"
SAVE_COMMAND = "./obj_dir/Vaxis_fifo"


CLEAN_COMMAND = "rm -r ./obj_dir"


def main():
    os.system(VERILATE_COMMAND_TRACE)
    os.system(MAKE_COMMAND)
    os.system(SAVE_COMMAND)
    os.system(CLEAN_COMMAND)


main()
