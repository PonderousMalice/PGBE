#pragma once
#include "MMU.h"
#include "Timer.h"
#include <variant>

using std::placeholders::_1;

namespace emulator
{
    using reg_t = std::variant<uint8_t*, uint16_t*>;
    using reg_v = std::variant<uint8_t, uint16_t>;

    enum reg_name
    {
        None,
        A,
        AF,
        B,
        BC,
        C,
        D,
        DE,
        E,
        H,
        L,
        HL,
        HL_inc,
        HL_dec,
        SP,
        SP_d,
        n,
        nn,
    };

    struct reg_s
    {
        reg_name name;
        bool indirect;
    };

    struct OP
    {
        enum
        {
            INVALID,
            ALU,
            ADD_16,
            BIT,
            CALL,
            CALL_CC,
            CCF,
            CPL,
            DAA,
            DEC,
            DEC_16,
            DI,
            EI,
            HALT,
            INC,
            INC_16,
            JP,
            JP_CC,
            JP_HL,
            JR,
            JR_CC,
            LD,
            NOP,
            POP,
            PUSH,
            RES,
            RET,
            RET_CC,
            RETI,
            RLA,
            RLCA,
            ROT,
            RRA,
            RRCA,
            RST,
            SCF,
            SET,
            STOP,
        } name;
        std::array<reg_s, 2> args;

        uint8_t y;
    };

    class SM83
    {
    public:
        SM83(MMU* mmu, Timer* t);

        void run();
        std::string dump();
        std::string print_dis(OP op);
    private:
        MMU* m_mmu;
        Timer* m_timer;
        struct registers
        {
            registers()
            {
                AF = BC = DE = HL = SP = PC = 0;
            }

            union
            {
                struct
                {
                    union
                    {
                        struct flag
                        {
                            uint8_t : 4; // padding
                            uint8_t c : 1;
                            uint8_t h : 1;
                            uint8_t n : 1;
                            uint8_t z : 1;
                        } flags;
                        uint8_t F;
                    };
                    uint8_t A;
                };
                uint16_t AF;
            };

            union
            {
                struct
                {
                    uint8_t C;
                    uint8_t B;
                };
                uint16_t BC;
            };

            union
            {
                struct
                {
                    uint8_t E;
                    uint8_t D;
                };
                uint16_t DE;
            };

            union
            {
                struct
                {
                    uint8_t L;
                    uint8_t H;
                };
                uint16_t HL;
            };

            uint16_t SP;
            uint16_t PC;
        } m_registers;
        bool m_ime, m_halted, m_halt_bug;
        uint8_t& m_IF, & m_IE;

        OP m_prev_op;

        OP m_decode(uint8_t opcode);
        void m_execute(OP instr);
        void m_advance_cycle(int m_cycles = 1);

        // Memory access
        uint8_t m_read(uint16_t adr);
        uint8_t m_fetch();
        uint16_t m_fetch_word();
        void m_write(uint16_t adr, uint8_t v);

        reg_v m_get_reg(reg_s r);
        void m_set_reg(reg_s r, reg_v v);

        // Interrupt handler
        void m_isr();

        // ALU
        void m_add(uint8_t v);
        void m_add_16(reg_s lv, reg_s rv);
        void m_adc(uint8_t v);
        void m_sub(uint8_t v);
        void m_sbc(uint8_t v);
        void m_and(uint8_t v);
        void m_xor(uint8_t v);
        void m_or(uint8_t v);
        void m_cp(uint8_t v);

        void m_dec(reg_t v);
        void m_inc(reg_t v);

        void m_cpl();
        void m_scf();
        void m_ccf();
        void m_daa();

        void m_ld(reg_s lv, reg_s rv);

        void m_pop(uint16_t& r);
        void m_push(uint16_t r);

        // Rotate and shi(f)t instructions
        void m_rlc(uint8_t& v);
        void m_rrc(uint8_t& v);
        void m_rl(uint8_t& v);
        void m_rr(uint8_t& v);
        void m_sla(uint8_t& v);
        void m_sra(uint8_t& v);
        void m_swap(uint8_t& v);
        void m_srl(uint8_t& v);
        void m_rla();
        void m_rlca();
        void m_rra();
        void m_rrca();

        // Conditions
        bool m_nz();
        bool m_z();
        bool m_nc();
        bool m_c();

        // Jump
        void m_call(uint16_t adr, std::function<bool(void)> cc = nullptr);
        void m_jp(std::function<bool(void)> cc = nullptr);
        void m_jr(std::function<bool(void)> cc = nullptr);
        void m_ret(std::function<bool(void)> cc = nullptr);
        void m_reti();

        // Bit op
        void m_bit(uint8_t v, uint8_t i);
        void m_res(uint8_t& v, uint8_t i);
        void m_set(uint8_t& v, uint8_t i);

        // Special
        void m_stop();
        void m_halt();

        const std::array<reg_s, 8> m_r
        {
            reg_s{ B, false },
            reg_s{ C, false },
            reg_s{ D, false },
            reg_s{ E, false },
            reg_s{ H, false },
            reg_s{ L, false },
            reg_s{ HL, true },
            reg_s{ A, false },
        };

        const std::array<reg_s, 4> m_rp
        {
            reg_s{ BC, false },
            reg_s{ DE, false },
            reg_s{ HL, false },
            reg_s{ SP, false },
        };

        const std::array<reg_s, 4> m_rp2
        {
            reg_s{ BC, false },
            reg_s{ DE, false },
            reg_s{ HL, false },
            reg_s{ AF, false },
        };

        const std::array<std::function<void(uint8_t)>, 8> m_alu
        {
            std::bind(&SM83::m_add, this, _1),
            std::bind(&SM83::m_adc, this, _1),
            std::bind(&SM83::m_sub, this, _1),
            std::bind(&SM83::m_sbc, this, _1),
            std::bind(&SM83::m_and, this, _1),
            std::bind(&SM83::m_xor, this, _1),
            std::bind(&SM83::m_or, this, _1),
            std::bind(&SM83::m_cp, this, _1),
        };

        const std::array<std::function<void(uint8_t&)>, 8> m_rot
        {
            std::bind(&SM83::m_rlc, this, _1),
            std::bind(&SM83::m_rrc, this, _1),
            std::bind(&SM83::m_rl, this, _1),
            std::bind(&SM83::m_rr, this, _1),
            std::bind(&SM83::m_sla, this, _1),
            std::bind(&SM83::m_sra, this, _1),
            std::bind(&SM83::m_swap, this, _1),
            std::bind(&SM83::m_srl, this, _1),
        };

        const std::array<std::function<bool(void)>, 4> m_cc
        {
            std::bind(&SM83::m_nz, this),
            std::bind(&SM83::m_z, this),
            std::bind(&SM83::m_nc, this),
            std::bind(&SM83::m_c, this),
        };
    };
}