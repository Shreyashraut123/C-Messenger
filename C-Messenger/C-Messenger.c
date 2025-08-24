
#include <gtk/gtk.h>
#include <string.h>
#include <time.h>

#define MAX_MESSAGES 100
#define MAX_USERNAME 50
#define MAX_USERS 10

typedef struct {
    char username[MAX_USERNAME];
    int user_id;
    int active;
} User;

typedef struct {
    User sender;
    char message[512];
    time_t timestamp;
} ChatMessage;

ChatMessage messages[MAX_MESSAGES];
int message_count = 0;
User users[MAX_USERS];
int current_user_index = -1;

GtkWidget *text_view;
GtkWidget *entry;

void update_chat_display() {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_text_buffer_set_text(buffer, "", -1);

    GtkTextIter iter;
    gtk_text_buffer_get_iter_at_offset(buffer, &iter, 0);

    for (int i = 0; i < message_count; ++i) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&messages[i].timestamp));

        char formatted[1024];
        snprintf(formatted, sizeof(formatted), "[%s] %s: %s\n", time_str, messages[i].sender.username, messages[i].message);

        gtk_text_buffer_insert(buffer, &iter, formatted, -1);
    }
}

void send_message(GtkWidget *widget, gpointer data) {
    const char *input = gtk_entry_get_text(GTK_ENTRY(entry));

    if (strlen(input) == 0 || current_user_index == -1) return;

    if (message_count >= MAX_MESSAGES) message_count = 0;

    strcpy(messages[message_count].sender.username, users[current_user_index].username);
    messages[message_count].sender.user_id = users[current_user_index].user_id;
    messages[message_count].timestamp = time(NULL);
    strncpy(messages[message_count].message, input, sizeof(messages[message_count].message));

    message_count++;
    gtk_entry_set_text(GTK_ENTRY(entry), "");
    update_chat_display();
}

void on_add_user(GtkWidget *widget, gpointer data) {
    if (current_user_index >= MAX_USERS - 1) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "New User",
        GTK_WINDOW(data),
        GTK_DIALOG_MODAL,
        "OK", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry_user = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_user), "Enter username");
    gtk_container_add(GTK_CONTAINER(content), entry_user);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        const char *username = gtk_entry_get_text(GTK_ENTRY(entry_user));
        if (strlen(username) > 0) {
            int i;
            for (i = 0; i < MAX_USERS; i++) {
                if (!users[i].active) {
                    strncpy(users[i].username, username, MAX_USERNAME);
                    users[i].user_id = rand() % 1000 + 1;
                    users[i].active = 1;
                    current_user_index = i;
                    break;
                }
            }
        }
    }

    gtk_widget_destroy(dialog);
}

void on_switch_user(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Switch User",
        GTK_WINDOW(data),
        GTK_DIALOG_MODAL,
        "OK", GTK_RESPONSE_OK,
        NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *combo = gtk_combo_box_text_new();

    for (int i = 0; i < MAX_USERS; ++i) {
        if (users[i].active) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), users[i].username);
        }
    }

    gtk_container_add(GTK_CONTAINER(content), combo);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
        int idx = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
        if (idx >= 0 && idx < MAX_USERS && users[idx].active) {
            current_user_index = idx;
        }
    }

    gtk_widget_destroy(dialog);
}

void save_chat_to_file() {
    FILE *file = fopen("chat_log.txt", "w");
    if (!file) return;

    for (int i = 0; i < message_count; ++i) {
        fprintf(file, "%s|%d|%ld|%s\n",
                messages[i].sender.username,
                messages[i].sender.user_id,
                messages[i].timestamp,
                messages[i].message);
    }

    fclose(file);
}

void on_destroy(GtkWidget *widget, gpointer data) {
    save_chat_to_file();
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Chat Application");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 5);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 5);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *send_btn = gtk_button_new_with_label("Send");
    GtkWidget *add_user_btn = gtk_button_new_with_label("Add User");
    GtkWidget *switch_user_btn = gtk_button_new_with_label("Switch User");

    gtk_box_pack_start(GTK_BOX(hbox), add_user_btn, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), switch_user_btn, FALSE, FALSE, 5);
    gtk_box_pack_end(GTK_BOX(hbox), send_btn, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

    g_signal_connect(send_btn, "clicked", G_CALLBACK(send_message), NULL);
    g_signal_connect(add_user_btn, "clicked", G_CALLBACK(on_add_user), window);
    g_signal_connect(switch_user_btn, "clicked", G_CALLBACK(on_switch_user), window);
    g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), NULL);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
