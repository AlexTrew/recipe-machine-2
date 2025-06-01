#include <gtk/gtk.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

GtkWidget *list_box;
GtkWidget *window;
GtkWidget *up_button;
char current_path[4096] = ".";
char initial_path[4095] = ".";

void on_entry_button_clicked(GtkButton *button, gpointer user_data);
void populate_list(const char *path);
void set_button_font(GtkWidget *btn, int pt_size);
int open_subprocess(const char *command, const char *arg);
int handle_file(const char *file_path);
void on_up_dir_button_clicked(GtkButton *button, gpointer user_data);
void on_window_destroy();


void set_button_font(GtkWidget *btn, int pt_size) {
    GtkStyleContext *context = gtk_widget_get_style_context(btn);
    GtkCssProvider *provider = gtk_css_provider_new();
    char css[64];
    snprintf(css, sizeof(css), "button { font-size: %dpt; }", pt_size);
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}

int open_subprocess(const char *command, const char *arg) {
    pid_t pid = fork();
    if (pid == 0) {
        execl(command, command, arg, (char *)NULL);
    } else if (pid < 0) {
        perror("fork failed");
        return -1;
    }
    return 0;
}

int handle_file(const char *file_path) {
    char* file_type = strrchr(file_path, '.');
    pid_t pid;
    if (file_type && strcmp(file_type, ".txt") == 0) {
        open_subprocess("/usr/bin/nedit", file_path);
    } else if (file_type && (strcmp(file_type, ".png") == 0 || 
               strcmp(file_type, ".jpg") == 0 || 
               strcmp(file_type, ".jpeg") == 0)) {
        open_subprocess("/usr/bin/feh", file_path);
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

void populate_list(const char *path) {
    // Clear the current list box
    GList *iter;
    GList *children = gtk_container_get_children(GTK_CONTAINER(list_box));
    for (iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    // re populate the list box with current directory contents
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        GtkWidget *btn = gtk_button_new_with_label(entry->d_name);
        gtk_widget_set_size_request(btn, 400, 50);
        set_button_font(btn, 14);
        g_object_set_data(G_OBJECT(btn), "full_path", g_strdup(full_path));
        g_signal_connect(btn, "clicked", G_CALLBACK(on_entry_button_clicked), NULL);
        gtk_box_pack_start(GTK_BOX(list_box), btn, FALSE, FALSE, 5);
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
    gtk_widget_destroy(up_button);
    gtk_widget_destroy(list_box);
    gtk_widget_destroy(window);
    gtk_main_quit();
}

int main(int argc, char *argv[]) {

    //parse args
    if (argc == 2) {
        strncpy(current_path, argv[1], sizeof(current_path) - 1);
        current_path[sizeof(current_path) - 1] = '\0'; 
        strncpy(initial_path, argv[1], sizeof(initial_path) - 1);

    } else if (argc != 1){
        fprintf(stderr, "Usage: %s <optional>[path]\n", argv[1]);
        return 1;
    }

    // initialise window and GTK
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), current_path);
    gtk_window_set_default_size(GTK_WINDOW(window), 599, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // initialize the ui elements
    up_button = gtk_button_new_with_label("Back");
    set_button_font(up_button, 16);
    gtk_widget_set_size_request(up_button, 100, 50);
    g_signal_connect(up_button, "clicked", G_CALLBACK(on_up_dir_button_clicked), NULL);

    list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    //scroll bar
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_container_add(GTK_CONTAINER(scroll), list_box);
    GtkWidget *vscrollbar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(scroll));
    if (vscrollbar) {
        gtk_widget_set_size_request(vscrollbar, 24, -1); // 24px wide
    }

    // Create the main layout
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), up_button, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    populate_list(current_path);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
