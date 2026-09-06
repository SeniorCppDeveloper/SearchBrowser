#include "mainwindow.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QDesktopServices>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QTimer>
#include <QFont>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), networkManager(new QNetworkAccessManager(this))
{
    setupUI();
    setWindowTitle("Mini Browser - Поисковая система");
    resize(1024, 768);
    initialWindowSize = size();
    setMinimumSize(800, 600);

    connect(searchButton, &QPushButton::clicked, this, &MainWindow::onSearch);
    connect(searchBar, &QLineEdit::returnPressed, this, &MainWindow::onReturnPressed);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    connect(backButton, &QPushButton::clicked, webView, &QWebEngineView::back);
    connect(forwardButton, &QPushButton::clicked, webView, &QWebEngineView::forward);
    connect(reloadButton, &QPushButton::clicked, webView, &QWebEngineView::reload);
    connect(webView, &QWebEngineView::loadFinished, this, &MainWindow::onLoadFinished);
    connect(webView, &QWebEngineView::urlChanged, this, &MainWindow::onUrlChanged);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI()
{
    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);

    topLayout = new QHBoxLayout();

    backButton = new QPushButton("◀");
    backButton->setFixedSize(30, 30);
    forwardButton = new QPushButton("▶");
    forwardButton->setFixedSize(30, 30);
    reloadButton = new QPushButton("⟳");
    reloadButton->setFixedSize(30, 30);

    QFont emojiFont("Segoe UI Emoji");
    backButton->setFont(emojiFont);
    forwardButton->setFont(emojiFont);
    reloadButton->setFont(emojiFont);

    searchBar = new QLineEdit();
    searchBar->setPlaceholderText("Введите запрос или URL (например: погода в москве)");
    searchBar->setMinimumHeight(30);

    searchButton = new QPushButton("🔍 Найти");
    searchButton->setFixedHeight(30);
    searchButton->setFont(emojiFont);

    topLayout->addWidget(backButton);
    topLayout->addWidget(forwardButton);
    topLayout->addWidget(reloadButton);
    topLayout->addWidget(searchBar);
    topLayout->addWidget(searchButton);

    progressBar = new QProgressBar();
    progressBar->setMaximumHeight(15);
    progressBar->setVisible(false);

    statusLabel = new QLabel("Готов к работе");
    statusLabel->setStyleSheet("QLabel { padding: 5px; background-color: #f0f0f0; border: 1px solid #ccc; }");

    webView = new QWebEngineView();
    webView->setUrl(QUrl("about:blank"));

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(webView);
    mainLayout->addWidget(statusLabel);

    setCentralWidget(centralWidget);

    setStyleSheet(R"(
        QMainWindow {
            background-color: #ffffff;
        }
        QLineEdit {
            border: 2px solid #ddd;
            border-radius: 15px;
            padding: 5px 15px;
            font-size: 14px;
        }
        QLineEdit:focus {
            border-color: #4CAF50;
        }
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 5px 15px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
        QProgressBar {
            border: 1px solid #ccc;
            border-radius: 5px;
            background-color: white;
        }
        QProgressBar::chunk {
            background-color: #4CAF50;
            border-radius: 5px;
        }
    )");
}

void MainWindow::onSearch()
{
    QString query = searchBar->text().trimmed();
    if (query.isEmpty()) {
        QMessageBox::information(this, "Информация", "Пожалуйста, введите запрос или URL");
        return;
    }
    performSearch(query);
}

void MainWindow::onReturnPressed()
{
    onSearch();
}

void MainWindow::performSearch(const QString& query)
{
    currentQuery = query;

    QUrl url(query);
    if (url.isValid() && (url.scheme() == "http" || url.scheme() == "https")) {
        loadUrl(query);
        return;
    }

    if (query.contains(".") && !query.contains(" ")) {
        loadUrl("https://" + query);
        return;
    }

    statusLabel->setText("Поиск: " + query);
    progressBar->setVisible(true);
    progressBar->setRange(0, 0);

    QString searchUrl = "https://api.duckduckgo.com/?q=" +
        QUrl::toPercentEncoding(query) +
        "&format=json&no_html=1&skip_disambig=1";

    networkManager->get(QNetworkRequest(QUrl(searchUrl)));
}

void MainWindow::loadUrl(const QString& urlString)
{
    QUrl url(urlString);
    if (!url.isValid()) {
        QMessageBox::warning(this, "Ошибка", "Неверный URL");
        return;
    }

    statusLabel->setText("Загрузка: " + url.toString());
    searchBar->setText(url.toString());
    webView->load(url);
}

void MainWindow::onNetworkReply(QNetworkReply* reply)
{
    progressBar->setVisible(false);

    if (reply->error() != QNetworkReply::NoError) {
        statusLabel->setText("Ошибка: " + reply->errorString());
        QString errorHtml = "<html><body><h2>Ошибка загрузки</h2><p>" +
            reply->errorString() + "</p></body></html>";
        webView->setHtml(errorHtml);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull()) {
        statusLabel->setText("Ошибка: Неверный формат ответа");
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();
    QString abstract = obj["Abstract"].toString();
    QString abstractText = obj["AbstractText"].toString();
    QString imageUrl = obj["Image"].toString();
    QJsonArray relatedTopics = obj["RelatedTopics"].toArray();
    QJsonArray infobox = obj["Infobox"].toArray();

    QString resultsHtml = "<html><head><style>"
        "body { font-family: Arial, sans-serif; margin: 20px; }"
        ".result { margin: 15px 0; padding: 15px; border: 1px solid #e0e0e0; border-radius: 8px; }"
        ".result h3 { color: #1a0dab; margin: 0 0 5px 0; }"
        ".result a { color: #1a0dab; text-decoration: none; }"
        ".result a:hover { text-decoration: underline; }"
        ".result p { margin: 5px 0; color: #333; }"
        ".result .url { color: #006621; font-size: 14px; }"
        ".highlight { background-color: #f0f0f0; padding: 20px; border-radius: 8px; }"
        ".suggestion { color: #777; font-size: 14px; }"
        "</style></head><body>";

    resultsHtml += "<h1>Результаты поиска: " + currentQuery + "</h1>";

    if (!imageUrl.isEmpty()) {
        resultsHtml += "<img src='" + imageUrl + "' style='max-width: 200px; float: right; margin: 10px;'/>";
    }

    if (!abstract.isEmpty()) {
        resultsHtml += "<div class='highlight'><h3>📌 Краткий ответ</h3><p>" + abstract + "</p>";
        if (!abstractText.isEmpty()) {
            resultsHtml += "<p><i>" + abstractText + "</i></p>";
        }
        resultsHtml += "</div>";
    }

    if (!infobox.isEmpty()) {
        resultsHtml += "<div class='highlight'><h3>📊 Информация</h3>";
        for (const QJsonValue& val : infobox) {
            QJsonObject item = val.toObject();
            QString label = item["label"].toString();
            QString value = item["value"].toString();
            if (!label.isEmpty() && !value.isEmpty()) {
                resultsHtml += "<p><b>" + label + ":</b> " + value + "</p>";
            }
        }
        resultsHtml += "</div>";
    }

    if (!relatedTopics.isEmpty()) {
        resultsHtml += "<h2>📚 Связанные результаты</h2>";
        int count = 0;
        for (const QJsonValue& val : relatedTopics) {
            if (count++ > 20) break;

            QJsonObject topic = val.toObject();
            QString text = topic["Text"].toString();
            QString firstUrl = topic["FirstURL"].toString();

            if (!text.isEmpty() && !firstUrl.isEmpty()) {
                resultsHtml += "<div class='result'>";

                QUrl url(firstUrl);
                QString domain = url.host();
                if (domain.isEmpty()) domain = firstUrl;

                QString title = text;
                QRegularExpression titleRegex("(.*?)(?=\\s*[\\.\\!\\?]|$)");
                QRegularExpressionMatch match = titleRegex.match(text);
                if (match.hasMatch()) {
                    title = match.captured(1);
                }

                resultsHtml += "<h3><a href='" + firstUrl + "'>" + title + "</a></h3>";
                resultsHtml += "<p class='url'>" + domain + "</p>";
                resultsHtml += "<p>" + text + "</p>";
                resultsHtml += "</div>";
            }
        }
    }

    if (relatedTopics.isEmpty() && abstract.isEmpty()) {
        resultsHtml += "<div class='highlight'><p>😕 По вашему запросу ничего не найдено.</p>";
        resultsHtml += "<p>Попробуйте переформулировать запрос или использовать другой поисковый сервис.</p>";
        resultsHtml += "</div>";

        resultsHtml += "<h3>🔍 Попробуйте поискать в других системах:</h3>";
        QStringList searchEngines = {
            "https://www.google.com/search?q=" + QUrl::toPercentEncoding(currentQuery),
            "https://yandex.ru/search/?text=" + QUrl::toPercentEncoding(currentQuery),
            "https://www.bing.com/search?q=" + QUrl::toPercentEncoding(currentQuery)
        };

        for (const QString& engine : searchEngines) {
            resultsHtml += "<p><a href='" + engine + "'>" + engine + "</a></p>";
        }
    }

    resultsHtml += "</body></html>";

    webView->setHtml(resultsHtml);
    statusLabel->setText("Найдено результатов: " + QString::number(relatedTopics.size()));
    searchBar->setText(currentQuery);

    reply->deleteLater();
}

void MainWindow::onLoadFinished(bool ok)
{
    progressBar->setVisible(false);
    if (ok) {
        statusLabel->setText("Страница загружена: " + webView->url().toString());
    }
    else {
        statusLabel->setText("Ошибка загрузки страницы");
    }

    QTimer::singleShot(0, this, [this]() {
        resize(initialWindowSize);
        });
}

void MainWindow::onUrlChanged(const QUrl& url)
{
    if (url.isValid() && !url.isEmpty()) {
        searchBar->setText(url.toString());
    }
}

void MainWindow::onProgress(int progress)
{
    if (progress > 0 && progress < 100) {
        progressBar->setVisible(true);
        progressBar->setRange(0, 100);
        progressBar->setValue(progress);
    }
    else {
        progressBar->setVisible(false);
    }
}
