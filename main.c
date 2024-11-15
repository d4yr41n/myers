#include <stdbool.h>
#include <stdlib.h>
#include <dirent.h>
#include <curses.h>
#include <sys/stat.h>
#include <unistd.h>

struct state {
  bool run;
  int index;
  int count;
  int tree_width;
  WINDOW *tree;
  WINDOW *view;
  char *files[BUFSIZ];
  bool dirs[BUFSIZ];
};

void render(struct state state) {
  werase(state.view);
  int i = 0;
  if (state.dirs[state.index]) {
    DIR *d = opendir(state.files[state.index]);
    if (d != NULL) {
      struct dirent *dir;
      readdir(d);
      readdir(d);
      while ((dir = readdir(d)) != NULL && i < LINES) {
        wprintw(state.view, "%s\n", dir->d_name);
        i++;
      }
    }
  } else {
    FILE *fp = fopen(state.files[state.index], "r");
    ssize_t read;
    char *line = NULL;
    size_t l = 0;
    if (fp != NULL) {
      while ((read = getline(&line, &l, fp)) != -1 && i < LINES) {
        waddnstr(state.view, line, COLS - state.tree_width);
        i++;
      }
    }
  }
  wrefresh(state.view);

  werase(state.tree);
  for (int i = 0; i < state.count; i++) {
    if (i == state.index) {
      wattrset(state.tree, A_REVERSE);
      waddnstr(state.tree, state.files[i], state.tree_width);
      waddstr(state.tree, "\n");
      wattroff(state.tree, A_REVERSE);
    }
    else
      wprintw(state.tree, "%s\n", state.files[i]);
  }
  wrefresh(state.tree);
}

void update(struct state *state) {
  int i = 0, length = 0;
  state->tree_width = 0;
  state->index = 0;
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
  state->tree = newwin(LINES, state->tree_width, 0, 0);
  state->view = newwin(LINES, COLS - state->tree_width - 4, 0, state->tree_width);
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
      render(*state);
      break;
    case 'k':
      if (!state->index)
        state->index = state->count - 1;
      else
	      state->index--;
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
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  struct state state = {
    .run = true
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
