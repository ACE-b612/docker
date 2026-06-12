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

// assign next_pc = pc + 4;

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

wire [31:0] rs1_data;
wire [31:0] rs2_data;
wire [31:0] a0_val;
wire reg_wen;
wire [31:0] wb_data;


//实例化寄存器堆
RegFile u_regfile(
    .clk(clock),

    .wen(reg_wen),
    .waddr(rd),
    .wdata(wb_data),

    .raddr1(rs1),
    .raddr2(rs2),

    .rdata1(rs1_data),
    .rdata2(rs2_data),

    .a0_val(a0_val)
);
//================================
// assign reg_wen = 1'b0;
// assign wb_data = 32'b0;
//================================

// =====================
// ImmGen 立即数生成器

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

// Step10 : Decode
// ===========================
wire is_addi;
wire is_auipc;
wire is_jal;
wire is_jalr;
wire is_lw;
wire is_sw;
wire is_ebreak;
//补全功能部分 
wire is_add;
wire is_sub;
wire is_and;
wire is_or;
wire is_xor;
wire is_sll;
wire is_srl;
wire is_sra;
wire is_slt;
wire is_sltu;
//译码 addi
assign is_addi =
    (opcode == 7'b0010011) &&
    (funct3 == 3'b000);
//译码 auipc
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
//==========================================
//补全部分译码
// R-Type
assign is_add =
    (opcode == 7'b0110011) &&
    (funct3 == 3'b000) &&
    (funct7 == 7'b0000000);

assign is_sub =
    (opcode == 7'b0110011) &&
    (funct3 == 3'b000) &&
    (funct7 == 7'b0100000);

assign is_and =
    (opcode == 7'b0110011) &&
    (funct3 == 3'b111) &&
    (funct7 == 7'b0000000);

assign is_or =
    (opcode == 7'b0110011) &&
    (funct3 == 3'b110) &&
    (funct7 == 7'b0000000);

assign is_xor =
    (opcode == 7'b0110011) &&
    (funct3 == 3'b100) &&
    (funct7 == 7'b0000000);

assign is_sll =
    (opcode == 7'b0110011) &&
    (funct3 == 3'b001) &&
    (funct7 == 7'b0000000);

assign is_srl =
    (opcode == 7'b0110011) &&
    (funct3 == 3'b101) &&
    (funct7 == 7'b0000000);

assign is_sra =
    (opcode == 7'b0110011) &&
    (funct3 == 3'b101) &&
    (funct7 == 7'b0100000);

assign is_slt =
    (opcode == 7'b0110011) &&
    (funct3 == 3'b010) &&
    (funct7 == 7'b0000000);

assign is_sltu =
    (opcode == 7'b0110011) &&
    (funct3 == 3'b011) &&
    (funct7 == 7'b0000000);
//====================================
//定义 ALU 控制信号
wire [3:0] alu_op;

localparam ALU_ADD  = 4'd0;
localparam ALU_SUB  = 4'd1;
localparam ALU_AND  = 4'd2;
localparam ALU_OR   = 4'd3;
localparam ALU_XOR  = 4'd4;
localparam ALU_SLL  = 4'd5;
localparam ALU_SRL  = 4'd6;
localparam ALU_SRA  = 4'd7;
localparam ALU_SLT  = 4'd8;
localparam ALU_SLTU = 4'd9;
//声明  选择 ALU 输入
wire [31:0] alu_src1;
wire [31:0] alu_src2;

wire [31:0] alu_result;
//alu赋值
assign alu_src1 =
    is_auipc ? pc :
    rs1_data;

assign alu_src2 =
    (is_addi || is_lw || is_jalr)
        ? imm_i :
    is_sw
        ? imm_s :
        rs2_data;
//实现alu_op
assign alu_op =
    (is_add || is_addi || is_auipc ||
     is_lw  || is_sw  || is_jalr)
        ? ALU_ADD :

    is_sub  ? ALU_SUB :

    is_and  ? ALU_AND :

    is_or   ? ALU_OR :

    is_xor  ? ALU_XOR :

    is_sll  ? ALU_SLL :

    is_srl  ? ALU_SRL :

    is_sra  ? ALU_SRA :

    is_slt  ? ALU_SLT :

    is_sltu ? ALU_SLTU :

    ALU_ADD;
//实例化ALU
ALU u_alu(
    .src1(alu_src1),
    .src2(alu_src2),
    .alu_op(alu_op),
    .result(alu_result)
);
//=====================================
wire [31:0] jal_target;
wire [31:0] jalr_target;
wire [31:0] branch_target;

assign jal_target  = pc + imm_j;

assign jalr_target =
    (rs1_data + imm_i)
    & ~32'b1;

assign branch_target =
    pc + imm_b;

assign next_pc =
    is_jal  ? jal_target  :
    is_jalr ? jalr_target :
              (pc + 32'd4);
//==========================================

// MEM Stage
// =====================
wire mem_read;
wire mem_write;

wire [31:0] mem_rdata;

wire [7:0] wmask;
//产生控制信号
assign mem_read  = is_lw;
assign mem_write = is_sw;
//============================
assign wmask =
    mem_write ? 8'h0f : 8'h00;
//实例化数据存储器
MemDPIC dmem(
    .clk(clock),

    .en(mem_read || mem_write),

    .addr(alu_result),

    .wmask(wmask),

    .wdata(rs2_data),

    .rdata(mem_rdata)
);
//=================================

// WB Stage

assign reg_wen =
    is_add  ||
    is_sub  ||
    is_and  ||
    is_or   ||
    is_xor  ||
    is_sll  ||
    is_srl  ||
    is_sra  ||
    is_slt  ||
    is_sltu ||
    is_addi ||
    is_auipc||
    is_jal  ||
    is_jalr ||
    is_lw;

assign wb_data =
    is_lw
        ? mem_rdata
        :
    is_auipc
        ? auipc_result
        :
    (is_jal || is_jalr)
        ? (pc + 32'd4)
        :
          alu_result;

// Regfile DPI
// =====================
RegfileDPIC reg_dpi(
    .clk(clock),
    .wen(reg_wen),
    .waddr(rd),
    .wdata(wb_data)
);
//======================

wire [31:0] auipc_result;

assign auipc_result = pc + imm_u;

//实例化EbreakDPIC
EbreakDPIC ebreak_dpi(
    .clk(clock),
    .ebreak_en(is_ebreak),
    .a0_val(a0_val)
);
//==========================
ITraceDPIC itrace(
    .clk(clock),
    .pc(pc),
    .inst(inst),
    .next_pc(next_pc)
);
//MMIO判断
assign io_is_mmio =
    mem_write &&
    (
        (alu_result < 32'h80000000)
        ||
        (alu_result >= 32'h88000000)
    );


endmodule