#include <errno.h>
#include <curses.h>
#include <menu.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>

enum Mode {
  CREATE,
  DELETE,
  RENAME,
  SEARCH,
  READ
};

struct state {
  ITEM **menu_items;
  MENU *menu;
  int entry_count;
  char *editor;
  char input[256];
  enum Mode mode;
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
  struct stat stbuf;
  stat(name, &stbuf);

  if (S_ISDIR(stbuf.st_mode)) {
    init_state(name, state);
    render(state);
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


int remove_entry(const char *path, const struct stat *stbuf, int type, struct FTW *ftwb) {
  return remove(path);
}


bool input(struct state *state) {
  MEVENT event;
  int length = strlen(state->input);
  if (state->mode != READ) {
    clear_line(LINES - 1);
    char *prompt;
    switch (state->mode) {
      case DELETE:
        prompt = "Are you sure? (Y/n)";
        break;
      case CREATE:
      case RENAME:
        prompt = "Enter name";
        break;
      case SEARCH:
        prompt = "Search";
        break;
    }
    mvprintw(LINES - 1, 0, "%s: %s\n", prompt, state->input);
    char c = getch();
    switch (c) {
      case 27:
        state->mode = READ;
        state->input[0] = '\0';
        break;
      case 10:
        switch (state->mode) {
          case CREATE:
            if (state->input[length - 1] == '/')
              mkdir(state->input, 0700);
            else
              fopen(state->input, "a");
            state->input[0] = '\0';
            state->mode = READ;
            init_state(".", state);
            render(state);
            break;
          case RENAME:
            rename(item_name(current_item(state->menu)), state->input);
            init_state(".", state);
            render(state);
            set_menu_pattern(state->menu, state->input);
            state->input[0] = '\0';
            state->mode = READ;
            break;
          case SEARCH:
            set_menu_pattern(state->menu, state->input);
            state->mode = READ;
            break;
          case DELETE:
            if (state->input[1] == '\0' &&
                (state->input[0] == 'y' || state->input[0] == 'Y')) {
              nftw(
                item_name(current_item(state->menu)),
                remove_entry,
                10,
                FTW_PHYS | FTW_DEPTH | FTW_MOUNT
              );
              init_state(".", state);
              render(state);
            }
            state->input[0] = '\0';
            state->mode = READ;
            break;
        }
        break;
      case 127:
        state->input[length - 1] = '\0';
        break;
      default:
        if (length < 255) {
          state->input[length] = c;
          state->input[length + 1] = '\0';
        }
    }
  } else {
    switch (getch()) {
      // Manual mouse handling seems smoother
      case KEY_MOUSE:
        if (getmouse(&event) == OK) {
          if (event.bstate & BUTTON4_PRESSED) {
            clear_line(LINES - 1);
            menu_driver(state->menu, REQ_UP_ITEM);
          } else if (event.bstate & BUTTON5_PRESSED) {
            clear_line(LINES - 1);
            menu_driver(state->menu, REQ_DOWN_ITEM);
          }
        }
        break;
      case 'd':
        state->input[0] = '\0';
        state->mode = DELETE;
        break;
      case 'c':
        state->input[0] = '\0';
        state->mode = CREATE;
        break;
      case 'r':
        state->input[0] = '\0';
        state->mode = RENAME;
        break;
      case '/':
        state->input[0] = '\0';
        state->mode = SEARCH;
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
      case 'n':
        if (state->input[0] != '\0')
          menu_driver(state->menu, REQ_NEXT_MATCH);
        break;
      case 'p':
        if (state->input[0] != '\0')
          menu_driver(state->menu, REQ_PREV_MATCH);
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
    .mode = READ
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
