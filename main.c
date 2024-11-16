#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <curses.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>

struct state {
  bool run;
  int index;
  int count;
  int tree_width;
  int tree_scroll;
  int view_scroll;
  WINDOW *tree;
  WINDOW *view;
  char *files[BUFSIZ];
  bool dirs[BUFSIZ];
};

void render(struct state state) {
  werase(state.view);
  if (state.dirs[state.index]) {
    DIR *d = opendir(state.files[state.index]);
    if (d != NULL) {
      struct dirent *dir;
      readdir(d);
      readdir(d);
      while ((dir = readdir(d)) != NULL) {
        wprintw(state.view, "%s\n", dir->d_name);
      }
    }
  } else {
    FILE *fp = fopen(state.files[state.index], "r");
    char c;
    if (fp != NULL) {
      while ((c = fgetc(fp)) != EOF)
        waddch(state.view, c);
    }
  }
  pnoutrefresh(state.view, state.view_scroll, 0, 0, state.tree_width, LINES - 1, COLS - 1);

  werase(state.tree);
  for (int i = 0; i < state.count; i++) {
    if (i == state.index) {
      wattron(state.tree, A_REVERSE);
      wprintw(state.tree, "%s\n", state.files[i]);
      wattroff(state.tree, A_REVERSE);
    }
    else {
      wprintw(state.tree, "%s\n", state.files[i]);
    }
  }
  pnoutrefresh(state.tree, state.tree_scroll, 0, 0, 0, LINES - 1, state.tree_width - 1);
  doupdate();
}

void update(struct state *state) {
  int i = 0, length = 0;
  state->tree_width = 0;
  state->index = 0;
  state->tree_scroll = 0;
  state->view_scroll = 0;
  DIR *d = opendir(".");
  struct dirent *dir;
  struct stat stbuf;
  readdir(d);
  if (!strcmp(getcwd(NULL, 0), "/"))
    readdir(d);
  while ((dir = readdir(d)) != NULL) {
    state->files[i] = dir->d_name;
    length = strlen(dir->d_name);
    stat(dir->d_name, &stbuf);
    state->dirs[i] = S_ISDIR(stbuf.st_mode);
    if (length > state->tree_width)
      state->tree_width = length;
    i++;
  }
  state->tree_width += 4;
  state->count = i;
}

void update_view(struct state *state) {
  state->view_scroll = 0;
}

void event(struct state *state) {
  char path[BUFSIZ];
  char cmd[BUFSIZ];
  switch (wgetch(state->tree)) {
    case 'j':
      state->view_scroll++;
      render(*state);
      break;
    case 'k':
      if (state->view_scroll)
        state->view_scroll--;
      render(*state);
      break;
    case 'q':
      erase();
      state->run = false;
      break;
    case 'h':
      if (state->index == state->count - 1) {
        state->index = 0;
  	    if (state->index < state->tree_scroll)
    	    state->tree_scroll = 0;
      } else {
	      state->index++;
  	    if (state->index + 1 > LINES)
    	    state->tree_scroll = state->index + 1 - LINES;
      }
      update_view(state);
      render(*state);
      break;
    case 'l':
      if (!state->index) {
        state->index = state->count - 1;
  	    if (state->index + 1 > LINES)
    	    state->tree_scroll = state->index + 1 - LINES;
      } else {
	      state->index--;
  	    if (state->index < state->tree_scroll)
    	    state->tree_scroll--;
    	}
      update_view(state);
      render(*state);
      break;
    case 10:
      if (state->dirs[state->index]) {
        chdir(state->files[state->index]);
        update(state);
      } else {
        strcpy(cmd, "${EDITOR}");
        strcat(cmd, " ");
        strcat(cmd, state->files[state->index]);
        system(cmd);
        wclear(state->tree);
        wclear(state->view);
        curs_set(1);
        curs_set(0);
      }
      render(*state);
	    break;
    case KEY_RESIZE:
      render(*state);
  }
}

int main() {
  setlocale(LC_ALL, "");
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  struct state state = {
    .run = true,
    .tree = newpad(BUFSIZ, COLS),
    .view = newpad(BUFSIZ, COLS)
  };

  update(&state);
  render(state);
	while (state.run) {
	  event(&state);
	  napms(33);
	};
	endwin();
	return 0;
}
