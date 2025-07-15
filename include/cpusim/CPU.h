#pragma once
#include <cstdint>
#include <memory>

struct Instr {
  uint8_t rs1;
  uint8_t rs2;
  uint8_t rd;
  uint8_t funct7;
  uint8_t funct3;
  int32_t imm;
};

class CPU {
public:
  uint32_t pc;
  std::unique_ptr<uint32_t[]> regs;
  std::unique_ptr<unsigned char[]> mem;  // 16 MB: 0x000000 to 0x1000000

  explicit CPU();
  inline uint32_t fetch(const uint32_t& pc) {
    return *reinterpret_cast<const uint32_t*>(mem.get() + pc);
  };

  void decode(const uint32_t& instr);

  void execSLL(const Instr& instr);
  void execSLT(const Instr& instr);
  void execSLTU(const Instr& instr);
  void execXOR(const Instr& instr);
  void execSRA(const Instr& instr);
  void execOR(const Instr& instr);
  void execAND(const Instr& instr);
  void execADD(const Instr& instr);
  void execSRL(const Instr& instr);
  void execSUB(const Instr& instr);

  void execADDI(const Instr& instr);
  void execSLLI(const Instr& instr);
  void execSLTI(const Instr& instr);
  void execXORI(const Instr& instr);
  void execORI(const Instr& instr);

  void execSRLAI(const Instr& instr);
  void execSRLI(const Instr& instr);
  void execSRAI(const Instr& instr);

  void execSLTIU(const Instr& instr);
  void execANDI(const Instr& instr);

  void execLB(const Instr& instr);
  void execLH(const Instr& instr);
  void execLW(const Instr& instr);
  void execLBU(const Instr& instr);
  void execLHU(const Instr& instr);

  void execSB(const Instr& instr);
  void execSH(const Instr& instr);
  void execSW(const Instr& instr);

  void execBEQ(const Instr& instr);
  void execBNE(const Instr& instr);
  void execBLT(const Instr& instr);
  void execBGE(const Instr& instr);
  void execBLTU(const Instr& instr);
  void execBGEU(const Instr& instr);

  void execLUI(const Instr& instr);
  void execAUIPC(const Instr& instr);

  void execJAL(const Instr& instr);
  void execJALR(const Instr& instr);

  void execE(const Instr& instr);
  void execECALL(const Instr& instr);
  void execEBREAK(const Instr& instr);
};