#ifndef ROBINWAITA_MAIN_H
#define ROBINWAITA_MAIN_H
#include <poll.h>
#include <glib.h>
#include <libadwaita-1/adwaita.h>
#include "scheduler.h"

struct Queue;

//data that gets passed through gpointer, contains queue and other info
typedef struct initData {
    //whether scheduler is running
    int schedulerRunning;

    //statistics variables
    int timerfd;
    int processAmt;
    double totalTime;
    int contextSwitches;

    //timer info
    guint timerWatchID;

    //allProcesses queue
    Queue *allProcesses;

    //gtk text view
    GtkTextView *textView;

    //gtk builder
    GtkBuilder *builder;

    //in-memory buffer
    GString *logBuffer;

} initData;

void updateStatistics(gpointer userData); //calculate & update statistics screen
gboolean schedulerUpdateFromTimerfd(gint fd, GIOCondition condition, gpointer userData); //call scheduler from timerfd updates
void addProcess(int processType, struct initData *data); //enqueue process and display newest
void addCPUProcess(GtkWidget *widget, gpointer userData); //button to add cpu process to queue
void addIOProcess(GtkWidget *widget, gpointer userData); //button to add io process to queue
void addMixedProcess(GtkWidget *widget, gpointer userData); //button to add mixed-bound process to queue
void updateButtonSensitivity(initData *data, gboolean sensitive); //gray out buttons if scheduler is running
void runScheduler(GtkButton *button, gpointer userData); //init & start running scheduler
static void activate(GtkApplication *app, gpointer userData); //init gtk window objects
void scrollToBottom(GtkTextView *textView); //keep gtk textview scrolled to bottom
void appendTextToView(const char *text, GtkTextView *textView); //add text to gtk textview
gboolean updateTextviewFromLog(gpointer userData); //copy text from buffer and call appendTextToView with the text
void freeAll(struct initData *initialData); //free dynamically allocated memory

#endif //ROBINWAITA_MAIN_H
