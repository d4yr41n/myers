#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <curses.h>
#include <sys/stat.h>

struct state {
  bool run;
  char pwd[BUFSIZ];
  char file[BUFSIZ];
  int index;
  int count;
  int tree_width;
  WINDOW *tree;
  WINDOW *view;
  char *files[BUFSIZ];
  bool dirs[BUFSIZ];
};

void render(struct state state) {
  werase(state.tree);
  wprintw(state.tree, "%s\n", state.pwd);
  for (int i = 0; i < state.count; i++) {
    if (i == state.index) {
      wattron(state.tree, A_REVERSE);
      wprintw(state.tree, "%s\n", state.files[i]);
      wattroff(state.tree, A_REVERSE);
    }
    else
      wprintw(state.tree, "%s\n", state.files[i]);
  }
  pnoutrefresh(state.tree, 0, 0, 0, 0, LINES - 1, COLS - 1);
  werase(state.view);
  int m = COLS - state.tree_width - 1;
  if (state.dirs[state.index]) {
    DIR *d = opendir(state.file);
    struct dirent *dir;
    readdir(d);
    readdir(d);
    while ((dir = readdir(d)) != NULL) {
      wprintw(state.view, "%s\n", dir->d_name);
    }
  } else {
    FILE *fp = fopen(state.file, "r");
    ssize_t read;
    char *line = NULL;
    size_t l = 0;
    if (fp != NULL) {
      while ((read = getline(&line, &l, fp)) != -1) {
          wprintw(state.view, "%s", line);
      }
    }
  }
  pnoutrefresh(state.view, 0, 0, 0, state.tree_width + 1, LINES - 1, COLS - 1);
  doupdate();
}

void update_view(struct state *state) {
  strcpy(state->file, state->pwd);
  int length = strlen(state->file);
  if (state->file[length - 1] != '/') {
    state->file[length] = '/';
    state->file[length + 1] = '\0';
  }
  strcat(state->file, state->files[state->index]);
}

void update_tree(struct state *state) {
  int i = 0, length = 0;
  state->tree_width = strlen(state->pwd);
  state->index = 0;
  DIR *d = opendir(state->pwd);
  struct dirent *dir;
  struct stat stbuf;
  readdir(d);
  if (!strcmp(state->pwd, "/"))
    readdir(d);
  while ((dir = readdir(d)) != NULL) {
    state->files[i] = dir->d_name;
    length = strlen(dir->d_name);
    stat(dir->d_name, &stbuf);
    if (S_ISDIR(stbuf.st_mode)) {
      state->dirs[i] = true;
    }
    if (length > state->tree_width)
      state->tree_width = length;
    i++;
  }
  state->count = i;
}

void event(struct state *state) {
  char path[BUFSIZ];
  char cmd[BUFSIZ];
  switch (wgetch(state->tree)) {
    case 'q':
      erase();
      state->run = false;
      break;
    case 'j':
      if (state->index == state->count - 1)
        state->index = 0;
      else
	      state->index++;
	    update_view(state);
      render(*state);
      break;
    case 'k':
      if (!state->index)
        state->index = state->count - 1;
      else
	      state->index--;
	    update_view(state);
      render(*state);
      break;
    case 10:
      break;
    case KEY_RESIZE:
      render(*state);
  }
}

int main() {
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  struct state state = {
    .tree = newpad(BUFSIZ, BUFSIZ),
    .view = newpad(BUFSIZ, BUFSIZ),
    .run = true
  };

  realpath(".", state.pwd),
  update_tree(&state);
  update_view(&state);
  render(state);
	while (state.run) {
	  event(&state);
	  napms(33);
	};
	endwin();
	return 0;
}
