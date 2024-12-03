#include <errno.h>
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
  char input[256];
  bool mode;
  WINDOW *main;
};

void clear_line(int y) {
  move(y, 0);
  clrtoeol();
}

void render(struct state *state) {
  clear_line(0);
  mvaddstr(0, 0, realpath(".", NULL));

  state->menu = new_menu(state->menu_items);
  set_menu_sub(state->menu, state->main);
  set_menu_mark(state->menu, NULL);
  set_menu_format(state->menu, LINES - 4, 1);
  post_menu(state->menu);
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

  chdir(dirname);

  int skip = 1 + (entry_count > 2);
  char *name;
  struct stat stbuf;
  for (int i = 0; i < entry_count - skip; i++) {
    name = entries[i + skip]->d_name;
    stat(name, &stbuf);
    if (S_ISDIR(stbuf.st_mode))
      strcat(name, "/");
    state->menu_items[i] = new_item(name, NULL);
  }

  return true;
}

void enter(const char *name, struct state *state) {
  if (init_state(name, state))
    render(state);

  // if (errno == ENOTENT)
  //   if (state->editor != NULL) {
  //     char *cmd = malloc(strlen(state->editor) + 1);
  //     strcpy(cmd, state->editor);
  //     strcat(cmd, " ");
  //     strcat(cmd, name);
  //     def_prog_mode();
  //     endwin();
  //     system(cmd);
  //     reset_prog_mode();
  //     refresh();
  //   } else {
  //     mvaddstr(LINES - 1, 0, "EDITOR is not set");
  //   }
  // }
}


bool input(struct state *state) {
  MEVENT event;
  int length = strlen(state->input);
  if (state->mode) {
    char c = getch();
    if (c == 27) {
      state->mode = false;
      state->input[0] = '\0';
      clear_line(LINES - 1);
    } else if (c == 10) {
      clear_line(LINES - 1);
      if (state->input[length - 1] == '/')
        mkdir(state->input, 0700);
      else
        fopen(state->input, "a");
      state->input[0] = '\0';
      state->mode = false;
      init_state(".", state);
      render(state);
    } else if (c == 127) {
      state->input[length - 1] = '\0';
      clear_line(LINES - 1);
      mvaddstr(LINES - 1, 0, state->input);
    } else if (length < 255) {;
      state->input[length] = c;
      state->input[length + 1] = '\0';
      clear_line(LINES - 1);
      mvaddstr(LINES - 1, 0, state->input);
    }
  } else {
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
      case 'd':
        remove(item_name(current_item(state->menu)));
        init_state(".", state);
        render(state);
        break;
      case 'c':
        state->mode = true;
        break;
      case 27:
      case 'q':
        return false;
      case KEY_LEFT:
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
      case KEY_RIGHT:
      case 'l':
      case 10:
        enter(item_name(current_item(state->menu)), state);
        break;
    }
  }
  return true;
}

int main(int argc, char *argv[]) {
  char *usage = "Usage: myers [-vh] [path]\n";
  int opt;
  while ((opt = getopt(argc, argv, "vh")) != -1) {
    switch (opt) {
      case 'v':
        printf("myers 0.0.0\n");
        return 0;
      default:
  			fprintf(stderr, "%s", usage);
  			return 1;
    }
  }

  if (argc > 2) {
		fprintf(stderr, "%s", usage);
		return 1;
  }

  char *path = ".";

  if (argc == 2) {
    if (argv[1] != NULL) {
      path = argv[1];
    }
  }

  struct state state = {
    .editor = getenv("EDITOR"),
    .entry_count = 0,
    .mode = false
  };

  if (init_state(path, &state)) {
    printf("\033[H\033[J");
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, true);
    ESCDELAY = 0;
    mousemask(ALL_MOUSE_EVENTS, NULL);
    state.main = subwin(stdscr, LINES - 4, COLS, 2, 0);
    render(&state);
    while (input(&state));
    free_state(&state);
    endwin();
    return 0;
  }

	switch (errno) {
	  case EACCES:
  		printf("myers: %s: Permission denied\n", path);
  		break;
  	case ENOENT: 
  		printf("myers: %s: No such directory\n", path);
  		break;
  	case ENOTDIR: 
  		printf("myers: %s: Not a directory\n", path);
  		break;
  }
  return 1;
}
