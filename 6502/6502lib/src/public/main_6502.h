#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <cassert>

// http://www.obelisk.me.uk/6502/

using Byte = unsigned char;
using Word = unsigned short;

using u32 = unsigned int;

struct Mem
{
	static constexpr u32 MAX_MEM = 1024 * 64;
	Byte Data[MAX_MEM];

	void Initialise()
	{
		for ( u32 i = 0; i < MAX_MEM; i++ )
		{
			Data[i] = 0;
		}
	}

	/** read 1 byte */
	Byte operator[]( u32 Address ) const
	{
		assert( Address < MAX_MEM );
		return Data[Address];
	}

	/** write 1 byte */
	Byte& operator[]( u32 Address )
	{
		assert( Address < MAX_MEM );
		return Data[Address];
	}

	/** write 2 bytes */
	void WriteWord( 
		Word Value, 
		u32 Address, 
		u32& Cycles )
	{
		assert( Address < MAX_MEM - 1 );
		Data[Address]		= Value & 0xFF;
		Data[Address + 1]   = (Value >> 8);
		Cycles -= 2;
	}

	/** read 2 bytes */
	Word ReadWord( 
		u32 Address, 
		u32& Cycles ) const
	{
		assert( Address < MAX_MEM - 1 );
		Word Value = Data[Address] | (Data[Address + 1] << 8);
		Cycles -= 2;
		return Value;
	}
};

struct CPU
{
	Word PC;		//program counter
	Word SP;		//stack pointer

	Byte A, X, Y;	//registers

	Byte C : 1;	//status flag
	Byte Z : 1;	//status flag
	Byte I : 1;//status flag
	Byte D : 1;//status flag
	Byte B : 1;//status flag
	Byte V : 1;//status flag
	Byte N : 1;//status flag

	void Reset( Mem& memory )
	{
		PC = 0xFFFC;
		SP = 0xFF;  // Stack pointer starts at 0xFF (stack is at 0x0100-0x01FF)
		C = Z = I = D = B = V = N = 0;
		A = X = Y = 0;
		memory.Initialise();
	}

	Byte FetchByte( u32& Cycles, Mem& memory )
	{
		Byte Data = memory[PC];
		PC++;
		Cycles--;
		return Data;
	}

	Word FetchWord( u32& Cycles, Mem& memory )
	{
		// 6502 is little endian
		Word Data = memory[PC];
		PC++;
		
		Data |= (memory[PC] << 8 );
		PC++;

		Cycles -= 2;

		// if you wanted to handle endianness
		// you would have to swap bytes here
		// if ( PLATFORM_BIG_ENDIAN )
		//	SwapBytesInWord(Data)

		return Data;
	}

	Byte ReadByte( 
		u32& Cycles, 
		Byte Address, 
		Mem& memory )
	{
		Byte Data = memory[Address];
		Cycles--;
		return Data;
	}

	// Stack operations - 6502 stack is at 0x0100-0x01FF
	void PushByteToStack( 
		u32& Cycles, 
		Byte Value, 
		Mem& memory )
	{
		memory[0x0100 | SP] = Value;
		SP--;
		Cycles--;
	}

	void PushWordToStack( 
		u32& Cycles, 
		Word Value, 
		Mem& memory )
	{
		// Push high byte first
		memory[0x0100 | SP] = (Value >> 8) & 0xFF;
		SP--;
		// Then push low byte
		memory[0x0100 | SP] = Value & 0xFF;
		SP--;
		Cycles -= 2;
	}

	Byte PopByteFromStack( 
		u32& Cycles, 
		Mem& memory )
	{
		SP++;
		Byte Value = memory[0x0100 | SP];
		Cycles--;
		return Value;
	}

	Word PopWordFromStack( 
		u32& Cycles, 
		Mem& memory )
	{
		// Pop low byte first (reverse of push order)
		SP++;
		Word Value = memory[0x0100 | SP];
		SP++;
		Value |= (memory[0x0100 | SP] << 8);
		Cycles -= 2;
		return Value;
	}

	// opcodes
	static constexpr Byte
		INS_LDA_IM = 0xA9,
		INS_LDA_ZP = 0xA5,
		INS_LDA_ZPX = 0xB5,
		INS_JSR = 0x20,
		INS_RTS = 0x60,
		INS_BRK = 0x00;

	void LDASetStatus()
	{
		Z = (A == 0);
		N = (A & 0b10000000) > 0;
	}

	void Execute( u32 Cycles, Mem& memory )
	{
		while ( Cycles > 0 )
		{
			Byte Ins = FetchByte( Cycles, memory );			
			switch ( Ins )
			{
			case INS_LDA_IM:
			{
				Byte Value = 
					FetchByte( Cycles, memory );
				A = Value;
				LDASetStatus();
			} break;
			case INS_LDA_ZP:
			{
				Byte ZeroPageAddr =
					FetchByte( Cycles, memory );
				A = ReadByte( 
					Cycles, ZeroPageAddr, memory );
				LDASetStatus();
			} break;
			case INS_LDA_ZPX:
			{
				Byte ZeroPageAddr =
					FetchByte( Cycles, memory );
				ZeroPageAddr += X;
				Cycles--;
				A = ReadByte(
					Cycles, ZeroPageAddr, memory );
				LDASetStatus();
			} break;
			case INS_JSR:
			{
				Word SubAddr = 
					FetchWord( Cycles, memory );
				// Push return address - 1 to stack (JSR pushes PC-1)
				PushWordToStack( Cycles, PC - 1, memory );
				PC = SubAddr;
				Cycles--;
			} break;
			case INS_RTS:
			{
				// Pop return address from stack and add 1
				Word ReturnAddr = PopWordFromStack( Cycles, memory );
				PC = ReturnAddr + 1;
				Cycles--;  // RTS takes 6 cycles total, but PopWordFromStack already takes 2
			} break;
			case INS_BRK:
			{
				// BRK instruction - stop execution
				printf("BRK instruction executed at PC=0x%04X\n", PC-1);
				return;
			} break;
			default:
			{
				printf("Instruction not handled 0x%02X at PC=0x%04X\n", Ins, PC-1 );
				return;  // Stop execution on unhandled instruction
			} break;
			}
		}
	}
};