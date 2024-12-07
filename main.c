#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib-unix.h>
#include "main.h"

int main(int argc, char **argv) {

    g_autoptr(AdwApplication) app = NULL;
    int status;

    //allocate initialData
    struct initData *initialData = malloc(sizeof(struct initData));
    if (initialData == NULL) {
        g_printerr("Failed to allocate memory for initialData\n");
        return -1;
    }

    //create the gtk application and pass initialData to activate
    app = adw_application_new("com.asolonari.Robinwaita", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), initialData);

    //start gtk application
    status = g_application_run(G_APPLICATION(app), argc, argv);
    fflush(stdout);

    //free memory after the event loop exits
    freeAll(initialData);

    return status;
}

void updateStatistics(gpointer userData){
    initData *data = (initData *) userData;
    AdwActionRow *averageRunningTimeRow, *totalContextSwitchesRow;
    double averageTime = data->totalTime / data->processAmt;

    //get objects from builder
    averageRunningTimeRow = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "averageRunningTimeRow"));
    if(!averageRunningTimeRow) g_printerr("Error: averageRunningTimeRow is NULL\n");
    totalContextSwitchesRow = ADW_ACTION_ROW(gtk_builder_get_object(data->builder, "totalContextSwitchesRow"));
    if(!totalContextSwitchesRow) g_printerr("Error: totalContextSwitchesRow is NULL\n");

    //format strings
    gchar *averageTimeStr = g_strdup_printf("%.1lf seconds", averageTime);
    gchar *totalContextSwitchesStr = g_strdup_printf("%d context switches", data->contextSwitches);

    //update subtitles with information
    adw_action_row_set_subtitle(averageRunningTimeRow, averageTimeStr);
    adw_action_row_set_subtitle(totalContextSwitchesRow, totalContextSwitchesStr);

    //free new strings
    g_free(averageTimeStr);
    g_free(totalContextSwitchesStr);
}

gboolean schedulerUpdateFromTimerfd(gint fd, GIOCondition condition, gpointer userData) {
    initData *data = (initData *) userData;

    if (condition & G_IO_IN) {
        uint64_t expirations;
        ssize_t bytes = read(fd, &expirations, sizeof(expirations));
        if (bytes != sizeof(expirations)) {
            g_printerr("Warning: read unexpected number of bytes from timerfd\n");
        }

        //scheduler logic
        manageQueue(data);
    }

    return TRUE; //while monitoring file descriptor
}

void addProcess(int processType, struct initData *data) {
    Process* createdProcess = newProcess(processType);

    enQueue(data->allProcesses, createdProcess);
    displayProcess(createdProcess, data->allProcesses, data);
}

void addCPUProcess(GtkWidget *widget, gpointer userData) {
    struct initData *data = (struct initData *) userData;
    addProcess(1, data);  // 1 = CPU-bound
}

void addIOProcess(GtkWidget *widget, gpointer userData) {
    struct initData *data = (struct initData *) userData;
    addProcess(2, data);  // 2 = IO-bound
}

void addMixedProcess(GtkWidget *widget, gpointer userData) {
    struct initData *data = (struct initData *) userData;
    addProcess(3, data);  // 3 = Mixed-bound
}

void updateButtonSensitivity(initData *data, gboolean sensitive) {
    //find buttons by id
    GtkWidget *cpuButton = GTK_WIDGET(gtk_builder_get_object(data->builder, "cpuButton"));
    GtkWidget *ioButton = GTK_WIDGET(gtk_builder_get_object(data->builder, "ioButton"));
    GtkWidget *mixedButton = GTK_WIDGET(gtk_builder_get_object(data->builder, "mixedButton"));
    GtkWidget *runAllButton = GTK_WIDGET(gtk_builder_get_object(data->builder, "runAllButton"));

    //set button sensitivity
    gtk_widget_set_sensitive(cpuButton, sensitive);
    gtk_widget_set_sensitive(ioButton, sensitive);
    gtk_widget_set_sensitive(mixedButton, sensitive);
    gtk_widget_set_sensitive(runAllButton, sensitive);
}

void runScheduler(GtkButton *button, gpointer userData) {
    initData *data = (initData *)userData;

    if (data->schedulerRunning) {
        g_printerr("Scheduler is already running.\n");
        return;
    }

    //reset statistics variables
    data->contextSwitches = 0;
    data->processAmt = 0;
    data->totalTime = 0;

    //call timerSetup to initialize timerfd
    data->timerfd = timerSetup(data->allProcesses);

    if (data->timerfd == -1) {
        g_printerr("Failed to set up the scheduler timer.\n");
        return;
    }

    //add timerfd to glib main loop to monitor it
    data->timerWatchID = g_unix_fd_add(data->timerfd, G_IO_IN | G_IO_HUP, (GUnixFDSourceFunc) schedulerUpdateFromTimerfd, data);
    if (data->timerWatchID == 0) {
        g_printerr("Failed to add timerfd to main loop.\n");
        close(data->timerfd);
        return;
    }

    updateButtonSensitivity(data, FALSE);

    data->schedulerRunning = 1;
    appendTextToView("Scheduler started.\n", data->textView);
}

static void activate(GtkApplication *app, gpointer userData) {
    GtkWidget *window, *runButton, *processListView, *cpuButton, *ioButton, *mixedButton;
    struct initData *data = (struct initData *) userData;

    //init gtk ui
    data->builder = gtk_builder_new_from_file("../user_interface.xml");
    window = GTK_WIDGET(gtk_builder_get_object(data->builder, "main_window"));
    gtk_window_set_application(GTK_WINDOW(window), GTK_APPLICATION(app));

    //retrieve textview
    processListView = GTK_WIDGET(gtk_builder_get_object(data->builder, "ProcessList"));
    if (!processListView) {
        g_printerr("Error: processListView is NULL\n");
    } else {
        data->textView = GTK_TEXT_VIEW(processListView);
    }

    //connect "run all" button to scheduler
    runButton = GTK_WIDGET(gtk_builder_get_object(data->builder, "runAllButton"));
    if (!runButton)
        g_printerr("Error: runButton is NULL\n");
    else
        g_signal_connect(runButton, "clicked", G_CALLBACK(runScheduler), data);

    //connect all buttons
    cpuButton = GTK_WIDGET(gtk_builder_get_object(data->builder, "cpuButton"));
    if (!cpuButton)
        g_printerr("Error: cpuButton is NULL\n");
    else
        g_signal_connect(cpuButton, "clicked", G_CALLBACK(addCPUProcess), data);

    ioButton = GTK_WIDGET(gtk_builder_get_object(data->builder, "ioButton"));
    if (!ioButton) {
        g_printerr("Error: ioButton is NULL\n");
    } else {
        g_signal_connect(ioButton, "clicked", G_CALLBACK(addIOProcess), data);
    }

    mixedButton = GTK_WIDGET(gtk_builder_get_object(data->builder, "mixedButton"));
    if (!mixedButton) {
        g_printerr("Error: mixedButton is NULL\n");
    } else {
        g_signal_connect(mixedButton, "clicked", G_CALLBACK(addMixedProcess), data);
    }

    //add a timeout to continue updating the textview every 100ms
    g_timeout_add(100, updateTextviewFromLog, data);

    //initialize rest of scheduler
    data->schedulerRunning = 0;
    data->timerWatchID = 0;
    data->logBuffer = g_string_new(NULL);

    //allocate queue
    data->allProcesses = (Queue *) malloc(sizeof(Queue));
    if (data->allProcesses == NULL) {
        g_printerr("main: Couldn't allocate memory for process queue");
        exit(1);
    }
    initQueue(data->allProcesses); //initialize variables

    //set callback functions
    setLogCallback(data, handleLog, data);
    setTimerCallback(data, handleTimerRemoval, data);

    //show gtk window
    gtk_window_present(GTK_WINDOW(window));
}

static void handleLog(const char *message, void *userData) {
    struct initData *data = userData;
    gchar *glibMessage = g_strdup_printf("%s", message);
    g_string_append(data->logBuffer, glibMessage);
    g_free(glibMessage);
}

static void handleTimerRemoval(unsigned timerID, void *userData){
    g_source_remove(timerID);
}

void scrollToBottom(GtkTextView *textView) {
    GtkAdjustment *adjustment = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(textView));

    //get upper limit of the vertical adjustment (the bottom)
    gdouble upper = gtk_adjustment_get_upper(adjustment);

    //set adjustment value to the upper limit (scroll to bottom)
    gtk_adjustment_set_value(adjustment, upper);
}

void appendTextToView(const char *text, GtkTextView *textView) {
    if (!textView) {
        g_printerr("Error: textView is NULL in appendTextToView\n");
        return;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(textView);
    if (!buffer) {
        g_printerr("Error: GtkTextBuffer is NULL\n");
        return;
    }

    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    //keep textview buffer within certain size
    int current_size = gtk_text_iter_get_offset(&end);
    if (current_size > 10000) {
        GtkTextIter new_start;
        gtk_text_buffer_get_iter_at_offset(buffer, &new_start, current_size - 10000);
        gtk_text_buffer_delete(buffer, &start, &new_start);
    }

    //insert new text
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, text, -1);

    //scroll to bottom
    scrollToBottom(textView);
}

gboolean updateTextviewFromLog(gpointer userData) {
    initData *data = (initData *) userData;

    if (data->logBuffer->len > 0) {
        //copy logBuffer content
        gchar *newText = g_strdup(data->logBuffer->str);

        //clear logBuffer
        g_string_truncate(data->logBuffer, 0);

        //append new text to GtkTextView
        appendTextToView(newText, data->textView);

        g_free(newText);
    }

    return TRUE; //call this function continuously
}

void freeAll(struct initData *initialData) {
    Queue *q = initialData->allProcesses;
    Node *current = q->front;
    Node *nextNode;

    while (current != NULL) {
        nextNode = current->next;

        //free process
        if (current->process != NULL) {
            free(current->process->name);
            for (int i = 0; current->process->args[i] != NULL; i++) {
                free(current->process->args[i]);
            }
            free(current->process->args);
            free(current->process);
        }

        //free node
        free(current);
        current = nextNode;
    }

    q->front = NULL;
    q->rear = NULL;
    q->numElements = 0;

    free(q);

    //unref gtk objects
    if (initialData->builder) {
        g_object_unref(initialData->builder);
    }

    if(initialData->logBuffer) {
        g_string_free(initialData->logBuffer, TRUE);
    }

    free(initialData);
}