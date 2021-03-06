
// CPU-simulator.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


//this is vector of vector thar contain the cmd line that been parsed.
std::vector<std::vector<std::string>> commands_vector;
//this var contain the number of line beeing parsed during the read of the cmd file
int line_counter;
//this is map that contain the label and the the line it was found
std::map<std::string,int> lables_map;
//pc register
int pc=0;


//the ram/disk
int* ram = 0;

//the registers
int reg[NUMBER_OF_REGISTERS];

//counter for the instructing that was exceuted
int instructioncount;
//counter for the excution time
int executetime;

ConfigurationStruct configuration;

int _tmain(int argc, char* argv[])
{
//1cmd_file.txt 2config_file.txt 3mem_init.txt 4regs_dump.txt 5mem_dump.txt 6time.txt 7committed.txt 8hitrate.txt 9L1i.txt 10L1d.txt 11L2i.txt 12L2d.txt
//1cmd_file1.txt 2cmd_file2.txt 3config_file.txt 4mem_init.txt 5regs_dump.txt 6mem_dump.txt 7time.txt 8committed.txt 9hitrate.txt 10trace1.txt 11trace2.txt

	if(argc!=12)
	{
		printf("Wrong number of command line arguments!\n");
		exit(1);
	}

	if (ini_parse(argv[3], handler, &configuration) < 0) {
		printf("Can't load '%s' file\n",argv[1]);
		exit(1);
	}

	ram = new int[MEMORY_SIZE/4];
	if (ram == NULL)
		exit(1);

	ReadMemInitFile(argv[4]);

	ParseCMDfile(argv[1]);

	InitCaches(&configuration);

	StartSimulator();

	printf("simulation done!\n");

	WriteMemoryDumpToFile(argv[6]);

	WriteRegisterDumpToFile(argv[5]);

	WriteExceutionTime(argv[7]);

	WriteInstructionCount(argv[8]);

	WriteHitRatioAndAMAT(argv[9]);

	
	printf("all results written to files!\n");

//	DestroyCaches();

	free(ram);

	return 0;
}

void WriteExceutionTime(char *file_name)
{
	FILE *file=fopen(file_name,"w");

	if (file==NULL)
	{
		printf("error in write execution time\n %s %s ",__FILE__,__LINE__);
		exit(1);
	}

	fprintf(file,"%d",executetime);

	fclose(file);
	return;
}

void WriteInstructionCount(char *file_name)
{
	FILE *file=fopen(file_name,"w");

	if (file==NULL)
	{
		printf("error in write instruction count file\n %s %s ",__FILE__,__LINE__);
		exit(1);
	}

	fprintf(file,"%d",instructioncount);

	fclose(file);
	return;

}

//for string "$1" return int 1
int GetRegNumberFromString(std::string reg)
{
	if(reg.at(0)!='$')	
	{
		printf("error in instruction at function GetRegNumberFromString pc=%d\n",pc);
		exit(1);
	}

	return atoi(reg.c_str()+1);//convert to int after the $ sign
}

int MyAtoi(std::string str)
{
	const char *number = str.c_str();

	char *end;
	long value = strtol(number, &end, 10); 
	if (end == number || *end != '\0' || errno == ERANGE)
	{
		printf("error in instruction at function MyAtoi pc=%d\n",pc);
		exit(1);
	}
	return value;
}

//return the string that in "(" ")" as int
int GetOffset(std::string s)
{
	size_t start,end;
	start=s.find_first_of("(",0)+1;
	end=s.find_first_of(")");
	std::string tmp=s.substr(start,end-start);
	return MyAtoi(tmp);
}


void StartSimulator()
{
	std::vector<std::string> current_instruction;
	int r0,r1,r2,res,desination;

	Tomasulo tomasulo(
		configuration.addsub_delay,
		configuration.mul_delay,
		configuration.div_delay,
		configuration.instruction_q_depth,
		configuration.addsub_rs,
		configuration.muldiv_rs,
		configuration.load_q_depth,
		configuration.store_q_depth);

	reg[0]=0;//make reg 0 always 0

	while(1)
	{
		
		current_instruction=commands_vector[pc];
		//simulate load instruction
		executetime += LoadWord(PCtoAddress(pc),&desination);
		executetime++;
		if (!tomasulo.isInstQueueFull()) {
			tomasulo.addToQueue(current_instruction);
		}
		tomasulo.doWork();
		DoWork();
		instructioncount++;
		if(current_instruction[1]=="halt")
			return;
		if(current_instruction[1]=="j")
		{
			pc=lables_map[current_instruction[2]];
			continue;
		}
		if(current_instruction[1]=="add")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			r1=GetRegNumberFromString(current_instruction[3]);
			r2=GetRegNumberFromString(current_instruction[4]);
			reg[r0]=reg[r1]+reg[r2];
			pc++;
			continue;
		}
		if(current_instruction[1]=="sub")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			r1=GetRegNumberFromString(current_instruction[3]);
			r2=GetRegNumberFromString(current_instruction[4]);
			reg[r0]=reg[r1]-reg[r2];
			pc++;
			continue;
		}
		if(current_instruction[1]=="mul")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			r1=GetRegNumberFromString(current_instruction[3]);
			r2=GetRegNumberFromString(current_instruction[4]);
			reg[r0]=reg[r1]*reg[r2];
			pc++;
			continue;
		}
		if(current_instruction[1]=="div")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			r1=GetRegNumberFromString(current_instruction[3]);
			r2=GetRegNumberFromString(current_instruction[4]);
			reg[r0]=reg[r1]/reg[r2];
			pc++;
			continue;
		}
		if(current_instruction[1]=="slt")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			r1=GetRegNumberFromString(current_instruction[3]);
			r2=GetRegNumberFromString(current_instruction[4]);
			reg[r0]=reg[r1]<reg[r2]?1:0;
			pc++;
			continue;
		}
		if(current_instruction[1]=="addi")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			r1=GetRegNumberFromString(current_instruction[3]);
			res=MyAtoi(current_instruction[4]);
			reg[r0]=reg[r1]+res;
			pc++;
			continue;
		}
		if(current_instruction[1]=="subi")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			r1=GetRegNumberFromString(current_instruction[3]);
			res=MyAtoi(current_instruction[4]);
			reg[r0]=reg[r1]-res;
			pc++;
			continue;
		}
		if(current_instruction[1]=="slti")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			r1=GetRegNumberFromString(current_instruction[3]);
			res=MyAtoi(current_instruction[4]);
			reg[r0]=reg[r1]<res?1:0;
			pc++;
			continue;
		}
		if(current_instruction[1]=="beq")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			r1=GetRegNumberFromString(current_instruction[3]);

			if(reg[r0]==reg[r1])
			{
				pc=lables_map[current_instruction[4]];
				continue;
			}
			pc++;
			continue;
		}
		if(current_instruction[1]=="bne")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			r1=GetRegNumberFromString(current_instruction[3]);

			if(reg[r0]!=reg[r1])
			{
				pc=lables_map[current_instruction[4]];
				continue;
			}
			pc++;
			continue;
		}
		if(current_instruction[1]=="lw")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			res=GetOffset(current_instruction[3]);
			r1=GetRegNumberFromString(current_instruction[3].substr(current_instruction[3].find_first_of(")")+1,std::string::npos));
			executetime += LoadWord((reg[r1]+res)/4,&reg[r0]);
			pc++;
			continue;
		}
		if(current_instruction[1]=="sw")
		{
			r0=GetRegNumberFromString(current_instruction[2]);
			res=GetOffset(current_instruction[3]);
			r1=GetRegNumberFromString(current_instruction[3].substr(current_instruction[3].find_first_of(")")+1,std::string::npos));
			executetime += StoreWord((reg[r1]+res)/4,&reg[r0]);
			pc++;
			continue;
		}

		printf("unknown instruction\n");
		exit(1);
	}
}

//private function used by ini_parse
static int handler(void* user, const char* section, const char* name,
	const char* value)
{
	ConfigurationStruct* pconfig = (ConfigurationStruct*)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	if (MATCH("", "addsub_delay"))
	{
		pconfig->addsub_delay = atoi(value);
	}
	else if (MATCH("", "mul_delay"))
	{
		pconfig->mul_delay = atoi(value);
	}
	else if (MATCH("", "div_delay"))
	{
		pconfig->div_delay = atoi(value);
	}
	else if (MATCH("", "instruction_q_depth"))
	{
		pconfig->instruction_q_depth = atoi(value);
	}
	else if (MATCH("", "addsub_rs"))
	{
		pconfig->addsub_rs = atoi(value);
	}
	else if (MATCH("", "muldiv_rs"))
	{
		pconfig->muldiv_rs = atoi(value);
	}
	else if (MATCH("", "load_q_depth"))
	{
		pconfig->load_q_depth = atoi(value);
	}
	else if (MATCH("", "reorder_buffer"))
	{
		pconfig->reorder_buffer = atoi(value);
	}
	else if (MATCH("", "ghr_width"))
	{
		pconfig->ghr_width = atoi(value);
	}
	else if (MATCH("", "two_threads_enabled"))
	{
		pconfig->two_threads_enabled = atoi(value);
	}
	else if (MATCH("", "l1_block_size"))
	{
		pconfig->l1_block_size = atoi(value);
	}
	else if (MATCH("", "l1_access_delay"))
	{
		pconfig->l1_access_delay = atoi(value);
	}
	else if (MATCH("", "l1_cache_size"))
	{
		pconfig->l1_cache_size = atoi(value);
	}
	else if (MATCH("", "l2_block_size"))
	{
		pconfig->l2_block_size = atoi(value);
	}
	else if (MATCH("", "l2_access_delay"))
	{
		pconfig->l2_access_delay = atoi(value);
	}
	else if (MATCH("", "l2_cache_size"))
	{
		pconfig->l2_cache_size = atoi(value);
	}
	else if (MATCH("", "mem_access_delay"))
	{
		pconfig->mem_access_delay = atoi(value);
	}
	else if (MATCH("","store_q_depth"))
	{	
		pconfig->store_q_depth = atoi(value);
	}
	else
	{
		return 0;  /* unknown section/name, error */
	}

	return 1;
}


int WriteRegisterDumpToFile(char *file_name)
{
	FILE *file=fopen(file_name,"w");
	int i;

	if (file==NULL)
	{
		printf("error in write registery dump file\n %s %s ",__FILE__,__LINE__);
		return 0;
	}

	for(i=0;i<NUMBER_OF_REGISTERS;i++)
	{
		fprintf(file,"$%d %d\n",i,reg[i]);
	}

	fclose(file);
	return 1;
}


int WriteMemoryDumpToFile(char *file_name)
{
	FILE *file=fopen(file_name,"w");
	int i;

	if (file==NULL)
	{
		printf("error in write memory dump file\n %s %s ",__FILE__,__LINE__);
		return 0;
	}

	for(i=0; i<MEMORY_SIZE/4; i++)
	{
		fprintf(file,"%02x ",ram[i]&0xFF);
		fprintf(file,"%02x ",(ram[i]>>8)&0xFF);
		fprintf(file,"%02x ",(ram[i]>>16)&0xFF);
		
		
		
		if((i!=0)&&(i%2!=0))
		{
			fprintf(file,"%02x",(ram[i]>>24)&0xFF);
			fprintf(file,"\n");
		}
		else
		{
			fprintf(file,"%02x ",(ram[i]>>24)&0xFF);
		}
	}

	fclose(file);
	return 1;
}


void ReadMemInitFile(char *file_name)
{
	FILE *file=fopen(file_name,"r");
	char lineBuffer[MAX_LINE_SIZE];
	int status;
	int x[8];
	int i=0;

	if (file==NULL)
	{
		printf("error in write CMD file\n %s %s ",__FILE__,__LINE__);
		exit(1);
	}


	while(fgets(lineBuffer, MAX_LINE_SIZE, file)!= NULL)
	{
		status = sscanf(lineBuffer, "%02x %02x %02x %02x %02x %02x %02x %02x\r\n",
			&x[0], &x[1], &x[2], &x[3], &x[4], &x[5], &x[6], &x[7]);
		if(status!=8)
		{
			printf("error in parsing the init mem file\n");
			exit(1);
		}

		ram[i]=x[0]+(x[1]<<8)+(x[2]<<16)+(x[3]<<24);
		ram[i+1]=x[4]+(x[5]<<8)+(x[6]<<16)+(x[7]<<24);
		i+=2;
	}
	for (int i = (0x00F00000/4); i < (MEMORY_SIZE/4); i++) {
		ram[i] = i*4;
	}
}

int ParseCMDfile(char *file_name)
{
	FILE *file=fopen(file_name,"r");
	char lineBuffer[MAX_LINE_SIZE];

	if (file==NULL)
	{
		printf("error in write CMD file\n %s %s ",__FILE__,__LINE__);
		return 0;
	}

	while ( fgets ( lineBuffer, sizeof lineBuffer, file ) != NULL ) 
	{
		lineBuffer[strlen(lineBuffer)-1]='\0';
		ParseLine(lineBuffer);
	}

	fclose(file);
	return 1;
}

void ParseLine(char *lineBuffer)
{
	std::string tmp(lineBuffer);
	std::string lable,cmd,rs,r0,r1,jump_lable;
	std::vector<std::string> instruction;
	size_t start=0,end=0;
	tmp.append("#");//mark end of line
	if(tmp.find_first_of(':',0)==std::string::npos)//regular line start with #
	{
		instruction.push_back("#");
	}
	else// lable line start ith the label
	{
		end=tmp.find_first_of(":",0);
		cmd=tmp.substr(start,end-start);
		instruction.push_back(cmd);
		lables_map[cmd]=line_counter;
	}
	start=tmp.find_first_not_of(" \t:",end);
	end=tmp.find_first_of(" ,\t\r\n#",start);

	bool flag=true;
	if(tmp.at(end)=='#')
	{
		cmd=tmp.substr(start,end-start);
		instruction.push_back(cmd);
		flag=false;
	}

	while(/*start!=std::string::npos*/ flag){
		cmd=tmp.substr(start,end-start);
		instruction.push_back(cmd);
		start=tmp.find_first_not_of(" ,\t",end);
		end=tmp.find_first_of(" ,\t\r\n#",start);
		//		std::cout <<"^^"<<tmp<<"^^"<<"%%" << tmp.at(end) <<"%%" << std::endl;
		if(tmp.at(end)=='#')
		{
			cmd=tmp.substr(start,end-start);
			instruction.push_back(cmd);
			break;
		}
	}
	line_counter++;
	commands_vector.push_back(instruction);
}


