#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include "cli_utils.h"

int get_pointer_array_size(void **pointer_array) {
	int i=0;
	while (pointer_array[i] != NULL) {
		i++;
	}
	return i;
}

void move_up(int amt_lines) {
	printf("\033[%dA", amt_lines);
	return;
}

void draw_menu(char *options[], int selected_entry) {
	for (int i = 0; options[i] != NULL; i++) {
		if (i != selected_entry) { 
			printf("  %s\n", options[i]);
		} else {
			printf("> %s\n", options[i]);
		}
	}
	fflush(stdout);
}

int cli_menu(char *options[]) {
	char ch;
	struct termios oldt, newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	// disable canonical mode and echo
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt); // apply to terminal

	draw_menu(options, 0);
	int selected_entry=0;
	int option_len=get_pointer_array_size((void **)options);

	while (1) {
		ch = getchar();
		fflush(stdout);
		if(ch == 'j' && selected_entry<option_len-1) {
			selected_entry++;
			move_up(option_len);
			draw_menu(options, selected_entry);
		} else if(ch == 'k' && selected_entry>0) {
			selected_entry--;
			move_up(option_len);
			draw_menu(options, selected_entry);
		} else if(ch == 10 || ch == 13) { // enter
			break;
		}
	}
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // restore old terminal settings
	return selected_entry;
}
