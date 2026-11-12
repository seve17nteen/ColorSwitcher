#include "mainwindow.h"
#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent), m_mode(Mode::Cycle), m_clickHandlerAlt(false)
{
    setupUi();
    m_timer.setInterval(600);
    rebuildTimeoutConnection(); // Подключаем таймер
    reconnectCanvasHandler();   // Подключаем сигнал холста
    setStatus("Готово. Нажмите «Старт/Стоп».");
}

MainWindow::~MainWindow()
{
    qDebug() << "[DESTROY] MainWindow";
    m_timer.stop();
}

void MainWindow::setupUi()
{
    setWindowTitle("QTimer + лямбда-слоты + динамические соединения");
    resize(560, 640);
    m_rootLayout = new QVBoxLayout(this);

    m_canvas.setParent(this);
    m_rootLayout->addWidget(&m_canvas);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_rootLayout->addWidget(m_statusLabel);

    m_intervalLabel = new QLabel(this);
    m_modelLabel = new QLabel(this);
    m_rootLayout->addWidget(m_intervalLabel);
    m_rootLayout->addWidget(m_modelLabel);

    // Кнопки таймера
    auto *controls = new QHBoxLayout();
    auto *btnStart = new QPushButton("Старт/Стоп", this);
    auto *btnSlow = new QPushButton("Медленнее", this);
    auto *btnFast = new QPushButton("Быстрее", this);
    auto *btnMode = new QPushButton("Режим", this);
    controls->addWidget(btnStart);
    controls->addWidget(btnSlow);
    controls->addWidget(btnFast);
    controls->addWidget(btnMode);
    m_rootLayout->addLayout(controls);

    connect(btnStart, &QPushButton::clicked, this, &MainWindow::toggleTimer);
    connect(btnSlow, &QPushButton::clicked, this, &MainWindow::decreaseInterval);
    connect(btnFast, &QPushButton::clicked, this, &MainWindow::increaseInterval);
    connect(btnMode, &QPushButton::clicked, this, &MainWindow::cycleMode);

    // Кнопки палитры
    m_rootLayout->addWidget(new QLabel("Палитра (кнопки создаются динамически):", this));
    m_paletteGrid = new QGridLayout();
    m_rootLayout->addLayout(m_paletteGrid);

    auto *palControls = new QHBoxLayout();
    auto *btnAdd = new QPushButton("+ Цвет", this);
    auto *btnClear = new QPushButton("Очистить палитру", this);
    auto *btnClick = new QPushButton("Сменить обработчик клика", this);
    palControls->addWidget(btnAdd);
    palControls->addWidget(btnClear);
    palControls->addWidget(btnClick);
    m_rootLayout->addLayout(palControls);

    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::addColorButton);
    connect(btnClear, &QPushButton::clicked, this, &MainWindow::clearPaletteButtons);
    connect(btnClick, &QPushButton::clicked, this, &MainWindow::toggleClickHandler);

    updateIntervalLabel();
    updateModeLabel();
}

void MainWindow::toggleTimer()
{
    if (m_timer.isActive()) {
        m_timer.stop();
        setStatus("Таймер остановлен.");
        qDebug() << "[TIMER] stop()";
    } else {
        m_timer.start();
        setStatus(QString("Таймер запущен (интервал %1 мс).").arg(m_timer.interval()));
        qDebug() << "[TIMER] start(), interval =" << m_timer.interval();
    }
}

void MainWindow::decreaseInterval()
{
    m_timer.setInterval(qMin(m_timer.interval() + 200, 5000));
    updateIntervalLabel();
}

void MainWindow::increaseInterval()
{
    m_timer.setInterval(qMax(m_timer.interval() - 200, 50));
    updateIntervalLabel();
}

void MainWindow::cycleMode()
{
    switch (m_mode) {
    case Mode::Cycle: m_mode = Mode::Random; break;
    case Mode::Random: m_mode = Mode::Blink; break;
    case Mode::Blink: m_mode = Mode::Cycle; break;
    }
    updateModeLabel();
    rebuildTimeoutConnection();
    setStatus(QString("Режим переключён: %1").arg(modeName()));
}

// ДИНАМИЧЕСКОЕ ПЕРЕПОДКЛЮЧЕНИЕ ТАЙМЕРА
void MainWindow::rebuildTimeoutConnection()
{
    if (m_timeoutConn) {
        disconnect(m_timeoutConn); // Рвем старое
        m_timeoutConn = QMetaObject::Connection();
    }

    // Создаем новое (лямбда захватывает this)
    m_timeoutConn = connect(&m_timer, &QTimer::timeout, this, [this]() {
        switch (m_mode) {
        case Mode::Cycle: m_canvas.stepForward(); break;
        case Mode::Random: m_canvas.setRandomColor(); break;
        case Mode::Blink: m_canvas.toggleBlink(); break;
        }
        qDebug() << "[TIMER] timeout, режим =" << modeName() << ", цвет =" << m_canvas.currentColor().name();
    });
}

// ПЕРЕПОДКЛЮЧЕНИЕ СИГНАЛА ХОЛСТА
void MainWindow::reconnectCanvasHandler()
{
    if (m_clickConn) {
        disconnect(m_clickConn);
        m_clickConn = QMetaObject::Connection();
    }
    if (!m_clickHandlerAlt) {
        // Вариант А: обновлять статус
        m_clickConn = connect(&m_canvas, &ColorWidget::colorChanged, this, [this](const QColor &c) {
            setStatus(QString("Цвет холста: %1").arg(c.name()));
        });
    } else {
        // Вариант Б: менять заголовок окна
        m_clickConn = connect(&m_canvas, &ColorWidget::colorChanged, this, [this](const QColor &c) {
            setWindowTitle(QString("ColorSwitcher - %1").arg(c.name()));
        });
    }
}

void MainWindow::toggleClickHandler()
{
    m_clickHandlerAlt = !m_clickHandlerAlt;
    reconnectCanvasHandler();
    setStatus(m_clickHandlerAlt ? "Обработчик: вариант Б (заголовок)" : "Обработчик: вариант А (статус)");
}

// ДИНАМИЧЕСКОЕ СОЗДАНИЕ КНОПКИ И СВЯЗИ
void MainWindow::addColorButton()
{
    QColor color = QColor::fromHsv(QRandomGenerator::global()->bounded(360), 255, 255);
    auto *btn = new PaletteButton(color, this);

    const int row = m_paletteButtons.size() / 4;
    const int col = m_paletteButtons.size() % 4;
    m_paletteGrid->addWidget(btn, row, col);

    // Сохраняем соединение в список!
    QMetaObject::Connection conn = connect(btn, &QPushButton::clicked, this, [this, btn]() {
        m_canvas.addColor(btn->color());
        setStatus(QString("Добавлен цвет %1").arg(btn->color().name()));
    });

    m_paletteButtons.append(QPointer<PaletteButton>(btn));
    m_dynamicConnections.append(conn);
}

// ОЧИСТКА И РАЗРЫВ ВСЕХ СВЯЗЕЙ
void MainWindow::clearPaletteButtons()
{
    for (QMetaObject::Connection &c : m_dynamicConnections) {
        if (c) disconnect(c); // Явно рвем
    }
    m_dynamicConnections.clear();

    for (QPointer<PaletteButton> &btn : m_paletteButtons) {
        if (!btn.isNull()) btn->deleteLater();
    }
    m_paletteButtons.clear();
    m_canvas.resetPalette();
    setStatus("Палитра очищена.");
}

void MainWindow::setStatus(const QString &text) { if (m_statusLabel) m_statusLabel->setText(text); }
void MainWindow::updateIntervalLabel() { m_intervalLabel->setText(QString("Интервал: %1 мс").arg(m_timer.interval())); }
void MainWindow::updateModeLabel() { m_modelLabel->setText(QString("Режим: %1").arg(modeName())); }
QString MainWindow::modeName() const {
    switch (m_mode) {
    case Mode::Cycle: return "Цикл";
    case Mode::Random: return "Случайный";
    case Mode::Blink: return "Мигание";
    }
    return "";
}