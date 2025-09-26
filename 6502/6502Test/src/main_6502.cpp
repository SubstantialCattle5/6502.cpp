#include "main_6502.h"
#include <gtest/gtest.h>

class M6502Test : public testing::Test
{
public:
	Mem mem;
	CPU cpu;

	virtual void SetUp()
	{
		cpu.Reset( mem );
	}

	virtual void TearDown()
	{
	}
};

TEST_F(M6502Test, CPUDoesNothingWhenWeExecuteZeroCycles)
{
	// given:
	constexpr u32 NUM_CYCLES = 0;

	// when:
	cpu.Execute( NUM_CYCLES, mem );

	// then:
	EXPECT_EQ( cpu.PC, 0xFFFC );
	EXPECT_EQ( cpu.SP, 0xFF );
	EXPECT_EQ( cpu.A, 0 );
	EXPECT_EQ( cpu.X, 0 );
	EXPECT_EQ( cpu.Y, 0 );
}

TEST_F(M6502Test, LDAImmediateCanLoadValueIntoARegister)
{
	// given:
	mem[0xFFFC] = CPU::INS_LDA_IM;
	mem[0xFFFD] = 0x84;

	// when:
	cpu.Execute( 2, mem );

	// then:
	EXPECT_EQ( cpu.A, 0x84 );
	EXPECT_EQ( cpu.Z, 0 );
	EXPECT_EQ( cpu.N, 1 );  // 0x84 has bit 7 set
}

TEST_F(M6502Test, LDAImmediateCanLoadZeroIntoARegister)
{
	// given:
	mem[0xFFFC] = CPU::INS_LDA_IM;
	mem[0xFFFD] = 0x00;

	// when:
	cpu.Execute( 2, mem );

	// then:
	EXPECT_EQ( cpu.A, 0x00 );
	EXPECT_EQ( cpu.Z, 1 );
	EXPECT_EQ( cpu.N, 0 );
}

TEST_F(M6502Test, LDAZeroPageCanLoadValueIntoARegister)
{
	// given:
	mem[0xFFFC] = CPU::INS_LDA_ZP;
	mem[0xFFFD] = 0x42;
	mem[0x0042] = 0x37;

	// when:
	cpu.Execute( 3, mem );

	// then:
	EXPECT_EQ( cpu.A, 0x37 );
	EXPECT_EQ( cpu.Z, 0 );
	EXPECT_EQ( cpu.N, 0 );
}

TEST_F(M6502Test, LDAZeroPageXCanLoadValueIntoARegister)
{
	// given:
	cpu.X = 5;
	mem[0xFFFC] = CPU::INS_LDA_ZPX;
	mem[0xFFFD] = 0x42;
	mem[0x0047] = 0x37;  // 0x42 + 5 = 0x47

	// when:
	cpu.Execute( 4, mem );

	// then:
	EXPECT_EQ( cpu.A, 0x37 );
	EXPECT_EQ( cpu.Z, 0 );
	EXPECT_EQ( cpu.N, 0 );
}

TEST_F(M6502Test, JSRCanJumpToSubroutineAndPushReturnAddressToStack)
{
	// given:
	mem[0xFFFC] = CPU::INS_JSR;
	mem[0xFFFD] = 0x42;
	mem[0xFFFE] = 0x42;
	mem[0x4242] = CPU::INS_LDA_IM;
	mem[0x4243] = 0x84;

	// when:
	cpu.Execute( 8, mem );

	// then:
	EXPECT_EQ( cpu.PC, 0x4244 );
	EXPECT_EQ( cpu.A, 0x84 );
	EXPECT_EQ( cpu.SP, 0xFD );  // Stack pointer should have moved down 2 bytes

	// Check that return address was pushed to stack
	// JSR should push (PC-1) where PC would be 0xFFFF after reading the address
	EXPECT_EQ( mem[0x01FF], 0xFF );  // High byte of return address - 1 
	EXPECT_EQ( mem[0x01FE], 0xFE );  // Low byte of return address - 1
}

TEST_F(M6502Test, RTSCanReturnFromSubroutine)
{
	// given:
	mem[0xFFFC] = CPU::INS_JSR;
	mem[0xFFFD] = 0x42;
	mem[0xFFFE] = 0x42;
	mem[0xFFFF] = CPU::INS_BRK;  // Add BRK after JSR to stop execution
	mem[0x4242] = CPU::INS_RTS;
	
	// Execute JSR first
	cpu.Execute( 6, mem );  // JSR(6)
	
	// Verify we're at the subroutine
	EXPECT_EQ( cpu.PC, 0x4242 );
	EXPECT_EQ( cpu.SP, 0xFD );
	
	// Execute RTS (just 1 cycle to execute the RTS instruction)
	cpu.Execute( 1, mem );  // Execute just the RTS instruction
	
	// Verify we've returned but haven't executed the BRK yet
	Word expectedPC = 0xFFFF;  // This is where RTS should return us to
	
	// then:
	EXPECT_EQ( cpu.PC, expectedPC );  // Should return to PC after JSR
	EXPECT_EQ( cpu.SP, 0xFF );        // Stack pointer should be back to original
}

TEST_F(M6502Test, StackOperationsWorkCorrectly)
{
	// Test stack push/pop operations directly
	u32 cycles = 10;
	
	// Push a byte
	cpu.PushByteToStack( cycles, 0x42, mem );
	EXPECT_EQ( cpu.SP, 0xFE );
	EXPECT_EQ( mem[0x01FF], 0x42 );

	// Push a word
	cpu.PushWordToStack( cycles, 0x1234, mem );
	EXPECT_EQ( cpu.SP, 0xFC );
	EXPECT_EQ( mem[0x01FE], 0x12 );  // High byte pushed first (at higher address)
	EXPECT_EQ( mem[0x01FD], 0x34 );  // Low byte pushed second (at lower address)

	// Pop word
	Word popped_word = cpu.PopWordFromStack( cycles, mem );
	EXPECT_EQ( popped_word, 0x1234 );
	EXPECT_EQ( cpu.SP, 0xFE );

	// Pop byte
	Byte popped_byte = cpu.PopByteFromStack( cycles, mem );
	EXPECT_EQ( popped_byte, 0x42 );
	EXPECT_EQ( cpu.SP, 0xFF );
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}