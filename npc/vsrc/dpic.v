//实验手册给出的4个模块
module MemDPIC(
    input wire clk,
    input wire en,
    input wire [31:0] addr,
    input wire [7:0] wmask,
    input wire [31:0] wdata,
    output reg [31:0] rdata
);

    import "DPI-C" function int pmem_read(input int raddr);
    import "DPI-C" function void pmem_write(
        input int waddr,
        input int wdata,
        input byte wmask
    );

    always @(*) begin
        if(en)
            rdata = pmem_read(addr);
        else
            rdata = 32'b0;
    end

    always @(posedge clk) begin
        if(en && wmask != 0)
            pmem_write(addr, wdata, wmask);
    end

endmodule


module RegfileDPIC(
    input clk,
    input wen,
    input [4:0] waddr,
    input [31:0] wdata
);

    import "DPI-C" function void set_gpr_value(
        input int idx,
        input int value
    );

    always @(posedge clk) begin
        if(wen && waddr != 0)
            set_gpr_value({27'b0,waddr}, wdata);
    end

endmodule


module EbreakDPIC(
    input wire clk,
    input wire ebreak_en,
    input wire [31:0] a0_val
);

    import "DPI-C" function void npc_ebreak(
        input int code
    );

    always @(posedge clk) begin
        if(ebreak_en)
            npc_ebreak(a0_val);
    end

endmodule


module ITraceDPIC(
    input clk,
    input [31:0] pc,
    input [31:0] inst,
    input [31:0] next_pc
);

    import "DPI-C" function void npc_itrace(
        input int pc,
        input int next_pc,
        input int inst
    );

    always @(posedge clk) begin
        if(pc != 0)
            npc_itrace(pc, next_pc, inst);
    end

endmodule