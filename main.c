#include <dirent.h>
#include <curses.h>
#include <sys/stat.h>

struct state {
  bool run;
  char path[BUFSIZ];
  unsigned int index;
  unsigned int count;
  struct dirent *files[BUFSIZ];
};

void render(struct state state) {
  erase();
  printw("%s\n\n", state.path);
  for (int i = 0; i < state.count; i++) {
    if (i == state.index)
      printw("> %s\n", state.files[i]->d_name);
    else
      printw("  %s\n", state.files[i]->d_name);
  }
	refresh();
}

void update_dir(struct state *state) {
  int i = 0;
  state->index = 0;
  DIR *d = opendir(state->path);
  struct dirent *dir;
  dir = readdir(d);
  while ((dir = readdir(d)) != NULL) {
    state->files[i] = dir;
    i++;
  }
  state->count = i;
}

int main() {
  struct state state = {
    .run = true,
    .index = 0
  };
  realpath(".", state.path);
  printf("%s\n", state.path);
  update_dir(&state);
  
  initscr();
  raw();
  noecho();
  curs_set(0);
  render(state);
  char p[BUFSIZ];
  struct stat stbuf;
	while (state.run) {
	  switch (getch()) {
	    case 'q':
	      erase();
	      state.run = false;
	      break;
	    case 'j':
	      if (state.index == state.count - 1)
	        state.index = 0;
	      else
  	      state.index++;
	      render(state);
	      break;
	    case 'k':
	      if (!state.index)
	        state.index = state.count - 1;
	      else
  	      state.index--;
	      render(state);
	      break;
	    case 10:
	      stat(state.files[state.index]->d_name, &stbuf);
	      if (S_ISDIR(stbuf.st_mode)) {
	        strcat(state.path, "/");
	        strcat(state.path, state.files[state.index]->d_name);
  	      // realpath(state.path, state.path);
  	      update_dir(&state);
  	    } else {
  	      // strcpy(p, "hx ");
  	      // strcat(p, state.path);
  	      // printw("%s\n", p);
  	      // refresh();
  	      // system(p);
         //  curs_set(1);
         //  curs_set(0);
  	      // clear();
  	    }
        render(state);
	      break;
	  }
	  napms(33);
	};
	endwin();
	return 0;
}
