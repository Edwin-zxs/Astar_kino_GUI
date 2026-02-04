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
#include <chrono>

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
    startXSpin->setValue(5);
    startYSpin = new QSpinBox(this);
    startYSpin->setRange(0, 9999);
    startYSpin->setValue(5);

    goalLabel = new QLabel("Goal (x, y):", this);
    goalXSpin = new QSpinBox(this);
    goalXSpin->setRange(0, 9999);
    goalXSpin->setValue(128);
    goalYSpin = new QSpinBox(this);
    goalYSpin->setRange(0, 9999);
    goalYSpin->setValue(168);

    inputLayout->addWidget(startLabel, 0, 0);
    inputLayout->addWidget(startXSpin, 0, 1);
    inputLayout->addWidget(startYSpin, 0, 2);
    inputLayout->addWidget(goalLabel, 1, 0);
    inputLayout->addWidget(goalXSpin, 1, 1);
    inputLayout->addWidget(goalYSpin, 1, 2);

    timeLabel = new QLabel("Time: N/A", this);
    inputLayout->addWidget(timeLabel, 2, 0, 1, 3); // Spanning 3 columns

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
    scrollArea->setWidgetResizable(false); // Disable auto-resize to allow scrollbars
    
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
            
            displayedImage = currentMap;
            updateMapDisplay();
            
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
    auto start_time = std::chrono::high_resolution_clock::now();
    std::vector<QPoint> path = findPath(currentMap, start, goal);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    timeLabel->setText(QString("Time: %1 s").arg(duration.count()/1000.0));

    if (path.empty()) {
        QMessageBox::information(this, "Result", "No path found!");
        return;
    }

    // Draw path
    QImage resultImage = currentMap.copy();
    QPainter painter(&resultImage);
    painter.setPen(QPen(Qt::red, 1));

    for (size_t i = 0; i < path.size() - 1; ++i) {
        painter.drawLine(path[i], path[i+1]);
    }
    painter.end();

    displayedImage = resultImage;
    updateMapDisplay();
}

void MainWindow::updateMapDisplay()
{
    if (displayedImage.isNull()) return;

    // Fixed resolution 1000x1000
    QSize fixedSize(1000, 1000);

    QPixmap px = QPixmap::fromImage(displayedImage);
    QPixmap scaledPx = px.scaled(fixedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    mapLabel->setPixmap(scaledPx);
    mapLabel->resize(scaledPx.size()); // Ensure label fits the pixmap
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateMapDisplay();
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
    //int dx[] = {1, 1, 0, -1, -1, -1, 0, 1, 0};
    //int dy[] = {0, 1, 1, 1, 0, -1, -1, -1, 0};


    // 9 choices of two motors
    int d_l[] = {0, 1, -1, 0, 1,  0, -1,  0,  1};
    int d_r[] = {0, 1, -1, 1, 0, -1,  0, -1, -1};
    float rpm_cost[] = {0, 0.1, 0.1, 0.2, 0.2, 0.2, 0.2, 0.2, 0.3};

    int layers = 360;
    std::priority_queue<Node*, std::vector<Node*>, NodeCompare> openSet;
    //std::vector<bool> closedSet(width * height * layers, false);
    std::vector<uint8_t> closedSet(width * height * layers, 0);

    /*std::vector<std::vector<std::vector<bool>>> closedSet(
        width,
        std::vector<std::vector<bool>>(
            height,
            std::vector<bool>(layers, false)
        )
    );
    */
    std::vector<std::vector<std::vector<Node*>>> allNodes(
        width,
        std::vector<std::vector<Node*>>(
            height,
            std::vector<Node*>(layers, nullptr)
        )
    );

    Node* startNode = new Node(start.x(), start.y(), 0);
    allNodes[start.x()][start.y()][0] = startNode;
    openSet.push(startNode);

    Node* goalNode = nullptr;
    qDebug() << "---------start---------";

    while (!openSet.empty()) {
        Node* current = openSet.top();
        openSet.pop();


        if (current->x == goal.x() && current->y == goal.y()) {
            goalNode = current;
            break;
        }

        if (closedSet[current->x + current->y * width + current->theta * width * height]==1) continue;
        closedSet[current->x + current->y * width + current->theta * width * height] = 1;
        for (int i = 0; i < 9; ++i) {
            float enlarge_factor = 10; //set basic grid to decimeter
            
            float L = 0.3*enlarge_factor; //distance between two wheels (meter)
            float d_wheel = 0.25*enlarge_factor; //wheel diameter (meter)
            float max_rpm = 5; //maximum rpm
            
            float next_l_rpm, next_r_rpm;
            float move_distance = 0;
            float turn_angle = 0;

            next_l_rpm = current->l_rpm + d_l[i];
            next_r_rpm = current->r_rpm + d_r[i];

            if(next_l_rpm>max_rpm) {next_l_rpm = max_rpm;}
            if(next_r_rpm>max_rpm) {next_r_rpm = max_rpm;}
            if(next_l_rpm<-max_rpm) {next_l_rpm = -max_rpm;}
            if(next_r_rpm<-max_rpm) {next_r_rpm = -max_rpm;}

            float next_l_v = (next_l_rpm) * 2 * M_PI * d_wheel / 60;
            float next_r_v = (next_r_rpm) * 2 * M_PI * d_wheel / 60;
            float next_v = (next_r_v + next_l_v) / 2;
            float next_omega = (next_r_v - next_l_v) / L;
            if (next_v/(next_omega + 1e-6) < 1) continue;
            if (next_v ==0) continue;

            int steps = 10;
            float dt = abs(1/next_v) / steps; //time step (second)

            float next_theta = current->theta, next_x = current->real_x, next_y = current->real_y;          
            int nx, ny;

            for(int i=0; i<steps; i++){
                next_theta = next_theta + next_omega * dt;
                next_x = next_x + next_v * cos(next_theta) * dt;
                next_y = next_y + next_v * sin(next_theta) * dt;
                move_distance = move_distance + abs(next_v) * dt;
                turn_angle = turn_angle + abs(next_omega) * dt;
            }
            //wrap theta to degree(0-359)
            next_theta = atan2(sin(next_theta), cos(next_theta));
            next_theta = next_theta * 180 / M_PI;
            if (next_theta < 0) next_theta = next_theta + 360;
            
            int i_theta = static_cast<int>(next_theta);
            if (i_theta >= 360) i_theta = 0; // Robustness against 360.0

            nx = round(next_x);
            ny = round(next_y);
            
            if (isValid(nx, ny, map)) {
                 int idx = nx + ny * width + i_theta * width * height;

                 if (closedSet[idx] == 0) {
                    float d_theta = atan2(goal.y()-next_y, goal.x()-next_x) - next_theta;
                    d_theta = atan2(sin(d_theta), cos(d_theta)) ;
                    float newG = current->g + move_distance + abs(0.1/next_v);               //qDebug() << next_x-current->real_x << " " << next_y-current->real_y << " hypot: " << std::hypot(next_x - current->real_x, next_y - current->real_y); 
                    Node* neighbor = allNodes[nx][ny][i_theta]; 
                    if (neighbor == nullptr) {
                        neighbor = new Node(nx, ny, i_theta);
                        allNodes[nx][ny][i_theta] = neighbor;
                        neighbor->g = newG;
                        neighbor->h = std::hypot(next_x - goal.x(), next_y - goal.y()) + abs(d_theta)*0;
                        neighbor->real_x = next_x;
                        neighbor->real_y = next_y;
                        neighbor->theta = i_theta;
                        neighbor->l_rpm = next_l_rpm;
                        neighbor->r_rpm = next_r_rpm;
                        neighbor->parent = current;
                        openSet.push(neighbor);
                        //qDebug() << "1neighbor: (" << neighbor->x << ", " << neighbor->y << ", " << neighbor->theta << ")" << "(" << neighbor->real_x << ", " << neighbor->real_y << "," << neighbor->theta << ")" << "RPM: (" << neighbor->l_rpm << ", " << neighbor->r_rpm << ")" <<"From: (" << current->x << ", " << current->y << current->theta <<")"; 
                        //qDebug() << "move_distance: " << move_distance << ", g: " << neighbor->g << ", h: " << neighbor->h << " " << neighbor->f() << "d_theta: " << d_theta;
                    } else if (newG < neighbor->g) {
                        neighbor->g = newG;
                        neighbor->h = std::hypot(next_x - goal.x(), next_y - goal.y()) + abs(d_theta)*0;
                        neighbor->real_x = next_x;
                        neighbor->real_y = next_y;
                        neighbor->theta = i_theta;
                        neighbor->l_rpm = next_l_rpm;
                        neighbor->r_rpm = next_r_rpm;
                        neighbor->parent = current;
                        openSet.push(neighbor);
                        //qDebug() << "2neighbor: (" << neighbor->x << ", " << neighbor->y << ", " << neighbor->theta << ")" << "(" << neighbor->real_x << ", " << neighbor->real_y << "," << neighbor->theta << ")" << "RPM: (" << neighbor->l_rpm << ", " << neighbor->r_rpm << ")" <<"From: (" << current->x << ", " << current->y << current->theta <<")"; 
                        //qDebug() << "move_distance: " << move_distance << ", g: " << neighbor->g << ", h: " << neighbor->h << " " << neighbor->f() << "d_theta: " << d_theta;

                    }
                }

            }
        }
    }

    std::vector<QPoint> path;
    if (goalNode) {
        Node* curr = goalNode;
        while (curr) {
            path.push_back(QPoint(curr->real_x, curr->real_y));
            curr = curr->parent;
        }
        // Memory cleanup could be done here (delete all nodes in allNodes)
        // For simple app, OS will reclaim, but better to be safe in long run.
    }
    
    // Cleanup
    for(int i=0; i<width; ++i) {
        for(int j=0; j<height; ++j) {
            for(int k=0; k<layers; ++k) {   
                if(allNodes[i][j][k]) delete allNodes[i][j][k];
            }
        }
    }

    return path;
}

void MainWindow::onExit()
{
    QApplication::quit();
}
