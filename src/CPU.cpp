#include "../include/cpusim/CPU.h"

#include <iostream>

const int NUM_REGS = 32;
const int MEM_SIZE = 16 * 1024 * 1024;  // 1024 bytes = 1 kb -> 1024 KB = 1 MB -> 16 * 1 MB = 16 MB

const int IO = 0xFFFF0000;

const uint8_t OPCODE_MASK = 0x7f;
const uint8_t FUNCT7_SHIFT = 0x19;
const uint8_t FUNCT7_MASK = 0x7f;
const uint8_t FUNCT3_SHIFT = 0xC;
const uint8_t FUNCT3_MASK = 0x7;

const uint8_t RTYPE_OPCODE = 0x33;
const uint8_t ITYPE_OPCODE1 = 0x13;
const uint8_t ITYPE_OPCODE2 = 0x3;
const uint8_t ITYPE_OPCODE3 = 0x67;
const uint8_t STYPE_OPCODE = 0x23;
const uint8_t UTYPE_OPCODE1 = 0x37;
const uint8_t UTYPE_OPCODE2 = 0x17;
const uint8_t BTYPE_OPCODE = 0x63;
const uint8_t JTYPE_OPCODE = 0x6F;

const uint8_t REG_MASK = 0x1F;

const uint8_t CONST_SHIFT_1 = 0x14;
const uint8_t CONST_SHIFT_2 = 0xC;
const uint8_t RD_SHIFT = 7;
const uint8_t RS1_SHIFT = 15;
const uint8_t RS2_SHIFT = 20;

const uint8_t BYTE_MASK = 0xFF;
const int HALF_MASK = 0xFFFF;

const uint8_t O1_SHIFT = 25;
const uint8_t O2_SHIFT = 7;
const uint8_t O2_MASK = 0x1f;
const uint8_t DIFF_32_12 = 20;
const uint8_t DIFF_32_13 = 19;
const uint8_t DIFF_32_21 = 11;
const uint8_t O3_SHIFT = 4;

const int B_O1_MASK = 0x80000000;
const int B_O2_MASK = 0x00000080;
const int B_O3_MASK = 0x7E000000;
const int B_O4_MASK = 0x00000F00;

const int J_O1_MASK = 0x80000000;
const int J_O2_MASK = 0x000FF000;
const int J_O3_MASK = 0x00100000;
const int J_O4_MASK = 0x7FE00000;
const uint8_t J_O1_SHIFT = 11;
const uint8_t J_O3_SHIFT = 9;
const uint8_t J_O4_SHIFT = 20;

using ExecFunc = void (CPU::*)(const Instr&);

std::unordered_map<uint32_t, ExecFunc> instruction_map = {
    // R-type
    {0x33000000, &CPU::execADD},
    {0x33000020, &CPU::execSUB},
    {0x33001000, &CPU::execSLL},
    {0x33002000, &CPU::execSLT},
    {0x33005000, &CPU::execSRL},
    {0x33005020, &CPU::execSRA},
    {0x33003000, &CPU::execSLTU},
    {0x33004000, &CPU::execXOR},
    {0x33006000, &CPU::execOR},
    {0x33007000, &CPU::execAND},

    // I-type
    {0x13000000, &CPU::execADDI},
    {0x13001000, &CPU::execSLLI},
    {0x13002000, &CPU::execSLTI},
    {0x13003000, &CPU::execSLTIU},
    {0x13004000, &CPU::execXORI},
    {0x13005000, &CPU::execSRLAI},
    {0x13006000, &CPU::execORI},
    {0x13007000, &CPU::execANDI},
    {0x03004000, &CPU::execLBU},
    {0x03005000, &CPU::execLHU},

    // I-Type
    {0x03000000, &CPU::execLB},
    {0x03001000, &CPU::execLH},
    {0x03002000, &CPU::execLW},

    // S-Type
    {0x23000000, &CPU::execSB},
    {0x23001000, &CPU::execSH},
    {0x23002000, &CPU::execSW},

    // B-Type
    {0x63000000, &CPU::execBEQ},
    {0x63001000, &CPU::execBNE},
    {0x63004000, &CPU::execBLT},
    {0x63005000, &CPU::execBGE},
    {0x63006000, &CPU::execBLTU},
    {0x63007000, &CPU::execBGEU},

    // U-type
    {0x37000000, &CPU::execLUI},
    {0x17000000, &CPU::execAUIPC},

    // J-type
    {0x6F000000, &CPU::execJAL},
    {0x67000000, &CPU::execJALR},

    // SYS
    {0x73000000, &CPU::execE},
};

CPU::CPU()
    : pc(0),
      regs(std::make_unique<uint32_t[]>(NUM_REGS)),
      mem(std::make_unique<unsigned char[]>(MEM_SIZE)) {
  mem.get()[0x7] = 0x00;
  mem.get()[0x6] = 0x00;
  mem.get()[0x5] = 0x00;
  mem.get()[0x4] = 0x73;

  mem.get()[0x3] = 0x00;
  mem.get()[0x2] = 0x53;
  mem.get()[0x1] = 0x20;
  mem.get()[0x0] = 0x23;

  // regs.get()[1] = 0x41;
  // regs.get()[17] = 93;
  regs[5] = 0x65626142;
  regs[6] = 0xFFFF0000;
}

uint32_t makeInstrKey(const uint32_t& instr) {
  uint8_t opcode = instr & OPCODE_MASK;
  uint8_t funct3 = (instr >> FUNCT3_SHIFT) & FUNCT3_MASK;
  uint8_t funct7 = (instr >> FUNCT7_SHIFT) & FUNCT7_MASK;

  if (opcode == RTYPE_OPCODE) {
    return (opcode << 24) | (funct3 << 12) | (funct7 << 0);
  } else if (opcode == JTYPE_OPCODE) {
    return (opcode << 24);
  } else {
    return (opcode << 24) | (funct3 << 12);
  }
}

Instr extractFields(const uint32_t& instr) {
  Instr i;
  i.rd = (instr >> RD_SHIFT) & REG_MASK;
  i.rs1 = (instr >> RS1_SHIFT) & REG_MASK;
  i.rs2 = (instr >> RS2_SHIFT) & REG_MASK;
  i.funct3 = (instr >> FUNCT3_SHIFT) & FUNCT3_MASK;
  i.funct7 = (instr >> FUNCT7_SHIFT) & FUNCT7_MASK;

  uint8_t opcode = instr & OPCODE_MASK;

  if (opcode == ITYPE_OPCODE1 || opcode == ITYPE_OPCODE2 || opcode == ITYPE_OPCODE3) {
    i.imm = static_cast<int32_t>(instr) >> CONST_SHIFT_1;
  } else if (opcode == UTYPE_OPCODE1 || opcode == UTYPE_OPCODE2) {
    i.imm = static_cast<int32_t>(instr) >> CONST_SHIFT_2;
  } else if (opcode == STYPE_OPCODE) {
    i.imm = ((instr >> O1_SHIFT) << 5) | ((instr >> O2_SHIFT) & O2_MASK);
    i.imm = static_cast<int32_t>(i.imm << DIFF_32_12) >> DIFF_32_12;
  } else if (opcode == BTYPE_OPCODE) {
    i.imm = ((instr & B_O1_MASK) >> DIFF_32_13) | ((instr & B_O2_MASK) << O3_SHIFT)
            | ((instr & B_O3_MASK) >> DIFF_32_12) | ((instr & B_O4_MASK) >> O2_SHIFT);
    i.imm = static_cast<int32_t>(i.imm << DIFF_32_13) >> DIFF_32_13;
  } else if (opcode == JTYPE_OPCODE) {
    i.imm = ((instr & J_O1_MASK) >> J_O1_SHIFT) | (instr & J_O2_MASK)
            | ((instr & J_O3_MASK) >> J_O3_SHIFT) | ((instr & J_O4_MASK) >> J_O4_SHIFT);
    i.imm = static_cast<int32_t>(i.imm << DIFF_32_21) >> DIFF_32_21;
  }

  return i;
}

void CPU::decode(const uint32_t& instr) {
  Instr i = extractFields(instr);
  uint32_t key = makeInstrKey(instr);

  auto it = instruction_map.find(key);
  if (it != instruction_map.end()) {
    std::invoke(it->second, *this, i);
  } else {
    std::cerr << "Unknown instruction: 0x" << std::hex << instr << std::endl;
  }
}

void CPU::execSLL(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] << (regs[instr.rs2] & REG_MASK);
  pc += 4;
}

void CPU::execSLT(const Instr& instr) {
  regs[instr.rd]
      = (static_cast<int32_t>(regs[instr.rs1]) < static_cast<int32_t>(regs[instr.rs2])) ? 1 : 0;
  pc += 4;
}

void CPU::execSLTU(const Instr& instr) {
  regs[instr.rd] = (regs[instr.rs1] < regs[instr.rs2]) ? 1 : 0;
  pc += 4;
}

void CPU::execXOR(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] ^ regs[instr.rs2];
  pc += 4;
}

void CPU::execOR(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] | regs[instr.rs2];
  pc += 4;
}

void CPU::execAND(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] & regs[instr.rs2];
  pc += 4;
}

void CPU::execSRA(const Instr& instr) {
  regs[instr.rd] = static_cast<uint32_t>(static_cast<int32_t>(regs[instr.rs1])
                                         >> static_cast<int32_t>(regs[instr.rs2] & REG_MASK));
  pc += 4;
}

void CPU::execSRL(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] >> (regs[instr.rs2] & REG_MASK);
  pc += 4;
}

void CPU::execADD(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] + regs[instr.rs2];
  pc += 4;
}

void CPU::execSUB(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] - regs[instr.rs2];
  pc += 4;
}

void CPU::execADDI(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] + instr.imm;
  pc += 4;
}

void CPU::execXORI(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] ^ instr.imm;
  pc += 4;
}

void CPU::execORI(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] | instr.imm;
  pc += 4;
}

void CPU::execANDI(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] & instr.imm;
  pc += 4;
}

void CPU::execSLLI(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] << instr.imm;
  pc += 4;
}

void CPU::execSLTI(const Instr& instr) {
  regs[instr.rd]
      = (static_cast<int32_t>(regs[instr.rs1]) < static_cast<int32_t>(instr.imm)) ? 1 : 0;
  pc += 4;
}

void CPU::execSLTIU(const Instr& instr) {
  regs[instr.rd]
      = (static_cast<uint32_t>(regs[instr.rs1]) < static_cast<uint32_t>(instr.imm)) ? 1 : 0;
  pc += 4;
}

void CPU::execSRLAI(const Instr& instr) {
  (instr.funct7) ? execSRAI(instr) : execSRLI(instr);
  pc += 4;
}

void CPU::execSRLI(const Instr& instr) {
  regs[instr.rd] = regs[instr.rs1] >> (instr.imm & REG_MASK);
  pc += 4;
}

void CPU::execSRAI(const Instr& instr) {
  regs[instr.rd] = static_cast<int32_t>(regs[instr.rs1]) >> (instr.imm & REG_MASK);
  pc += 4;
}

void CPU::execLB(const Instr& instr) {
  uint32_t addr = regs[instr.rs1] + instr.imm;
  regs[instr.rd]
      = (int32_t)((int8_t)(*reinterpret_cast<const uint32_t*>(mem.get() + addr) & BYTE_MASK));
  pc += 4;
}

void CPU::execLH(const Instr& instr) {
  uint32_t addr = regs[instr.rs1] + instr.imm;
  regs[instr.rd]
      = (int32_t)((int16_t)(*reinterpret_cast<const uint32_t*>(mem.get() + addr) & HALF_MASK));
  pc += 4;
}

void CPU::execLW(const Instr& instr) {
  uint32_t addr = regs[instr.rs1] + instr.imm;
  regs[instr.rd] = *reinterpret_cast<const uint32_t*>(mem.get() + addr);
  pc += 4;
}

void CPU::execLBU(const Instr& instr) {
  uint32_t addr = regs[instr.rs1] + instr.imm;
  regs[instr.rd] = *reinterpret_cast<const uint32_t*>(mem.get() + addr) & BYTE_MASK;
  pc += 4;
}

void CPU::execLHU(const Instr& instr) {
  uint32_t addr = regs[instr.rs1] + instr.imm;
  regs[instr.rd] = *reinterpret_cast<const uint32_t*>(mem.get() + addr) & HALF_MASK;
  pc += 4;
}

void CPU::execSB(const Instr& instr) {
  uint32_t addr = regs[instr.rs1] + instr.imm;
  if (addr == IO) {
    std::cout << static_cast<char>(regs[instr.rs2] & BYTE_MASK) << std::endl;
  } else {
    *reinterpret_cast<uint32_t*>(mem.get() + addr) = regs[instr.rs2] & BYTE_MASK;
  }
  pc += 4;
}

void CPU::execSH(const Instr& instr) {
  uint32_t addr = regs[instr.rs1] + instr.imm;
  if (addr == IO) {
    auto bytes = std::bit_cast<std::array<char, 4>>(regs[instr.rs2]);
    std::cout << bytes[0] << bytes[1] << std::endl;
  } else {
    *reinterpret_cast<uint32_t*>(mem.get() + addr) = regs[instr.rs2] & HALF_MASK;
  }
  pc += 4;
}

void CPU::execSW(const Instr& instr) {
  uint32_t addr = regs[instr.rs1] + instr.imm;
  if (addr == IO) {
    auto bytes = std::bit_cast<std::array<char, 4>>(regs[instr.rs2]);
    std::cout << bytes[0] << bytes[1] << bytes[2] << bytes[3] << std::endl;
  } else {
    *reinterpret_cast<uint32_t*>(mem.get() + addr) = regs[instr.rs2];
  }
  pc += 4;
}

void CPU::execBEQ(const Instr& instr) {
  if (regs[instr.rs1] == regs[instr.rs2]) {
    pc += instr.imm;
  } else {
    pc += 4;
  }
}

void CPU::execBNE(const Instr& instr) {
  if (regs[instr.rs1] != regs[instr.rs2]) {
    pc += instr.imm;
  } else {
    pc += 4;
  }
}

void CPU::execBLT(const Instr& instr) {
  if (static_cast<int32_t>(regs[instr.rs1]) < static_cast<int32_t>(regs[instr.rs2])) {
    pc += instr.imm;
  } else {
    pc += 4;
  }
}
void CPU::execBGE(const Instr& instr) {
  if (static_cast<int32_t>(regs[instr.rs1]) >= static_cast<int32_t>(regs[instr.rs2])) {
    pc += instr.imm;
  } else {
    pc += 4;
  }
}
void CPU::execBLTU(const Instr& instr) {
  if (regs[instr.rs1] < regs[instr.rs2]) {
    pc += instr.imm;
  } else {
    pc += 4;
  }
}
void CPU::execBGEU(const Instr& instr) {
  if (regs[instr.rs1] >= regs[instr.rs2]) {
    pc += instr.imm;
  } else {
    pc += 4;
  }
}

void CPU::execLUI(const Instr& instr) {
  regs[instr.rd] = instr.imm << 12;
  pc += 4;
}
void CPU::execAUIPC(const Instr& instr) {
  regs[instr.rd] = pc + (instr.imm << 12);
  pc += 4;
}

void CPU::execJAL(const Instr& instr) {
  regs[instr.rd] = pc + 4;
  pc += instr.imm;
}

void CPU::execJALR(const Instr& instr) {
  uint32_t t = pc + 4;
  pc = (regs[instr.rs1] + instr.imm) & ~1;
  regs[instr.rd] = t;
}

void CPU::execE(const Instr& instr) { (instr.rs2) ? execEBREAK(instr) : execECALL(instr); }

void CPU::execECALL(const Instr& instr) {
  uint32_t syscall_num = regs[17];
  switch (syscall_num) {
    case 93:
      int exit_code = regs[10];
      std::cout << "ECALL: Program exited with code " << exit_code << std::endl;
      exit(0);
  }
}

void CPU::execEBREAK(const Instr& instr) {
  std::cout << "EBREAK encountered at PC: " << pc << std::endl;
  exit(0);
}