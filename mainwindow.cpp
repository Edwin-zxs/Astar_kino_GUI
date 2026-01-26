#include "mainwindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QImageReader>
#include <QDebug>
#include <QPainter>
#include <queue>
#include <cmath>
#include <set>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout (Vertical)
    mainLayout = new QVBoxLayout(centralWidget);

    // --- Top Control Area ---
    topContainer = new QWidget(this);
    topLayout = new QHBoxLayout(topContainer);

    // 1. Left: Inputs
    inputGroup = new QGroupBox("Settings", this);
    inputLayout = new QGridLayout(inputGroup);

    startLabel = new QLabel("Start (x, y):", this);
    startXSpin = new QSpinBox(this);
    startXSpin->setRange(0, 9999);
    startYSpin = new QSpinBox(this);
    startYSpin->setRange(0, 9999);

    goalLabel = new QLabel("Goal (x, y):", this);
    goalXSpin = new QSpinBox(this);
    goalXSpin->setRange(0, 9999);
    goalYSpin = new QSpinBox(this);
    goalYSpin->setRange(0, 9999);

    inputLayout->addWidget(startLabel, 0, 0);
    inputLayout->addWidget(startXSpin, 0, 1);
    inputLayout->addWidget(startYSpin, 0, 2);
    inputLayout->addWidget(goalLabel, 1, 0);
    inputLayout->addWidget(goalXSpin, 1, 1);
    inputLayout->addWidget(goalYSpin, 1, 2);

    topLayout->addWidget(inputGroup);
    topLayout->addStretch(); // Spacer between inputs and buttons

    // 2. Right: Buttons
    buttonContainer = new QWidget(this);
    buttonLayout = new QVBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    loadMapBtn = new QPushButton("Load map", this);
    searchPathBtn = new QPushButton("Search path(A*)", this);
    exitBtn = new QPushButton("Exit", this);

    buttonLayout->addWidget(loadMapBtn);
    buttonLayout->addWidget(searchPathBtn);
    buttonLayout->addWidget(exitBtn);

    topLayout->addWidget(buttonContainer);

    mainLayout->addWidget(topContainer);

    // --- Bottom: Map Display ---
    scrollArea = new QScrollArea(this);
    scrollArea->setBackgroundRole(QPalette::Dark);
    mapLabel = new QLabel(this);
    mapLabel->setAlignment(Qt::AlignCenter);
    scrollArea->setWidget(mapLabel);
    scrollArea->setWidgetResizable(true); 
    
    mainLayout->addWidget(scrollArea);

    // Connect signals and slots
    connect(loadMapBtn, &QPushButton::clicked, this, &MainWindow::onLoadMap);
    connect(searchPathBtn, &QPushButton::clicked, this, &MainWindow::onSearchPath);
    connect(exitBtn, &QPushButton::clicked, this, &MainWindow::onExit);

    setWindowTitle("A* Kino GUI");
    resize(900, 700);
}

MainWindow::~MainWindow()
{
}

void MainWindow::onLoadMap()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
                                                    "Open Map File", 
                                                    "", 
                                                    "Images (*.pgm *.png *.jpg *.bmp);;All Files (*)",
                                                    nullptr,
                                                    QFileDialog::DontUseNativeDialog);
    
    if (!fileName.isEmpty()) {
        if (currentMap.load(fileName)) {
            // Convert to format suitable for pixel manipulation if needed, 
            // but keeping original is usually fine for reading.
            // We want ARGB32 for drawing the path on top later easily.
            currentMap = currentMap.convertToFormat(QImage::Format_ARGB32);
            
            mapLabel->setPixmap(QPixmap::fromImage(currentMap));
            mapLabel->adjustSize();
            
            // Update spinbox ranges based on map size
            int maxX = currentMap.width() - 1;
            int maxY = currentMap.height() - 1;
            startXSpin->setRange(0, maxX);
            startYSpin->setRange(0, maxY);
            goalXSpin->setRange(0, maxX);
            goalYSpin->setRange(0, maxY);
            
            qDebug() << "Map loaded. Size:" << currentMap.size();
        } else {
            QMessageBox::warning(this, "Error", "Failed to load image.");
        }
    }
}

void MainWindow::onSearchPath()
{
    if (currentMap.isNull()) {
        QMessageBox::warning(this, "Warning", "Please load a map first.");
        return;
    }

    QPoint start(startXSpin->value(), startYSpin->value());
    QPoint goal(goalXSpin->value(), goalYSpin->value());

    if (!isValid(start.x(), start.y(), currentMap)) {
        QMessageBox::warning(this, "Warning", "Start point is obstacle or out of bounds.");
        return;
    }
    if (!isValid(goal.x(), goal.y(), currentMap)) {
        QMessageBox::warning(this, "Warning", "Goal point is obstacle or out of bounds.");
        return;
    }

    // Run A*
    std::vector<QPoint> path = findPath(currentMap, start, goal);

    if (path.empty()) {
        QMessageBox::information(this, "Result", "No path found!");
        return;
    }

    // Draw path
    QImage resultImage = currentMap.copy();
    QPainter painter(&resultImage);
    painter.setPen(QPen(Qt::red, 2));

    for (size_t i = 0; i < path.size() - 1; ++i) {
        painter.drawLine(path[i], path[i+1]);
    }
    painter.end();

    mapLabel->setPixmap(QPixmap::fromImage(resultImage));
}

bool MainWindow::isValid(int x, int y, const QImage& map) {
    if (x < 0 || x >= map.width() || y < 0 || y >= map.height()) return false;
    // Assume obstacle if not white (255, 255, 255)
    // Adjust threshold as needed. 
    QRgb pixel = map.pixel(x, y);
    // Simple check: if it's too dark, it's an obstacle.
    // Or strictly check for black. Let's use a brightness threshold.
    int gray = qGray(pixel);
    return gray > 200; // Treat light pixels as free, dark as obstacle
}

struct NodeCompare {
    bool operator()(const Node* a, const Node* b) {
        return a->f() > b->f();
    }
};

std::vector<QPoint> MainWindow::findPath(QImage map, QPoint start, QPoint goal)
{
    int width = map.width();
    int height = map.height();

    // 8 Directions
    int dx[] = {1, 1, 0, -1, -1, -1, 0, 1, 0};
    int dy[] = {0, 1, 1, 1, 0, -1, -1, -1, 0};

    std::priority_queue<Node*, std::vector<Node*>, NodeCompare> openSet;
    std::vector<std::vector<bool>> closedSet(width, std::vector<bool>(height, false));
    std::vector<std::vector<Node*>> allNodes(width, std::vector<Node*>(height, nullptr));

    Node* startNode = new Node(start.x(), start.y());
    allNodes[start.x()][start.y()] = startNode;
    openSet.push(startNode);

    Node* goalNode = nullptr;

    while (!openSet.empty()) {
        Node* current = openSet.top();
        openSet.pop();

        if (current->x == goal.x() && current->y == goal.y()) {
            goalNode = current;
            break;
        }

        if (closedSet[current->x][current->y]) continue;
        closedSet[current->x][current->y] = true;

        for (int i = 0; i < 8; ++i) {
            int nx = current->x + dx[i];
            int ny = current->y + dy[i];

            if (isValid(nx, ny, map) && !closedSet[nx][ny]) {
                float newG = current->g + std::sqrt(dx[i]*dx[i] + dy[i]*dy[i]);
                
                Node* neighbor = allNodes[nx][ny];
                if (neighbor == nullptr) {
                    neighbor = new Node(nx, ny);
                    allNodes[nx][ny] = neighbor;
                    neighbor->g = newG;
                    neighbor->h = std::hypot(nx - goal.x(), ny - goal.y());
                    neighbor->parent = current;
                    openSet.push(neighbor);
                } else if (newG < neighbor->g) {
                    neighbor->g = newG;
                    neighbor->parent = current;
                    openSet.push(neighbor);
                }
            }
        }
    }

    std::vector<QPoint> path;
    if (goalNode) {
        Node* curr = goalNode;
        while (curr) {
            path.push_back(QPoint(curr->x, curr->y));
            curr = curr->parent;
        }
        // Memory cleanup could be done here (delete all nodes in allNodes)
        // For simple app, OS will reclaim, but better to be safe in long run.
    }
    
    // Cleanup
    for(int i=0; i<width; ++i) {
        for(int j=0; j<height; ++j) {
            if(allNodes[i][j]) delete allNodes[i][j];
        }
    }

    return path;
}

void MainWindow::onExit()
{
    QApplication::quit();
}
