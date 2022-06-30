+#include <iostream>
#include <fstream>
#include <vector>
#include <climits>
#include <iomanip>

using namespace std;

typedef unsigned char BYTE;
typedef unsigned int usint;

#define ADD 0
#define SUB 1
#define MUL 2
#define INC 3
#define AND 4
#define OR 5
#define NOT 6
#define XOR 7
#define LOAD 8
#define STORE 9
#define JMP 10
#define BEQZ 11
#define HALT 15 

#define NUMBER_OF_BLOCKS 64
#define BLOCK_SIZE 4
#define CACHE_SIZE 256

int total_instructions = 0;
int arith_intsr = 0;
int logic_intsr = 0;
int data_intsr = 0;
int control_intsr = 0;
int halt_intsr = 0;
int total_cycles = 0;
int stat_stalls = 0;
int data_stalls = 0;
int control_stalls = 0;

class Register_file
{
	int no_of_registers = 16;
	vector<BYTE> RF;
	vector<bool>status;

  public:
	Register_file(ifstream * fp);
	BYTE getValue(BYTE index);
	void setValue(BYTE index, BYTE data);
	void setStatus(BYTE index, bool currStatus);
	bool getStatus(BYTE index);
};

Register_file::Register_file(ifstream * fp)
{
	RF = vector<BYTE> (no_of_registers , 0);
	status = vector<bool> (no_of_registers,false);

	string str;
	int register_index = 0;
	while(*fp >> str)
	{
		RF[register_index] = stoi(str,0,16);
		register_index++;
	}
    fp->close();
};

BYTE Register_file::getValue(BYTE reg_index)  
{   
	return RF[reg_index];     
} 


void Register_file::setValue(BYTE reg_index , BYTE value)
{
    if(reg_index != 0)
	{ 
		RF[reg_index] = value;
	}	
}

bool Register_file::getStatus(BYTE reg_index)
{
	return status[reg_index];
}

void Register_file::setStatus(BYTE reg_index, bool reg_status)
{
	status[reg_index] = reg_status;
}


class ICache
{
	vector <uint> vec;

	public:
	ICache(ifstream * fp);
	BYTE readInstruction(BYTE address);
	void dumpCache( string filename);
};

ICache::ICache( ifstream * fp) 
{
	vec =  vector<uint> (CACHE_SIZE , 0);

	string str;
	uint value;
	int blockNum = 0;
	int offset = 0;
	while(*fp >> str){
		if(offset == 4)
		{
			offset = 0;
			blockNum++;
		}

		value =  stoi(str,0,16);
		for(int i = 0; i < offset; i++)
		{
			value = value << 8;
		}
		vec[blockNum] += value;
		offset++;
	}

    fp->close();
}

BYTE ICache::readInstruction(BYTE address)
{
	uint data = vec[address >> 2];
	uint offset = address & 3;
	for(int i = offset; i < 3; i++)
	{
		data = data << 8;
	}
	for(int i = 0; i < 3; i++)
	{
		data = data >> 8;
	}
	return (BYTE)data;
}

class DCache
{
	vector <uint> vec;
	
	public:
	DCache(ifstream * fp);
	BYTE getData(BYTE address);
	void setData(BYTE address, BYTE data);
	void writeCache( string filename);
};

DCache::DCache( ifstream * fp) 
{
	vec =  vector<uint> (CACHE_SIZE , 0);

	string str;
	uint value;
	int blockNum = 0;
	int offset = 0;
	while(*fp >> str){
		if(offset == 4)
		{
			offset = 0;
			blockNum++;
		}

		value =  stoi(str,0,16);
		for(int i = 0; i < offset; i++)
		{
			value = value << 8;
		}
		vec[blockNum] += value;
		offset++;
	}

    fp->close();
}

BYTE DCache::getData(BYTE address)
{
	uint data = vec[address >> 2];
	uint offset = address & 3;
	for(int i = offset; i < 3; i++)
	{
		data = data << 8;
	}
	for(int i = 0; i < 3; i++)
	{
		data = data >> 8;
	}
	return (BYTE)data;
}

void DCache::setData(BYTE address, BYTE value)
{
	uint temp  = (uint)value;
	BYTE block_index = address >> 2;
	BYTE offset = address & 3;
	uint mask = CACHE_SIZE - 1;
	for(int i = 0; i < (int)offset; i++)
	{
		mask = mask << 8;
		temp = temp << 8;
	}
	mask = UINT_MAX - mask;
	vec[block_index] = (vec[block_index] & mask) + temp;
  }

void DCache::writeCache( string filename)  
{
	ofstream outfile;
	outfile.open(filename,  ofstream::trunc);
	
	uint i;
	outfile << setfill('0') << setw(2) <<  hex;

	for(i = 0; i < CACHE_SIZE; i++)  {
		outfile << setw(2) << ((int) getData(i)) <<  endl;
	}
	outfile.close();
}

class Processor  
{
	ICache * I$;
    DCache * D$;
	Register_file * R$;

	bool haltScheduled = false;
	bool halted = false;

	bool IF_stall = false;
	bool ID_stall = false;
	bool EX_stall = false;
	bool MM_stall = false;
	bool WB_stall = false;


	BYTE IF_PC = 0u; 
	bool IF_run = true;

	usint ID_IR = 0u;
	BYTE  ID_PC = 0u;
	bool  ID_run = false;
	
	usint EX_IR = 0u;
	BYTE  EX_PC = 0u;
	BYTE  EX_A  = 0u;
	BYTE  EX_B  = 0u;
	bool  EX_run = false;
	
	usint MM_IR = 0u;
	BYTE  ALU_output = 0u;
    bool  MM_COND = false;
	bool  MM_run = false;
	
	usint WB_IR   = 0u;
	BYTE  WB_AO   = 0u;
	BYTE  WB_LMD  = 0u;
	bool  WB_COND = false;
	bool  WB_run = false;

	void IF();
	void ID();
	void  EX();
	void  MEM();
	void  WB();

	void flushPipeline();

	public:
	Processor( ifstream * Icache,  ifstream * Dcache,  ifstream * RegFile);
	~Processor();
	void run();
	void cycle();
	bool isHalted();
	void writeToFile( string fnameCache);

	
};

Processor::Processor( ifstream * Icache,  ifstream * Dcache,  ifstream * regFile)  
{
	I$ = new ICache(Icache);
	D$ = new DCache(Dcache);
	this->R$ = new Register_file(regFile);
}

Processor::~Processor()  
{
	delete I$;
	delete D$;
	delete R$;
}

void Processor::run()  
{
	while(!isHalted())  
	{
		cycle();
	}
	total_instructions = arith_intsr + control_intsr +  data_intsr + logic_intsr +  halt_intsr;
}

void Processor::cycle()  
{
	total_cycles++;

	WB();
	MEM();
	EX();
	ID();
	IF();
}

bool Processor::isHalted()  
{
	return halted;
}

void Processor::EX()
{
	if(!EX_run || EX_stall)  {
		return;
	}

	int opcode = EX_IR >> 12;
	switch(opcode)
	{
		case ADD :	ALU_output = EX_A + EX_B; 
						arith_intsr++;	
						break;

		case SUB :	ALU_output = EX_A - EX_B;
						arith_intsr++; 
						break;

		case MUL :	ALU_output = EX_A * EX_B; 
						arith_intsr++; 
						break;

		case INC :	ALU_output = EX_A + 1; 		 
						arith_intsr++; 
						break;

		case AND :	ALU_output = EX_A & EX_B; 
						logic_intsr++; 	
						break;

		case OR :	ALU_output = EX_A | EX_B; 
						logic_intsr++; 	
						break;

		case NOT :	ALU_output = ~EX_A;
						logic_intsr++; 	
						break;

		case XOR :	ALU_output = EX_A ^ EX_B;
						logic_intsr++; 	
						break;

		case LOAD :	ALU_output = EX_A + EX_B;
						data_intsr++;	
						break;
						
		case STORE :ALU_output = EX_A + EX_B;
						data_intsr++;	
						break;

		case JMP:
			EX_stall = true;
			control_intsr++;
			ALU_output = EX_PC + (BYTE) ((usint) EX_A << 1);
			break;

		case BEQZ:
			EX_stall = true;
			control_intsr++;
			if((int)EX_A == 0)
			{ 
				ALU_output =  EX_PC +((usint) ( EX_B << 1));
			}
			else
			{
				ALU_output =  EX_PC;
			}
			break;

		default:	break;
	}
	MM_IR = EX_IR;
	MM_run = true;
	EX_run = false;	
}

void Processor::IF()  
{
	
	if(!IF_run)  { 
		return;
	}
	if(ID_run || ID_stall)  { 
		IF_run = true;
		return;
	}

	ID_IR = (((usint) I$->readInstruction(IF_PC)) << 8) + ((usint) I$->readInstruction(IF_PC+1));
	ID_run = true;

	IF_PC += 2;
}

void Processor::ID()  
{
	if((!ID_run) || ID_stall)  {
		return;
	}

	if(EX_run || EX_stall)  {
		ID_run = true;
		return;
	}

	EX_IR = ID_IR;
	BYTE opcode = (ID_IR & 0xf000) >> 12;
	
	if(opcode == HALT)  {
		IF_run = false;
		ID_run = false;
		EX_run = true;
		halt_intsr++;
		
		return;
	}
	ID_run = false;
	EX_run = true;

	
	if(opcode == JMP)  {
		ID_stall = true;
		
		EX_A = (ID_IR & 0x0ff0) >> 4;
		EX_PC = IF_PC;
		control_stalls += 2;
		return;
	}
	if(opcode == BEQZ)  {
		ID_stall = true;

		if(R$->getStatus((ID_IR & 0x0f00) >> 8))
		{
			data_stalls++;
			EX_run = false;
			ID_run = true;
            ID_stall = false; 
			return;
		}
		control_stalls += 2; 
		EX_PC = IF_PC;
		EX_A = R$->getValue((ID_IR & 0x0f00) >> 8);
		EX_B = ID_IR & 0x00ff;
		return;
	}
	
	usint address_1;
	usint address_2;
	if(opcode == STORE)  {
		address_1 = (ID_IR & 0x0f00) >> 8;
		address_2 = (ID_IR & 0x00f0) >> 4;
		if(R$->getStatus(address_1) || R$->getStatus(address_2))
		{
			data_stalls++;
			EX_run = false;
			ID_run = true;
			return;
		}
		EX_A = R$->getValue((ID_IR & 0x00f0) >> 4);
		EX_B = ID_IR & 0x000f;
		return;
	}

	if((opcode == LOAD))  {
		address_1 = (ID_IR & 0x00f0) >> 4;
		if(R$->getStatus(address_1))
		{
			data_stalls++;
			EX_run = false;
			ID_run = true;
			return;
		}
		EX_A = R$->getValue((ID_IR & 0x00f0) >> 4);
		EX_B = ID_IR & 0x000f;

        R$->setStatus((BYTE) ((ID_IR & 0x0f00) >> 8), true);
		return;
	}


	if((opcode >= ADD) && (opcode <= MUL))  {
		address_1 = (ID_IR & 0x00f0) >> 4;
		address_2 = ID_IR & 0x000f;
		if(R$->getStatus(address_1) || R$->getStatus(address_2))
		{
			data_stalls++;
			EX_run = false;
			ID_run = true;
			return;
		}

		EX_A = R$->getValue((ID_IR & 0x00f0) >> 4);
		EX_B = R$->getValue(ID_IR & 0x000f);
        R$->setStatus((BYTE) ((ID_IR & 0x0f00) >> 8), true);
		return;
	}
	if(opcode == INC)  {
		address_1 = (ID_IR & 0x0f00) >> 8;
		if(R$->getStatus(address_1))
		{
			data_stalls++;
			EX_run = false;
			ID_run = true;
			return;
		}

		EX_A = R$->getValue((ID_IR & 0x0f00) >> 8);
        R$->setStatus((BYTE) ((ID_IR & 0x0f00) >> 8), true);
		return;
	}


	if((opcode != NOT))  {
		address_1 = (ID_IR & 0x00f0) >> 4;
		address_2 = ID_IR & 0x000f;
		if(R$->getStatus(address_1) || R$->getStatus(address_2))
		{
			data_stalls++;
			EX_run = false;
			ID_run = true;
			return;
		}

		EX_A = R$->getValue((ID_IR & 0x00f0) >> 4);
		EX_B = R$->getValue(ID_IR & 0x000f);

        R$->setStatus((BYTE) ((ID_IR & 0x0f00) >> 8), true);
		return;
	}


	if(opcode == NOT)	{
		address_1 = (ID_IR & 0x00f0) >> 4;
        if(R$->getStatus(address_1))
        {
        	data_stalls++;
            EX_run = false;
            ID_run = true;
            return;
        }

		EX_A = R$->getValue((ID_IR & 0x00f0) >> 4);

        R$->setStatus((BYTE) ((ID_IR & 0x0f00) >> 8), true);
	}
}

void Processor::MEM()
{
	if(!MM_run || MM_stall)
	{
		return;
	}
	MM_run = false;
	usint opCode = MM_IR >> 12;
	BYTE offset = (BYTE) ((MM_IR & 0x0f00) >> 8);
	if(opCode == HALT)
	{
		WB_run = true;
		WB_IR = MM_IR;
		return;
	}
	else if((opCode == JMP) || (opCode == BEQZ))  {
		if(IF_PC == ALU_output)  {
			ID_run = false;
			ID_stall = false;
			EX_stall = false;
		}  else  {
			IF_PC = ALU_output;
			flushPipeline();
		}
		WB_run = false;
		return;
	}
	
	else if(opCode == STORE)
	{
		D$->setData(ALU_output,R$->getValue((MM_IR & 0x0f00) >> 8));
		MM_run = false;
        WB_AO = 0u;
        WB_IR = 0u;
        WB_LMD = 0u;
        WB_COND = false;
        WB_run = true;
		return;
	}
	else if(opCode == LOAD)
	{
		WB_LMD = D$->getData(ALU_output);
		WB_IR = MM_IR;
		WB_AO = ALU_output;
		WB_run = true;
	}  
	else  {
		MM_run = false;
        WB_AO = ALU_output;
        WB_IR = MM_IR;
        WB_LMD = 0u;
        WB_COND = MM_COND;
        WB_run = true;
		return;
	}

	MM_run = false;
	return;
}

void Processor::WB()
{
	if(!WB_run || WB_stall)
	{
		return;
	}
	usint opCode = WB_IR >> 12;
	BYTE offset = (BYTE) ((WB_IR & 0x0f00) >> 8);
	if(opCode == HALT){
		WB_run = false;
		halted = true;
		return;
	}
	if(opCode == LOAD) 
	{
		R$->setValue(offset, WB_LMD);
	}
	else 
	{
		R$->setValue(offset, WB_AO);
	}
	R$->setStatus(offset, false);
	WB_run = false;
}

void Processor::flushPipeline() 
{
	IF_run = true;
	ID_run = false;
	EX_run = false;
	MM_run = false;
	WB_run = false;

	ID_IR = 0u;
	ID_PC = 0u;
	EX_A = 0u;
	EX_B = 0u;
	EX_IR = 0u;
	EX_PC = 0u;
	ALU_output = 0u;
	MM_IR = 0u;
	WB_AO = 0u;
	WB_IR = 0u;
	WB_COND = 0u;
	WB_LMD = 0u;

	IF_stall = false;
	ID_stall = false;
	EX_stall = false;
	MM_stall = false;
	WB_stall = false;
}

void Processor::writeToFile( string fnameCache)  
{
	D$->writeCache(fnameCache);
}


int main()
{
	ifstream Icache, Dcache, RegFile;

	Icache.open("ICache.txt");
	Dcache.open("DCache.txt");
	RegFile.open("RF.txt");	

	Processor processor(&Icache, &Dcache, &RegFile);
	processor.run();
	processor.writeToFile("DCache_output.txt");

	ofstream outFile;
	outFile.open("stats_output.txt");

	outFile << "Total number of instructions executed: " << total_instructions << endl;
	outFile << "Number of instructions in each class" <<  endl;
	outFile << "Arithmetic instructions              : " << arith_intsr <<  endl;
	outFile << "Logical instructions                 : " << logic_intsr <<  endl;
	outFile << "Data instructions                    : " << data_intsr <<  endl;
	outFile << "Control instructions                 : " << control_intsr <<  endl;
	outFile << "Halt instructions                    : " << halt_intsr <<  endl;
	outFile << "Cycles Per Instruction               : " << ((double) total_cycles / total_instructions) <<  endl;
	outFile << "Total number of stalls               : " << data_stalls+control_stalls <<  endl;
	outFile << "Data stalls (RAW)                    : " << data_stalls <<  endl;
	outFile << "Control stalls                       : " << control_stalls <<  endl;

	return 0;
}