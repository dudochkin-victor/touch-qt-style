#include <qtscrollerfilter_p.h>
#include <qtscrollevent.h>
#include <qtscroller.h>
#include <QAbstractItemView>
#include <QScrollBar>
#ifndef NOWEBKIT
#include <QWebFrame>
#include <QWebView>
#endif
#include <QGestureEvent>
#include <QGesture>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QCoreApplication>

#ifdef Q_WS_WIN
#  include <qlibrary.h>
#  include <windows.h>
#endif

#include <QtDebug>

bool qt_sendSpontaneousEvent(QObject *receiver, QEvent *event)
{
    return QCoreApplication::sendSpontaneousEvent(receiver, event);
}


QtScrollerFilter *QtScrollerFilter::inst = 0;

QtScrollerFilter *QtScrollerFilter::instance()
{
    if (!inst)
        inst = new QtScrollerFilter();
    return inst;
}

QtScrollerFilter::QtScrollerFilter()
{ }

bool QtScrollerFilter::eventFilter(QObject *o, QEvent *e)
{
    bool result = false;
    
    //qWarning() << o << e->type();

    if (o->isWidgetType()) {
        QWidget *w = static_cast<QWidget *>(o);

#ifndef NOWEBKIT
        if (QWebView *web = qobject_cast<QWebView *>(w))
            result |= eventFilter_QWebView(web, e);
#endif

         if (w->parentWidget()) {
            if (QAbstractScrollArea *area = qobject_cast<QAbstractScrollArea *>(w->parentWidget())) {
                if (area->viewport() == w) {             
                    if (QAbstractItemView *view = qobject_cast<QAbstractItemView *>(area))
                        result |= eventFilter_QAbstractItemView(view, e);

                    result |= eventFilter_QAbstractScrollArea(area, e);
                }
            }
        }
    }
    return result;
}

#ifndef NOWEBKIT
bool QtScrollerFilter::eventFilter_QWebView(QWebView *view, QEvent *event)
{
    switch (event->type()) {
    case QtScrollPrepareEvent::ScrollPrepare: {
        QtScrollPrepareEvent *se = static_cast<QtScrollPrepareEvent *>(event);
        scrollingFrames[view] = 0;

        if (view->page()) {
            if (QWebFrame *frame = scrollingFrameAt_QWebView(view, se->startPos().toPoint())) {
                scrollingFrames[view] = frame;
                
                // there's no way to do that from outside of QtWebKit
                //// remember old selection so that we can restore it later
                //oldWebSelection = d->frame->selection()->selection();

                se->setViewportSize(frame->geometry().size());
                se->setContentPosRange(QRectF(frame->scrollBarMinimum(Qt::Horizontal),
                                              frame->scrollBarMinimum(Qt::Vertical),
                                              frame->scrollBarMaximum(Qt::Horizontal),
                                              frame->scrollBarMaximum(Qt::Vertical)));
                se->setContentPos(QPointF(frame->scrollPosition()));
                se->accept();                
                return true;
            }
        }
        return false;
        break; //to make coverity satisfied! 
    } 
    case QtScrollEvent::Scroll: {
        if (QWebFrame *frame = scrollingFrames.value(view)) {
            QtScrollEvent *se = static_cast<QtScrollEvent *>(event);
            // no way to do that from outside of QtWebKit
            //// restore the old selection
            //if (se->scrollState() == QScrollEvent::ScrollStarted)
            //    frame->d->frame->selection()->setSelection(d->oldWebSelection, CharacterGranularity);

            frame->setScrollPosition(se->contentPos().toPoint());

        } else {
            return false;
        }
        break; //to make coverity satisfied! 
    }
    default:
        return false;
    }
    return false;
}
#endif

#ifndef NOWEBKIT
QWebFrame *QtScrollerFilter::scrollingFrameAt_QWebView(QWebView *view, const QPoint &pos) const
{
    if (!view->page())
         return 0;

    QWebFrame *mainFrame = view->page()->mainFrame();
    QWebHitTestResult hitTest = mainFrame->hitTestContent(pos);
    QWebFrame *hitFrame = hitTest.frame();
         
    if (!hitFrame)
        return 0;

    QRect vsbrect = hitFrame->scrollBarGeometry(Qt::Vertical);
    QRect hsbrect = hitFrame->scrollBarGeometry(Qt::Horizontal);

    if (!vsbrect.isEmpty() && vsbrect.contains(hitTest.pos() - hitFrame->scrollPosition()))
        return 0;
    if (!hsbrect.isEmpty() && hsbrect.contains(hitTest.pos() - hitFrame->scrollPosition()))
        return 0;

    QSize range = hitFrame->contentsSize() - hitFrame->geometry().size();
    
    while (hitFrame && range.width() <= 1 && range.height() <= 1)
        hitFrame = hitFrame->parentFrame();
    return hitFrame;
}
#endif
    
bool QtScrollerFilter::eventFilter_QAbstractScrollArea(QAbstractScrollArea *area, QEvent *event)
{
    switch (event->type()) {
    case QtScrollPrepareEvent::ScrollPrepare:
    {
        QtScrollPrepareEvent *se = static_cast<QtScrollPrepareEvent *>(event);
        if (canStartScrollingAt_QAbstractScrollArea(area, se->startPos().toPoint())) {
            QScrollBar *hBar = area->horizontalScrollBar();
            QScrollBar *vBar = area->verticalScrollBar();

            se->setViewportSize(QSizeF(area->viewport()->size()));
            se->setContentPosRange(QRectF(0, 0, hBar->maximum(), vBar->maximum()));
            se->setContentPos(QPointF(hBar->value(), vBar->value()));
            se->accept();
            return true;
        }
        return false;
    }
    case QtScrollEvent::Scroll:
    {
        QtScrollEvent *se = static_cast<QtScrollEvent *>(event);

        QScrollBar *hBar = area->horizontalScrollBar();
        QScrollBar *vBar = area->verticalScrollBar();
        hBar->setValue(se->contentPos().x());
        vBar->setValue(se->contentPos().y());

        QPoint os = overshoot.value(area);
#ifdef Q_WS_WIN
        typedef BOOL (*PtrBeginPanningFeedback)(__in HWND);
        typedef BOOL (*PtrUpdatePanningFeedback)(__in HWND, __in LONG, __in LONG, __in BOOL
        typedef BOOL (*PtrEndPanningFeedback)(__in HWND, BOOL);

        static PtrBeginPanningFeedback ptrBeginPanningFeedback = 0;
        static PtrUpdatePanningFeedback ptrUpdatePanningFeedback = 0;
        static PtrEndPanningFeedback ptrEndPanningFeedback = 0;

        if (!ptrBeginPanningFeedback)
            ptrBeginPanningFeedback = (PtrBeginPanningFeedback) QLibrary::resolve(QLatin1St
        if (!ptrUpdatePanningFeedback)
            ptrUpdatePanningFeedback = (PtrUpdatePanningFeedback) QLibrary::resolve(QLatin1
        if (!ptrEndPanningFeedback)
            ptrEndPanningFeedback = (PtrEndPanningFeedback) QLibrary::resolve(QLatin1String

        if (ptrBeginPanningFeedback && ptrUpdatePanningFeedback && ptrEndPanningFeedback) {
            if (!se->overshootDistance().isNull() && os.isNull())
                ptrBeginPanningFeedback(window()->winId());
            if (!se->overshootDistance().isNull())
                ptrUpdatePanningFeedback(window()->winId(), -se->overshootDistance().x(), -
            if (se->overshootDistance().isNull() && !os.isNull())
                ptrEndPanningFeedback(window()->winId(), true);
        } else
#endif
        {
            QPoint delta = os - se->overshootDistance().toPoint();
            if (!delta.isNull())
                area->viewport()->move(area->viewport()->pos() + delta);
        }
        overshoot[area] = se->overshootDistance().toPoint();
        return true;
    }
    default:
        return false;
    }
}

struct HackedAbstractItemView : public QAbstractItemView
{
    void hackedExecuteDelayedItemsLayout()
    {
        executeDelayedItemsLayout();
    }
};

bool QtScrollerFilter::eventFilter_QAbstractItemView(QAbstractItemView *view, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (view->selectionModel())
            oldSelection = view->selectionModel()->selection();
        else
            oldSelection = QItemSelection();
        break;

    case QEvent::Gesture:
    {
        QGestureEvent *ge = static_cast<QGestureEvent *>(event);
        QGesture *g = ge->gesture(QtScroller::grabbedGesture(view->viewport()));
        if (g && g->state() == Qt::GestureStarted) {
            // restore the old selection if we really start scrolling
            if (view->selectionModel()) {
                view->selectionModel()->select(oldSelection, QItemSelectionModel::ClearAndSelect);
                oldSelection = QItemSelection();
            }
        }
        break;
    }

    case QtScrollPrepareEvent::ScrollPrepare:
        static_cast<HackedAbstractItemView *>(view)->hackedExecuteDelayedItemsLayout();
        break;
        
    default:
        break;
    }
  
    return false;
}

bool QtScrollerFilter::canStartScrollingAt_QAbstractScrollArea(QAbstractScrollArea *area, const QPoint &startPos) const
{
    // don't start scrolling when a drag mode has been set.
    // don't start scrolling on a movable item.
    if (QGraphicsView *view = qobject_cast<QGraphicsView *>(area)) {
        if (view->dragMode() != QGraphicsView::NoDrag)
            return false;

        QGraphicsItem *childItem = view->itemAt(startPos);

        if (childItem && (childItem->flags() & QGraphicsItem::ItemIsMovable))
            return false;
    }

    // don't start scrolling on a QAbstractSlider
    if (qobject_cast<QAbstractSlider *>(area->viewport()->childAt(startPos))) {
        return false;
    }

    return true;
}

