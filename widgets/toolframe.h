#ifndef TOOLFRAME_H
#define TOOLFRAME_H

#include <QMdiSubWindow>
#include <QWidget>
#include <QPlainTextEdit>
#include <QTextStream>
#include "stopwatch.h"
#include <QTimer>
#include <QPushButton>
#include "statusbitwidget.h"
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QSettings>
#include <QSpacerItem>

#define BUTTON_SIZE 45

class SideBar;
class EventLogger;
class ToolSettings;
class ToolFrame : public QMdiSubWindow
{
    Q_OBJECT
public:
    explicit ToolFrame(QWidget *parent = 0);
    void useTimer(); // startTimer ?
    void usePlot(); // currently not working
    void useEventlog();
    void putContent(QWidget *w);
    void putSettings(QWidget *w);

signals:
    void timeout();
    void start();
    void stop();

protected slots:
    void started();
    void running();
    void stopped();
    void error();
    void plot(int value);

    void resize();
    void request_settings();

private slots:
    void show_content();
    void show_settings();
    void toggle_eventlog();
    void load_settings();

protected:
    QTextStream &output() const;
    QSettings &store() const; // Maybe store the referenced member in m_settings instead of m_toolsettings (since its used for all settings)

private:
    bool m_usingTimer;
    bool m_usingPlot;
    bool m_usingEventlog;

    QPushButton *m_start_button;
    QPushButton *m_settings_button;
    QPushButton *m_plot_button;
    QPushButton *m_eventlog_button;
    QSpacerItem *sidebar_separator;
    StatusBitWidget *m_status_led;

    QString logbuffer;
    QTextStream *eventlog;

    QWidget *m_sidebar;
    QWidget *m_content;
    Stopwatch *m_timer;
    QWidget *m_settings;
    QLabel *m_settings_header;
    ToolSettings *m_toolsettings;
    EventLogger *m_eventlog;
    QWidget *m_wrapper;

    QString context();
    void resetSidebar();
};

class EventLogger : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit EventLogger(QTextStream &events, QWidget *parent = 0);
    void setSampleInterval(int interval);

private slots:
    void poll_stream();

private:
    QTimer ticker;
    QTextStream &stream;

    void print(QString line);
};

class ToolSettings : public QWidget
{
    Q_OBJECT
public:
    explicit ToolSettings(QWidget *parent = 0);
    void setContext(QString context);
    void addTimer();
    void addPlot();
    void addEventlog();
    QSettings *store() const;

signals:
    void first_time_setup();
    void update_frame();

private slots:
    void update_form();
    void set_timeout();
    void set_eventlog_sampling();

private:
    QFrame *timeout_line;
    QCheckBox *timeout_checker;
    QLineEdit *timeout_edit;
    QFrame *graph_line;
    QCheckBox *graph_checker;
    QLineEdit *graph_sample_edit;
    QFrame *eventlog_line;
    QCheckBox *eventlog_checker;
    QLineEdit *eventlog_edit;

    QSettings *m_store;

    bool m_usingTimer;
    bool m_usingPlot;
    bool m_usingEventlog;

    QFrame *separator();

    void loadSettings(); // break into one loader for each widget
    void saveSettings(); // break into one saver for each widget
};

#endif // TOOLFRAME_H
