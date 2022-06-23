#include "animestackedwidget.h"


AnimeStackedWidget::AnimeStackedWidget(QWidget *parent)
    : QStackedWidget(parent), m_speed(200), m_vertical(false),
      m_now(0), m_next(0), m_wrap(false), m_active(false),
      m_pnow(QPoint(0,0)), m_animationtype(QEasingCurve::OutQuart)
{
    if (parent) {
      m_mainwindow = parent;
    } else {
      m_mainwindow = this;
    }
}


void AnimeStackedWidget::slideInIdx(int idx, enum Direction direction)
{
    if (m_active) {
        animgroup->stop();
    }

    if (idx > count()-1) {
      direction = m_vertical ? TOP2BOTTOM : RIGHT2LEFT;
      idx = idx % count();
    } else if (idx < 0) {
      direction = m_vertical ? BOTTOM2TOP : LEFT2RIGHT;
      idx = (idx+count()) % count();
    }

    slideInWgt(widget(idx), direction);
}

void AnimeStackedWidget::slideInWgt(QWidget* newWidget, enum Direction direction)
{
    m_active = true;

    enum Direction directionhint;

    int now = currentIndex();
    int next = indexOf(newWidget);
    if (now == next) {
          m_active = false;
          return;
    }
    else if (now < next) {
          directionhint = m_vertical ? TOP2BOTTOM : RIGHT2LEFT;
    } else {
          directionhint = m_vertical ? BOTTOM2TOP : LEFT2RIGHT;
    }

    if (direction == AUTOMATIC) {
          direction = directionhint;
    }


    int offsetx = frameRect().width();
    int offsety = frameRect().height();


    widget(next)->setGeometry(0, 0, offsetx, offsety);
    if (direction == BOTTOM2TOP) {
          offsetx = 0;
          offsety = -offsety;
    } else if (direction == TOP2BOTTOM) {
          offsetx = 0;
    } else if (direction == RIGHT2LEFT) {
          offsetx = -offsetx;
          offsety = 0;
    } else if (direction == LEFT2RIGHT) {
          offsety = 0;
    }

    QPoint pnext=widget(next)->pos();
    QPoint pnow=widget(now)->pos();
    m_pnow=pnow;
    widget(next)->move(pnext.x()-offsetx,pnext.y()-offsety);

    widget(next)->show();
    widget(next)->raise();

    QPropertyAnimation *animnow = new QPropertyAnimation(widget(now), "pos");
    animnow->setDuration(m_speed);
    animnow->setEasingCurve(m_animationtype);
    animnow->setStartValue(QPoint(pnow.x(), pnow.y()));
    animnow->setEndValue(QPoint(offsetx+pnow.x(), offsety+pnow.y()));

    QGraphicsOpacityEffect *animnow_op_eff = new QGraphicsOpacityEffect();
    widget(now)->setGraphicsEffect(animnow_op_eff);
    QPropertyAnimation *animnow_op = new QPropertyAnimation(animnow_op_eff, "opacity");
    animnow_op->setDuration(m_speed/2);
    animnow_op->setStartValue(1);
    animnow_op->setEndValue(0);

    QGraphicsOpacityEffect *animnext_op_eff = new QGraphicsOpacityEffect();
    animnext_op_eff->setOpacity(0);
    widget(next)->setGraphicsEffect(animnext_op_eff);
    QPropertyAnimation *animnext_op = new QPropertyAnimation(animnext_op_eff, "opacity");
    animnext_op->setDuration(m_speed/2);
    animnext_op->setStartValue(0);
    animnext_op->setEndValue(1);

    QPropertyAnimation *animnext = new QPropertyAnimation(widget(next), "pos");
    animnext->setDuration(m_speed);
    animnext->setEasingCurve(m_animationtype);
    animnext->setStartValue(QPoint(-offsetx+pnext.x(), offsety+pnext.y()));
    animnext->setEndValue(QPoint(pnext.x(), pnext.y()));

    animgroup = new QParallelAnimationGroup;
    animgroup->addAnimation(animnow);
    animgroup->addAnimation(animnext);
    animgroup->addAnimation(animnow_op);
    animgroup->addAnimation(animnext_op);

    connect(animgroup, SIGNAL(stateChanged(QAbstractAnimation::State,QAbstractAnimation::State)),
            this, SLOT(animationDoneSlot(QAbstractAnimation::State,QAbstractAnimation::State)));
    m_next=next;
    m_now=now;
    m_active=true;
    animgroup->start();
}


void AnimeStackedWidget::animationDoneSlot(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
{
    Q_UNUSED(oldState);

    if (newState == QAbstractAnimation::State::Running)
        return;

    setCurrentIndex(m_next);
    widget(m_now)->hide();
    widget(m_now)->move(m_pnow);
    m_active = false;
}


