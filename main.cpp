#include <adwaita.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#define APP_ID "org.gnome.BirthdayCountdown"

// Helper to compute Easter Sunday date for a given year (Anonymous Gregorian algorithm)
static std::tm get_easter_date(int year) {
    int a = year % 19;
    int b = year / 100;
    int c = year % 100;
    int d = b / 4;
    int e = b % 4;
    int f = (b + 8) / 25;
    int g = (b - f + 1) / 3;
    int h = (19 * a + b - d - g + 15) % 30;
    int i = c / 4;
    int k = c % 4;
    int l = (32 + 2 * e + 2 * i - h - k) % 7;
    int m = (a + 11 * h + 22 * l) / 451;
    int month = (h + l - 7 * m + 114) / 31;
    int day = ((h + l - 7 * m + 114) % 31) + 1;

    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    return tm;
}

// Helper to compute Thanksgiving (4th Thursday of November) for a given year
static std::tm get_thanksgiving_date(int year) {
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = 10; // November
    tm.tm_mday = 1;

    std::time_t t = std::mktime(&tm);
    std::tm *nov1 = std::localtime(&t);

    int first_thursday = 1 + ((4 - nov1->tm_wday + 7) % 7);
    int fourth_thursday = first_thursday + 21;

    tm.tm_mday = fourth_thursday;
    return tm;
}

static std::string calculate_days_until_birthday(const std::string& date_str) {
    if (date_str.empty()) {
        return "Double-click box to set date";
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
        return "🎉 It's today!";
    }

    double diff_seconds = std::difftime(target_time, today_time);
    long days = static_cast<long>(diff_seconds / (60 * 60 * 24));

    return std::to_string(days) + " days remaining!";
}

struct AppWidgets {
    GtkWidget *entry;
    GtkWidget *result_label;
    GSettings *settings;
};

static void lock_entry(GtkWidget *entry) {
    gtk_widget_set_can_focus(entry, FALSE);
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
}

static void unlock_entry(GtkWidget *entry) {
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE);
    gtk_widget_set_can_focus(entry, TRUE);
    gtk_widget_grab_focus(entry);
    gtk_editable_set_position(GTK_EDITABLE(entry), -1);
}

static void set_date_and_calculate(AppWidgets *widgets, const std::string& date_str, bool persist = true) {
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(widgets->entry));
    gtk_entry_buffer_set_text(buffer, date_str.c_str(), -1);

    if (persist) {
        g_settings_set_string(widgets->settings, "birthday", date_str.c_str());
    }

    std::string result = calculate_days_until_birthday(date_str);
    gtk_label_set_text(GTK_LABEL(widgets->result_label), result.c_str());

    lock_entry(widgets->entry);
}

static void save_and_calculate(AppWidgets *widgets) {
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(widgets->entry));
    const char *text = gtk_entry_buffer_get_text(buffer);
    set_date_and_calculate(widgets, text, true);
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

static void set_holiday(AppWidgets *widgets, int month, int day, bool is_easter = false, bool is_thanksgiving = false) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm today_tm = *std::localtime(&now_c);
    int current_year = today_tm.tm_year + 1900;

    std::tm holiday_tm = {};
    if (is_easter) {
        holiday_tm = get_easter_date(current_year);
    } else if (is_thanksgiving) {
        holiday_tm = get_thanksgiving_date(current_year);
    } else {
        holiday_tm.tm_year = current_year - 1900;
        holiday_tm.tm_mon = month - 1;
        holiday_tm.tm_mday = day;
    }

    std::time_t today_time = std::mktime(&today_tm);
    std::time_t holiday_time = std::mktime(&holiday_tm);

    if (holiday_time < today_time) {
        if (is_easter) {
            holiday_tm = get_easter_date(current_year + 1);
        } else if (is_thanksgiving) {
            holiday_tm = get_thanksgiving_date(current_year + 1);
        } else {
            holiday_tm.tm_year += 1;
        }
    }

    char date_buf[11];
    std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &holiday_tm);
    set_date_and_calculate(widgets, date_buf, false);
}

static void show_saved_birthday(AppWidgets *widgets) {
    g_autofree char *saved_date = g_settings_get_string(widgets->settings, "birthday");
    if (saved_date && strlen(saved_date) > 0) {
        set_date_and_calculate(widgets, saved_date, false);
    } else {
        // Fallback if no date is saved yet
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm today_tm = *std::localtime(&now_c);
        char date_buf[11];
        std::strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &today_tm);
        set_date_and_calculate(widgets, date_buf, false);
    }
}

static void on_activate(AdwApplication *app, gpointer user_data) {
    GtkWidget *window = adw_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "Event Countdown");
    gtk_window_set_default_size(GTK_WINDOW(window), 440, 360);

    GtkWidget *header = adw_header_bar_new();

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(vbox), header);

    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_margin_top(content_box, 16);
    gtk_widget_set_margin_bottom(content_box, 16);
    gtk_widget_set_margin_start(content_box, 20);
    gtk_widget_set_margin_end(content_box, 20);
    gtk_box_append(GTK_BOX(vbox), content_box);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Double-click to set YYYY-MM-DD");
    gtk_widget_set_tooltip_text(entry, "Double-click to edit target date");
    gtk_box_append(GTK_BOX(content_box), entry);

    lock_entry(entry);

    GtkGesture *gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_PRIMARY);
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(gesture), GTK_PHASE_CAPTURE);
    g_signal_connect(gesture, "pressed", G_CALLBACK(on_entry_double_clicked), entry);
    gtk_widget_add_controller(entry, GTK_EVENT_CONTROLLER(gesture));

    // Quick-Select Grid
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    GtkWidget *btn_bday = gtk_button_new_with_label("🎂 Birthday");
    GtkWidget *btn_xmas = gtk_button_new_with_label("🎄 Christmas");
    GtkWidget *btn_easter = gtk_button_new_with_label("🐰 Easter");
    GtkWidget *btn_thanksgiving = gtk_button_new_with_label("🍗 Thanksgiving");
    GtkWidget *btn_halloween = gtk_button_new_with_label("🎃 Halloween");

    // Layout buttons across 2 columns
    gtk_grid_attach(GTK_GRID(grid), btn_bday, 0, 0, 2, 1); // Spans across top row
    gtk_grid_attach(GTK_GRID(grid), btn_xmas, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_easter, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_thanksgiving, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), btn_halloween, 1, 2, 1, 1);

    gtk_box_append(GTK_BOX(content_box), grid);

    GtkWidget *button = gtk_button_new_with_label("Calculate Days");
    gtk_widget_add_css_class(button, "suggested-action");
    gtk_box_append(GTK_BOX(content_box), button);

    GtkWidget *result_label = gtk_label_new("");
    gtk_widget_add_css_class(result_label, "title-2");
    gtk_label_set_wrap(GTK_LABEL(result_label), TRUE);
    gtk_box_append(GTK_BOX(content_box), result_label);

    GSettings *settings = g_settings_new(APP_ID);
    auto *widgets = new AppWidgets{ entry, result_label, settings };

    // Button Signal Handlers
    g_signal_connect_swapped(btn_bday, "clicked", G_CALLBACK(show_saved_birthday), widgets);
    g_signal_connect_swapped(btn_xmas, "clicked", G_CALLBACK(+[](AppWidgets *w) { set_holiday(w, 12, 25); }), widgets);
    g_signal_connect_swapped(btn_easter, "clicked", G_CALLBACK(+[](AppWidgets *w) { set_holiday(w, 0, 0, true); }), widgets);
    g_signal_connect_swapped(btn_thanksgiving, "clicked", G_CALLBACK(+[](AppWidgets *w) { set_holiday(w, 0, 0, false, true); }), widgets);
    g_signal_connect_swapped(btn_halloween, "clicked", G_CALLBACK(+[](AppWidgets *w) { set_holiday(w, 10, 31); }), widgets);

    g_autofree char *saved_date = g_settings_get_string(settings, "birthday");
    if (saved_date && strlen(saved_date) > 0) {
        set_date_and_calculate(widgets, saved_date, false);
    } else {
        gtk_label_set_text(GTK_LABEL(result_label), "Select an event or double-click box");
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
