/*
    Copyright (C) 2017  Peter Baxendale

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    model.setRootPath("");

    loadFileList();
    showLabels(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::loadFileList(void)
{
    ui->fileTreeView->setModel(&model);

    QString rootPath = QDir::homePath();
    const QModelIndex rootIndex = model.index(QDir::cleanPath(rootPath));
    if (rootIndex.isValid())
          ui->fileTreeView->setRootIndex(rootIndex);

    ui->fileTreeView->setColumnHidden(1,true);
    ui->fileTreeView->setColumnHidden(2,true);
    ui->fileTreeView->setColumnHidden(3,true);
    ui->fileTreeView->setHeaderHidden(true);
    ui->fileTreeView->header()->setResizeMode(QHeaderView::ResizeToContents);
    ui->fileTreeView->header()->setStretchLastSection(false);

}


void MainWindow::showLabels(bool show)
{
    ui->pushButton_next->setVisible(show);
    ui->pushButton_previous->setVisible(show);
    ui->label_comment->setVisible(show);
    ui->label_date->setVisible(show);
    ui->label_description->setVisible(show);
    ui->graphicsView_image->setVisible(show);
}

void MainWindow::clearLabels()
{
    ui->label_comment->clear();
    ui->label_date->clear();
    ui->label_description->clear();
    scene->clear();
}

void MainWindow::on_fileTreeView_doubleClicked(const QModelIndex &index)
{
   nextImage(index);
}




void MainWindow::nextImage(const QModelIndex &index)
{
    // get filename as qstring and C++ string and show in status bar
    fnam = model.filePath(index);

    QFileInfo info(fnam);       // ignore if it's a directory
    if(info.isDir()) return;

    fnam_utf8 = fnam.toUtf8().constData();
    ui->statusBar->showMessage(fnam);

    int ftype = Exiv2::ImageFactory::getType(fnam_utf8);
    if(ftype == Exiv2::ImageType::none)
    {
        clearLabels();
        QMessageBox errmsg;
        errmsg.setText("Unsuported File Type for file:\n"+fnam);
        errmsg.setIcon(QMessageBox::Critical);
        errmsg.exec();

        return;
    }

    // load the image meta data
    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(fnam_utf8);
    if(image.get() != 0)
    {
      image->readMetadata();
      exif = image->exifData();
      if (!exif.empty())
      {
        showDate();
        showDescription();
        showComment();
        getOrientation();
      }
      else
      {
          ui->statusBar->showMessage("no exif data: "+fnam);
          return;
      }
    }
    else
    {
        ui->statusBar->showMessage("failed to open file: "+fnam);
        return;
    }

    // show the picture

    delete scene;       // delete previous instances (if any)
    delete imageObject;
    delete picture;

    scene = new QGraphicsScene();
    imageObject = new QImage;

    imageObject->load(fnam);
    picture = new QPixmap(QPixmap::fromImage(*imageObject));

    showLabels(true);
    displayImage();     // show the graphics box before scaling to fit
}


void MainWindow::showDate()
{
    Exiv2::ExifKey key("Exif.Photo.DateTimeOriginal");
    Exiv2::ExifData::iterator pos = exif.findKey(key);
    if (! (pos == exif.end()))
    {
        std::string s = pos->getValue()->toString();
        const char * str = s.c_str();
        QString d(str);
        ui->label_date->setText("Date taken: "+d);
    }
    else ui->label_date->setText("(date taken not available)");
}


void MainWindow::showDescription()
{
    Exiv2::ExifKey key("Exif.Image.ImageDescription");
    Exiv2::ExifData::iterator pos = exif.findKey(key);
    if (! (pos == exif.end()))
    {
        std::string s = pos->getValue()->toString();
        const char * str = s.c_str();
        QString d(str);
        ui->label_description->setText(d);
    }
    else  ui->label_description->setText("");
}


void MainWindow::showComment()
{
    Exiv2::ExifKey key("Exif.Photo.UserComment");
    Exiv2::ExifData::iterator pos = exif.findKey(key);
    if (! (pos == exif.end()))
    {
        std::string s = pos->getValue()->toString();
        const char * str = s.c_str();
        QString d(str);
        if(d.startsWith("charset=\"Ascii\" "))
        {
          d.remove("charset=\"Ascii\" ");
        }

        ui->label_comment->setText(d);
    }
    else ui->label_comment->setText("");
}


void MainWindow::getOrientation()
{
    Exiv2::ExifKey key("Exif.Image.Orientation");
    Exiv2::ExifData::iterator pos = exif.findKey(key);

    if(! (pos == exif.end())) orientation = pos->toLong(); // get exif orientation value
    else orientation = 1;
}

void MainWindow::displayImage()
{
    int angle=0;

    switch(orientation)
    {
    case 8:
        angle = 270;
        break;
    case 3:
        angle = 180;
        break;
    case 6:
        angle = 90;
    }

    scene->clear();

    if(angle != 0)
    {
        QTransform transform;
        QTransform trans = transform.rotate(angle);
        QPixmap transImage = QPixmap(picture->transformed(trans));
        scene->addPixmap(transImage);
        scene->setSceneRect(transImage.rect());
    }

    else
    {
        scene->addPixmap(*picture);
        scene->setSceneRect(picture->rect());
    }

    ui->graphicsView_image->setScene(scene);
    ui->graphicsView_image->fitInView(ui->graphicsView_image->sceneRect(),Qt::KeepAspectRatio);
}



void MainWindow::on_pushButton_next_clicked()
{
    int currentRow = ui->fileTreeView->currentIndex().row();
    QModelIndex p = ui->fileTreeView->currentIndex().parent();
    QModelIndex index = ui->fileTreeView->model()->index(currentRow+1,0,p);
    if(index.isValid())
    {
        ui->fileTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
        ui->fileTreeView->selectionModel()->select(index,QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        nextImage(index);
    }
}

void MainWindow::on_pushButton_previous_clicked()
{
    int currentRow = ui->fileTreeView->currentIndex().row();
    QModelIndex p = ui->fileTreeView->currentIndex().parent();
    QModelIndex index = ui->fileTreeView->model()->index(currentRow-1,0,p);
    if(index.isValid())
    {
        ui->fileTreeView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::SelectCurrent);
        ui->fileTreeView->selectionModel()->select(index,QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        nextImage(index);
    }
}
