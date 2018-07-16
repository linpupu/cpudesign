#include <cstdint>
#include <iostream>

using namespace std;

// Instructions identified by opcode
#define AUIPC 0x17
#define LUI 0x37
#define JAL 0x6F
#define JALR 0x67


// Branches using BRANCH as the label for common opcode
#define BRANCH 0x63

#define BEQ 0x0
#define BNE 0x1
#define BLT 0x4
#define BGE 0x5
#define BLTU 0x6
#define BGEU 0x7


// Loads using LOAD as the label for common opcode
#define LOAD 0x03

#define LB 0x0
#define LH 0x1
#define LW 0x2
#define LBU 0x4
#define LHU 0x5


// Stores using STORE as the label for common opcode
#define STORE 0x23

#define SB 0x0
#define SH 0x1
#define SW 0x2


// ALU ops with one immediate
#define ALUR1 0x13

#define ADDI 0x0
#define SLTI 0x2
#define SLTIU 0x3
#define XORI 0x4
#define ORI 0x6
#define ANDI 0x7
#define SLLI 0x1

#define SHR 0x5  // common funct3 for SRLI and SRAI

#define SRLI 0x0
#define SRAI 0x20


// ALU ops with all register operands
#define ALUR2 0x33

#define ADDSUB 0x0  // common funct3 for ADD and SUB
#define ADD 0x0
#define SUB 0x20

#define SLL 0x1
#define SLT 0x2
#define SLTU 0x3
#define XOR 0x4
#define OR 0x6
#define AND 0x7

#define SRLA 0x5  // common funct3 for SRL and SRA

#define SRL 0x0
#define SRA 0x20

// Fences using FENCES as the label for common opcode

#define FENCES 0x0F
#define FENCE 0x0
#define FENCE_I 0x1

// CSR related instructions
#define CSRX 0x73

#define CALLBREAK 0x0  // common funct3 for ECALL and EBREAK
#define ECALL 0x0
#define EBREAK 0x1

#define CSRRW 0x1
#define CSRRS 0x2
#define CSRRC 0x3
#define CSRRWI 0x5
#define CSRRSI 0x6
#define CSRRCI 0x7


// Data for memory
const int WORDSIZE = sizeof(uint32_t);
unsigned int MSize = 4096;
char* M;

// Functions for memory
int allocMem(uint32_t s) {
	M = new char[s];
	MSize = s;

	return s;
}

void freeMem() {
	delete[] M;
	MSize = 0;
}

char readByte(unsigned int address) {
	if(address >= MSize) {
		cout << "ERROR: Address out of range in readByte" << endl;
		return 0;
	}

	return M[address];
}

void writeByte(unsigned int address, char data) {
	if(address >= MSize) {
		cout << "ERROR: Address out of range in writeByte" << endl;
		return;
	}

	M[address] = data;
}

uint32_t readWord(unsigned int address) {
	if(address >= MSize-WORDSIZE) {
		cout << "ERROR: Address out of range in readWord" << endl;
		return 0;
	}

	return *((uint32_t*)&(M[address]));
}

uint32_t readHalfWord(unsigned int address){
	if(address >= MSize-WORDSIZE/2) {
		cout << "ERROR: Address out of range in readWord" << endl;
		return 0;
	}

	return *((uint16_t*)&(M[address]));
}

void writeWord(unsigned int address, uint32_t data) {
	if(address >= MSize-WORDSIZE) {
		cout << "ERROR: Address out of range in writeWord" << endl;
		return;
	}

	*((uint32_t*)&(M[address])) = data;
}

void writeHalfWord(unsigned int address, uint32_t data) {
	if(address >= MSize-WORDSIZE/2) {
		cout << "ERROR: Address out of range in writeWord" << endl;
		return;
	}

	*((uint16_t*)&(M[address])) = data;
}

// Write memory with instructions to test
void progMem() {
	// Write starts with PC at 0
	writeWord(0, (1 << 12) | (5 << 7) | (AUIPC));
}

// ============================================================================


// data for CPU
uint32_t PC, NextPC;
uint32_t R[32];
uint32_t IR;

unsigned int opcode;
unsigned int rs1, rs2, rd;
unsigned int funct7, funct3;
unsigned int shamt;
unsigned int pred, succ;
unsigned int csr, zimm;

// immediate values for I-type, S-type, B-type, U-type, J-type
unsigned int imm11_0i;
unsigned int imm11_5s, imm4_0s;
unsigned int imm12b, imm10_5b, imm4_1b, imm11b;
unsigned int imm31_12u;
unsigned int imm20j, imm10_1j, imm11j, imm19_12j;

unsigned int imm_temp;
unsigned int src1,src2;


unsigned int Imm11_0ItypeZeroExtended;
int Imm11_0ItypeSignExtended;
int Imm11_0StypeSignExtended;
unsigned int Imm12_1BtypeZeroExtended;
int Imm12_1BtypeSignExtended;
unsigned int Imm31_12Utype;
int Imm20_1JtypeSignExtended;
int Imm20_1JtypeZeroExtended;

// Functions for CPU
void decode(uint32_t instruction) {
	// Extract all bit fields from instruction
	opcode = instruction & 0x7F;
	rd = (instruction & 0x0F80) >> 7;
	rs1 = (instruction & 0xF8000) >> 15;
	zimm = rs1;
	rs2 = (instruction & 0x1F00000) >> 20;
	shamt = rs2;
	funct3 = (instruction & 0x7000) >> 12;
	funct7 = instruction >> 25;
	imm11_0i = ((int32_t)instruction) >> 20;
	csr = instruction >> 20;
	imm11_5s = ((int32_t)instruction) >> 25;
	imm4_0s = (instruction >> 7) & 0x01F;
	imm12b = ((int32_t)instruction) >> 31;
	imm10_5b = (instruction >> 25) & 0x3F;
	imm4_1b = (instruction & 0x0F00) >> 8;
	imm11b = (instruction & 0x080) >> 7;
	imm31_12u = instruction >> 12;
	imm20j = ((int32_t)instruction) >> 31;
	imm10_1j = (instruction >> 21) & 0x3FF;
	imm11j = (instruction >> 20) & 1;
	imm19_12j = (instruction >> 12) & 0x0FF;
	pred = (instruction >> 24) & 0x0F;
	succ = (instruction >> 20) & 0x0F;

	// ========================================================================
	// Get values of rs1 and rs2
	src1 = R[rs1];
	src2 = R[rs2];

	// Immediate values
	Imm11_0ItypeZeroExtended = imm11_0i & 0x0FFF;
	Imm11_0ItypeSignExtended = imm11_0i;

	Imm11_0StypeSignExtended = (imm11_5s << 5) | imm4_0s;

	Imm12_1BtypeZeroExtended = imm12b & 0x00001000 | (imm11b << 11) | (imm10_5b << 5) | (imm4_1b << 1);
	Imm12_1BtypeSignExtended = imm12b & 0xFFFFF000 | (imm11b << 11) | (imm10_5b << 5) | (imm4_1b << 1);

	Imm31_12Utype = instruction & 0xFFFFF000;

	Imm20_1JtypeSignExtended = (imm20j & 0xFFF00000) | (imm19_12j << 12) | (imm11j << 11) | (imm10_1j << 1);
	Imm20_1JtypeZeroExtended = (imm20j & 0x00100000) | (imm19_12j << 12) | (imm11j << 11) | (imm10_1j << 1);
	// ========================================================================
}

void showRegs() {
	cout << "PC=" << PC << " " << "IR=" << IR << endl;

	for(int i=0; i<32; i++) {
		cout << "R[" << i << "]=" << R[i] << " ";
	}
	cout << endl;
}

int main(int argc, char const *argv[]) {
	/* code */
	allocMem(4096);
	progMem();

	PC = 0;

	char c = 'Y';

	while(c != 'n') {
		cout << "Registers bofore executing the instruction at" << PC << endl;
		showRegs();

		IR = readWord(PC);
		NextPC = PC + WORDSIZE;

		decode(IR);

		switch(opcode) {
			case LUI:
				cout << "Do LUI" << endl;
				R[rd] = Imm31_12Utype;
				break;
			case AUIPC:
				cout << "Do AUIPC" << endl;
				R[rd] = PC + (imm31_12u << 12);
				break;
			case JAL:
				cout << "Do JAL" << endl;
				R[rd]=PC+4;
				NextPC = PC+ Imm20_1JtypeSignExtended;    
				break;
			case JALR:
				cout << "DO JALR" << endl;
				R[rd]=PC+4;
				NextPC=R[rs1]+Imm20_1JtypeSignExtended;
				break;
			case BRANCH:
				switch(funct3) {
					case BEQ:
						cout << "DO BEQ" << endl;
						if(src1==src2){
							NextPC = PC + Imm11_0ItypeSignExtended;
						}
						break;
					case BNE:
						cout << "Do BNE " << endl;
						if(src1!=src2){
							NextPC = PC + Imm11_0ItypeSignExtended;
						}
						break;
					case BLT:
						//TODO: Fill code for the instruction here
						break;
					case BGE:
						cout << "Do BGE" << endl;
						if((int)src1 >= (int)src2)
							NextPC = PC + Imm12_1BtypeSignExtended;
						break;
					case BLTU:
						cout << "Do BLTU" << endl;
						if(src1<src2){
							NextPC=PC+Imm12_1BtypeSignExtended;
						}
						break;
					case BGEU:
						cout<<"Do BGEU"<<endl;

						if(src1>=src2){
							NextPC=PC+Imm12_1BtypeSignExtended;
						}    
						break;
					default:
						cout << "ERROR: Unknown funct3 in BRANCH instruction " << IR << endl;
				}
				break;
			case LOAD:
				switch(funct3) {
					case LB:
						cout << "DO LB" << endl;
						unsigned int temp_LH,temp_LH_UP;
						temp_LH=readHalfWord(src1+Imm11_0ItypeSignExtended);
						temp_LH_UP=temp_LH>>15;
						if(temp_UP==1){
							temp_LH=0xff000000 | temp_LH);
						}else{
							temp_LH=0| temp_LH);
						}
						R[rd]=temp_LH; 
						break;
					case LH:
						cout << "Do LH " << endl;
						unsigned int temp_LH,temp_LH_UP;
						temp_LH=readHalfWord(src1+Imm11_0ItypeSignExtended);
						temp_LH_UP=temp_LH>>15;
						if(temp_LH_UP==1){
							temp_LH=0xffff0000 | temp_LH;
						}else{
							temp_LH=0| temp_LH;
						}
						R[rd]=temp_LH; 
						break;
					case LW:
						//TODO: Fill code for the instruction here
						break;
					case LBU:
						cout << "Do LBU" << endl;
						R[rd] = readByte(Imm11_0ItypeSignExtended + src1) & 0x000000ff;
						break;
					case LHU:
						//TODO: Fill code for the instruction here
						break;
					default:
						cout << "ERROR: Unknown funct3 in LOAD instruction " << IR << endl;
				}
				break;
			case STORE:
				switch(funct3) {
					case SB:
						cout << "Do SB" << endl;
						char d1;
						d1=R[rs2] & 0xff;
						unsigned int a1;
						a1 = R[rs1] +Imm11_0ItypeSignExtended;
						writeByte(a1, d1);
						break;
					case SH:
						cout<<"Do SH"<<endl;
						uint16_t j;
						j=R[rs2]&0xffff;
						unsigned int x;


						x = R[rs1] + Imm11_0ItypeSignExtended;

						writeByte(x,j);
						break;
					case SW:
						cout << "DO SW" << endl;
						//unsigned int imm_temp;
						char d1;
						d1=R[rs2] & 0xffffff;
						unsigned int a1;
						imm_temp= Imm11_0ItypeZeroExtended;
						if(imm11_5s & 0x800){
							imm_temp=Imm11_0ItypeSignExtended;
						}
						a1 = R[rs1] + imm_temp;
						writeByte(a1, d1);
						break;
					default:
						cout << "ERROR: Unknown funct3 in STORE instruction " << IR << endl;
				}
				break;
			case ALUR1:
				switch(funct3) {
					case ADDI:
						cout <<    "Do ADDI" << endl;
						R[rd]=src1+Imm11_0ItypeSignExtended;
						break;
					case SLTI:
						//TODO: Fill code for the instruction here
						break;
					case SLTIU:
						cout << "Do SLTIU" << endl;
						if(src1<(unsigned int)Imm11_0ItypeSignExtended)
							R[rd] = 1;
						else
							R[rd] = 0;
						break;
					case XORI:
						cout << "Do XORI" << endl;
						R[rd]=(Imm11_0ItypeSignExtended)^R[rs1];
						break;
					case ORI:
						cout<<"Do ORI"<<endl;
						R[rd]=R[rs1]|Imm11_0ItypeSignExtended;
						break;
					case ANDI:
						cout << "DO ANDI"<<endl;
						unsigned int re3,imm3;
						imm3=imm11_0i>>11;
						if(imm3==1){
							re3=(0xfffff000|imm11_0i);
						}else{
							re3=(0|imm11_0i);    
						}
						R[rd]=R[rs1]&re3;
						break;
					case SLLI:
						cout << "Do SLLI " << endl;
						R[rd]=src1<<shamt;
						break;
					case SHR:
						switch(funct7) {
							case SRLI:
								//TODO: Fill code for the instruction here
								break;
							case SRAI:
								cout << "Do SRAI" << endl;
								R[rd] = (src1 & 0x80000000) + (src1 >> 1);
								for(int i=1;i<shamt;i++){
									R[rd] = (R[rd] & 0x80000000) | (R[rd] >> 1);
								}
								break;
							default:
								cout << "ERROR: Unknown (imm11_0i >> 5) in ALUR1 SHR instruction " << IR << endl;
						}
						break;
					default:
						cout << "ERROR: Unknown funct3 in ALUR1 instruction " << IR << endl;
				}
				break;
			case ALUR2:
				switch(funct3) {
					case ADDSUB:
						switch(funct7) {
							case ADD:
								cout << "Do ADD" << endl;
								R[rd]=R[rs1]+R[rs2];
								break;
							case SUB:
								cout<<" Do SUB"<<endl;
								R[rd]=R[rs1]-R[rs2];
								break;
							default:
								cout << "ERROR: Unknown funct7 in ALUR2 ADDSUB instruction " << IR << endl;
						}
						break;
					case SLL:
						cout<<"DO SLL"<<endl;
						unsigned int rsTransform;
						rsTransform=R[rs1]&0x1f;
						R[rs2]<<rsTransform;
						break;
					case SLT:
						cout << "Do SLT " << endl;
						if(src1<src2){
							R[rd]=1;
						}else{
							R[rd]=0;
						}
						break;
					case SLTU:
						//TODO: Fill code for the instruction here
						break;
					case XOR:
						//TODO: Fill code for the instruction here
						break;
					case OR:
						//TODO: Fill code for the instruction here
						break;
					case AND:
						//TODO: Fill code for the instruction here
						break;

					case SRLA:
						switch(funct7) {
							case SRL:
								//TODO: Fill code for the instruction here
								break;
							case SRA:
								//TODO: Fill code for the instruction here
								break;
							default:
								cout << "ERROR: Unknown funct7 in ALUR2 SRLA instruction " << IR << endl;
						}
						break;
					default:
						cout << "ERROR: Unknown funct3 in ALUR2 instruction " << IR << endl;
				}
				break;
			case FENCES:
				switch(funct3) {
					case FENCE:
						//TODO: Fill code for the instruction here
						break;
					case FENCE_I:
						//TODO: Fill code for the instruction here
						break;
					default:
						cout << "ERROR: Unknown funct3 in FENCES instruction " << IR << endl;
				}
				break;
			case CSRX:
				switch(funct3) {
					case CALLBREAK:
						switch(Imm11_0ItypeZeroExtended) {
							case ECALL:
								//TODO: Fill code for the instruction here
								break;
							case EBREAK:
								//TODO: Fill code for the instruction here
								break;
							default:
								cout << "ERROR: Unknown imm11_0i in CSRX CALLBREAK instruction " << IR << endl;
						}
						break;
					case CSRRW:
						//TODO: Fill code for the instruction here
						break;
					case CSRRS:
						//TODO: Fill code for the instruction here
						break;
					case CSRRC:
						//TODO: Fill code for the instruction here
						break;
					case CSRRWI:
						//TODO: Fill code for the instruction here
						break;
					case CSRRSI:
						//TODO: Fill code for the instruction here
						break;
					case CSRRCI:
						//TODO: Fill code for the instruction here
						break;
					default:
						cout << "ERROR: Unknown funct3 in CSRX instruction " << IR << endl;
				}
				break;
			default:
				cout << "ERROR: Unkown instruction " << IR << endl;
				break;
		}

		// Update PC
		PC = NextPC;

		cout << "Registers after executing the instruction" << endl;
		showRegs();

		cout << "Continue simulation (Y/n)? [Y]" << endl;
		cin.get(c);
	}

	freeMem();

	return 0;
}


