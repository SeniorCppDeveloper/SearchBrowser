#pragma once
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QWebEngineView>
#include <QProgressBar>
#include <QLabel>
#include <QSize>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onSearch();
    void onReturnPressed();
    void onNetworkReply(QNetworkReply* reply);
    void onLoadFinished(bool ok);
    void onUrlChanged(const QUrl& url);
    void onProgress(int progress);

private:
    QWidget* centralWidget;
    QHBoxLayout* topLayout;
    QVBoxLayout* mainLayout;
    QLineEdit* searchBar;
    QPushButton* searchButton;
    QPushButton* backButton;
    QPushButton* forwardButton;
    QPushButton* reloadButton;
    QTextBrowser* resultsBrowser;
    QWebEngineView* webView;
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QNetworkAccessManager* networkManager;

    QString currentQuery;
    QSize initialWindowSize;
    void setupUI();
    void performSearch(const QString& query);
    void showSearchResults(const QString& results);
    void loadUrl(const QString& urlString);
};

#endif 
