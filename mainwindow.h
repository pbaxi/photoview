#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QGraphicsScene>
#include <exiv2/exiv2.hpp>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_fileTreeView_doubleClicked(const QModelIndex &index);
    void on_pushButton_next_clicked();

    void on_pushButton_previous_clicked();

private:
    Ui::MainWindow *ui;
    QFileSystemModel model;

    QString fnam;
    std::string fnam_utf8;

    QImage  *imageObject = NULL;
    QGraphicsScene *scene = NULL;
    QPixmap *picture = NULL;

    Exiv2::ExifData exif;
    int orientation;

    void loadFileList(void);
    void showLabels(bool show);
    void clearLabels(void);

    void showDate(void);
    void showDescription(void);
    void showComment(void);
    void getOrientation(void);
    void displayImage(void);
    void nextImage(const QModelIndex &index);
};

#endif // MAINWINDOW_H
