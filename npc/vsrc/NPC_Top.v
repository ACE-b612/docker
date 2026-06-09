module NPC_Top(
    input clock,
    input reset,

    output [31:0] io_pc,
    output io_is_mmio
);

reg [31:0] pc;

wire [31:0] next_pc;
wire [31:0] inst;

assign io_pc = pc;

assign next_pc = pc + 4;

always @(posedge clock) begin
    if(reset)
        pc <= 32'h80000000;
    else
        pc <= next_pc;
end

MemDPIC imem(
    .clk(clock),
    .en(1'b1),
    .addr(pc),
    .wmask(8'b0),
    .wdata(32'b0),
    .rdata(inst)
);

ITraceDPIC itrace(
    .clk(clock),
    .pc(pc),
    .inst(inst),
    .next_pc(next_pc)
);

assign io_is_mmio = 1'b0;

endmodule