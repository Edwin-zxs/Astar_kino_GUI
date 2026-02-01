#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QSpinBox>
#include <QGroupBox>
#include <vector>

struct Node {
    int x, y, theta; // theta in degree(0-359)
    float real_x, real_y;
    float g, h;
    int l_rpm, r_rpm;
    Node* parent;

    Node(int _x, int _y, int _theta) : x(_x), y(_y), real_x(_x), real_y(_y), theta(_theta), g(0), h(0), l_rpm(0), r_rpm(0), parent(nullptr) {}
    float f() const { return g + h; }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoadMap();
    void onExit();
    void onSearchPath();

private:
    // Helper for A*
    std::vector<QPoint> findPath(QImage map, QPoint start, QPoint goal);
    bool isValid(int x, int y, const QImage& map);
    
    // Map Display Helper
    void updateMapDisplay();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
    
    // Top Control Area
    QWidget *topContainer;
    QHBoxLayout *topLayout;

    // Left: Inputs
    QGroupBox *inputGroup;
    QGridLayout *inputLayout;
    QLabel *startLabel;
    QSpinBox *startXSpin;
    QSpinBox *startYSpin;
    QLabel *goalLabel;
    QSpinBox *goalXSpin;
    QSpinBox *goalYSpin;

    // Right: Buttons
    QWidget *buttonContainer;
    QVBoxLayout *buttonLayout;
    QPushButton *loadMapBtn;
    QPushButton *searchPathBtn;
    QPushButton *exitBtn;
    
    // Bottom: Map
    QScrollArea *scrollArea;
    QLabel *mapLabel;
    QImage currentMap;
    QImage displayedImage; // Image currently shown (raw or with path)

    // Execution Time Display
    QLabel *timeLabel;
};

#endif // MAINWINDOW_H
