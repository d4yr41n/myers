#include <curses.h>
#include <menu.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

struct state {
  ITEM **menu_items;
  MENU *menu;
  int entry_count;
  char *editor;
  WINDOW *main;
};

void clear_line(int y) {
  move(y, 0);
  clrtoeol();
}

void free_state(struct state *state) {
  unpost_menu(state->menu);
  free_menu(state->menu);
  for (int i = 0; i < state->entry_count; i++)
    free_item(state->menu_items[i]);
  free(state->menu_items);
}

bool init_state(const char *dirname, struct state *state) {
  struct dirent **entries;
  int entry_count = scandir(dirname, &entries, NULL, alphasort);
  if (entry_count == -1)
    return false;

  if (state->entry_count)
    free_state(state);

  state->menu_items = calloc(entry_count + 1, sizeof(ITEM *));
  state->entry_count = entry_count;

  for (int i = 0; i < entry_count; i++) {
    state->menu_items[i] = new_item(entries[i]->d_name, NULL);
  }

  clear_line(0);
  mvaddstr(0, 0, realpath(dirname, NULL));

  state->menu = new_menu(state->menu_items);
  set_menu_mark(state->menu, NULL);
  set_menu_format(state->menu, LINES - 2, 1);
  set_menu_sub(state->menu, state->main);
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
    if (state->editor != NULL) {
      char *cmd = malloc(strlen(state->editor) + 1);
      strcpy(cmd, state->editor);
      strcat(cmd, " ");
      strcat(cmd, name);
      def_prog_mode();
      endwin();
      system(cmd);
      reset_prog_mode();
      refresh();
    } else {
      mvaddstr(LINES - 1, 0, "EDITOR is not set");
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
    case 'h':
      enter("..", state);
      break;
    case KEY_DOWN:
    case 'j':
      clear_line(LINES - 1);
      menu_driver(state->menu, REQ_DOWN_ITEM);
      break;
    case KEY_UP:
    case 'k':
      clear_line(LINES - 1);
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
    .entry_count = 0,
    .main = subwin(stdscr, LINES - 2, COLS, 1, 0)
  };
  init_state(".", &state);
  while (input(&state));
  free_state(&state);
  endwin();
}
