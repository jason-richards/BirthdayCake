#include <adwaita.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#define APP_ID "org.gnome.BirthdayCountdown"

static std::string calculate_days_until_birthday(const std::string& date_str) {
    if (date_str.empty()) {
        return "Double-click box to set birthday";
    }

    std::tm tm = {};
    std::istringstream ss(date_str);
    
    ss >> std::get_time(&tm, "%Y-%m-%d");
    if (ss.fail()) {
        return "Invalid date format! Use YYYY-MM-DD.";
    }

    int birth_month = tm.tm_mon;
    int birth_mday = tm.tm_mday;

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm today_tm = *std::localtime(&now_c);

    std::tm target_tm = today_tm;
    target_tm.tm_mon = birth_month;
    target_tm.tm_mday = birth_mday;
    target_tm.tm_hour = 0;
    target_tm.tm_min = 0;
    target_tm.tm_sec = 0;

    today_tm.tm_hour = 0;
    today_tm.tm_min = 0;
    today_tm.tm_sec = 0;

    std::time_t today_time = std::mktime(&today_tm);
    std::time_t target_time = std::mktime(&target_tm);

    if (target_time < today_time) {
        target_tm.tm_year += 1;
        target_time = std::mktime(&target_tm);
    }

    if (target_time == today_time) {
        return "🎉 Happy Birthday! It's today!";
    }

    double diff_seconds = std::difftime(target_time, today_time);
    long days = static_cast<long>(diff_seconds / (60 * 60 * 24));

    return std::to_string(days) + " days until your birthday!";
}

struct AppWidgets {
    GtkWidget *entry;
    GtkWidget *result_label;
    GSettings *settings;
};

static void lock_entry(GtkWidget *entry) {
    // Lock by removing focus capability rather than setting editable to FALSE
    gtk_widget_set_can_focus(entry, FALSE);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
}

static void unlock_entry(GtkWidget *entry) {
    // Unlock by restoring editability and focus
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    gtk_widget_set_can_focus(entry, TRUE);
    gtk_widget_grab_focus(entry);
    gtk_editable_set_position(GTK_EDITABLE(entry), -1);
}

static void save_and_calculate(AppWidgets *widgets) {
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(widgets->entry));
    const char *text = gtk_entry_buffer_get_text(buffer);

    g_settings_set_string(widgets->settings, "birthday", text);

    std::string result = calculate_days_until_birthday(text);
    gtk_label_set_text(GTK_LABEL(widgets->result_label), result.c_str());

    lock_entry(widgets->entry);
}

static void on_entry_double_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data) {
    if (n_press == 2) {
        auto *entry = GTK_WIDGET(user_data);
        unlock_entry(entry);
    }
}

static void on_calculate_clicked(GtkWidget *widget, gpointer user_data) {
    auto *widgets = static_cast<AppWidgets*>(user_data);
    save_and_calculate(widgets);
}

static void on_activate(AdwApplication *app, gpointer user_data) {
    GtkWidget *window = adw_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "Birthday Countdown");
    gtk_window_set_default_size(GTK_WINDOW(window), 360, 260);

    GtkWidget *header = adw_header_bar_new();

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(vbox), header);

    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_top(content_box, 24);
    gtk_widget_set_margin_bottom(content_box, 24);
    gtk_widget_set_margin_start(content_box, 24);
    gtk_widget_set_margin_end(content_box, 24);
    gtk_box_append(GTK_BOX(vbox), content_box);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Double-click to set YYYY-MM-DD");
    gtk_widget_set_tooltip_text(entry, "Double-click to edit your birthday");
    gtk_box_append(GTK_BOX(content_box), entry);

    // Initial state: locked
    lock_entry(entry);

    // Attach click gesture with CAPTURE phase so it intercepts before GtkEntry's internal handlers
    GtkGesture *gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_PRIMARY);
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(gesture), GTK_PHASE_CAPTURE);
    g_signal_connect(gesture, "pressed", G_CALLBACK(on_entry_double_clicked), entry);
    gtk_widget_add_controller(entry, GTK_EVENT_CONTROLLER(gesture));

    GtkWidget *button = gtk_button_new_with_label("Calculate Days");
    gtk_widget_add_css_class(button, "suggested-action");
    gtk_box_append(GTK_BOX(content_box), button);

    GtkWidget *result_label = gtk_label_new("");
    gtk_widget_add_css_class(result_label, "title-2");
    gtk_label_set_wrap(GTK_LABEL(result_label), TRUE);
    gtk_box_append(GTK_BOX(content_box), result_label);

    GSettings *settings = g_settings_new(APP_ID);
    auto *widgets = new AppWidgets{ entry, result_label, settings };

    g_autofree char *saved_date = g_settings_get_string(settings, "birthday");
    if (saved_date && strlen(saved_date) > 0) {
        gtk_entry_buffer_set_text(gtk_entry_get_buffer(GTK_ENTRY(entry)), saved_date, -1);
        std::string result = calculate_days_until_birthday(saved_date);
        gtk_label_set_text(GTK_LABEL(result_label), result.c_str());
    } else {
        gtk_label_set_text(GTK_LABEL(result_label), "Double-click box to set birthday");
    }

    g_signal_connect(button, "clicked", G_CALLBACK(on_calculate_clicked), widgets);
    g_signal_connect(entry, "activate", G_CALLBACK(on_calculate_clicked), widgets);

    adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), vbox);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
    g_autoptr(AdwApplication) app = adw_application_new(APP_ID, G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    return g_application_run(G_APPLICATION(app), argc, argv);
}
