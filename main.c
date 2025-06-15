#include <gtk/gtk.h>
#include <dirent.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

GtkWidget *list_box;
GtkWidget *window;

GtkWidget *scroll;
GtkWidget *vscrollbar;
GtkCssProvider *provider = NULL;
GtkWidget *vbox;
GtkWidget *up_button;

size_t MAX_BUF_SIZE = 4096;

char current_path[4096] = ".";
char initial_path[4095] = ".";


void create_button(char* full_path, char* label);
void on_entry_button_clicked(GtkButton *button, gpointer user_data);
void on_refresh_button_clicked(GtkButton *button, gpointer user_data);
void populate_list(const char *path);
void set_button_font(GtkWidget *btn, int pt_size);
int open_subprocess(const char *command, const char *arg);
int handle_file(const char *file_path);
void on_up_dir_button_clicked(GtkButton *button, gpointer user_data);
void on_window_destroy();
void on_scroll_up_clicked(GtkButton *button, gpointer user_data);
void on_scroll_down_clicked(GtkButton *button, gpointer user_data);


void set_button_font(GtkWidget *btn, int pt_size) {
    GtkStyleContext *context = gtk_widget_get_style_context(btn);
    char css[64];
    if (!provider) {
        provider = gtk_css_provider_new();
        snprintf(css, sizeof(css), "button { font-size: %dpt; }", pt_size);
        gtk_css_provider_load_from_data(provider, css, -1, NULL);
    }
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
}

int open_subprocess(const char *command, const char *arg) {
    pid_t pid = fork();
    if (pid > 0) {
      // let the browser process sleep for a second to stop a user opening multiple subprocesses
      sleep(1);
    }
    else if (pid == 0) {
        execl(command, command, arg, (char *)NULL);
	_exit(-1);
    } else {
        perror("fork failed");
        return -1;
    }
    return 0;
}

void clear_button_list() {
    GList *children = gtk_container_get_children(GTK_CONTAINER(list_box));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
}

int handle_file(const char *file_path) {
    char* file_type = strrchr(file_path, '.');
    if (file_type && strcmp(file_type, ".txt") == 0) {
        open_subprocess("/usr/bin/nedit", file_path);
    } else if (file_type && (strcmp(file_type, ".png") == 0 || 
               strcmp(file_type, ".jpg") == 0 || 
               strcmp(file_type, ".jpeg") == 0)) {
        open_subprocess("/usr/bin/eog", file_path);
    } else if (file_type && strcmp(file_type, ".pdf") == 0) {
        open_subprocess("/usr/bin/xpdf", file_path);
    } else {
        g_print("File type not supported: %s\n, file type: %s\n", file_path, file_type);
    }
    return 0;
}

void on_entry_button_clicked(GtkButton *button , gpointer user_data) {
    const char *full_path = g_object_get_data(G_OBJECT(button), "full_path");
    DIR *dir = opendir(full_path);
    if (dir) {
        closedir(dir);
        strcpy(current_path, full_path);
        populate_list(current_path);
        gtk_window_set_title(GTK_WINDOW(window), current_path);
    } else {
        handle_file(full_path);
    }
}


void on_refresh_button_clicked(GtkButton *button, gpointer user_data){
    populate_list(current_path);
}


void create_button(char* full_path, char* label) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    gtk_widget_set_size_request(btn, 400, 50);
    set_button_font(btn, 14);
    g_object_set_data(G_OBJECT(btn), "full_path", g_strdup(full_path));
    g_signal_connect(btn, "clicked", G_CALLBACK(on_entry_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(list_box), btn, FALSE, FALSE, 5);
}

void populate_list(const char *path) {// Clear the current list box
    printf("current directory %s\n", path);
    clear_button_list();

    // re populate the list box with current directory contents
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {

	if (strnlen(entry->d_name, MAX_BUF_SIZE) == MAX_BUF_SIZE){
	    printf("Unterminated filename! exiting...\n");
	    return;
	}
	
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

	char full_path[MAX_BUF_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
	
	// remove the file extension
	char* file_extension = strrchr(entry->d_name, '.');
	char file_without_extension[4096];

	/*check that the last '.' isnt the first char ie a hidden file*/
	if (file_extension && entry->d_name != file_extension){
	    size_t label_len = strlen(entry->d_name) - strlen(file_extension);
	    strncpy(file_without_extension, entry->d_name, label_len);
	    file_without_extension[label_len] = '\0';
	    create_button(full_path, file_without_extension);
	}
	else{
	    create_button(full_path, entry->d_name);
	}
    }
    closedir(dir);
    gtk_widget_show_all(list_box);
}


void on_up_dir_button_clicked(GtkButton *button, gpointer user_data) {
    // Prevent navigating above initial_path
    if (strcmp(current_path, initial_path) == 0) {
        // Already at the root allowed by user
        return;
    }
    char *slash = strrchr(current_path, '/');
    if (slash && slash != current_path) {
        *slash = '\0';
    } else {
        strcpy(current_path, initial_path);
    }
    // If after going up, we're above initial_path, reset to initial_path
    if (strlen(current_path) < strlen(initial_path) || strncmp(current_path, initial_path, strlen(initial_path)) != 0) {
        strcpy(current_path, initial_path);
    }
    populate_list(current_path);
    gtk_window_set_title(GTK_WINDOW(window), current_path);
}

void on_window_destroy() {
    if (provider) {
        g_object_ref_sink(G_OBJECT(provider));
        g_object_unref(provider);
    }
    clear_button_list();
    gtk_main_quit();
}

void on_scroll_up_clicked(GtkButton *button, gpointer user_data) {
    GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scroll));
    gdouble value = gtk_adjustment_get_value(vadj);
    gdouble step = gtk_adjustment_get_step_increment(vadj);
    gtk_adjustment_set_value(vadj, value - step);
}

void on_scroll_down_clicked(GtkButton *button, gpointer user_data) {
    GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scroll));
    gdouble value = gtk_adjustment_get_value(vadj);
    gdouble step = gtk_adjustment_get_step_increment(vadj);
    gtk_adjustment_set_value(vadj, value + step);
}

int main(int argc, char *argv[]) {

    //parse args
    if (argc == 2) {
	// ensure args are null terminated
	size_t input_buf_limit = 4096;
	if (strnlen(argv[1], input_buf_limit) == input_buf_limit){
	    return -1;
	}
        strcpy(current_path, argv[1]);
        strcpy(initial_path, argv[1]);

    } else if (argc != 1){
        fprintf(stderr, "Usage: %s <optional>[path]\n", argv[1]);
        return 1;
    }

    // initialise window and GTK
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_fullscreen(GTK_WINDOW(window));

    gtk_window_set_title(GTK_WINDOW(window), "Recipe Machine 2");
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // initialize the ui elements
    up_button = gtk_button_new_with_label("🡅");

    set_button_font(up_button, 16);
    gtk_widget_set_size_request(up_button, 100, 50);
    g_signal_connect(up_button, "clicked", G_CALLBACK(on_up_dir_button_clicked), NULL);

    list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    //scroll bar
    scroll = gtk_scrolled_window_new(NULL, NULL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(scroll), list_box);
    vscrollbar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(scroll));
    if (vscrollbar) {
        gtk_widget_set_size_request(vscrollbar, 24, -1); // 24px wide
    }

    // buttons on the right pane
    GtkWidget *right_side_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // refresh page button
    GtkWidget *refresh_button = gtk_button_new_with_label("↻");
    set_button_font(refresh_button, 24);
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_button_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(right_side_box), refresh_button, FALSE, FALSE, 2);

    // up and down scroll buttons
    GtkWidget *scroll_up_btn = gtk_button_new_with_label("▲");
    GtkWidget *scroll_down_btn = gtk_button_new_with_label("▼");
    gtk_widget_set_size_request(scroll_up_btn, 50, 40);
    gtk_widget_set_size_request(scroll_down_btn, 50, 40);
    set_button_font(scroll_up_btn, 18);
    set_button_font(scroll_down_btn, 18);
    g_signal_connect(scroll_up_btn, "clicked", G_CALLBACK(on_scroll_up_clicked), NULL);
    g_signal_connect(scroll_down_btn, "clicked", G_CALLBACK(on_scroll_down_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(right_side_box), scroll_up_btn, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(right_side_box), scroll_down_btn, FALSE, FALSE, 2);

    // Create a horizontal box for main content and scroll buttons
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), scroll, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), right_side_box, FALSE, FALSE, 5);

    // Main vertical layout
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), up_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    populate_list(current_path);
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
