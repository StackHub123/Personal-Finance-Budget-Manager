#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QButtonGroup;
class QStackedWidget;
class QLabel;
class QPushButton;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QWidget *buildSidebar();
    QWidget *buildDashboardPage();
    QWidget *buildPlaceholderPage(const QString &title, const QString &description);
    QWidget *buildMetricCard(const QString &caption,
                             const QString &value,
                             const QString &detail);
    void navigateTo(int pageIndex);
    void applyStyles();

    QStackedWidget *m_pages = nullptr;
    QButtonGroup *m_navigationGroup = nullptr;
    QLabel *m_pageTitle = nullptr;
    QLabel *m_statusLabel = nullptr;
};

#endif // MAINWINDOW_H
