#include <curses.h>
#include <menu.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

struct state {
  ITEM **entries;
  MENU *menu;
  int entry_count;
  char *editor;
};

void free_state(struct state *state) {
  unpost_menu(state->menu);
  free_menu(state->menu);
  for (int i = 0; i < state->entry_count; i++)
    free_item(state->entries[i]);
  free(state->entries);
}

bool init_state(const char *dirname, struct state *state) {
  DIR *dir = opendir(dirname);
  if (dir == NULL)
    return false;

  if (state->entry_count)
    free_state(state);

  state->entry_count = 0;

  while (readdir(dir) != NULL) {
    state->entry_count++;
  }

  state->entries = calloc(state->entry_count + 1, sizeof(ITEM *));

  dir = opendir(dirname);
  for (int i = 0; i < state->entry_count; i++)
    state->entries[i] = new_item(readdir(dir)->d_name, NULL);

  state->menu = new_menu(state->entries);
  set_menu_mark(state->menu, NULL);
  set_menu_format(state->menu, LINES, 1);
  post_menu(state->menu);
  return true;
}

void enter(const char *name, struct state *state) {
  struct stat stbuf;
  stat(name, &stbuf);
  if (S_ISDIR(stbuf.st_mode)) {
    if (init_state(name, state))
      chdir(name);
  } else {
    if (state->editor) {
      char *cmd = malloc(strlen(state->editor) + 1);
      strcpy(cmd, state->editor);
      strcat(cmd, " ");
      strcat(cmd, name);
      def_prog_mode();
      endwin();
      system(cmd);
      reset_prog_mode();
      refresh();
    }
  }
}

bool input(struct state *state) {
  bool run = true;
  MEVENT event;
  switch (getch()) {
    // Manual mouse handling seems smoother
    case KEY_MOUSE:
      if (getmouse(&event) == OK) {
        if (event.bstate & BUTTON4_PRESSED)
          menu_driver(state->menu, REQ_UP_ITEM);
        else if (event.bstate & BUTTON5_PRESSED)
          menu_driver(state->menu, REQ_DOWN_ITEM);
      }
      break;
    case 'q':
      run = false;
      break;
    case KEY_DOWN:
    case 'h':
      enter("..", state);
      break;
    case 'j':
      menu_driver(state->menu, REQ_DOWN_ITEM);
      break;
    case KEY_UP:
    case 'k':
      menu_driver(state->menu, REQ_UP_ITEM);
      break;
    case 'l':
    case 10:
      enter(item_name(current_item(state->menu)), state);
      break;
  }
  return run;
}

int main() {
  printf("\033[H\033[J");
  initscr();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, true);
  mousemask(ALL_MOUSE_EVENTS, NULL);
  struct state state = {
    .editor = getenv("EDITOR"),
    .entry_count = 0
  };
  init_state(".", &state);
  while (input(&state));
  free_state(&state);
  endwin();
}
