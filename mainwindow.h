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
    int x, y;
    float g, h;
    int l_rpm, r_rpm;
    Node* parent;

    Node(int _x, int _y) : x(_x), y(_y), g(0), h(0), l_rpm(0), r_rpm(0), parent(nullptr) {}
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
};

#endif // MAINWINDOW_H
