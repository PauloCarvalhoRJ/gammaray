/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "widgets/imageviewerwidget.h"
#include "domain/application.h"

#include <QApplication>
#include <QClipboard>
// #include <QColorSpace> TODO Qt 5.14
#include <QDir>
#include <QFileDialog>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStatusBar>
#include <QVBoxLayout>

#if defined(QT_PRINTSUPPORT_LIB)
#  include <QtPrintSupport/qtprintsupportglobal.h>

#  if QT_CONFIG(printdialog)
#    include <QPrintDialog>
#  endif
#endif

ImageViewerWidget::ImageViewerWidget(QWidget *parent)
   : QWidget(parent), m_imageLabel(new QLabel)
   , m_scrollArea(new QScrollArea),
     m_saveAsAct(nullptr),
     m_printAct(nullptr),
     m_copyAct(nullptr),
     m_zoomInAct(nullptr),
     m_zoomOutAct(nullptr),
     m_normalSizeAct(nullptr),
     m_fitToWindowAct(nullptr)
{
    this->setLayout( new QVBoxLayout() );

    m_imageLabel->setBackgroundRole(QPalette::Base);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_imageLabel->setScaledContents(true);

    m_scrollArea->setBackgroundRole(QPalette::Dark);
    m_scrollArea->setWidget(m_imageLabel);
    //scrollArea->setVisible(false);
    this->layout()->addWidget( m_scrollArea );

    createActions();

    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}

bool ImageViewerWidget::loadFile(const QString &fileName)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }

    setImage(newImage);

    setWindowFilePath(fileName);

    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
        .arg(QDir::toNativeSeparators(fileName)).arg(m_image.width()).arg(m_image.height()).arg(m_image.depth());

    Application::instance()->logInfo("ImageViewerWidget::loadFile(): " + message );

    return true;
}

void ImageViewerWidget::setImage(const QImage &newImage)
{
    m_image = newImage;
//    if (image.colorSpace().isValid()) TODO: Qt 5.14
//        image.convertToColorSpace(QColorSpace::SRgb); TODO: Qt 5.14
    m_imageLabel->setPixmap(QPixmap::fromImage(m_image));
    m_scaleFactor = 1.0;

    m_scrollArea->setVisible(true);
    if( m_printAct )
        m_printAct->setEnabled(true);
    if( m_fitToWindowAct )
        m_fitToWindowAct->setEnabled(true);
    updateActions();

    if ( m_fitToWindowAct && !m_fitToWindowAct->isChecked())
        m_imageLabel->adjustSize();
    if( ! m_fitToWindowAct )
        m_imageLabel->adjustSize();
}

bool ImageViewerWidget::saveFile(const QString &fileName)
{
    QImageWriter writer(fileName);

    if (!writer.write(m_image)) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot write %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName)), writer.errorString());
        return false;
    }
    const QString message = tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
    Application::instance()->logInfo("ImageViewerWidget::loadFile(): " + message );
    return true;
}

static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
        ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    for (const QByteArray &mimeTypeName : supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

void ImageViewerWidget::open()
{
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}

void ImageViewerWidget::saveAs()
{
    QFileDialog dialog(this, tr("Save File As"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptSave);

    while (dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().first())) {}
}

void ImageViewerWidget::print()
{
 //   Q_ASSERT(!imageLabel->pixmap(Qt::ReturnByValue).isNull());  TODO: Qt 5.14
#if defined(QT_PRINTSUPPORT_LIB) && QT_CONFIG(printdialog)
    QPrintDialog dialog(&printer, this);
    if (dialog.exec()) {
        QPainter painter(&printer);
        QPixmap pixmap = imageLabel->pixmap(Qt::ReturnByValue);
        QRect rect = painter.viewport();
        QSize size = pixmap.size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(pixmap.rect());
        painter.drawPixmap(0, 0, pixmap);
    }
#endif
}

void ImageViewerWidget::copy()
{
#ifndef QT_NO_CLIPBOARD
    QGuiApplication::clipboard()->setImage(m_image);
#endif // !QT_NO_CLIPBOARD
}

#ifndef QT_NO_CLIPBOARD
static QImage clipboardImage()
{
    if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData()) {
        if (mimeData->hasImage()) {
            const QImage image = qvariant_cast<QImage>(mimeData->imageData());
            if (!image.isNull())
                return image;
        }
    }
    return QImage();
}
#endif // !QT_NO_CLIPBOARD

void ImageViewerWidget::paste()
{
#ifndef QT_NO_CLIPBOARD
    const QImage newImage = clipboardImage();
    if (newImage.isNull()) {
        Application::instance()->logWarn("ImageViewerWidget::paste(): No image in clipboard.");
    } else {
        setImage(newImage);
        setWindowFilePath(QString());
        const QString message = tr("Obtained image from clipboard, %1x%2, Depth: %3")
            .arg(newImage.width()).arg(newImage.height()).arg(newImage.depth());
        Application::instance()->logInfo("ImageViewerWidget::paste(): " + message);
    }
#endif // !QT_NO_CLIPBOARD
}

void ImageViewerWidget::zoomIn()
{
    scaleImage(1.25);
}

void ImageViewerWidget::zoomOut()
{
    scaleImage(0.8);
}

void ImageViewerWidget::normalSize()
{
    m_imageLabel->adjustSize();
    m_scaleFactor = 1.0;
}

void ImageViewerWidget::fitToWindow()
{
    bool fitToWindow = true;
    if( m_fitToWindowAct )
        fitToWindow = m_fitToWindowAct->isChecked();
    m_scrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow)
        normalSize();
    updateActions();
}

void ImageViewerWidget::about()
{
    QMessageBox::about(this, tr("About Image Viewer"),
            tr("<p>The <b>Image Viewer</b> example shows how to combine QLabel "
               "and QScrollArea to display an image. QLabel is typically used "
               "for displaying a text, but it can also display an image. "
               "QScrollArea provides a scrolling view around another widget. "
               "If the child widget exceeds the size of the frame, QScrollArea "
               "automatically provides scroll bars. </p><p>The example "
               "demonstrates how QLabel's ability to scale its contents "
               "(QLabel::scaledContents), and QScrollArea's ability to "
               "automatically resize its contents "
               "(QScrollArea::widgetResizable), can be used to implement "
               "zooming and scaling features. </p><p>In addition the example "
               "shows how to use QPainter to print an image.</p>"));
}

void ImageViewerWidget::createActions()
{
//    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

//    QAction *openAct = fileMenu->addAction(tr("&Open..."), this, &ImageViewerWidget::open);
//    openAct->setShortcut(QKeySequence::Open);

//    m_saveAsAct = fileMenu->addAction(tr("&Save As..."), this, &ImageViewerWidget::saveAs);
//    m_saveAsAct->setEnabled(false);

//    m_printAct = fileMenu->addAction(tr("&Print..."), this, &ImageViewerWidget::print);
//    m_printAct->setShortcut(QKeySequence::Print);
//    m_printAct->setEnabled(false);

//    fileMenu->addSeparator();

//    QAction *exitAct = fileMenu->addAction(tr("E&xit"), this, &QWidget::close);
//    exitAct->setShortcut(tr("Ctrl+Q"));

//    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

//    m_copyAct = editMenu->addAction(tr("&Copy"), this, &ImageViewerWidget::copy);
//    m_copyAct->setShortcut(QKeySequence::Copy);
//    m_copyAct->setEnabled(false);

//    QAction *pasteAct = editMenu->addAction(tr("&Paste"), this, &ImageViewerWidget::paste);
//    pasteAct->setShortcut(QKeySequence::Paste);

//    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

//    m_zoomInAct = viewMenu->addAction(tr("Zoom &In (25%)"), this, &ImageViewerWidget::zoomIn);
//    m_zoomInAct->setShortcut(QKeySequence::ZoomIn);
//    m_zoomInAct->setEnabled(false);

//    m_zoomOutAct = viewMenu->addAction(tr("Zoom &Out (25%)"), this, &ImageViewerWidget::zoomOut);
//    m_zoomOutAct->setShortcut(QKeySequence::ZoomOut);
//    m_zoomOutAct->setEnabled(false);

//    m_normalSizeAct = viewMenu->addAction(tr("&Normal Size"), this, &ImageViewerWidget::normalSize);
//    m_normalSizeAct->setShortcut(tr("Ctrl+S"));
//    m_normalSizeAct->setEnabled(false);

//    viewMenu->addSeparator();

//    m_fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"), this, &ImageViewerWidget::fitToWindow);
//    m_fitToWindowAct->setEnabled(false);
//    m_fitToWindowAct->setCheckable(true);
//    m_fitToWindowAct->setShortcut(tr("Ctrl+F"));

//    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

//    helpMenu->addAction(tr("&About"), this, &ImageViewerWidget::about);
//    helpMenu->addAction(tr("About &Qt"), &QApplication::aboutQt);
}

void ImageViewerWidget::updateActions()
{
    if( m_saveAsAct && m_copyAct && m_zoomInAct && m_zoomOutAct && m_normalSizeAct ) {
        m_saveAsAct->setEnabled(!m_image.isNull());
        m_copyAct->setEnabled(!m_image.isNull());
        m_zoomInAct->setEnabled(!m_fitToWindowAct->isChecked());
        m_zoomOutAct->setEnabled(!m_fitToWindowAct->isChecked());
        m_normalSizeAct->setEnabled(!m_fitToWindowAct->isChecked());
    }
}

void ImageViewerWidget::scaleImage(double factor)
{
    m_scaleFactor *= factor;
//    imageLabel->resize(scaleFactor * imageLabel->pixmap(Qt::ReturnByValue).size()); TODO: Qt 5.14
    m_imageLabel->resize(m_scaleFactor * m_imageLabel->pixmap()->size());

    adjustScrollBar(m_scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(m_scrollArea->verticalScrollBar(), factor);

    if( m_zoomInAct && m_zoomInAct ) {
        m_zoomInAct->setEnabled(m_scaleFactor < 3.0);
        m_zoomOutAct->setEnabled(m_scaleFactor > 0.333);
    }
}

void ImageViewerWidget::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value()
                            + ((factor - 1) * scrollBar->pageStep()/2)));
}
