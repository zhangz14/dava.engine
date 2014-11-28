#ifndef __DROPPPER_SHADE_
#define __DROPPPER_SHADE_

#include <QWidget>
#include <QPointer>
#include <QImage>
#include <QPixmap>


class MouseHelper;

class DropperShade
    : public QWidget
{
    Q_OBJECT

signals:
    void canceled();
    void picked(const QColor& color);
    void changed(const QColor& color);
    void moved(const QPoint& pos);
    void zoomFactorChanged(int delta);

public:
    DropperShade( const QPixmap& src, const QRect& rect );
    ~DropperShade();

private slots:
    void OnMouseMove(const QPoint& pos);
    void OnClicked(const QPoint& pos);
    void OnMouseWheel(int delta);

private:
    void paintEvent(QPaintEvent* e);
    void keyPressEvent(QKeyEvent* e);
    QColor GetPixel(const QPoint& pos) const;

    const QPixmap pixmap;
    const QImage image;
    QPointer<MouseHelper> mouse;
};



#endif
