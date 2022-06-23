#ifndef ANIMESTACKEDWIDGET_H
#define ANIMESTACKEDWIDGET_H


#include <QStackedWidget>
#include <QEasingCurve>
#include <QParallelAnimationGroup>

#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

#include <QStyledItemDelegate>


class AnimeStackedWidget : public QStackedWidget {

    Q_OBJECT

public:
    // Animation direction
    enum Direction {
        LEFT2RIGHT,
        RIGHT2LEFT,
        TOP2BOTTOM,
        BOTTOM2TOP,
        AUTOMATIC
    };

    AnimeStackedWidget(QWidget *parent);

    void slideInIdx(int idx, enum Direction direction=AUTOMATIC);

private slots:
    void animationDoneSlot(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);

private:
    void slideInWgt(QWidget * widget, enum Direction direction=AUTOMATIC);

    QWidget *m_mainwindow;

    int m_speed;
    bool m_vertical;
    int m_now;
    int m_next;
    bool m_wrap;
    bool m_active;
    QPoint m_pnow;
    enum QEasingCurve::Type m_animationtype;
    QList<QWidget*> blockedPageList;
    QParallelAnimationGroup *animgroup;
};


// Bonus for treewidget
class TreeWidgetFontDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->font = QFont("Consolas", 10, 100);
    }
};


#endif // ANIMESTACKEDWIDGET_H
