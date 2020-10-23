#include "ValueSlider.h"

#include <QPainter>
#include <QApplication>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QStyleOption>
#include <QKeyEvent>
#include <QDebug>

#include "../Helpers/MouseHelper.h"
#include "../Helpers/PaintingHelper.h"

namespace
{
const QString editorQss =
"border: 1px solid black;\n"
"background: transparent;"
"padding: 0px;"
"margin: 0px;"
;
}

ValueSlider::ValueSlider(QWidget* parent)
    : QWidget(parent)
    , minVal(0)
    , maxVal(1)
    , val(0)
    , clickVal(0)
    , mouse(new MouseHelper(this))
    , digitsAfterDot(6)
{
    connect(mouse, SIGNAL(mousePress(const QPoint&)), SLOT(OnMousePress(const QPoint&)));
    connect(mouse, SIGNAL(mouseMove(const QPoint&)), SLOT(OnMouseMove(const QPoint&)));
    connect(mouse, SIGNAL(mouseRelease(const QPoint&)), SLOT(OnMouseRelease(const QPoint&)));
    connect(mouse, SIGNAL(clicked()), SLOT(OnMouseClick()));
}

ValueSlider::~ValueSlider()
{
}

void ValueSlider::SetDigitsAfterDot(int c)
{
    digitsAfterDot = c;
}

void ValueSlider::SetRange(double min, double max)
{
    minVal = qMin(min, max);
    maxVal = qMax(min, max);
    SetValue(val); // Validation
}

void ValueSlider::SetValue(double _val)
{
    val = _val;
    normalize();
    update();
}

double ValueSlider::GetValue() const
{
    return val;
}

void ValueSlider::DrawBackground(QPainter* p) const
{
    const QRect rc = PosArea().adjusted(0, 0, -1, -1);

    p->fillRect(rc, Qt::gray);
    p->setPen(QPen(Qt::black, 1));
    p->drawRect(rc);
}

void ValueSlider::DrawForeground(QPainter* p) const
{
    const double relVal = (val - minVal) / (maxVal - minVal);
    const int w = int((PosArea().width() - 2) * relVal);
    const QRect rc(1, 1, w, PosArea().height() - 2);
    const QRect clip = PosArea().adjusted(1, 1, -2, -1);

    const QColor c1(235, 235, 235);
    const QColor c2(200, 200, 200);
    const QColor c3(235, 235, 235);

    QLinearGradient gradient(1, 1, 1, rc.height());
    gradient.setColorAt(0.0, c1);
    gradient.setColorAt(0.8, c2);
    gradient.setColorAt(1.0, c3);

    p->setClipRect(clip);
    p->fillRect(rc, gradient);

    if (!IsEditorMode())
    {
        QStyleOptionFrameV2 panel;
        panel.initFrom(this);
        style()->drawItemText(p, clip.adjusted(3, 0, 0, 0), (Qt::AlignLeft | Qt::AlignVCenter), palette(), true, QString::number(val, 'f', digitsAfterDot), QPalette::WindowText);

        if (arrows.isNull())
        {
            const int div = 3;
            const QSize arrowSize(clip.height() / div + 1, clip.height() / div + 1);
            const QImage& left = PaintingHelper::DrawArrowIcon(arrowSize, Qt::RightEdge);
            const QImage& right = PaintingHelper::DrawArrowIcon(arrowSize, Qt::LeftEdge);

            arrows = QPixmap(arrowSize.width() * 2, arrowSize.height());
            arrows.fill(Qt::transparent);
            QPainter pa(&arrows);
            pa.drawImage(0, 0, left);
            pa.drawImage(arrowSize.width(), 0, right);
        }

        p->drawPixmap(clip.width() - arrows.width(), (clip.height() - arrows.height()) / 2, arrows);
    }
}

QRect ValueSlider::PosArea() const
{
    return QRect(0, 0, width(), height());
}

void ValueSlider::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter p(this);

    p.save();
    DrawBackground(&p);
    p.restore();
    p.save();
    DrawForeground(&p);
    p.restore();
}

void ValueSlider::resizeEvent(QResizeEvent* e)
{
    Q_UNUSED(e);
}

bool ValueSlider::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == editor)
    {
        switch (e->type())
        {
        case QEvent::KeyPress:
        {
            QKeyEvent* ke = static_cast<QKeyEvent*>(e);
            switch (ke->key())
            {
            case Qt::Key_Enter:
            case Qt::Key_Return:
            case Qt::Key_Escape:
            case Qt::Key_Tab:
                acceptEditing();
                break;

            default:
                break;
            }
        }
        break;

        case QEvent::FocusOut:
            acceptEditing();
            break;

        default:
            break;
        }
    }

    return QWidget::eventFilter(obj, e);
}

bool ValueSlider::IsEditorMode() const
{
    return !editor.isNull();
}

void ValueSlider::OnMousePress(const QPoint& pos)
{
    clickPos = pos;
    clickVal = val;

    QApplication::setOverrideCursor(Qt::BlankCursor);
    emit started(GetValue());
}

void ValueSlider::OnMouseMove(const QPoint& pos)
{
    if (mouse->IsPressed())
    {
        const int dist = pos.x() - clickPos.x();
        const int w = width();
        const double dd = double(dist) / double(w);
        const double dw = (maxVal - minVal);
        const double ofs = dd * dw;

        val = clickVal + ofs;
        normalize();
        emit changing(GetValue());

        repaint();
    }
}

void ValueSlider::OnMouseRelease(const QPoint& pos)
{
    QCursor::setPos(mapToGlobal(clickPos));
    QApplication::restoreOverrideCursor();

    if (clickVal != val)
    {
        emit changed(GetValue());
    }
    else
    {
        emit canceled();
    }
}

void ValueSlider::OnMouseClick()
{
    if (editor)
        delete editor;

    QRegExpValidator* validator = new QRegExpValidator(QRegExp("\\s*-? \\d*[, \\.] ? \\d*\\s*"));

    editor = new QLineEdit(this);
    editor->installEventFilter(this);
    editor->setValidator(validator);
    editor->setStyleSheet(editorQss);
    editor->move(0, 0);
    editor->resize(width(), height());

    editor->setText(QString::number(val, 'f', digitsAfterDot));

    editor->show();
    editor->setFocus();
    update();
}

void ValueSlider::normalize()
{
    if (val < minVal)
    {
        val = minVal;
    }
    if (val > maxVal)
    {
        val = maxVal;
    }
}

void ValueSlider::undoEditing()
{
    if (editor)
    {
        editor->deleteLater();
    }
}

void ValueSlider::acceptEditing()
{
    if (editor)
    {
        const QString text = editor->text();

        bool ok = false;
        const double newVal = text.toDouble(&ok);
        if (ok)
        {
            val = newVal;
            normalize();
        }

        editor->deleteLater();
        repaint();
    }
}
