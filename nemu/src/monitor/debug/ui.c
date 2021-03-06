#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
WP* new_wp(char *args);
void free_wp(int );
void print_w();

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

/**************user add functions*************/

/*execute by one step*/
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);
static int cmd_x(char *);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
	{ "si", "execute * step by one step", cmd_si},
	{ "info", "print reg or watchpoint info", cmd_info},
	{ "p", "calculate the expression", cmd_p},
	{ "w", "new watchpoint", cmd_w},
	{ "d", "delete awtchpoint NO", cmd_d},
	{ "x", "x n expr:scan the memory",cmd_x},
  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

/*added by sherwin*/
static int cmd_x(char *args){
	char *arg=strtok(NULL," ");
	if(arg==NULL){
		printf("please input right arguement\n");
		return 0;
	}
	else{
		int n=atoi(arg);
		Log("n=%d",n);
		if(n==0) return 0;
		char *exp=strtok(NULL," ");
		Log("%s",exp);
		bool success;
		int result;
		result=expr(exp,&success);
		for(int i=0;i<n;i++){
			printf("address=0x%x,res=0x%x\n",result+i*4,vaddr_read(result+i*4,4));
		}
		return 0;
	}
}


static int cmd_si(char *args){
	char *arg=strtok(NULL,"");
//	printf("s=%s",arg);
	int i=-1;
	if(arg==NULL)
		i=1;
	else {
		i=atoi(arg);
	}
	cpu_exec(i);
	return 0;
}

static int cmd_info(char *args){
	char *arg=strtok(NULL,"");
	if(arg==NULL) {
		printf("please input arguement!\n");
		return 0;
	}
	if (!strcmp(arg,"r")){
		for(int index=0;index<8;index++){
			printf(" %s: 0x%x",reg_name(index,4),reg_l(index));
			printf(" %s: 0x%x",reg_name(index,2),reg_w(index));
			printf(" %s: 0x%x\n",reg_name(index,1),reg_b(index));
		}
		printf(" eip: 0x%x\n",cpu.eip);
	}
	else if(!strcmp(arg,"w")){
		print_w();
	}
	return 0;
}

static int cmd_p(char *args){
	char *arg=strtok(NULL,"");
	int result;
	bool success = false;
	if(arg==NULL) {
		printf("please input arguement!\n");
		return 0;
	}
  else {	
		result=expr(arg,&success);
		if(success)
			printf("DEC=%d  HEX=%x\n",result,result);	
		else printf("wrong!\n");
	}
	return 0;
}

static int cmd_w(char *args){
	char *arg=strtok(NULL,"");
	if(arg==NULL) {
		printf("wrong watchpoint!\n");
		return 0;
	}
	else{
		new_wp(arg);
		return 0;
	}
}

static int cmd_d(char *args){
	char *arg=strtok(NULL,"");
	if(arg==NULL){
		printf("wrong watchpoint NO");
		return 0;
	}
	else{
		free_wp(atoi(arg));
		return 0;
	}
}

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
