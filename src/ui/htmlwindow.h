#ifndef HTMLWINDOW_H
#define HTMLWINDOW_H

typedef void (*ui_trigger_event)(void *window);
typedef void (*ui_idle_event)(void *userdata);
typedef void (*ui_tray_event)(void *window);
typedef void (*ui_event)(void *event_data, void *userdata);

#define UI_EVENT_WINDOW_CREATE  0
#define UI_EVENT_WINDOW_CLOSE   1
#define UI_EVENT_LOOP_EXIT      2
#define UI_EVENTS               3

#define UI_SCHEDULER_SIZE       100

int ui_app_init(ui_trigger_event event_handler);
void ui_app_tray_icon(const char *tooltip, const char *notification_title, const char *notification_text, ui_tray_event event_tray);
void ui_app_tray_remove();
void ui_app_run_with_notify(ui_idle_event event_idle, void *userdata);
void ui_app_run_schedule_once(ui_idle_event scheduled, void *userdata);
void ui_app_iterate();
void ui_app_run();
int ui_app_done();
void ui_app_quit();
void ui_set_event(int eid, ui_event callback, void *event_userdata);

void ui_message(const char *title, const char *body, int level);
int ui_question(const char *title, const char *body, int level);
int ui_input(const char *title, const char *body, char *val, int val_len, int masked);
void ui_js(void *wnd, const char *js);
char *ui_call(void *wnd, const char *function, const char *arguments[]);
void ui_free_string(void *ptr);
void *ui_window(const char *title, const char *body);
void ui_window_maximize(void *wnd);
void ui_window_minimize(void *wnd);
void ui_window_restore(void *wnd);
void ui_window_top(void *wnd);
void ui_window_close(void *wnd);
void ui_window_set_content(void *wnd, const char *body);
int ui_window_count();

void ui_lock();
void ui_unlock();

#endif
