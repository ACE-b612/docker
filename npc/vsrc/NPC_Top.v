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

//step7 解析指令字段
wire [6:0] opcode;
wire [4:0] rd;
wire [2:0] funct3;
wire [4:0] rs1;
wire [4:0] rs2;
wire [6:0] funct7;

assign opcode = inst[6:0];
assign rd     = inst[11:7];
assign funct3 = inst[14:12];
assign rs1    = inst[19:15];
assign rs2    = inst[24:20];
assign funct7 = inst[31:25];

//step8 实例化寄存器堆
wire [31:0] rs1_data;
wire [31:0] rs2_data;

wire reg_wen;
wire [31:0] wb_data;
// =====================

//实例化寄存器堆
RegFile u_regfile(
    .clk(clock),

    .wen(reg_wen),
    .waddr(rd),
    .wdata(wb_data),

    .raddr1(rs1),
    .raddr2(rs2),

    .rdata1(rs1_data),
    .rdata2(rs2_data)
);
//================================
assign reg_wen = 1'b0;
assign wb_data = 32'b0;
//================================

// =====================
// Step9 : ImmGen 立即数生成器

wire [31:0] imm_i;
wire [31:0] imm_s;
wire [31:0] imm_b;
wire [31:0] imm_u;
wire [31:0] imm_j;

assign imm_i = {
    {20{inst[31]}},
    inst[31:20]
};

assign imm_s = {
    {20{inst[31]}},
    inst[31:25],
    inst[11:7]
};

assign imm_b = {
    {19{inst[31]}},
    inst[31],
    inst[7],
    inst[30:25],
    inst[11:8],
    1'b0
};

assign imm_u = {
    inst[31:12],
    12'b0
};

assign imm_j = {
    {11{inst[31]}},
    inst[31],
    inst[19:12],
    inst[20],
    inst[30:21],
    1'b0
};
//============================

// =====================
// Step10 : Decode
// =====================

wire is_addi;
wire is_auipc;
wire is_jal;
wire is_jalr;
wire is_lw;
wire is_sw;
wire is_ebreak;

assign is_addi =
    (opcode == 7'b0010011) &&
    (funct3 == 3'b000);

assign is_auipc =
    (opcode == 7'b0010111);

assign is_jal =
    (opcode == 7'b1101111);

assign is_jalr =
    (opcode == 7'b1100111) &&
    (funct3 == 3'b000);

assign is_lw =
    (opcode == 7'b0000011) &&
    (funct3 == 3'b010);

assign is_sw =
    (opcode == 7'b0100011) &&
    (funct3 == 3'b010);

assign is_ebreak =
    (inst == 32'h00100073);
//====================================

ITraceDPIC itrace(
    .clk(clock),
    .pc(pc),
    .inst(inst),
    .next_pc(next_pc)
);

assign io_is_mmio = 1'b0;

endmodule