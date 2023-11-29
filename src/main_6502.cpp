#include <stdio.h>
#include <stdlib.h>

// https://web.archive.org/web/20210909190432/http://www.obelisk.me.uk/6502/
using Byte = unsigned char;
using Word = unsigned short;
using u32 = unsigned int;

struct Mem
{
    static constexpr u32 MAX_MEM = 1024 * 64;
    Byte Data[MAX_MEM];

    void Initialize()
    {
        for (u32 i = 0; i < MAX_MEM; i++)
        {
            Data[i] = 0;
        }
    }

    // read 1 byte
    Byte operator[](u32 Address) const
    {
        // assert here Address is < MAX_MEM
        return Data[Address];
    }
    Byte &operator[](u32 Address)
    {
        // assert here Address is < MAX_MEM
        return Data[Address];
    }
    /**
     * Write two bytes to memory
     */
    void WriteWord(Word Value, u32 &Cycles, u32 Address)
    {
        Data[Address] = Value & 0xFF;
        Data[Address + 1] = (Value >> 8);
        Cycles -= 2;
    };
};

struct CPU
{

    Word PC; // program counter

    Word SP; // stack pointer

    Byte A, X, Y; // registers

    Byte C : 1; // status flags
    Byte Z : 1; // status flags
    Byte I : 1; // status flags
    Byte D : 1; // status flags
    Byte B : 1; // status flags
    Byte V : 1; // status flags
    Byte N : 1; // status flags

    void Reset(Mem &memory)
    {
        PC = 0xFFFC; // INITIALIZATION VECTOR
        SP = 0x0100;
        A = X = Y = 0;                 // registers
        D = C = Z = I = B = V = N = 0; // status flags
        memory.Initialize();
    }

    Byte FetchByte(u32 &Cycles, Mem &memory)
    {
        Byte Data = memory[PC];
        PC++;
        Cycles--;
        return Data;
    }

    Word FetchWord(u32 &Cycles, Mem &memory)
    {
        //! TODO : swapp the order if not little endian
        // 6502 is little endian
        Word Data = memory[PC];
        PC++;
        Data |= (memory[PC] << 8);
        PC++;
        Cycles -= 2;
        return Data;
    }

    Byte ReadByte(u32 &Cycles, Byte Address, Mem &memory)
    {
        Byte Data = memory[Address];
        Cycles--;
        return Data;
    };

    //* OPCODES
    static constexpr Byte INS_LDA_IM = 0xA9,
                          INS_LDA_ZP = 0xA5,
                          INS_LDA_ZPX = 0xB5,
                          INS_JSR = 0x20;

    void LDASetStatus()
    {
        Z = (A == 0);
        N = (A & 0b10000000) > 0; // IF A is 0b1xxxxxx then N = 1 else N = 0 negative flag
    };

    void Execute(u32 Cycles, Mem &memory)
    {
        while (Cycles > 0)
        {
            Byte Ins = FetchByte(Cycles, memory);
            switch (Ins)
            {
            case INS_LDA_IM:
            {
                Byte Value = FetchByte(Cycles, memory); // fetch value
                A = Value;
                LDASetStatus();
            }
            break;
            case INS_LDA_ZP:
            {
                Byte ZeroPageAddress = FetchByte(Cycles, memory);
                A = ReadByte(Cycles, ZeroPageAddress, memory);
                LDASetStatus();
            }
            break;
            case INS_LDA_ZPX:
            {
                Byte ZeroPageAddress = FetchByte(Cycles, memory);
                ZeroPageAddress += X;
                Cycles--;
                A = ReadByte(Cycles, ZeroPageAddress, memory);
                LDASetStatus();
            }
            break;
            case INS_JSR:
            {
                Word SubAddress = FetchWord(Cycles, memory);
                memory[SP] = PC - 1;
                Cycles--;
                SP++;
                PC = SubAddress;
                Cycles--;
            }
            break;
            default:
            {
                printf("Instruction not handled %d", Ins);
            }
            }
        }
    };
};

int main()
{
    Mem mem;
    CPU cpu;
    cpu.Reset(mem);
    cpu.Execute(2, mem);

    return 0;
}