/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (directui@nokia.com)
**
** This file is part of libmeegotouch.
**
** If you have questions regarding the use of this file, please contact
** Nokia at directui@nokia.com.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#ifndef QTMAEMO6STYLE_P_H
#define QTMAEMO6STYLE_P_H

#include <QPointer>

#include "qtmaemo6style.h"
#include "qtmaemo6styleeventfilter.h"
#include "qtmaemo6scrollbareventfilter.h"

#ifdef HAVE_CONTEXTSUBSCRIBER
#include "contextsubscriber/contextproperty.h"
#endif //HAVE_CONTEXTSUBSCRIBER
//krazy:excludeall=qclasses

class MComponentData;
class MWidgetController;
class QToolButton;
class QAction;
class QStatusBar;
class QMenuBar;
class QStyleOptionButton;
class QComboBox;

class MComboBoxStyle;
class MListItemStyle;
class MLabelStyle;


class QtMaemo6StylePrivate
{
public:
    Q_DECLARE_PUBLIC(QtMaemo6Style)

    QtMaemo6StylePrivate();
    virtual ~QtMaemo6StylePrivate();

    /*!
     * Initialization of the M framework.
     * init() gets called by the style constructor.
     * At that point we don't know yet whether the application object is a M
     * application or not. As we only need to fully initialize the M framework
     * for non-M application we need to defer the initialization of the M
     * framework to a later point of the application life cycle.
     */
    void initM();

    /*!
     * returns a MStyle corresponding the given parameters
     */
    static const MStyle *mStyle(QStyle::State state,
                                    const QString &styleClass,
                                    const QString &styleObject = QString(),
                                    const QString &type = QString(),
                                    const bool hasFocus = false,
                                    const MWidgetController *parent = NULL);

    /*!
     * returns a string generated from the QStyle::state, m can use
     */
    static QString modeFromState(QStyle::State state) ;

    /*!
     * draws a scalable image onto painter
     * \param p the painter to draw on
     * \param option style option used for drawing
     * \param rect the rect into which is drawn, the scalable image will be
     *             drawn to fit into this rect
     * \param scalableImage the image that should be drawn
     * \param style optional if given transparency and background color of the
     *                       style will be used if there is no suitable image
     * \param widget optional if image should be used as background of a widget
     *                        you must provide the widget here. The widget will
     *                        be marked as dirty due asynchronous loading.
     *                        The MScalableImage will then be set as background for
     *                        this widget by the style as soon as the image is
     *                        loaded and the widget gets an update.
     * \param purpose optional provide a key used for caching. This is needed if
     *                         you want to use different backgrounds for the same
     *                         widget.
     * \param enableCache optional if true the images will be cached locally by
     *                             the style
     * \return true if the image is loaded and drawn
     *         false if the image is not loaded yet (and then of course wasn't drawn)
     *               but the loading process was invoked then.
     */
    bool drawScalableImage(QPainter *p,
                           const QStyleOption *option,
                           const QRect &rect,
                           const MScalableImage *scalableImage,
                           const MWidgetStyle *style,
                           const QWidget* widget = 0,
                           const QString &purpose = "bg",
                           bool enableCache = true) const;

    /*!
     * draws the background of a slider
     */
    static void drawSliderBaseBackground(QPainter *p,
                                         const QStyleOption *option,
                                         const QRect &rect,
                                         const MSliderStyle *style,
                                         int maxSliderLength);

    /*!
     * Draws the text on the button.
     * \param style the m style which is used to draw the button
     * \param painter
     * \param textRect the rect within the button in which the text (and icon) is drawn
     * \param text the text
     * \param align the alignment the text uses within the textRect!!
     * \param font font used instead of font from the style
     */
    void drawButtonText(const MButtonStyle *style,
                        QPainter *painter,
                        const QRectF &textRect,
                        const QString &text,
                        Qt::Alignment align,
                        const QFont &font) const;
    /*!
     * Draws the text on the button.
     *  overload: uses the font from style to draw the text
     * \param style the m style which is used to draw the button
     * \param painter
     * \param textRect the rect within the button in which the text (and icon) is drawn
     * \param text the text
     * \param align the alignment the text uses within the textRect!!
     */
    void drawButtonText(const MButtonStyle *style,
                        QPainter *painter,
                        const QRectF &textRect,
                        const QString &text,
                        Qt::Alignment align) const;

    /*!
     * Draws the icon on the button.
     * \param style the m style which is used to draw the button
     * \param painter
     * \param contentsRect the rect within the button in which the text (and icon) is drawn
     * \param icon the icon
     * \param iconSize optional overrides the icon size from style
     * \param centerAlways don't use the icon-align property, but center always
     */
    void drawButtonIcon(const MButtonStyle *style,
                        QPainter *painter,
                        const QRect &contentsRect,
                        const QIcon &icon,
                        const QSize &iconSize = QSize(),
                        const bool centerAlways = false) const;

    /*!
     * Draws the button.
     * \param painter
     * \param text the text on the button
     * \param icon the icon on the button
     * \param rect the rect in which the button is drawn
     * \param option the QStyleOption used to draw the button
     * \param style the m style used to draw the button
     */
    void drawBasicButton(QPainter *painter,
                         const QString &text,
                         const QIcon &icon,
                         const QRect &rect,
                         const QStyleOption *option,
                         const MButtonStyle *style) const;

    /*!
     * Draws the button (overloaded)
     * \param painter
     * \param text the text on the button
     * \param icon the icon on the button
     * \param rect the rect in which the button is drawn
     * \param option the QStyleOption used for the button
     * \param styleClass style class that should be used
     * \param styleObject style object that should be used
     */
    void drawBasicButton(QPainter *painter,
                         const QString &text,
                         const QIcon &icon,
                         const QRect &rect,
                         const QStyleOption *option,
                         const QString &styleClass,
                         const QString &styleObject = QString()) const;

    /*!
     * Draws the button.
     * \param painter
     * \param text the text on the button
     * \param icon the icon on the button
     * \param rect the rect in which the button is drawn
     * \param option the QStyleOption used to draw the button
     * \param style the m style used to draw the button
     * \param font use this font instead of the font from the style
     * \param font use this icon size instead of icon size from the style
     */
    void drawBasicButton(QPainter *painter,
                         const QString &text,
                         const QIcon &icon,
                         const QRect &rect,
                         const QStyleOption *option,
                         const MButtonStyle *style,
                         const QFont &font,
                         const QSize &iconSize) const;

    /*!
     * Draws the toggle button
     * \param painter
     * \param text the text on the button
     * \param rect the rect in which the button is drawn
     * \param icon the icon on the button
     * \param option the QStyleOption used to draw the button
     * \param styleClass style class that should be used
     * \param styleObject style object that should be used
     */
    void drawToggleButton(QPainter *painter,
                          const QString &text,
                          const QIcon &icon,
                          const QRect &rect,
                          const QStyleOption *option,
                          const QString &styleClass,
                          const QString &styleObject = QString()) const;

    /*!
     * Draws the combo box button, i.e. a button which informs about the current
     * selection. If the user clicks on the button, a popup list should be opeend.
     * \param painter
     * \param cbStyle the m style that delivers the indicator image
     * \param liStyle the m style used to draw the button
     * \param currentText the selected text
     * \param rect the rect in which the combobox button is drawn
     * \param option the QStyleOption used to draw the button
     * \param layoutDirection the direction (either Qt::LeftToRight or Qt::RightToLeft)
     */
    void drawComboBoxButton(QPainter *p,
                            const MComboBoxStyle *cbStyle,
                            const MListItemStyle *liStyle,
                            const QString &currentText,
                            const QRect &rect,
                            const QStyleOption *option,
                            const Qt::LayoutDirection layoutDirection = Qt::LeftToRight) const;


    /*!
     * Draws a label regarding MLabelStyle attributes
     * \param painter
     * \param labelStyle the style used for drawing the label text
     * \param textRect the rect in which the painter draws the text
     * \param text the text to be drawn
     * \param alignment the text alignment inside the given rect
     */
    void drawLabel(QPainter *p,
                   const MLabelStyle *labelStyle,
                   const QRect &textRect,
                   const QString &text,
                   const Qt::Alignment alignment = Qt::AlignTop | Qt::AlignLeft) const;

    /*!
     * Draws the checkbox
     * \param painter
     * \param text the text besides the checkBox
     * \param rect the rect in which the checkBox is drawn
     * \param icon the icon on the button
     * \param option the QStyleOption used to draw the checkBox
     */
    void drawCheckBox(QPainter *painter,
                      const QString &text,
                      const QIcon &icon,
                      const QRect &rect,
                      const QStyleOption *option) const;

    /*!
     * calculates a rect the text and icon earn
     * returns a rect within the button's rect used by the text and icon
     * the rect already includes the margins
     * \param style the m style used to draw the button
     * \param text the text on the button
     * \param icon optional the icon on the button
     * \param font optional used instead of the font from the style
     * \param iconSize optional used instead of iconSize from the style if valid
     */
    QRect getTextAndIconRect(const MButtonStyle *style,
                             const QString &text,
                             const QIcon &icon = QIcon(),
                             const QFont &font = QFont(),
                             const QSize &iconSize = QSize()) const;
    /*!
     * returns the rect of the text with font font
     * includes no margins
     * \param text the text
     * \param font font used to calculate the text size
     */
    QRect textBoundingRect(const QString &text, const QFont &font) const;

    /*!
     * returns the effective padding used for the style
     * uses the borders of the MScalableImage, but if the padding specified in the style is
     * greater than the border of the MScalableImage, the borders from style are used
     * \param style the used style
     * \param left border in the left side
     * \param top border in the top side
     * \param right border in the right side
     * \param bottom border in the bottom side
     */
    void paddingFromStyle(const MWidgetStyle *style,
                          int *left,
                          int *top,
                          int *right,
                          int *bottom) const;

      /*!
     * returns a size computed from the style properties maximumSize, minimumSize
     * and preferredSize. This method uses a heuristic to follow m sizes as far as possible
     * \param style the style from which the optimal size is computed
     */
    QSize heuristicSizeFromStyle(const MWidgetStyle *style) const;

      /*!
     * Updates the text color due to the widget's current state
     * \param the widget which palette should be updated
     */
    void setPaletteTextColor(QWidget *widget) const;

    /*!
     * returns the rect a slider uses
     */
    QRect scrollBarSliderRect(const QStyleOptionComplex *option, const QWidget *widget = 0) const;

    /*!
     * \brief returns a Pixmap with the given borders cropped away
     * The given borders of the scalable image are cut away, the resulting pixmap will
     * have size as size. If drawLines is true, lines are drawn on the cut away borders
     * to close the pixmap. The color of these lines is taken from one of the pixmaps
     * outermost pixels.
     * \param image the image used as base
     * \param size target size of the pixmap
     * \param borders combined Borders flags that should be cut away
     * \param borderLines optional draw closing lines on the given border sides. If not
     *        given border lines are drawn for all cut borders
     */
    enum Borders {
        leftBorder = 1,
        topBorder = 2,
        rightBorder = 4,
        bottomBorder = 8
    };
    QPixmap borderCroppedPixmap(const MScalableImage* image, QSize size, int borders, int borderLines) const;
    QPixmap borderCroppedPixmap(const MScalableImage* image, QSize size, int borders) const {
        return borderCroppedPixmap(image, size, borders, borders);
    }

    /*!
     * returns an inverted Alignment, so align right will become align left, top
     * will become bottom and so on
     */
    Qt::Alignment invertAlignment(Qt::Alignment align) const;

    bool hasVerticalAlignment(Qt::Alignment align) const {
        return (align & 0xF0) > 0;
    };
    bool hasHorizontalAlignment(Qt::Alignment align) const {
        return (align & 0xF) > 0;
    };
    Qt::Alignment verticalAlignment(Qt::Alignment align) const {
        return align & 0xF0;
    };
    Qt::Alignment horizontalAlignment(Qt::Alignment align) const {
        return align & 0xF;
    };

    void ensureWidgetVisible(QWidget* widget, QRect visibleArea);

    bool hasMWindowParent(const QWidget*) const;
public:
    QList<QToolButton *> m_toolButtonsInTitleBar;
    QList<QAction *> m_toolBarActions;
    QPointer<QStatusBar> m_statusBar;

    const int m_actionsInTitleBarCount;

    MComponentData *m_componentData;

#ifdef HAVE_CONTEXTSUBSCRIBER
    ContextProperty m_orientation;
#endif //HAVE_CONTEXTSUBSCRIBER

    bool m_isMInitialized;
    bool m_isMApplication;
    bool m_isOrientationChangeEnabled;

    QComboBox *m_pressedComboBox;

    QtMaemo6ScrollBarEventFilter *m_scrollBarEventFilter;
    QtMaemo6StyleEventFilter *m_windowEventFilter;

    QMenuBar *m_menuBar;

    QList<QString> m_excludeClasses;

    //this is the cache list for widgets that need updating due to asynchronous
    // image loading. If there is an MScalabaleImage for the widget in this list,
    // this image is supposed to be used as background for the widget.
    // if the image is NULL, the widget just needs an update.
    mutable QMap<QWidget*, const MScalableImage*> m_dirtyWidgets;

    //prints style information
    //useful for finding adequate style --> mode mappings
    static void printStateFlags(QStyle::State state);

     struct WidgetPos {
        WidgetPos() : widget(NULL) {};
        WidgetPos(QWidget* w, QPoint p) : widget(w), position(p) {};
        WidgetPos(const WidgetPos& other) { widget = other.widget; position = other.position; }
        QPointer<QWidget> widget;
        QPoint position;
    };
    WidgetPos m_originalWidgetPos;

protected:
    QtMaemo6Style *q_ptr;
};

#endif
