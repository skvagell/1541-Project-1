/**************************************************************/
/* CS/COE 1541
   just compile with gcc -o CPU CPU.c
   and execute using
   ./CPU  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0
***************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h"

#define IF1 0
// IF1 load buffer, contains info about instruction currently in IF1
#define IF2 1
 //IF1-IF2 buffer, contains info about instruction currently in IF2
#define ID 2
//IF2-ID buffer, contains info about instruction currently in ID
#define EX 3
//ID-EX buffer, contains info about instruction currently in EX
#define MEM1 4
//EX-MEM1 buffer, contains info about instruction currently in Mem1
#define MEM2 5 //MEM1-MEM2 buffer, contains info about instruction currently in Mem2
#define WB 6 //MEM2-WB buffer, contains info about instruction currently in WB

struct hash_entry {
    unsigned int address;
    unsigned int value;
};

#define BRANCH_TABLE_SIZE 64

/* Functions prototypes */
void branch_table_init(struct hash_entry table[BRANCH_TABLE_SIZE]);
void branch_table_taken(struct hash_entry table[BRANCH_TABLE_SIZE], unsigned int address, int prediction_method);
void branch_table_not_taken(struct hash_entry table[BRANCH_TABLE_SIZE], unsigned int address, int prediction_method);
int branch_table_predict(struct hash_entry table[BRANCH_TABLE_SIZE], unsigned int address, int prediction_method);
void stall(struct trace_item *buffers, struct trace_item NO_OP, int stallPosition);
void flush(struct trace_item *buffers, struct trace_item NO_OP, struct trace_item *queue, int *queue_size);
void advance(struct trace_item *buffers);
int structural_hazard(struct trace_item *buffers);
int data_hazard_one(struct trace_item *buffers);
int data_hazard_two(struct trace_item *buffers);
void print_trace(int trace_view_on, int cycle_number, struct trace_item *tr_entry, int stage);
int get_next_instr(struct trace_item **tr_entry, struct trace_item NO_OP, struct trace_item *queue, int *queue_size);
void init_buffers(struct trace_item *buffers);
int is_empty(struct trace_item *buffers);

int main(int argc, char **argv)
{
  struct trace_item buffers[7];
  struct trace_item queue[3];

  struct trace_item *tr_entry;

  struct trace_item NO_OP;
  NO_OP.type = ti_NOP;

  struct hash_entry branch_table[BRANCH_TABLE_SIZE];

  int trace_ended = 0;
  int queue_size = 0;
  char *trace_file_name;
  int trace_view_on = 0;
  int prediction_method = 0;

  unsigned int cycle_number = 0;

  if (argc == 1) {
    fprintf(stdout, "\nUSAGE: tv <trace_file> <prediction_method> <trace_view_on>\n");
    fprintf(stdout, "\n(prediction_method) - to choose not-taken prediction(0), 1-bit branch prediction(1) or 2-bit(2).\n\n");
    fprintf(stdout, "\n(trace_view_on) - to turn on(1) or off(0) individual item view.\n\n");
    exit(0);
  }

  trace_file_name = argv[1];
  if (argc >= 3) prediction_method = atoi(argv[2]) ;
  if (argc == 4) trace_view_on = atoi(argv[3]) ;

  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  /* Fill Branch Prediction Hash Table with no-element */
  branch_table_init(branch_table);

  /* Fill stage buffers with NOPs */
  init_buffers(buffers);

  trace_init();

  while(1) {
    /* Move new instruction into the pipeline */
    if (trace_ended && is_empty(buffers)) {  /* no more instructions (trace_items) to simulate and the pipeline is empty */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      break;
    }
    else {              /* parse the next instruction to simulate */
      int control_hazard = 0;
      cycle_number++;

      /* Handle possible control hazard first */
      if ((buffers[EX].type == ti_JTYPE) || (buffers[EX].type == ti_JRTYPE) || (buffers[EX].type == ti_BRANCH)) {
        /* If the branch was predicted to be taken */
        int prediction = branch_table_predict(branch_table, buffers[EX].PC, prediction_method);
        /* Check if it was really taken */
        int was_taken = buffers[ID].PC != (buffers[EX].PC + 4);

        /* Remember branching result in the predictor */
        if (was_taken) {
          branch_table_taken(branch_table, buffers[EX].PC, prediction_method);
        } else {
          branch_table_not_taken(branch_table, buffers[EX].PC, prediction_method);
        }

        /* Check if control hazard happened */
        control_hazard = prediction != was_taken;
      }

      if (control_hazard) {
        if (trace_view_on) {
          printf("Control hazard, flush\n");
        }
        flush(buffers, NO_OP, queue, &queue_size);
      }

      else if (structural_hazard(buffers)) {
        if (trace_view_on) {
          printf("Structural hazard, stall\n");
        }
      	stall(buffers, NO_OP, EX);
      }
//
      else if(data_hazard_one(buffers)) {
        if (trace_view_on) {
          printf("Data hazard 1, stall\n");
        }
      	stall(buffers, NO_OP, MEM1);
      }
//
      else if(data_hazard_two(buffers)) {
        if (trace_view_on) {
          printf("Data hazard 2, stall\n");
        }
		    stall(buffers, NO_OP, MEM2);
      }
//
      else {
        if (trace_view_on) {
          printf("No hazards, advance\n");
        }
      	advance(buffers);
        trace_ended = !get_next_instr(&tr_entry, NO_OP, queue, &queue_size);
        buffers[IF1] = *tr_entry;
      }
//
//
    }

    int i;
    for(i = IF1; i <= WB ; i++){
    	print_trace(trace_view_on, cycle_number, &(buffers[i]), i);
    }
  }

  trace_uninit();

  exit(0);
}

void init_buffers(struct trace_item *buffers) {
  int i = 0;
  for (i = IF1; i <= WB; i++) {
    buffers[i].type = ti_NOP;
  }
}

void branch_table_init(struct hash_entry table[BRANCH_TABLE_SIZE]) {
  int i;
  for (i = 0; i < BRANCH_TABLE_SIZE; i++) {
    table[i].address = -1;
    table[i].value = -1;
  }
}

void branch_table_taken(struct hash_entry table[BRANCH_TABLE_SIZE], unsigned int address, int prediction_method) {
    /* Take only bits 8 - 3 of address to index the table */
    int index = (address >> 3) % BRANCH_TABLE_SIZE;
    /* Limit value with 1 or 2 bits */
    int limit = (prediction_method == 1) ? 1 : 3;

    /* Check if this address isn't present in the table */
    if (table[index].address != address) {
      table[index].address = address;
      table[index].value = 0;
    }

    /* Increase value */
    table[index].value++;

    /* Limit value */
    if (table[index].value > limit) {
      table[index].value = limit;
    }
}

void branch_table_not_taken(struct hash_entry table[BRANCH_TABLE_SIZE], unsigned int address, int prediction_method) {
  /* Take only bits 8 - 3 of address to index the table */
  int index = (address >> 3) % BRANCH_TABLE_SIZE;

  /* Check if this address isn't present in the table */
  if (table[index].address != address) {
    table[index].address = address;
    table[index].value = 0;
  }

  /* Try to decrease hits count */
  if (table[index].value > 0) {
    table[index].value--;
  }
}

int branch_table_predict(struct hash_entry table[BRANCH_TABLE_SIZE], unsigned int address, int prediction_method) {
  /* Take only bits 8 - 3 of address to index the table */
  int index = (address >> 3) % BRANCH_TABLE_SIZE;

  /* Assume not taken if the prediction_method is 0 or we have no info about this branch */
  if (prediction_method == 0 || table[index].address != address) {
    return 0;
  }

  /* Use 1-bit predicter */
  if (prediction_method == 1) {
    return table[index].value > 0;
  }

  /* Use 2-bit predicter */
  if (prediction_method == 2) {
    return table[index].value > 1;
  }

  /* Shouldn't get here, but assume not taken */
  return 0;
}

void stall(struct trace_item *buffers, struct trace_item NO_OP, int stallPosition){
	int i;
	//Advance everything ahead of the stall position
	for(i = WB; i > stallPosition; i--){
		buffers[i] = buffers[i-1];
	}
	//Inject a no op and leave everything behind it where it was
	buffers[stallPosition] = NO_OP;
}

void flush(struct trace_item *buffers, struct trace_item NO_OP, struct trace_item *queue, int *queue_size){
	int i;
	//Advance everything ahead of the branch/jump
	for(i = WB; i >= EX; i--){
		buffers[i] = buffers[i-1];
	}
	//Inject a no op
	buffers[EX] = NO_OP;
	//Squash the instructions behind the branch/jump
	for(i = IF1; i < EX; i++){
		queue[(*queue_size)++] = buffers[i];
    buffers[i] = NO_OP;
	}
}

void advance(struct trace_item *buffers){
	int i;
	for(i = WB; i > IF1; i--){
		buffers[i] = buffers[i-1];
	}
  /* Don't need to read buffers[IF1] here, it's done in the main loop */
}


int structural_hazard(struct trace_item *buffers){
	if((buffers[WB].type == ti_RTYPE || buffers[WB].type == ti_ITYPE || buffers[WB].type == ti_LOAD) &&(buffers[WB].dReg == buffers[ID].sReg_a || buffers[WB].dReg == buffers[ID].sReg_b)){
		return 1;
	}
	return 0;
}

int data_hazard_one(struct trace_item *buffers){
	if((buffers[MEM1].type == ti_LOAD) && ((buffers[MEM1].dReg == buffers[EX].sReg_a) || buffers[MEM1].dReg == buffers[EX].sReg_b)){
		return 1;
	}
	return 0;
}

int data_hazard_two(struct trace_item *buffers){
	if((buffers[MEM2].type == ti_LOAD) && ((buffers[MEM2].dReg == buffers[EX].sReg_a) || (buffers[MEM2].dReg == buffers[EX].sReg_b))){
		return 1;
	}
	return 0;
}

void print_trace(int trace_view_on, int cycle_number, struct trace_item *tr_entry, int stage) {
  if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
    printf("[cycle %d] [stage %d] ", cycle_number, stage);
    switch (tr_entry->type) {
      case ti_NOP:
        printf("NOP\n") ;
        break;
      case ti_RTYPE:
        printf("RTYPE: (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg);
        break;
      case ti_ITYPE:
        printf("ITYPE: (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
        break;
      case ti_LOAD:
        printf("LOAD: (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
        break;
      case ti_STORE:
        printf("STORE: (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
        break;
      case ti_BRANCH:
        printf("BRANCH: (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
        break;
      case ti_JTYPE:
        printf("JTYPE: (PC: %x)(addr: %x)\n", tr_entry->PC,tr_entry->Addr);
        break;
      case ti_SPECIAL:
        printf("SPECIAL\n") ;
        break;
      case ti_JRTYPE:
        printf("RTYPE: (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_entry->PC, tr_entry->dReg, tr_entry->Addr);
        break;
    }
  }
}

/* Returns 1 and sets instruction info when next instruction exists
   Returns 0 if not */
int get_next_instr(struct trace_item **tr_entry, struct trace_item NO_OP, struct trace_item *queue, int *queue_size) {
  /* Firstly search in queue for the next instruction */
  if ((*queue_size) > 0)
  {
    (*queue_size)--;
    *tr_entry = &(queue[*queue_size]);
  }
  /* Then try to take instruction from the trace */
  else if (!trace_get_item(tr_entry))
  {
    /* There are no instruction in the trace, return a no op */
    *tr_entry = &NO_OP;
    return 0;
  }
  return 1;
}

int is_empty(struct trace_item *buffers){
	int i;
	for(i = 0; i < 6; i++){
		if(buffers[i].type != ti_NOP){
			return 0;
		}
	}
	return 1;
}
