#pragma once

#include <QObject>

class MainWindow;

class AppMenu : public QObject {
    Q_OBJECT

public:
    explicit AppMenu(MainWindow* window);
};
