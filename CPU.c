/**************************************************************/
/* CS/COE 1541				 			
   just compile with gcc -o pipeline pipeline.c			
   and execute using							
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0  
***************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 

#define IF2IF2 0
#define IF2ID 1
#define IDEX 2
#define EXMEM1 3
#define MEM1MEM2 4
#define MEM2WB 5

int main(int argc, char **argv)
{
  struct trace_item *tr_entry;

  struct trace_item[6] buffers;  

  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  
  unsigned char t_type = 0;
  unsigned char t_sReg_a= 0;
  unsigned char t_sReg_b= 0;
  unsigned char t_dReg= 0;
  unsigned int t_PC = 0;
  unsigned int t_Addr = 0;

  unsigned int cycle_number = 0;

  if (argc == 1) {
    fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character>\n");
    fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
    exit(0);
  }
    
  trace_file_name = argv[1];
  if (argc == 3) trace_view_on = atoi(argv[2]) ;

  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();

  while(1) {
    size = trace_get_item(&tr_entry);
   
    if (!size) {       /* no more instructions (trace_items) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      break;
    }
    else{              /* parse the next instruction to simulate */
      cycle_number++;
      //I think we should check for control hazards first, 'cause if we flush the whole pipeline, any other hazards are kind of irrelevant
//      if((buffers[EXMEM1]->type == ti_BRANCH || buffers[EXMEM1]->type == ti_JTYPE) && buffers[IDEXE]->PC != buffers[EXMEM1] + 4){
      //This doesn't account for branch prediction
//      	flush(buffers);
//      }//end if

//      else if(buffers[MEM2WB]->dReg == buffers[IF2ID]->sRegA || buffers[MEM2WB]->dReg == buffers[IF2ID]->sRegB){
//      	stall(buffers, IDEX);
//      }
//
//      else if((buffers[EXMEM1]->type == ti_LOAD) && ((buffers[EXMEM1]->dReg == buffers[IDEX]->sRegA) || buffers[EXMEM1]->dReg == buffers[IDEX]->sRegB)){
//      	stall(buffers, EXMEM1);
//      }
//
//      else if((buffers[MEM1MEM2]->type == ti_LOAD) && ((buffers[MEM1MEM2]->dReg == buffers[IDEX]->sRegA) || buffers[EXMEM1]->dReg == buffers[IDEX]->sRegB)){
//		stall(buffers, MEM1MEM2);
//      }
//
//      else{
//      	advance(buffers);
//      }
//
//
    }  

// SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL - EACH INSTRUCTION IS EXECUTED
// IN ONE CYCLE

    if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
      switch(tr_entry->type) {
        case ti_NOP:
          printf("[cycle %d] NOP\n:",cycle_number) ;
          break;
        case ti_RTYPE:
          printf("[cycle %d] RTYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg);
          break;
        case ti_ITYPE:
          printf("[cycle %d] ITYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
          break;
        case ti_LOAD:
          printf("[cycle %d] LOAD:",cycle_number) ;      
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
          break;
        case ti_STORE:
          printf("[cycle %d] STORE:",cycle_number) ;      
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          break;
        case ti_BRANCH:
          printf("[cycle %d] BRANCH:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          break;
        case ti_JTYPE:
          printf("[cycle %d] JTYPE:",cycle_number) ;
          printf(" (PC: %x)(addr: %x)\n", tr_entry->PC,tr_entry->Addr);
          break;
        case ti_SPECIAL:
          printf("[cycle %d] SPECIAL:\n",cycle_number) ;      	
          break;
        case ti_JRTYPE:
          printf("[cycle %d] JRTYPE:",cycle_number) ;
          printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_entry->PC, tr_entry->dReg, tr_entry->Addr);
          break;
      }
    }
  }

  trace_uninit();

  exit(0);
}

void stall(struct trace_item* buffers, int stallPosition){
	/*for(int i = 6; i > stallPosition; i--){
		buffers[i] = buffers[i-1];
	}
	buffers[stallPosition] = NO_OP;
	*/
}

void flush(struct trace_item* buffers){
	/*for(int i = 0; i < 3; i++){
		buffers[i] = NO_OP;
	}
*/
}

void advance(struct trace_item* buffers, struct trace_item* entry){
	/*for(int i = 5; i > 1; i--){
		buffers[i] = buffers[i-1];
	}
	buffers[0] = entry;
}
